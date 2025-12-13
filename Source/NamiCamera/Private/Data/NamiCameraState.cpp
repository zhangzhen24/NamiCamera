// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/NamiCameraState.h"
#include "Logging/LogNamiCamera.h"
#include "Logging/LogNamiCameraMacros.h"
#include "Math/NamiCameraMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraState)

FNamiCameraState::FNamiCameraState()
	: PivotLocation(FVector::ZeroVector)
	, PivotRotation(FRotator::ZeroRotator)
	, ArmLength(300.0f)
	, ArmRotation(FRotator::ZeroRotator)
	, ArmOffset(FVector::ZeroVector)
	, CameraLocationOffset(FVector::ZeroVector)
	, CameraRotationOffset(FRotator::ZeroRotator)
	, FieldOfView(90.0f)
	, CameraLocation(FVector::ZeroVector)
	, CameraRotation(FRotator::ZeroRotator)
{
}

void FNamiCameraState::ComputeOutput()
{
	// ========== 第一步：计算基础枢轴旋转 ==========
	const FRotator BasePivotRotation = PivotRotation;
	const FQuat BasePivotQuat = BasePivotRotation.Quaternion();

	NAMI_LOG_STATE(VeryVerbose, TEXT("[FNamiCameraState::ComputeOutput] 基础枢轴旋转: PivotRotation=%s"),
		*PivotRotation.ToString());
	
	// ========== 第二步：计算基础相机偏移向量（未应用 ArmRotation）==========
	// 相机在枢轴点后方，距离为 ArmLength
	// 从 PivotLocation 指向相机的向量（世界空间）
	const FVector BaseArmDirection = BasePivotQuat.GetForwardVector();
	const FVector BaseCameraOffset = -BaseArmDirection * ArmLength;
	
	NAMI_LOG_STATE(VeryVerbose, TEXT("[FNamiCameraState::ComputeOutput] 基础相机偏移: ArmLength=%.2f, BaseArmDirection=%s, BaseCameraOffset=%s"),
		ArmLength, *BaseArmDirection.ToString(), *BaseCameraOffset.ToString());
	
	// ========== 第三步：以 PivotLocation 为中心旋转相机位置向量 ==========
	// 核心概念：旋转吊臂，相机沿着以 PivotLocation 为圆心、ArmLength 为半径的圆弧移动
	// 
	// 旋转原理：
	// 1. 将相机偏移向量转到枢轴本地空间
	// 2. 在本地空间中应用 ArmRotation 旋转
	// 3. 转回世界空间
	// 
	// 四元数组合：LocalRotationQuat = BasePivotQuat * ArmQuat * BasePivotQuat.Inverse()
	// 表示：在 BasePivot 的局部空间中应用 ArmRotation，然后转到世界空间
	const FQuat ArmQuat = ArmRotation.Quaternion();
	const FQuat LocalRotationQuat = BasePivotQuat * ArmQuat * BasePivotQuat.Inverse();
	
	// 旋转相机偏移向量
	FVector RotatedCameraOffset = LocalRotationQuat.RotateVector(BaseCameraOffset);
	
	// 计算最终相机位置（以 PivotLocation 为中心）
	CameraLocation = PivotLocation + RotatedCameraOffset;
	
	NAMI_LOG_STATE(VeryVerbose, TEXT("[FNamiCameraState::ComputeOutput] 吊臂旋转: ArmRotation=%s, RotatedCameraOffset=%s, CameraLocation=%s"),
		*ArmRotation.ToString(), *RotatedCameraOffset.ToString(), *CameraLocation.ToString());
	
	// 记录最终相机位置计算（用于验证 Additive vs Override 差异）
	// 使用 Warning 级别，确保能看到最终位置（用于调试 Additive vs Override）
	NAMI_LOG_WARNING(TEXT("[ComputeOutput] ★ 最终相机位置: PivotLocation=%s, ArmRotation=%s, RotatedOffset=%s, FinalLocation=%s"),
		*PivotLocation.ToString(), *ArmRotation.ToString(), *RotatedCameraOffset.ToString(), *CameraLocation.ToString());
	
	// ========== 第四步：应用吊臂末端偏移（吊臂本地空间）==========
	// 吊臂末端的实际旋转 = BasePivot + ArmRotation
	const FQuat FinalArmQuat = BasePivotQuat * ArmQuat;
	if (!ArmOffset.IsNearlyZero())
	{
		FVector ArmOffsetWorld = FinalArmQuat.RotateVector(ArmOffset);
		CameraLocation += ArmOffsetWorld;
		NAMI_LOG_STATE(VeryVerbose, TEXT("[FNamiCameraState::ComputeOutput] 吊臂偏移: ArmOffset=%s, ArmOffsetWorld=%s, CameraLocation=%s"),
			*ArmOffset.ToString(), *ArmOffsetWorld.ToString(), *CameraLocation.ToString());
	}
	
	// ========== 第五步：计算相机旋转 ==========
	// 相机朝向 = 最终吊臂旋转 + 相机旋转偏移
	// - FinalArmQuat 决定相机默认朝向（看向角色）
	// - CameraRotationOffset：额外的视觉偏移（不受玩家输入影响）
	CameraRotation = FinalArmQuat.Rotator() + CameraRotationOffset;
	CameraRotation.Normalize();
	
	NAMI_LOG_STATE(VeryVerbose, TEXT("[FNamiCameraState::ComputeOutput] 相机旋转: FinalArmQuat=%s, CameraRotationOffset=%s, CameraRotation=%s"),
		*FinalArmQuat.Rotator().ToString(), *CameraRotationOffset.ToString(), *CameraRotation.ToString());
	
	// ========== 第六步：应用相机位置偏移（相机本地空间）==========
	if (!CameraLocationOffset.IsNearlyZero())
	{
		FVector CameraLocationOffsetWorld = CameraRotation.RotateVector(CameraLocationOffset);
		CameraLocation += CameraLocationOffsetWorld;
		NAMI_LOG_STATE(VeryVerbose, TEXT("[FNamiCameraState::ComputeOutput] 相机位置偏移: CameraLocationOffset=%s, CameraLocationOffsetWorld=%s, FinalCameraLocation=%s"),
			*CameraLocationOffset.ToString(), *CameraLocationOffsetWorld.ToString(), *CameraLocation.ToString());
	}
	
	NAMI_LOG_STATE(Verbose, TEXT("[FNamiCameraState::ComputeOutput] 最终结果: Location=%s, Rotation=%s, FOV=%.2f"),
		*CameraLocation.ToString(), *CameraRotation.ToString(), FieldOfView);
}

