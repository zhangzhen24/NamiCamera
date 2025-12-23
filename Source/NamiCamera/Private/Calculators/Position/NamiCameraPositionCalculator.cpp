// Copyright Qiu, Inc. All Rights Reserved.

#include "Calculators/Position/NamiCameraPositionCalculator.h"
#include "CameraModes/NamiCameraModeBase.h"

UNamiCameraPositionCalculator::UNamiCameraPositionCalculator()
	: bFirstFrameProcessed(false)
{
	CameraOffset = FVector(-300.0f, 0.0f, 100.0f);
	bUseControlRotation = true;
	bUseYawOnly = false;
	CurrentCameraPosition = FVector::ZeroVector;
}

void UNamiCameraPositionCalculator::Activate_Implementation()
{
	Super::Activate_Implementation();
	bFirstFrameProcessed = false;
}

FVector UNamiCameraPositionCalculator::CalculateCameraPosition_Implementation(
	const FVector& PivotLocation,
	const FRotator& ControlRotation,
	float DeltaTime)
{
	// 默认实现：简单偏移
	FRotator RotationToUse = bUseControlRotation ? ControlRotation : FRotator::ZeroRotator;

	if (bUseYawOnly)
	{
		RotationToUse.Pitch = 0.0f;
		RotationToUse.Roll = 0.0f;
	}

	FVector RotatedOffset = RotationToUse.RotateVector(CameraOffset);
	CurrentCameraPosition = PivotLocation + RotatedOffset;

	bFirstFrameProcessed = true;
	return CurrentCameraPosition;
}
