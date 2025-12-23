// Copyright Qiu, Inc. All Rights Reserved.

#include "ModeComponents/NamiCameraShakeComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/NamiCameraComponent.h"
#include "Components/NamiPlayerCameraManager.h"
#include "CameraModes/NamiCameraModeBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraShakeComponent)

UNamiCameraShakeComponent::UNamiCameraShakeComponent()
{
	ComponentName = TEXT("Shake");
	Priority = 10;
}

void UNamiCameraShakeComponent::Initialize_Implementation(UNamiCameraModeBase* InCameraMode)
{
	Super::Initialize_Implementation(InCameraMode);
}

void UNamiCameraShakeComponent::Activate_Implementation()
{
	Super::Activate_Implementation();

	// 如果配置了 CameraShake，自动启动
	if (CameraShake)
	{
		StartShake();
	}
}

void UNamiCameraShakeComponent::Deactivate_Implementation()
{
	// 停用时停止抖动
	StopShake(false);

	Super::Deactivate_Implementation();
}

void UNamiCameraShakeComponent::StartShake()
{
	// 检查 CameraShake 是否有效
	if (!CameraShake)
	{
		return;
	}

	// 获取 PlayerCameraManager
	APlayerCameraManager* CameraManager = GetPlayerCameraManager();
	if (!CameraManager)
	{
		return;
	}

	// 如果已经有激活的抖动，先停止
	if (ActiveShakeInstance.IsValid())
	{
		StopShake(true);
	}

	// 启动新的 CameraShake
	ActiveShakeInstance = CameraManager->StartCameraShake(CameraShake, ShakeScale);
}

void UNamiCameraShakeComponent::StopShake(bool bImmediate)
{
	// 检查是否有激活的抖动实例
	if (!ActiveShakeInstance.IsValid())
	{
		return;
	}

	// 获取 PlayerCameraManager
	APlayerCameraManager* CameraManager = GetPlayerCameraManager();
	if (CameraManager)
	{
		// 停止 CameraShake
		CameraManager->StopCameraShake(ActiveShakeInstance.Get(), bImmediate);
	}

	// 清空引用
	ActiveShakeInstance = nullptr;
}

APlayerCameraManager* UNamiCameraShakeComponent::GetPlayerCameraManager() const
{
	// 通过 CameraMode -> CameraComponent -> PlayerCameraManager 获取
	if (UNamiCameraModeBase* Mode = GetCameraMode())
	{
		if (UNamiCameraComponent* CameraComp = Mode->GetCameraComponent())
		{
			if (ANamiPlayerCameraManager* NamiCameraManager = CameraComp->GetOwnerPlayerCameraManager())
			{
				return NamiCameraManager;
			}
		}
	}

	return nullptr;
}
