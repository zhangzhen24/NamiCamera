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

	// 混合 Camera Rotation（使用 DeltaRotation 方式）
	const FRotator DeltaRotation = (Other.CameraRotation - CameraRotation).GetNormalized();
	CameraRotation = CameraRotation + (OtherWeight * DeltaRotation);

	// 混合 Control Location
	ControlLocation = FMath::Lerp(ControlLocation, Other.ControlLocation, OtherWeight);

	// 混合 Control Rotation（使用 DeltaRotation 方式）
	const FRotator DeltaControlRotation = (Other.ControlRotation - ControlRotation).GetNormalized();
	ControlRotation = ControlRotation + (OtherWeight * DeltaControlRotation);

	// 混合 FOV
	FOV = FMath::Lerp(FOV, Other.FOV, OtherWeight);
}

