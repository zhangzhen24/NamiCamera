// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/NamiCameraView.h"

#include "Math/NamiCameraMath.h"

FNamiCameraView::FNamiCameraView()
{
	PivotLocation = FVector::ZeroVector;
	CameraLocation = FVector::ZeroVector;
	CameraRotation = FRotator::ZeroRotator;
	ControlLocation = FVector::ZeroVector;
	ControlRotation = FRotator::ZeroRotator;
	FOV = 80.0f;
}

void FNamiCameraView::Blend(const FNamiCameraView& Other, float OtherWeight)
{
	if (OtherWeight <= 0.f)
	{
		return;
	}

	if (OtherWeight >= 1.f)
	{
		*this = Other;
		return;
	}
	
	// 计算相机相对于 Pivot 的偏移旋转
	const FQuat OffsetRotation = (CameraLocation - PivotLocation).GetSafeNormal().Rotation().Quaternion();
	const FQuat OtherOffsetRotation = (Other.CameraLocation - Other.PivotLocation).GetSafeNormal().Rotation().Quaternion();
	const FQuat DiffRotation = FMath::Lerp(OffsetRotation, OtherOffsetRotation, OtherWeight);

	// 计算相机距离
	const float CameraDistance = FVector::Distance(CameraLocation, PivotLocation);
	const float OtherCameraDistance = FVector::Distance(Other.CameraLocation, Other.PivotLocation);
	const float NewCameraDistance = FMath::Lerp(CameraDistance, OtherCameraDistance, OtherWeight);

	// 混合 Pivot Location
	PivotLocation = FMath::Lerp(PivotLocation, Other.PivotLocation, OtherWeight);

	// 基于新的 PivotLocation 和混合后的旋转、距离重新计算 Camera Location
	CameraLocation = PivotLocation + DiffRotation.Vector() * NewCameraDistance;

	// 混合 Camera Rotation（优化：只在混合前归一化一次，混合后归一化一次）
	FRotator NormalizedCurrentCameraRot = FNamiCameraMath::NormalizeRotatorTo360(CameraRotation);
	FRotator NormalizedOtherCameraRot = FNamiCameraMath::NormalizeRotatorTo360(Other.CameraRotation);
	
	float CameraPitchDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrentCameraRot.Pitch, NormalizedOtherCameraRot.Pitch);
	float CameraYawDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrentCameraRot.Yaw, NormalizedOtherCameraRot.Yaw);
	float CameraRollDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrentCameraRot.Roll, NormalizedOtherCameraRot.Roll);
	
	// 应用差值（结果会在最后归一化）
	CameraRotation.Pitch = NormalizedCurrentCameraRot.Pitch + CameraPitchDelta * OtherWeight;
	CameraRotation.Yaw = NormalizedCurrentCameraRot.Yaw + CameraYawDelta * OtherWeight;
	CameraRotation.Roll = NormalizedCurrentCameraRot.Roll + CameraRollDelta * OtherWeight;
	// 确保结果在0-360度范围（只归一化一次）
	CameraRotation = FNamiCameraMath::NormalizeRotatorTo360(CameraRotation);

	// 混合 Control Location
	ControlLocation = FMath::Lerp(ControlLocation, Other.ControlLocation, OtherWeight);

	// 混合 Control Rotation（优化：只在混合前归一化一次，混合后归一化一次）
	FRotator NormalizedCurrentControlRot = FNamiCameraMath::NormalizeRotatorTo360(ControlRotation);
	FRotator NormalizedOtherControlRot = FNamiCameraMath::NormalizeRotatorTo360(Other.ControlRotation);
	
	float ControlPitchDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrentControlRot.Pitch, NormalizedOtherControlRot.Pitch);
	float ControlYawDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrentControlRot.Yaw, NormalizedOtherControlRot.Yaw);
	float ControlRollDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrentControlRot.Roll, NormalizedOtherControlRot.Roll);
	
	// 应用差值（结果会在最后归一化）
	ControlRotation.Pitch = NormalizedCurrentControlRot.Pitch + ControlPitchDelta * OtherWeight;
	ControlRotation.Yaw = NormalizedCurrentControlRot.Yaw + ControlYawDelta * OtherWeight;
	ControlRotation.Roll = NormalizedCurrentControlRot.Roll + ControlRollDelta * OtherWeight;
	// 确保结果在0-360度范围（只归一化一次）
	ControlRotation = FNamiCameraMath::NormalizeRotatorTo360(ControlRotation);

	// 混合 FOV
	FOV = FMath::Lerp(FOV, Other.FOV, OtherWeight);
}

