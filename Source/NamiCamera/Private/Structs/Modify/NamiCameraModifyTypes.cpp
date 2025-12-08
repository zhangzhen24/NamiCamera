// Copyright Epic Games, Inc. All Rights Reserved.

#include "Structs/Modify/NamiCameraModifyTypes.h"
#include "LogNamiCamera.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraModifyTypes)

// ==================== FNamiFloatModify ====================

float FNamiFloatModify::Apply(float InValue, float Weight) const
{
	if (!bEnabled)
	{
		return InValue;
	}
	
	switch (BlendMode)
	{
	case ENamiCameraBlendMode::Additive:
		return InValue + Value * Weight;
		
	case ENamiCameraBlendMode::Override:
		return FMath::Lerp(InValue, Value, Weight);
		
	default:
		return InValue;
	}
}

// ==================== FNamiVectorModify ====================

FVector FNamiVectorModify::Apply(const FVector& InValue, float Weight) const
{
	if (!bEnabled)
	{
		return InValue;
	}
	
	switch (BlendMode)
	{
	case ENamiCameraBlendMode::Additive:
		return InValue + Value * Weight;
		
	case ENamiCameraBlendMode::Override:
		return FMath::Lerp(InValue, Value, Weight);
		
	default:
		return InValue;
	}
}

// ==================== FNamiRotatorModify ====================

FRotator FNamiRotatorModify::Apply(const FRotator& InValue, float Weight) const
{
	if (!bEnabled)
	{
		return InValue;
	}
	
	FRotator Result;
	
	switch (BlendMode)
	{
	case ENamiCameraBlendMode::Additive:
		// Additive: 在当前值基础上增加偏移
		// 使用 FindDeltaAngleDegrees 确保选择最短路径，避免跨越 360 度边界跳变
		{
			FRotator AdditiveValue = Value * Weight;
			FRotator RawTarget;
			RawTarget.Pitch = InValue.Pitch + AdditiveValue.Pitch;
			RawTarget.Yaw = InValue.Yaw + AdditiveValue.Yaw;
			RawTarget.Roll = InValue.Roll + AdditiveValue.Roll;
			RawTarget.Normalize();
			
			Result.Pitch = InValue.Pitch + FMath::FindDeltaAngleDegrees(InValue.Pitch, RawTarget.Pitch);
			Result.Yaw = InValue.Yaw + FMath::FindDeltaAngleDegrees(InValue.Yaw, RawTarget.Yaw);
			Result.Roll = InValue.Roll + FMath::FindDeltaAngleDegrees(InValue.Roll, RawTarget.Roll);
		}
		break;
		
	case ENamiCameraBlendMode::Override:
		// Override: 从当前值过渡到目标值（覆盖）
		// 使用 FindDeltaAngleDegrees 确保选择最短路径，避免跨越 360 度边界跳变
		{
			FRotator NormalizedValue = Value;
			NormalizedValue.Normalize();
			
			Result.Pitch = InValue.Pitch + FMath::FindDeltaAngleDegrees(InValue.Pitch, NormalizedValue.Pitch) * Weight;
			Result.Yaw = InValue.Yaw + FMath::FindDeltaAngleDegrees(InValue.Yaw, NormalizedValue.Yaw) * Weight;
			Result.Roll = InValue.Roll + FMath::FindDeltaAngleDegrees(InValue.Roll, NormalizedValue.Roll) * Weight;
		}
		break;
		
	default:
		Result = InValue;
		break;
	}
	
	Result.Normalize();
	return Result;
}

// ==================== FNamiCameraStateModify ====================

void FNamiCameraStateModify::ApplyToState(FNamiCameraState& InOutState, float Weight) const
{
	ApplyInputToState(InOutState, Weight);
	ApplyOutputToState(InOutState, Weight);
}

void FNamiCameraStateModify::ApplyInputToState(FNamiCameraState& InOutState, float Weight) const
{
	if (PivotLocation.bEnabled)
	{
		InOutState.PivotLocation = PivotLocation.Apply(InOutState.PivotLocation, Weight);
		InOutState.ChangedFlags.bPivotLocation = true;
	}
	
	if (PivotRotation.bEnabled)
	{
		InOutState.PivotRotation = PivotRotation.Apply(InOutState.PivotRotation, Weight);
		InOutState.ChangedFlags.bPivotRotation = true;
	}
	
	if (ArmLength.bEnabled)
	{
		InOutState.ArmLength = FMath::Max(0.0f, ArmLength.Apply(InOutState.ArmLength, Weight));
		InOutState.ChangedFlags.bArmLength = true;
	}
	
	if (ArmRotation.bEnabled)
	{
		InOutState.ArmRotation = ArmRotation.Apply(InOutState.ArmRotation, Weight);
		InOutState.ChangedFlags.bArmRotation = true;
	}
	
	if (ArmOffset.bEnabled)
	{
		InOutState.ArmOffset = ArmOffset.Apply(InOutState.ArmOffset, Weight);
		InOutState.ChangedFlags.bArmOffset = true;
	}
	
	// 控制旋转参数
	if (ControlRotationOffset.bEnabled)
	{
		InOutState.ControlRotationOffset = ControlRotationOffset.Apply(InOutState.ControlRotationOffset, Weight);
		InOutState.ChangedFlags.bControlRotationOffset = true;
	}
	
	// 相机参数
	if (CameraLocationOffset.bEnabled)
	{
		InOutState.CameraLocationOffset = CameraLocationOffset.Apply(InOutState.CameraLocationOffset, Weight);
		InOutState.ChangedFlags.bCameraLocationOffset = true;
	}
	
	if (CameraRotationOffset.bEnabled)
	{
		InOutState.CameraRotationOffset = CameraRotationOffset.Apply(InOutState.CameraRotationOffset, Weight);
		InOutState.ChangedFlags.bCameraRotationOffset = true;
	}
	
	if (FieldOfView.bEnabled)
	{
		InOutState.FieldOfView = FMath::Clamp(
			FieldOfView.Apply(InOutState.FieldOfView, Weight),
			5.0f, 170.0f);
		InOutState.ChangedFlags.bFieldOfView = true;
	}
}

void FNamiCameraStateModify::ApplyOutputToState(FNamiCameraState& InOutState, float Weight) const
{
	if (CameraLocation.bEnabled)
	{
		InOutState.CameraLocation = CameraLocation.Apply(InOutState.CameraLocation, Weight);
		InOutState.ChangedFlags.bCameraLocation = true;
	}
	
	if (CameraRotation.bEnabled)
	{
		InOutState.CameraRotation = CameraRotation.Apply(InOutState.CameraRotation, Weight);
		InOutState.ChangedFlags.bCameraRotation = true;
	}
}

bool FNamiCameraStateModify::HasAnyModification() const
{
	return HasInputModification() || HasOutputModification();
}

bool FNamiCameraStateModify::HasInputModification() const
{
	return PivotLocation.bEnabled || PivotRotation.bEnabled ||
		   ArmLength.bEnabled || ArmRotation.bEnabled || ArmOffset.bEnabled ||
		   ControlRotationOffset.bEnabled ||
		   CameraLocationOffset.bEnabled || CameraRotationOffset.bEnabled || FieldOfView.bEnabled;
}

bool FNamiCameraStateModify::HasOutputModification() const
{
	return CameraLocation.bEnabled || CameraRotation.bEnabled;
}

