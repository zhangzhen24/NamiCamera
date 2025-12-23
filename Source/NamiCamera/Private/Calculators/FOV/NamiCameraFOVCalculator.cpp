// Copyright Qiu, Inc. All Rights Reserved.

#include "Calculators/FOV/NamiCameraFOVCalculator.h"
#include "CameraModes/NamiCameraModeBase.h"

UNamiCameraFOVCalculator::UNamiCameraFOVCalculator()
	: bFirstFrameProcessed(false)
{
	BaseFOV = 90.0f;
	MinFOV = 60.0f;
	MaxFOV = 100.0f;
	FOVTransitionSpeed = 5.0f;
	CurrentFOV = 90.0f;
}

void UNamiCameraFOVCalculator::Activate_Implementation()
{
	Super::Activate_Implementation();
	bFirstFrameProcessed = false;
	CurrentFOV = BaseFOV;
}

float UNamiCameraFOVCalculator::CalculateFOV_Implementation(
	const FVector& CameraLocation,
	const FVector& PivotLocation,
	float DeltaTime)
{
	// 默认实现：返回基础 FOV
	if (!bFirstFrameProcessed)
	{
		CurrentFOV = BaseFOV;
		bFirstFrameProcessed = true;
	}

	return CurrentFOV;
}