void FNamiCameraState::ApplyChanged(const FNamiCameraState& Other, ENamiCameraBlendMode BlendMode, float Weight)
{
	const FNamiCameraStateFlags& OtherFlags = Other.ChangedFlags;
	
	// 记录被修改的参数
	TArray<FString> ChangedParams;
	if (OtherFlags.bPivotLocation) ChangedParams.Add(TEXT("PivotLocation"));
	if (OtherFlags.bPivotRotation) ChangedParams.Add(TEXT("PivotRotation"));
	if (OtherFlags.bArmLength) ChangedParams.Add(TEXT("ArmLength"));
	if (OtherFlags.bArmRotation) ChangedParams.Add(TEXT("ArmRotation"));
	if (OtherFlags.bArmOffset) ChangedParams.Add(TEXT("ArmOffset"));
	if (OtherFlags.bCameraLocationOffset) ChangedParams.Add(TEXT("CameraLocationOffset"));
	if (OtherFlags.bCameraRotationOffset) ChangedParams.Add(TEXT("CameraRotationOffset"));
	if (OtherFlags.bFieldOfView) ChangedParams.Add(TEXT("FieldOfView"));
	if (OtherFlags.bCameraLocation) ChangedParams.Add(TEXT("CameraLocation"));
	if (OtherFlags.bCameraRotation) ChangedParams.Add(TEXT("CameraRotation"));
	
	NAMI_LOG_STATE(Verbose, TEXT("[FNamiCameraState::ApplyChanged] 开始混合: BlendMode=%d, Weight=%.3f, 修改参数: [%s]"),
		(int32)BlendMode, Weight, ChangedParams.Num() > 0 ? *FString::Join(ChangedParams, TEXT(", ")) : TEXT("无"));
	
	// 应用输入参数
	if (OtherFlags.bPivotLocation)
	{
		switch (BlendMode)
		{
		case ENamiCameraBlendMode::Additive:
			PivotLocation += Other.PivotLocation * Weight;
			break;
		case ENamiCameraBlendMode::Override:
			PivotLocation = FMath::Lerp(PivotLocation, Other.PivotLocation, Weight);
			break;
		}
		ChangedFlags.bPivotLocation = true;
	}
	
	if (OtherFlags.bPivotRotation)
	{
		// 优化：只在混合前归一化一次，混合后归一化一次
		// FindDeltaAngle360 内部会归一化输入，所以不需要提前归一化每个分量
		FRotator NormalizedCurrent = FNamiCameraMath::NormalizeRotatorTo360(PivotRotation);
		FRotator NormalizedTarget = FNamiCameraMath::NormalizeRotatorTo360(Other.PivotRotation);
		
		switch (BlendMode)
		{
		case ENamiCameraBlendMode::Additive:
			// Additive: 在当前值基础上增加偏移
			{
				// 计算加性值（目标值 * 权重）
				FRotator AdditiveValue = NormalizedTarget * Weight;
				
				// 直接相加，FindDeltaAngle360 会处理归一化
				float PitchDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Pitch, NormalizedCurrent.Pitch + AdditiveValue.Pitch);
				float YawDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Yaw, NormalizedCurrent.Yaw + AdditiveValue.Yaw);
				float RollDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Roll, NormalizedCurrent.Roll + AdditiveValue.Roll);
				
				// 应用差值（结果会在最后归一化）
				PivotRotation.Pitch = NormalizedCurrent.Pitch + PitchDelta;
				PivotRotation.Yaw = NormalizedCurrent.Yaw + YawDelta;
				PivotRotation.Roll = NormalizedCurrent.Roll + RollDelta;
			}
			break;
		case ENamiCameraBlendMode::Override:
			// Override: 从当前值过渡到目标值
			{
				// 计算最短路径差值
				float PitchDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Pitch, NormalizedTarget.Pitch);
				float YawDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Yaw, NormalizedTarget.Yaw);
				float RollDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Roll, NormalizedTarget.Roll);
				
				// 应用权重插值（结果会在最后归一化）
				PivotRotation.Pitch = NormalizedCurrent.Pitch + PitchDelta * Weight;
				PivotRotation.Yaw = NormalizedCurrent.Yaw + YawDelta * Weight;
				PivotRotation.Roll = NormalizedCurrent.Roll + RollDelta * Weight;
			}
			break;
		}
		// 确保结果在0-360度范围（只归一化一次）
		PivotRotation = FNamiCameraMath::NormalizeRotatorTo360(PivotRotation);
		ChangedFlags.bPivotRotation = true;
	}
	
	if (OtherFlags.bArmLength)
	{
		float OldArmLength = ArmLength;
		switch (BlendMode)
		{
		case ENamiCameraBlendMode::Additive:
			ArmLength += Other.ArmLength * Weight;
			break;
		case ENamiCameraBlendMode::Override:
			ArmLength = FMath::Lerp(ArmLength, Other.ArmLength, Weight);
			break;
		}
		ArmLength = FMath::Max(0.0f, ArmLength);
		ChangedFlags.bArmLength = true;
		
		NAMI_LOG_STATE(VeryVerbose, TEXT("[FNamiCameraState::ApplyChanged] ArmLength混合: 旧值=%.2f, 新值=%.2f, 目标值=%.2f, 模式=%d, 权重=%.3f"),
			OldArmLength, ArmLength, Other.ArmLength, (int32)BlendMode, Weight);
	}
	
	if (OtherFlags.bArmRotation)
	{
		FRotator OldArmRotation = ArmRotation;
		FRotator TargetArmRotation = Other.ArmRotation;
		
		// 优化：只在混合前归一化一次，混合后归一化一次
		FRotator NormalizedCurrent = FNamiCameraMath::NormalizeRotatorTo360(ArmRotation);
		FRotator NormalizedTarget = FNamiCameraMath::NormalizeRotatorTo360(TargetArmRotation);
		
		switch (BlendMode)
		{
		case ENamiCameraBlendMode::Additive:
			// Additive: 在当前值基础上增加偏移
			{
				// 计算加性值（目标值 * 权重）
				FRotator AdditiveValue = NormalizedTarget * Weight;
				
				// 直接相加，FindDeltaAngle360 会处理归一化
				float PitchDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Pitch, NormalizedCurrent.Pitch + AdditiveValue.Pitch);
				float YawDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Yaw, NormalizedCurrent.Yaw + AdditiveValue.Yaw);
				float RollDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Roll, NormalizedCurrent.Roll + AdditiveValue.Roll);
				
				// 应用差值（结果会在最后归一化）
				ArmRotation.Pitch = NormalizedCurrent.Pitch + PitchDelta;
				ArmRotation.Yaw = NormalizedCurrent.Yaw + YawDelta;
				ArmRotation.Roll = NormalizedCurrent.Roll + RollDelta;
			}
			// 使用 Warning 级别，确保能看到混合过程（用于调试 Additive vs Override）
			NAMI_LOG_WARNING(TEXT("[ApplyChanged] ★ ArmRotation Additive: 当前值=%s + 目标值=%s * 权重=%.3f = 结果=%s"),
				*OldArmRotation.ToString(), *TargetArmRotation.ToString(), Weight, *ArmRotation.ToString());
			break;
		case ENamiCameraBlendMode::Override:
			// Override: 从当前值过渡到目标值（覆盖）
			{
				// 计算最短路径差值
				float PitchDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Pitch, NormalizedTarget.Pitch);
				float YawDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Yaw, NormalizedTarget.Yaw);
				float RollDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Roll, NormalizedTarget.Roll);
				
				// 应用权重插值（结果会在最后归一化）
				ArmRotation.Pitch = NormalizedCurrent.Pitch + PitchDelta * Weight;
				ArmRotation.Yaw = NormalizedCurrent.Yaw + YawDelta * Weight;
				ArmRotation.Roll = NormalizedCurrent.Roll + RollDelta * Weight;
			}
			// 使用 Warning 级别，确保能看到混合过程（用于调试 Additive vs Override）
			NAMI_LOG_WARNING(TEXT("[ApplyChanged] ★ ArmRotation Override: Lerp(当前值=%s, 目标值=%s, 权重=%.3f) = 结果=%s"),
				*OldArmRotation.ToString(), *TargetArmRotation.ToString(), Weight, *ArmRotation.ToString());
			break;
		}
		// 确保结果在0-360度范围（只归一化一次）
		ArmRotation = FNamiCameraMath::NormalizeRotatorTo360(ArmRotation);
		ChangedFlags.bArmRotation = true;
		
		NAMI_LOG_STATE(VeryVerbose, TEXT("[FNamiCameraState::ApplyChanged] ArmRotation混合: 旧值=%s, 新值=%s, 目标值=%s, 模式=%d, 权重=%.3f"),
			*OldArmRotation.ToString(), *ArmRotation.ToString(), *TargetArmRotation.ToString(), (int32)BlendMode, Weight);
	}
	
	if (OtherFlags.bArmOffset)
	{
		switch (BlendMode)
		{
		case ENamiCameraBlendMode::Additive:
			ArmOffset += Other.ArmOffset * Weight;
			break;
		case ENamiCameraBlendMode::Override:
			ArmOffset = FMath::Lerp(ArmOffset, Other.ArmOffset, Weight);
			break;
		}
		ChangedFlags.bArmOffset = true;
	}
	
	// 相机参数
	if (OtherFlags.bCameraLocationOffset)
	{
		switch (BlendMode)
		{
		case ENamiCameraBlendMode::Additive:
			CameraLocationOffset += Other.CameraLocationOffset * Weight;
			break;
		case ENamiCameraBlendMode::Override:
			CameraLocationOffset = FMath::Lerp(CameraLocationOffset, Other.CameraLocationOffset, Weight);
			break;
		}
		ChangedFlags.bCameraLocationOffset = true;
	}
	
	if (OtherFlags.bCameraRotationOffset)
	{
		// 优化：只在混合前归一化一次，混合后归一化一次
		FRotator NormalizedCurrent = FNamiCameraMath::NormalizeRotatorTo360(CameraRotationOffset);
		FRotator NormalizedTarget = FNamiCameraMath::NormalizeRotatorTo360(Other.CameraRotationOffset);
		
		switch (BlendMode)
		{
		case ENamiCameraBlendMode::Additive:
			// Additive: 在当前值基础上增加偏移
			{
				// 计算加性值（目标值 * 权重）
				FRotator AdditiveValue = NormalizedTarget * Weight;
				
				// 直接相加，FindDeltaAngle360 会处理归一化
				float PitchDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Pitch, NormalizedCurrent.Pitch + AdditiveValue.Pitch);
				float YawDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Yaw, NormalizedCurrent.Yaw + AdditiveValue.Yaw);
				float RollDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Roll, NormalizedCurrent.Roll + AdditiveValue.Roll);
				
				// 应用差值（结果会在最后归一化）
				CameraRotationOffset.Pitch = NormalizedCurrent.Pitch + PitchDelta;
				CameraRotationOffset.Yaw = NormalizedCurrent.Yaw + YawDelta;
				CameraRotationOffset.Roll = NormalizedCurrent.Roll + RollDelta;
			}
			break;
		case ENamiCameraBlendMode::Override:
			// Override: 从当前值过渡到目标值
			{
				// 计算最短路径差值
				float PitchDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Pitch, NormalizedTarget.Pitch);
				float YawDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Yaw, NormalizedTarget.Yaw);
				float RollDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Roll, NormalizedTarget.Roll);
				
				// 应用权重插值（结果会在最后归一化）
				CameraRotationOffset.Pitch = NormalizedCurrent.Pitch + PitchDelta * Weight;
				CameraRotationOffset.Yaw = NormalizedCurrent.Yaw + YawDelta * Weight;
				CameraRotationOffset.Roll = NormalizedCurrent.Roll + RollDelta * Weight;
			}
			break;
		}
		// 确保结果在0-360度范围（只归一化一次）
		CameraRotationOffset = FNamiCameraMath::NormalizeRotatorTo360(CameraRotationOffset);
		ChangedFlags.bCameraRotationOffset = true;
	}
	
	if (OtherFlags.bFieldOfView)
	{
		switch (BlendMode)
		{
		case ENamiCameraBlendMode::Additive:
			FieldOfView += Other.FieldOfView * Weight;
			break;
		case ENamiCameraBlendMode::Override:
			FieldOfView = FMath::Lerp(FieldOfView, Other.FieldOfView, Weight);
			break;
		}
		FieldOfView = FMath::Clamp(FieldOfView, 5.0f, 170.0f);
		ChangedFlags.bFieldOfView = true;
	}
	
	// 应用输出结果
	if (OtherFlags.bCameraLocation)
	{
		switch (BlendMode)
		{
		case ENamiCameraBlendMode::Additive:
			CameraLocation += Other.CameraLocation * Weight;
			break;
		case ENamiCameraBlendMode::Override:
			CameraLocation = FMath::Lerp(CameraLocation, Other.CameraLocation, Weight);
			break;
		}
		ChangedFlags.bCameraLocation = true;
	}
	
	if (OtherFlags.bCameraRotation)
	{
		switch (BlendMode)
		{
		case ENamiCameraBlendMode::Additive:
			CameraRotation += Other.CameraRotation * Weight;
			break;
		case ENamiCameraBlendMode::Override:
			CameraRotation = FMath::Lerp(CameraRotation, Other.CameraRotation, Weight);
			break;
		}
		CameraRotation.Normalize();
		ChangedFlags.bCameraRotation = true;
	}
	
	NAMI_LOG_STATE(Verbose, TEXT("[FNamiCameraState::ApplyChanged] 混合完成: 当前状态 - PivotRotation=%s, ArmRotation=%s, ArmLength=%.2f"),
		*PivotRotation.ToString(), *ArmRotation.ToString(), ArmLength);
}

