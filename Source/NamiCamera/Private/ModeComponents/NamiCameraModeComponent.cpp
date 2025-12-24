// Copyright Qiu, Inc. All Rights Reserved.

#include "ModeComponents/NamiCameraModeComponent.h"
#include "CameraModes/NamiCameraModeBase.h"
#include "Core/NamiCameraPipelineContext.h"

UNamiCameraModeComponent::UNamiCameraModeComponent()
	: Priority(0)
	, bEnabled(true)
{
}

void UNamiCameraModeComponent::Initialize_Implementation(UNamiCameraModeBase* InCameraMode)
{
	CameraMode = InCameraMode;
}

void UNamiCameraModeComponent::Activate_Implementation()
{
}

void UNamiCameraModeComponent::Deactivate_Implementation()
{
}

void UNamiCameraModeComponent::Update_Implementation(float DeltaTime)
{
}

void UNamiCameraModeComponent::ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime)
{
}

void UNamiCameraModeComponent::ApplyToViewWithContext_Implementation(
	FNamiCameraView& InOutView,
	float DeltaTime,
	FNamiCameraPipelineContext& Context)
{
	// 默认实现：调用 ApplyToView（向后兼容）
	ApplyToView(InOutView, DeltaTime);
}

UWorld* UNamiCameraModeComponent::GetWorld() const
{
	if (CameraMode.IsValid())
	{
		return CameraMode->GetWorld();
	}
	return nullptr;
}
