// Copyright Epic Games, Inc. All Rights Reserved.

#include "CameraFeatures/NamiCameraFeature.h"
#include "CameraModes/NamiCameraModeBase.h"
#include "Data/NamiCameraPipelineContext.h"

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

void UNamiCameraFeature::ApplyToViewWithContext_Implementation(
	FNamiCameraView& InOutView,
	float DeltaTime,
	FNamiCameraPipelineContext& Context)
{
	// 默认实现：调用 ApplyToView（向后兼容）
	ApplyToView(InOutView, DeltaTime);
}

UWorld* UNamiCameraFeature::GetWorld() const
{
	if (CameraMode.IsValid())
	{
		return CameraMode->GetWorld();
	}
	return nullptr;
}