void FNamiCameraState::Lerp(const FNamiCameraState& To, float Alpha)
{
	// 枢轴点参数
	PivotLocation = FMath::Lerp(PivotLocation, To.PivotLocation, Alpha);
	PivotRotation = FMath::Lerp(PivotRotation, To.PivotRotation, Alpha);
	// 吊臂参数
	ArmLength = FMath::Lerp(ArmLength, To.ArmLength, Alpha);
	ArmRotation = FMath::Lerp(ArmRotation, To.ArmRotation, Alpha);
	ArmOffset = FMath::Lerp(ArmOffset, To.ArmOffset, Alpha);
	// 相机参数
	CameraLocationOffset = FMath::Lerp(CameraLocationOffset, To.CameraLocationOffset, Alpha);
	CameraRotationOffset = FMath::Lerp(CameraRotationOffset, To.CameraRotationOffset, Alpha);
	FieldOfView = FMath::Lerp(FieldOfView, To.FieldOfView, Alpha);
	// 输出结果
	CameraLocation = FMath::Lerp(CameraLocation, To.CameraLocation, Alpha);
	CameraRotation = FMath::Lerp(CameraRotation, To.CameraRotation, Alpha);
	
	// 合并标志
	ChangedFlags |= To.ChangedFlags;
}

void FNamiCameraState::LerpChanged(const FNamiCameraState& To, float Alpha, const FNamiCameraStateFlags& Mask)
{
	const FNamiCameraStateFlags EffectiveMask = To.ChangedFlags & Mask;
	
	// 枢轴点参数
	if (EffectiveMask.bPivotLocation)
	{
		PivotLocation = FMath::Lerp(PivotLocation, To.PivotLocation, Alpha);
		ChangedFlags.bPivotLocation = true;
	}
	if (EffectiveMask.bPivotRotation)
	{
		PivotRotation = FMath::Lerp(PivotRotation, To.PivotRotation, Alpha);
		ChangedFlags.bPivotRotation = true;
	}
	// 吊臂参数
	if (EffectiveMask.bArmLength)
	{
		ArmLength = FMath::Lerp(ArmLength, To.ArmLength, Alpha);
		ChangedFlags.bArmLength = true;
	}
	if (EffectiveMask.bArmRotation)
	{
		ArmRotation = FMath::Lerp(ArmRotation, To.ArmRotation, Alpha);
		ChangedFlags.bArmRotation = true;
	}
	if (EffectiveMask.bArmOffset)
	{
		ArmOffset = FMath::Lerp(ArmOffset, To.ArmOffset, Alpha);
		ChangedFlags.bArmOffset = true;
	}
	// 相机参数
	if (EffectiveMask.bCameraLocationOffset)
	{
		CameraLocationOffset = FMath::Lerp(CameraLocationOffset, To.CameraLocationOffset, Alpha);
		ChangedFlags.bCameraLocationOffset = true;
	}
	if (EffectiveMask.bCameraRotationOffset)
	{
		CameraRotationOffset = FMath::Lerp(CameraRotationOffset, To.CameraRotationOffset, Alpha);
		ChangedFlags.bCameraRotationOffset = true;
	}
	if (EffectiveMask.bFieldOfView)
	{
		FieldOfView = FMath::Lerp(FieldOfView, To.FieldOfView, Alpha);
		ChangedFlags.bFieldOfView = true;
	}
	// 输出结果
	if (EffectiveMask.bCameraLocation)
	{
		CameraLocation = FMath::Lerp(CameraLocation, To.CameraLocation, Alpha);
		ChangedFlags.bCameraLocation = true;
	}
	if (EffectiveMask.bCameraRotation)
	{
		CameraRotation = FMath::Lerp(CameraRotation, To.CameraRotation, Alpha);
		ChangedFlags.bCameraRotation = true;
	}
}

