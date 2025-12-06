// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modifiers/NamiCameraShakeModifier.h"
#include "Camera/CameraShakeBase.h"
#include "LogNamiCamera.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraShakeModifier)

UNamiCameraShakeModifier::UNamiCameraShakeModifier()
{
}

bool UNamiCameraShakeModifier::ApplyEffect(FMinimalViewInfo& InOutPOV, float Weight, float DeltaTime)
{
	// 如果震动待启动且 CameraOwner 现在可用，启动震动
	if (bPendingShakeStart && CameraOwner && bPlayShakeOnStart && CameraShake)
	{
		ActiveShakeInstance = CameraOwner->StartCameraShake(CameraShake, ShakeScale);
		
		if (ActiveShakeInstance.IsValid())
		{
			UE_LOG(LogNamiCamera, Log, TEXT("[UNamiCameraShakeModifier] 延迟启动相机震动成功：%s，强度：%.2f"), 
				*CameraShake->GetName(), ShakeScale);
		}
		else
		{
			UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraShakeModifier] 延迟启动相机震动失败：%s，强度：%.2f，StartCameraShake 返回 nullptr"), 
				*CameraShake->GetName(), ShakeScale);
		}
		bPendingShakeStart = false;
	}
	
	// 震动是通过 StartCameraShake 启动的，它会在 APlayerCameraManager::UpdateCamera 中自动应用
	// 我们只需要保持 Modifier 激活状态即可
	return true;
}

bool UNamiCameraShakeModifier::ShouldKeepActive(float Weight) const
{
	// 如果有激活的震动，即使权重为 0 也要保持激活
	return ActiveShakeInstance.IsValid();
}

void UNamiCameraShakeModifier::AddedToCamera(APlayerCameraManager* Camera)
{
	Super::AddedToCamera(Camera);
}

void UNamiCameraShakeModifier::ActivateEffect(bool bResetTimer)
{
	Super::ActivateEffect(bResetTimer);
	
	// 播放相机震动
	if (bPlayShakeOnStart && CameraShake)
	{
		if (CameraOwner)
		{
			// CameraOwner 已设置，立即启动震动
			ActiveShakeInstance = CameraOwner->StartCameraShake(CameraShake, ShakeScale);
			
			if (ActiveShakeInstance.IsValid())
			{
				UE_LOG(LogNamiCamera, Log, TEXT("[UNamiCameraShakeModifier] 播放相机震动成功：%s，强度：%.2f，实例类：%s"), 
					*CameraShake->GetName(), ShakeScale,
					*ActiveShakeInstance->GetClass()->GetName());
			}
			else
			{
				UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraShakeModifier] 播放相机震动失败：%s，强度：%.2f，StartCameraShake 返回 nullptr"), 
					*CameraShake->GetName(), ShakeScale);
			}
			bPendingShakeStart = false;
		}
		else
		{
			// CameraOwner 未设置，标记为待启动，将在 ApplyEffect 中启动
			bPendingShakeStart = true;
			UE_LOG(LogNamiCamera, Verbose, TEXT("[UNamiCameraShakeModifier] CameraOwner 未设置，震动将在 ApplyEffect 中延迟启动：%s"), 
				*CameraShake->GetName());
		}
	}
	else
	{
		bPendingShakeStart = false;
		if (bPlayShakeOnStart && CameraShake)
		{
			UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraShakeModifier] 未播放相机震动：bPlayShakeOnStart=%s, CameraShake=%s, CameraOwner=%s"), 
				bPlayShakeOnStart ? TEXT("true") : TEXT("false"),
				CameraShake ? *CameraShake->GetName() : TEXT("nullptr"),
				CameraOwner ? TEXT("Valid") : TEXT("nullptr"));
		}
	}
}

void UNamiCameraShakeModifier::DeactivateEffect(bool bForceImmediate)
{
	// 清除待启动标志
	bPendingShakeStart = false;
	
	// 停止相机震动
	if (ActiveShakeInstance.IsValid() && CameraOwner)
	{
		CameraOwner->StopCameraShake(ActiveShakeInstance.Get(), bForceImmediate);
		ActiveShakeInstance = nullptr;
	}
	
	Super::DeactivateEffect(bForceImmediate);
}

