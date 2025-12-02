// Copyright Epic Games, Inc. All Rights Reserved.

#include "Notifies/AnimNotifyState_NamiCameraEffect.h"
#include "Modifiers/NamiCameraEffectModifier.h"
#include "Data/NamiCameraEffectPreset.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/NamiPlayerCameraManager.h"
#include "LogNamiCamera.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyState_NamiCameraEffect)

void UAnimNotifyState_NamiCameraEffect::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, 
													float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	if (!MeshComp)
	{
		return;
	}
	
	// 获取 PlayerCameraManager
	APlayerCameraManager* CameraManager = GetPlayerCameraManager(MeshComp);
	if (!CameraManager)
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[AnimNotifyState_NamiCameraEffect] NotifyBegin: 无法获取 PlayerCameraManager"));
		return;
	}
	
	// 创建修改器
	UNamiCameraEffectModifier* Modifier = CreateModifier(MeshComp);
	if (!Modifier)
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[AnimNotifyState_NamiCameraEffect] NotifyBegin: 创建修改器失败"));
		return;
	}
	
	// 设置持续时间（使用 ANS 的持续时间）
	Modifier->Duration = TotalDuration;
	
	// 添加到相机管理器（尝试使用 NamiPlayerCameraManager 的公共方法）
	ANamiPlayerCameraManager* NamiCameraManager = Cast<ANamiPlayerCameraManager>(CameraManager);
	if (NamiCameraManager)
	{
		NamiCameraManager->AddCameraModifier(Modifier);
	}
	else
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[AnimNotifyState_NamiCameraEffect] NotifyBegin: 不是 ANamiPlayerCameraManager，无法添加修改器"));
		return;
	}
	
	// 激活效果
	Modifier->ActivateEffect(false);
	
	// 保存引用
	ActiveModifier = Modifier;
	
	UE_LOG(LogNamiCamera, Log, TEXT("[AnimNotifyState_NamiCameraEffect] NotifyBegin: 激活相机效果 '%s'，持续时间：%.2f"), 
		*EffectName.ToString(), TotalDuration);
}

void UAnimNotifyState_NamiCameraEffect::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
												  const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	// 停止效果
	if (ActiveModifier.IsValid())
	{
		ActiveModifier->DeactivateEffect(false);
		ActiveModifier = nullptr;
		
		UE_LOG(LogNamiCamera, Log, TEXT("[AnimNotifyState_NamiCameraEffect] NotifyEnd: 停止相机效果 '%s'"), 
			*EffectName.ToString());
	}
}

FString UAnimNotifyState_NamiCameraEffect::GetNotifyName_Implementation() const
{
	if (EffectPreset)
	{
		return FString::Printf(TEXT("相机效果: %s"), *EffectPreset->PresetName.ToString());
	}
	else if (!EffectName.IsNone())
	{
		return FString::Printf(TEXT("相机效果: %s"), *EffectName.ToString());
	}
	else
	{
		return TEXT("相机效果");
	}
}

#if WITH_EDITOR
void UAnimNotifyState_NamiCameraEffect::ValidateAssociatedAssets()
{
	Super::ValidateAssociatedAssets();
	
	// 验证预设资产
	if (EffectPreset && EffectPreset->IsAsset())
	{
		// 预设资产有效
	}
}
#endif

UNamiCameraEffectModifier* UAnimNotifyState_NamiCameraEffect::CreateModifier(USkeletalMeshComponent* MeshComp)
{
	if (!MeshComp)
	{
		return nullptr;
	}
	
	// 创建修改器实例
	UNamiCameraEffectModifier* Modifier = NewObject<UNamiCameraEffectModifier>(MeshComp);
	if (!Modifier)
	{
		return nullptr;
	}
	
	// 设置效果名称
	Modifier->EffectName = EffectName;
	
	// 如果有预设，先应用预设
	if (EffectPreset)
	{
		ApplyPresetToModifier(Modifier);
	}
	
	// 应用直接配置（可能覆盖预设）
	ApplyDirectConfigToModifier(Modifier);
	
	return Modifier;
}

