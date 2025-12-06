// Copyright Epic Games, Inc. All Rights Reserved.

#include "Structs/State/NamiCameraState.h"
#include "LogNamiCamera.h"
#include "LogNamiCameraMacros.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraState)

FNamiCameraState::FNamiCameraState()
	: PivotLocation(FVector::ZeroVector)
	, PivotRotation(FRotator::ZeroRotator)
	, ArmLength(300.0f)
	, ArmRotation(FRotator::ZeroRotator)
	, ArmOffset(FVector::ZeroVector)
	, ControlRotationOffset(FRotator::ZeroRotator)
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
	// PivotRotation（来自 ControlRotation）+ ControlRotationOffset（视角控制偏移）
	const FRotator BasePivotRotation = PivotRotation + ControlRotationOffset;
	const FQuat BasePivotQuat = BasePivotRotation.Quaternion();
	
	NAMI_LOG_STATE(VeryVerbose, TEXT("[FNamiCameraState::ComputeOutput] 基础枢轴旋转: PivotRotation=%s, ControlRotationOffset=%s, BasePivotRotation=%s"),
		*PivotRotation.ToString(), *ControlRotationOffset.ToString(), *BasePivotRotation.ToString());
	
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
	if (OtherFlags.bControlRotationOffset) ChangedParams.Add(TEXT("ControlRotationOffset"));
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
		switch (BlendMode)
		{
		case ENamiCameraBlendMode::Additive:
			// Additive: 使用 FindDeltaAngleDegrees 确保选择最短路径，避免跨越 360 度边界跳变
			{
				FRotator AdditiveValue = Other.PivotRotation * Weight;
				FRotator RawTarget;
				RawTarget.Pitch = PivotRotation.Pitch + AdditiveValue.Pitch;
				RawTarget.Yaw = PivotRotation.Yaw + AdditiveValue.Yaw;
				RawTarget.Roll = PivotRotation.Roll + AdditiveValue.Roll;
				RawTarget.Normalize();
				
				PivotRotation.Pitch = PivotRotation.Pitch + FMath::FindDeltaAngleDegrees(PivotRotation.Pitch, RawTarget.Pitch);
				PivotRotation.Yaw = PivotRotation.Yaw + FMath::FindDeltaAngleDegrees(PivotRotation.Yaw, RawTarget.Yaw);
				PivotRotation.Roll = PivotRotation.Roll + FMath::FindDeltaAngleDegrees(PivotRotation.Roll, RawTarget.Roll);
			}
			break;
		case ENamiCameraBlendMode::Override:
			// Override: 使用 FindDeltaAngleDegrees 确保选择最短路径，避免跨越 360 度边界跳变
			{
				FRotator NormalizedTarget = Other.PivotRotation;
				NormalizedTarget.Normalize();
				
				PivotRotation.Pitch = PivotRotation.Pitch + FMath::FindDeltaAngleDegrees(PivotRotation.Pitch, NormalizedTarget.Pitch) * Weight;
				PivotRotation.Yaw = PivotRotation.Yaw + FMath::FindDeltaAngleDegrees(PivotRotation.Yaw, NormalizedTarget.Yaw) * Weight;
				PivotRotation.Roll = PivotRotation.Roll + FMath::FindDeltaAngleDegrees(PivotRotation.Roll, NormalizedTarget.Roll) * Weight;
			}
			break;
		}
		PivotRotation.Normalize();
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
		
		switch (BlendMode)
		{
		case ENamiCameraBlendMode::Additive:
			// Additive: 在当前值基础上增加偏移
			// 使用 FindDeltaAngleDegrees 确保选择最短路径，避免跨越 360 度边界跳变
			{
				FRotator AdditiveValue = TargetArmRotation * Weight;
				FRotator RawTarget;
				RawTarget.Pitch = ArmRotation.Pitch + AdditiveValue.Pitch;
				RawTarget.Yaw = ArmRotation.Yaw + AdditiveValue.Yaw;
				RawTarget.Roll = ArmRotation.Roll + AdditiveValue.Roll;
				RawTarget.Normalize();
				
				ArmRotation.Pitch = ArmRotation.Pitch + FMath::FindDeltaAngleDegrees(ArmRotation.Pitch, RawTarget.Pitch);
				ArmRotation.Yaw = ArmRotation.Yaw + FMath::FindDeltaAngleDegrees(ArmRotation.Yaw, RawTarget.Yaw);
				ArmRotation.Roll = ArmRotation.Roll + FMath::FindDeltaAngleDegrees(ArmRotation.Roll, RawTarget.Roll);
			}
			// 使用 Warning 级别，确保能看到混合过程（用于调试 Additive vs Override）
			NAMI_LOG_WARNING(TEXT("[ApplyChanged] ★ ArmRotation Additive: 当前值=%s + 目标值=%s * 权重=%.3f = 结果=%s"),
				*OldArmRotation.ToString(), *TargetArmRotation.ToString(), Weight, *ArmRotation.ToString());
			break;
		case ENamiCameraBlendMode::Override:
			// Override: 从当前值过渡到目标值（覆盖）
			// 使用 FindDeltaAngleDegrees 确保选择最短路径，避免跨越 360 度边界跳变
			{
				FRotator NormalizedTarget = TargetArmRotation;
				NormalizedTarget.Normalize();
				
				ArmRotation.Pitch = ArmRotation.Pitch + FMath::FindDeltaAngleDegrees(ArmRotation.Pitch, NormalizedTarget.Pitch) * Weight;
				ArmRotation.Yaw = ArmRotation.Yaw + FMath::FindDeltaAngleDegrees(ArmRotation.Yaw, NormalizedTarget.Yaw) * Weight;
				ArmRotation.Roll = ArmRotation.Roll + FMath::FindDeltaAngleDegrees(ArmRotation.Roll, NormalizedTarget.Roll) * Weight;
			}
			// 使用 Warning 级别，确保能看到混合过程（用于调试 Additive vs Override）
			NAMI_LOG_WARNING(TEXT("[ApplyChanged] ★ ArmRotation Override: Lerp(当前值=%s, 目标值=%s, 权重=%.3f) = 结果=%s"),
				*OldArmRotation.ToString(), *TargetArmRotation.ToString(), Weight, *ArmRotation.ToString());
			break;
		}
		ArmRotation.Normalize();
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
	
	// 控制旋转参数
	if (OtherFlags.bControlRotationOffset)
	{
		FRotator OldControlRotationOffset = ControlRotationOffset;
		switch (BlendMode)
		{
		case ENamiCameraBlendMode::Additive:
			// Additive: 使用 FindDeltaAngleDegrees 确保选择最短路径，避免跨越 360 度边界跳变
			{
				FRotator AdditiveValue = Other.ControlRotationOffset * Weight;
				FRotator RawTarget;
				RawTarget.Pitch = ControlRotationOffset.Pitch + AdditiveValue.Pitch;
				RawTarget.Yaw = ControlRotationOffset.Yaw + AdditiveValue.Yaw;
				RawTarget.Roll = ControlRotationOffset.Roll + AdditiveValue.Roll;
				RawTarget.Normalize();
				
				ControlRotationOffset.Pitch = ControlRotationOffset.Pitch + FMath::FindDeltaAngleDegrees(ControlRotationOffset.Pitch, RawTarget.Pitch);
				ControlRotationOffset.Yaw = ControlRotationOffset.Yaw + FMath::FindDeltaAngleDegrees(ControlRotationOffset.Yaw, RawTarget.Yaw);
				ControlRotationOffset.Roll = ControlRotationOffset.Roll + FMath::FindDeltaAngleDegrees(ControlRotationOffset.Roll, RawTarget.Roll);
			}
			break;
		case ENamiCameraBlendMode::Override:
			// Override: 使用 FindDeltaAngleDegrees 确保选择最短路径，避免跨越 360 度边界跳变
			{
				FRotator NormalizedTarget = Other.ControlRotationOffset;
				NormalizedTarget.Normalize();
				
				ControlRotationOffset.Pitch = ControlRotationOffset.Pitch + FMath::FindDeltaAngleDegrees(ControlRotationOffset.Pitch, NormalizedTarget.Pitch) * Weight;
				ControlRotationOffset.Yaw = ControlRotationOffset.Yaw + FMath::FindDeltaAngleDegrees(ControlRotationOffset.Yaw, NormalizedTarget.Yaw) * Weight;
				ControlRotationOffset.Roll = ControlRotationOffset.Roll + FMath::FindDeltaAngleDegrees(ControlRotationOffset.Roll, NormalizedTarget.Roll) * Weight;
			}
			break;
		}
		ControlRotationOffset.Normalize();
		ChangedFlags.bControlRotationOffset = true;
		
		NAMI_LOG_STATE(VeryVerbose, TEXT("[FNamiCameraState::ApplyChanged] ControlRotationOffset混合: 旧值=%s, 新值=%s, 目标值=%s, 模式=%d, 权重=%.3f"),
			*OldControlRotationOffset.ToString(), *ControlRotationOffset.ToString(), *Other.ControlRotationOffset.ToString(), (int32)BlendMode, Weight);
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
		switch (BlendMode)
		{
		case ENamiCameraBlendMode::Additive:
			// Additive: 使用 FindDeltaAngleDegrees 确保选择最短路径，避免跨越 360 度边界跳变
			{
				FRotator AdditiveValue = Other.CameraRotationOffset * Weight;
				FRotator RawTarget;
				RawTarget.Pitch = CameraRotationOffset.Pitch + AdditiveValue.Pitch;
				RawTarget.Yaw = CameraRotationOffset.Yaw + AdditiveValue.Yaw;
				RawTarget.Roll = CameraRotationOffset.Roll + AdditiveValue.Roll;
				RawTarget.Normalize();
				
				CameraRotationOffset.Pitch = CameraRotationOffset.Pitch + FMath::FindDeltaAngleDegrees(CameraRotationOffset.Pitch, RawTarget.Pitch);
				CameraRotationOffset.Yaw = CameraRotationOffset.Yaw + FMath::FindDeltaAngleDegrees(CameraRotationOffset.Yaw, RawTarget.Yaw);
				CameraRotationOffset.Roll = CameraRotationOffset.Roll + FMath::FindDeltaAngleDegrees(CameraRotationOffset.Roll, RawTarget.Roll);
			}
			break;
		case ENamiCameraBlendMode::Override:
			// Override: 使用 FindDeltaAngleDegrees 确保选择最短路径，避免跨越 360 度边界跳变
			{
				FRotator NormalizedTarget = Other.CameraRotationOffset;
				NormalizedTarget.Normalize();
				
				CameraRotationOffset.Pitch = CameraRotationOffset.Pitch + FMath::FindDeltaAngleDegrees(CameraRotationOffset.Pitch, NormalizedTarget.Pitch) * Weight;
				CameraRotationOffset.Yaw = CameraRotationOffset.Yaw + FMath::FindDeltaAngleDegrees(CameraRotationOffset.Yaw, NormalizedTarget.Yaw) * Weight;
				CameraRotationOffset.Roll = CameraRotationOffset.Roll + FMath::FindDeltaAngleDegrees(CameraRotationOffset.Roll, NormalizedTarget.Roll) * Weight;
			}
			break;
		}
		CameraRotationOffset.Normalize();
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
	
	NAMI_LOG_STATE(Verbose, TEXT("[FNamiCameraState::ApplyChanged] 混合完成: 当前状态 - PivotRotation=%s, ArmRotation=%s, ArmLength=%.2f, ControlRotationOffset=%s"),
		*PivotRotation.ToString(), *ArmRotation.ToString(), ArmLength, *ControlRotationOffset.ToString());
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
	// 控制旋转参数
	ControlRotationOffset = FMath::Lerp(ControlRotationOffset, To.ControlRotationOffset, Alpha);
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
	// 控制旋转参数
	if (EffectiveMask.bControlRotationOffset)
	{
		ControlRotationOffset = FMath::Lerp(ControlRotationOffset, To.ControlRotationOffset, Alpha);
		ChangedFlags.bControlRotationOffset = true;
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
	// 控制旋转参数
	ControlRotationOffset = FRotator::ZeroRotator;
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

void FNamiCameraState::ApplyFramingResult(const FNamiCameraFramingResult& Result)
{
	if (!Result.bHasResult)
	{
		return;
	}

	// 应用推荐中心 -> PivotLocation
	PivotLocation = Result.TargetCenter;
	ChangedFlags.bPivotLocation = true;

	// 应用推荐吊臂长度
	if (Result.RecommendedArmLength > 0.0f)
	{
		ArmLength = Result.RecommendedArmLength;
		ChangedFlags.bArmLength = true;
	}

	// 应用推荐吊臂旋转
	if (!Result.RecommendedArmRotation.IsNearlyZero())
	{
		ArmRotation = Result.RecommendedArmRotation;
		ArmRotation.Normalize();
		ChangedFlags.bArmRotation = true;
	}

	// 额外相机偏移（可选）
	if (!Result.RecommendedCameraOffset.IsNearlyZero())
	{
		CameraLocationOffset = Result.RecommendedCameraOffset;
		ChangedFlags.bCameraLocationOffset = true;
	}
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

void FNamiCameraState::SetControlRotationOffset(const FRotator& InValue)
{
	ControlRotationOffset = InValue;
	ChangedFlags.bControlRotationOffset = true;
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
	PivotRotation += InDelta;
	PivotRotation.Normalize();
	ChangedFlags.bPivotRotation = true;
}

void FNamiCameraState::AddArmLength(float InDelta)
{
	ArmLength = FMath::Max(0.0f, ArmLength + InDelta);
	ChangedFlags.bArmLength = true;
}

void FNamiCameraState::AddArmRotation(const FRotator& InDelta)
{
	ArmRotation += InDelta;
	ArmRotation.Normalize();
	ChangedFlags.bArmRotation = true;
}

void FNamiCameraState::AddArmOffset(const FVector& InDelta)
{
	ArmOffset += InDelta;
	ChangedFlags.bArmOffset = true;
}

void FNamiCameraState::AddControlRotationOffset(const FRotator& InDelta)
{
	ControlRotationOffset += InDelta;
	ControlRotationOffset.Normalize();
	ChangedFlags.bControlRotationOffset = true;
}

void FNamiCameraState::AddCameraLocationOffset(const FVector& InDelta)
{
	CameraLocationOffset += InDelta;
	ChangedFlags.bCameraLocationOffset = true;
}

void FNamiCameraState::AddCameraRotationOffset(const FRotator& InDelta)
{
	CameraRotationOffset += InDelta;
	CameraRotationOffset.Normalize();
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