void FNamiCameraState::Reset()
{
	// 枢轴点参数
	PivotLocation = FVector::ZeroVector;
	PivotRotation = FRotator::ZeroRotator;
	// 吊臂参数
	ArmLength = 300.0f;
	ArmRotation = FRotator::ZeroRotator;
	ArmOffset = FVector::ZeroVector;
	// 相机参数
	CameraLocationOffset = FVector::ZeroVector;
	CameraRotationOffset = FRotator::ZeroRotator;
	FieldOfView = 90.0f;
	// 输出结果
	CameraLocation = FVector::ZeroVector;
	CameraRotation = FRotator::ZeroRotator;
	ChangedFlags.Clear();
}

void FNamiCameraState::ClearChangedFlags()
{
	ChangedFlags.Clear();
}

void FNamiCameraState::SetAllChangedFlags()
{
	ChangedFlags.SetAll(true);
}

// ==================== 便捷设置方法 ====================

void FNamiCameraState::SetPivotLocation(const FVector& InValue)
{
	PivotLocation = InValue;
	ChangedFlags.bPivotLocation = true;
}

void FNamiCameraState::SetPivotRotation(const FRotator& InValue)
{
	PivotRotation = InValue;
	ChangedFlags.bPivotRotation = true;
}

void FNamiCameraState::SetArmLength(float InValue)
{
	ArmLength = FMath::Max(0.0f, InValue);
	ChangedFlags.bArmLength = true;
}