void UAnimNotifyState_NamiCameraEffect::ApplyPresetToModifier(UNamiCameraEffectModifier* Modifier)
{
	if (!Modifier || !EffectPreset)
	{
		return;
	}
	
	// 应用预设配置
	EffectPreset->ApplyToModifier(Modifier);
	
	UE_LOG(LogNamiCamera, Verbose, TEXT("[AnimNotifyState_NamiCameraEffect] 应用预设：%s"), 
		*EffectPreset->GetName());
}

void UAnimNotifyState_NamiCameraEffect::ApplyDirectConfigToModifier(UNamiCameraEffectModifier* Modifier)
{
	if (!Modifier)
	{
		return;
	}
	
	// 混合时间（总是应用，或者根据覆盖标志）
	if (!EffectPreset || bOverrideBlendTime)
	{
		Modifier->BlendInTime = BlendInTime;
		Modifier->BlendOutTime = BlendOutTime;
	}
	
	// 位置偏移
	if (bEnableLocationOffset && (!EffectPreset || bOverrideLocationOffset))
	{
		Modifier->bEnableLocationOffset = true;
		Modifier->LocationOffset = LocationOffset;
	}
	
	// 旋转偏移
	if (bEnableRotationOffset && (!EffectPreset || bOverrideRotationOffset))
	{
		Modifier->bEnableRotationOffset = true;
		Modifier->RotationOffset = RotationOffset;
	}
	
	// 距离偏移
	if (bEnableDistanceOffset && (!EffectPreset || bOverrideDistanceOffset))
	{
		Modifier->bEnableDistanceOffset = true;
		Modifier->DistanceOffset = DistanceOffset;
	}
	
	// FOV 偏移
	if (bEnableFOVOffset && (!EffectPreset || bOverrideFOVOffset))
	{
		Modifier->bEnableFOVOffset = true;
		Modifier->FOVOffset = FOVOffset;
	}
	
	// LookAt 目标
	if (bOverrideLookAtTarget)
	{
		Modifier->bOverrideLookAtTarget = true;
		Modifier->LookAtWeight = LookAtWeight;
		Modifier->LookAtOffset = LookAtOffset;
		
		// 解析目标
		AActor* Target = ResolveLookAtTarget(Cast<USkeletalMeshComponent>(Modifier->GetOuter()));
		if (Target)
		{
			Modifier->bUseLookAtActor = true;
			Modifier->LookAtTarget = Target;
		}
	}
	
	// 相机震动
	if (CameraShake && (!EffectPreset || bOverrideCameraShake))
	{
		Modifier->CameraShake = CameraShake;
		Modifier->ShakeScale = ShakeScale;
	}
}

AActor* UAnimNotifyState_NamiCameraEffect::ResolveLookAtTarget(USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return nullptr;
	}
	
	switch (LookAtTargetType)
	{
	case ELookAtTargetType::AnimNotifyActor:
		// 返回动画所在的 Actor
		return MeshComp->GetOwner();
		
	case ELookAtTargetType::PlayerPawn:
		// 返回玩家控制的 Pawn
		if (AActor* Owner = MeshComp->GetOwner())
		{
			if (APawn* Pawn = Cast<APawn>(Owner))
			{
				if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
				{
					return PC->GetPawn();
				}
			}
		}
		break;
		
	case ELookAtTargetType::CustomActor:
		// 返回自定义 Actor
		return CustomLookAtTarget.Get();
		
	case ELookAtTargetType::None:
	default:
		break;
	}
	
	return nullptr;
}

APlayerCameraManager* UAnimNotifyState_NamiCameraEffect::GetPlayerCameraManager(USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return nullptr;
	}
	
	// 尝试从 Owner 获取
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return nullptr;
	}
	
	// 如果 Owner 是 Pawn，获取其 Controller
	APawn* Pawn = Cast<APawn>(Owner);
	if (Pawn)
	{
		APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
		if (PC)
		{
			return PC->PlayerCameraManager;
		}
	}
	
	// 尝试从 World 获取第一个玩家的 CameraManager
	UWorld* World = MeshComp->GetWorld();
	if (World)
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (PC)
		{
			return PC->PlayerCameraManager;
		}
	}
	
	return nullptr;
}

