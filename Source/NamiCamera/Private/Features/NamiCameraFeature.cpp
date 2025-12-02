// Copyright Epic Games, Inc. All Rights Reserved.

#include "Features/NamiCameraFeature.h"
#include "Modes/NamiCameraModeBase.h"

UNamiCameraFeature::UNamiCameraFeature()
	: Priority(0)
	, bEnabled(true)
{
}

void UNamiCameraFeature::Initialize_Implementation(UNamiCameraModeBase* InCameraMode)
{
	CameraMode = InCameraMode;
}

void UNamiCameraFeature::Activate_Implementation()
{
}

void UNamiCameraFeature::Deactivate_Implementation()
{
}

void UNamiCameraFeature::Update_Implementation(float DeltaTime)
{
}

void UNamiCameraFeature::ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime)
{
}

UWorld* UNamiCameraFeature::GetWorld() const
{
	if (CameraMode.IsValid())
	{
		return CameraMode->GetWorld();
	}
	return nullptr;
}