void FNamiCameraState::SetArmRotation(const FRotator& InValue)
{
	ArmRotation = InValue;
	ChangedFlags.bArmRotation = true;
}

void FNamiCameraState::SetArmOffset(const FVector& InValue)
{
	ArmOffset = InValue;
	ChangedFlags.bArmOffset = true;
}

void FNamiCameraState::SetCameraLocationOffset(const FVector& InValue)
{
	CameraLocationOffset = InValue;
	ChangedFlags.bCameraLocationOffset = true;
}

void FNamiCameraState::SetCameraRotationOffset(const FRotator& InValue)
{
	CameraRotationOffset = InValue;
	ChangedFlags.bCameraRotationOffset = true;
}

void FNamiCameraState::SetFieldOfView(float InValue)
{
	FieldOfView = FMath::Clamp(InValue, 5.0f, 170.0f);
	ChangedFlags.bFieldOfView = true;
}

void FNamiCameraState::SetCameraLocation(const FVector& InValue)
{
	CameraLocation = InValue;
	ChangedFlags.bCameraLocation = true;
}

void FNamiCameraState::SetCameraRotation(const FRotator& InValue)
{
	CameraRotation = InValue;
	ChangedFlags.bCameraRotation = true;
}

// ==================== 便捷叠加方法 ====================

