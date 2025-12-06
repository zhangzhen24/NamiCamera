// Copyright Epic Games, Inc. All Rights Reserved.

#include "Structs/View/NamiCameraView.h"

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

	// 混合 Camera Rotation（使用 FindDeltaAngleDegrees 确保选择最短路径，避免跨越 360 度边界跳变）
	FRotator TargetCameraRotation;
	TargetCameraRotation.Pitch = CameraRotation.Pitch + FMath::FindDeltaAngleDegrees(CameraRotation.Pitch, Other.CameraRotation.Pitch) * OtherWeight;
	TargetCameraRotation.Yaw = CameraRotation.Yaw + FMath::FindDeltaAngleDegrees(CameraRotation.Yaw, Other.CameraRotation.Yaw) * OtherWeight;
	TargetCameraRotation.Roll = CameraRotation.Roll + FMath::FindDeltaAngleDegrees(CameraRotation.Roll, Other.CameraRotation.Roll) * OtherWeight;
	CameraRotation = TargetCameraRotation;
	CameraRotation.Normalize();

	// 混合 Control Location
	ControlLocation = FMath::Lerp(ControlLocation, Other.ControlLocation, OtherWeight);

	// 混合 Control Rotation（使用 FindDeltaAngleDegrees 确保选择最短路径，避免跨越 360 度边界跳变）
	FRotator TargetControlRotation;
	TargetControlRotation.Pitch = ControlRotation.Pitch + FMath::FindDeltaAngleDegrees(ControlRotation.Pitch, Other.ControlRotation.Pitch) * OtherWeight;
	TargetControlRotation.Yaw = ControlRotation.Yaw + FMath::FindDeltaAngleDegrees(ControlRotation.Yaw, Other.ControlRotation.Yaw) * OtherWeight;
	TargetControlRotation.Roll = ControlRotation.Roll + FMath::FindDeltaAngleDegrees(ControlRotation.Roll, Other.ControlRotation.Roll) * OtherWeight;
	ControlRotation = TargetControlRotation;
	ControlRotation.Normalize();

	// 混合 FOV
	FOV = FMath::Lerp(FOV, Other.FOV, OtherWeight);
}

