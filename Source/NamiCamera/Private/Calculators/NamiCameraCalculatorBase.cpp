// Copyright Qiu, Inc. All Rights Reserved.

#include "Calculators/NamiCameraCalculatorBase.h"
#include "CameraModes/NamiCameraModeBase.h"

UNamiCameraCalculatorBase::UNamiCameraCalculatorBase()
	: bInitialized(false)
	, bIsActive(false)
{
}

void UNamiCameraCalculatorBase::Initialize_Implementation(UNamiCameraModeBase* InCameraMode)
{
	CameraMode = InCameraMode;
	bInitialized = true;
}

void UNamiCameraCalculatorBase::Activate_Implementation()
{
	bIsActive = true;
}

void UNamiCameraCalculatorBase::Deactivate_Implementation()
{
	bIsActive = false;
}

UWorld* UNamiCameraCalculatorBase::GetWorld() const
{
	if (CameraMode.IsValid())
	{
		return CameraMode->GetWorld();
	}
	return nullptr;
}