void FNamiCameraState::AddPivotLocation(const FVector& InDelta)
{
	PivotLocation += InDelta;
	ChangedFlags.bPivotLocation = true;
}

void FNamiCameraState::AddPivotRotation(const FRotator& InDelta)
{
	// 使用0-360度归一化，避免180/-180跳变问题
	FRotator NormalizedCurrent = FNamiCameraMath::NormalizeRotatorTo360(PivotRotation);
	FRotator NormalizedDelta = FNamiCameraMath::NormalizeRotatorTo360(InDelta);
	
	PivotRotation.Pitch = FNamiCameraMath::NormalizeAngleTo360(NormalizedCurrent.Pitch + NormalizedDelta.Pitch);
	PivotRotation.Yaw = FNamiCameraMath::NormalizeAngleTo360(NormalizedCurrent.Yaw + NormalizedDelta.Yaw);
	PivotRotation.Roll = FNamiCameraMath::NormalizeAngleTo360(NormalizedCurrent.Roll + NormalizedDelta.Roll);
	
	ChangedFlags.bPivotRotation = true;
}

void FNamiCameraState::AddArmLength(float InDelta)
{
	ArmLength = FMath::Max(0.0f, ArmLength + InDelta);
	ChangedFlags.bArmLength = true;
}

void FNamiCameraState::AddArmRotation(const FRotator& InDelta)
{
	// 使用0-360度归一化，避免180/-180跳变问题
	FRotator NormalizedCurrent = FNamiCameraMath::NormalizeRotatorTo360(ArmRotation);
	FRotator NormalizedDelta = FNamiCameraMath::NormalizeRotatorTo360(InDelta);
	
	ArmRotation.Pitch = FNamiCameraMath::NormalizeAngleTo360(NormalizedCurrent.Pitch + NormalizedDelta.Pitch);
	ArmRotation.Yaw = FNamiCameraMath::NormalizeAngleTo360(NormalizedCurrent.Yaw + NormalizedDelta.Yaw);
	ArmRotation.Roll = FNamiCameraMath::NormalizeAngleTo360(NormalizedCurrent.Roll + NormalizedDelta.Roll);
	
	ChangedFlags.bArmRotation = true;
}

