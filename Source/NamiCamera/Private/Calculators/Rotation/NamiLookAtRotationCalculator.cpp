// Copyright Qiu, Inc. All Rights Reserved.

#include "Calculators/Rotation/NamiLookAtRotationCalculator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiLookAtRotationCalculator)

UNamiLookAtRotationCalculator::UNamiLookAtRotationCalculator()
{
	RotationSmoothSpeed = 8.0f;
}

FRotator UNamiLookAtRotationCalculator::CalculateCameraRotation_Implementation(
	const FVector& CameraLocation,
	const FVector& PivotLocation,
	const FRotator& ControlRotation,
	float DeltaTime)
{
	// 计算看向方向
	FVector Direction = PivotLocation - CameraLocation;
	if (Direction.IsNearlyZero())
	{
		return CurrentCameraRotation;
	}

	FRotator TargetRotation = Direction.Rotation();

	if (bLockRoll)
	{
		TargetRotation.Roll = 0.0f;
	}

	// 首帧直接设置
	if (!bFirstFrameProcessed)
	{
		CurrentCameraRotation = TargetRotation;
		bFirstFrameProcessed = true;
		return TargetRotation;
	}

	// 平滑过渡
	if (RotationSmoothSpeed > 0.0f && DeltaTime > 0.0f)
	{
		CurrentCameraRotation = FMath::RInterpTo(CurrentCameraRotation, TargetRotation, DeltaTime, RotationSmoothSpeed);
	}
	else
	{
		CurrentCameraRotation = TargetRotation;
	}

	return CurrentCameraRotation;
}
