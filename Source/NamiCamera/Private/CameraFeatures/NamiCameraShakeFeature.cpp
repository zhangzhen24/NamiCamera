// Copyright Qiu, Inc. All Rights Reserved.

#include "CameraFeatures/NamiCameraShakeFeature.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/NamiCameraComponent.h"
#include "Components/NamiPlayerCameraManager.h"
#include "CameraModes/NamiCameraModeBase.h"

UNamiCameraShakeFeature::UNamiCameraShakeFeature()
{
    // 设置默认优先级
    Priority = 10;
}

void UNamiCameraShakeFeature::Initialize_Implementation(UNamiCameraModeBase* InCameraMode)
{
    Super::Initialize_Implementation(InCameraMode);
}

void UNamiCameraShakeFeature::Activate_Implementation()
{
    Super::Activate_Implementation();
    
    // 如果配置了 CameraShake，自动启动
    if (CameraShake)
    {
        StartShake();
    }
}

void UNamiCameraShakeFeature::Deactivate_Implementation()
{
    // 停用时停止抖动
    StopShake(false);
    
    Super::Deactivate_Implementation();
}

void UNamiCameraShakeFeature::StartShake()
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

void UNamiCameraShakeFeature::StopShake(bool bImmediate)
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

APlayerCameraManager* UNamiCameraShakeFeature::GetPlayerCameraManager() const
{
    // 通过 CameraMode -> CameraComponent -> PlayerCameraManager 获取
    if (UNamiCameraModeBase* Mode = GetCameraMode())
    {
        if (UNamiCameraComponent* CameraComponent = Mode->GetCameraComponent())
        {
            if (ANamiPlayerCameraManager* NamiCameraManager = CameraComponent->GetOwnerPlayerCameraManager())
            {
                return NamiCameraManager;
            }
        }
    }

    return nullptr;
}