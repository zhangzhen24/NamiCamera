// Copyright Qiu, Inc. All Rights Reserved.

#include "Calculators/NamiCameraRotationCalculator.h"
#include "CameraModes/NamiCameraModeBase.h"
#include "GameFramework/PlayerController.h"

UNamiCameraRotationCalculator::UNamiCameraRotationCalculator()
	: bFirstFrameProcessed(false)
{
	RotationSmoothSpeed = 10.0f;
	bLockRoll = true;
	CurrentCameraRotation = FRotator::ZeroRotator;
}

void UNamiCameraRotationCalculator::Activate_Implementation()
{
	Super::Activate_Implementation();
	bFirstFrameProcessed = false;
}

FRotator UNamiCameraRotationCalculator::CalculateCameraRotation_Implementation(
	const FVector& CameraLocation,
	const FVector& PivotLocation,
	const FRotator& ControlRotation,
	float DeltaTime)
{
	// 默认实现：使用控制旋转
	CurrentCameraRotation = ControlRotation;

	if (bLockRoll)
	{
		CurrentCameraRotation.Roll = 0.0f;
	}

	bFirstFrameProcessed = true;
	return CurrentCameraRotation;
}

FRotator UNamiCameraRotationCalculator::GetControlRotation_Implementation() const
{
	if (UNamiCameraModeBase* Mode = GetCameraMode())
	{
		if (APawn* Pawn = Cast<APawn>(Mode->GetOwnerActor()))
		{
			if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
			{
				return PC->GetControlRotation();
			}
		}
	}
	return FRotator::ZeroRotator;
}