void FNamiCameraState::AddArmOffset(const FVector& InDelta)
{
	ArmOffset += InDelta;
	ChangedFlags.bArmOffset = true;
}

void FNamiCameraState::AddCameraLocationOffset(const FVector& InDelta)
{
	CameraLocationOffset += InDelta;
	ChangedFlags.bCameraLocationOffset = true;
}

void FNamiCameraState::AddCameraRotationOffset(const FRotator& InDelta)
{
	// 使用0-360度归一化，避免180/-180跳变问题
	FRotator NormalizedCurrent = FNamiCameraMath::NormalizeRotatorTo360(CameraRotationOffset);
	FRotator NormalizedDelta = FNamiCameraMath::NormalizeRotatorTo360(InDelta);
	
	CameraRotationOffset.Pitch = FNamiCameraMath::NormalizeAngleTo360(NormalizedCurrent.Pitch + NormalizedDelta.Pitch);
	CameraRotationOffset.Yaw = FNamiCameraMath::NormalizeAngleTo360(NormalizedCurrent.Yaw + NormalizedDelta.Yaw);
	CameraRotationOffset.Roll = FNamiCameraMath::NormalizeAngleTo360(NormalizedCurrent.Roll + NormalizedDelta.Roll);
	
	ChangedFlags.bCameraRotationOffset = true;
}

void FNamiCameraState::AddFieldOfView(float InDelta)
{
	FieldOfView = FMath::Clamp(FieldOfView + InDelta, 5.0f, 170.0f);
	ChangedFlags.bFieldOfView = true;
}

void FNamiCameraState::AddCameraLocation(const FVector& InDelta)
{
	CameraLocation += InDelta;
	ChangedFlags.bCameraLocation = true;
}

void FNamiCameraState::AddCameraRotation(const FRotator& InDelta)
{
	CameraRotation += InDelta;
	CameraRotation.Normalize();
	ChangedFlags.bCameraRotation = true;
}

