// Copyright Epic Games, Inc. All Rights Reserved.

#include "Libraries/NamiCameraEffectLibrary.h"
#include "Modifiers/NamiCameraEffectModifierBase.h"
#include "Modifiers/NamiCameraStateModifier.h"
#include "Modifiers/NamiCameraShakeModifier.h"
#include "Modifiers/NamiCameraPostProcessModifier.h"
#include "Structs/State/NamiCameraFramingTypes.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/NamiPlayerCameraManager.h"
#include "LogNamiCamera.h"
#include "LogNamiCameraMacros.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraEffectLibrary)

// 静态映射：跟踪每个 PlayerController 的修改器
static TMap<TWeakObjectPtr<APlayerController>, TArray<TWeakObjectPtr<UNamiCameraEffectModifierBase>>> ActiveModifiersMap;

/**
 * 清理无效的引用
 * 移除已销毁的 PlayerController 和 Modifier 的引用
 */
static void CleanupInvalidReferences()
{
	for (auto It = ActiveModifiersMap.CreateIterator(); It; ++It)
	{
		// 检查 PlayerController 是否有效
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
			continue;
		}
		
		// 清理无效的修改器引用
		TArray<TWeakObjectPtr<UNamiCameraEffectModifierBase>>& Modifiers = It.Value();
		Modifiers.RemoveAll([](const TWeakObjectPtr<UNamiCameraEffectModifierBase>& Modifier)
		{
			return !Modifier.IsValid();
		});
		
		// 如果列表为空，移除映射条目
		if (Modifiers.Num() == 0)
		{
			It.RemoveCurrent();
		}
	}
}

UNamiCameraEffectModifierBase* UNamiCameraEffectLibrary::ActivateCameraEffect(
	UObject* WorldContextObject,
	APlayerController* PlayerController,
	TSubclassOf<UNamiCameraEffectModifierBase> ModifierClass,
	FName EffectName)
{
	if (!PlayerController)
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] ActivateCameraEffect: PlayerController 为空"));
		return nullptr;
	}
	
	APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
	if (!CameraManager)
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] ActivateCameraEffect: 无法获取 PlayerCameraManager"));
		return nullptr;
	}
	
	// 如果没有指定类，使用StateModifier作为默认
	UClass* ClassToUse = ModifierClass ? ModifierClass.Get() : UNamiCameraStateModifier::StaticClass();
	
	// 创建修改器
	UNamiCameraEffectModifierBase* Modifier = NewObject<UNamiCameraEffectModifierBase>(CameraManager, ClassToUse);
	if (!Modifier)
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] ActivateCameraEffect: 创建修改器失败"));
		return nullptr;
	}
	
	// 设置效果名称
	if (!EffectName.IsNone())
	{
		Modifier->EffectName = EffectName;
	}
	
	// 添加到相机管理器（尝试使用 NamiPlayerCameraManager 的公共方法）
	ANamiPlayerCameraManager* NamiCameraManager = Cast<ANamiPlayerCameraManager>(CameraManager);
	if (NamiCameraManager)
	{
		NamiCameraManager->AddCameraModifier(Modifier);
	}
	else
	{
		// 如果不是 NamiPlayerCameraManager，使用反射或其他方式
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] ActivateCameraEffect: 不是 ANamiPlayerCameraManager，无法添加修改器"));
		return nullptr;
	}
	
	// 激活效果
	Modifier->ActivateEffect(false);
	
	// 添加到跟踪映射
	TWeakObjectPtr<APlayerController> PCWeak(PlayerController);
	if (!ActiveModifiersMap.Contains(PCWeak))
	{
		ActiveModifiersMap.Add(PCWeak, TArray<TWeakObjectPtr<UNamiCameraEffectModifierBase>>());
	}
	ActiveModifiersMap[PCWeak].Add(Modifier);
	
	NAMI_LOG_LIBRARY(Log, TEXT("[UNamiCameraEffectLibrary] 激活相机效果：%s，修改器：%s"), 
		*EffectName.ToString(), *Modifier->GetName());
	
	return Modifier;
}

UNamiCameraStateModifier* UNamiCameraEffectLibrary::ActivateStateModifier(
	UObject* WorldContextObject,
	APlayerController* PlayerController,
	FName EffectName)
{
	if (!PlayerController)
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] ActivateStateModifier: PlayerController 为空"));
		return nullptr;
	}
	
	APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
	if (!CameraManager)
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] ActivateStateModifier: 无法获取 PlayerCameraManager"));
		return nullptr;
	}
	
	// 创建State修改器
	UNamiCameraStateModifier* Modifier = NewObject<UNamiCameraStateModifier>(CameraManager);
	if (!Modifier)
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] ActivateStateModifier: 创建修改器失败"));
		return nullptr;
	}
	
	// 设置效果名称
	if (!EffectName.IsNone())
	{
		Modifier->EffectName = EffectName;
	}
	
	// 添加到相机管理器
	ANamiPlayerCameraManager* NamiCameraManager = Cast<ANamiPlayerCameraManager>(CameraManager);
	if (NamiCameraManager)
	{
		NamiCameraManager->AddCameraModifier(Modifier);
		Modifier->ActivateEffect(false);
		
		// 添加到跟踪映射
		TWeakObjectPtr<APlayerController> PCWeak(PlayerController);
		if (!ActiveModifiersMap.Contains(PCWeak))
		{
			ActiveModifiersMap.Add(PCWeak, TArray<TWeakObjectPtr<UNamiCameraEffectModifierBase>>());
		}
		ActiveModifiersMap[PCWeak].Add(Modifier);
		
		NAMI_LOG_LIBRARY(Log, TEXT("[UNamiCameraEffectLibrary] 激活State修改器：%s"), 
			*EffectName.ToString());
		
		return Modifier;
	}
	
	NAMI_LOG_WARNING(TEXT("[UNamiCameraEffectLibrary] ActivateStateModifier: 不是 ANamiPlayerCameraManager"));
	return nullptr;
}

UNamiCameraShakeModifier* UNamiCameraEffectLibrary::ActivateShakeModifier(
	UObject* WorldContextObject,
	APlayerController* PlayerController,
	TSubclassOf<UCameraShakeBase> CameraShake,
	float ShakeScale,
	FName EffectName)
{
	if (!PlayerController || !CameraShake)
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] ActivateShakeModifier: 参数无效"));
		return nullptr;
	}
	
	APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
	if (!CameraManager)
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] ActivateShakeModifier: 无法获取 PlayerCameraManager"));
		return nullptr;
	}
	
	// 创建震动修改器
	UNamiCameraShakeModifier* Modifier = NewObject<UNamiCameraShakeModifier>(CameraManager);
	if (!Modifier)
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] ActivateShakeModifier: 创建修改器失败"));
		return nullptr;
	}
	
	// 配置震动
	Modifier->CameraShake = CameraShake;
	Modifier->ShakeScale = ShakeScale;
	Modifier->bPlayShakeOnStart = true;
	
	// 设置效果名称
	if (!EffectName.IsNone())
	{
		Modifier->EffectName = EffectName;
	}
	
	// 添加到相机管理器
	ANamiPlayerCameraManager* NamiCameraManager = Cast<ANamiPlayerCameraManager>(CameraManager);
	if (NamiCameraManager)
	{
		NamiCameraManager->AddCameraModifier(Modifier);
		Modifier->ActivateEffect(false);
		
		// 添加到跟踪映射
		TWeakObjectPtr<APlayerController> PCWeak(PlayerController);
		if (!ActiveModifiersMap.Contains(PCWeak))
		{
			ActiveModifiersMap.Add(PCWeak, TArray<TWeakObjectPtr<UNamiCameraEffectModifierBase>>());
		}
		ActiveModifiersMap[PCWeak].Add(Modifier);
		
		NAMI_LOG_LIBRARY(Log, TEXT("[UNamiCameraEffectLibrary] 激活震动修改器：%s，震动：%s，强度：%.2f"), 
			*EffectName.ToString(), *CameraShake->GetName(), ShakeScale);
		
		return Modifier;
	}
	
	NAMI_LOG_WARNING(TEXT("[UNamiCameraEffectLibrary] ActivateShakeModifier: 不是 ANamiPlayerCameraManager"));
	return nullptr;
}

UNamiCameraPostProcessModifier* UNamiCameraEffectLibrary::ActivatePostProcessModifier(
	UObject* WorldContextObject,
	APlayerController* PlayerController,
	FName EffectName)
{
	if (!PlayerController)
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] ActivatePostProcessModifier: PlayerController 为空"));
		return nullptr;
	}
	
	APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
	if (!CameraManager)
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] ActivatePostProcessModifier: 无法获取 PlayerCameraManager"));
		return nullptr;
	}
	
	// 创建后处理修改器
	UNamiCameraPostProcessModifier* Modifier = NewObject<UNamiCameraPostProcessModifier>(CameraManager);
	if (!Modifier)
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] ActivatePostProcessModifier: 创建修改器失败"));
		return nullptr;
	}
	
	// 设置效果名称
	if (!EffectName.IsNone())
	{
		Modifier->EffectName = EffectName;
	}
	
	// 添加到相机管理器
	ANamiPlayerCameraManager* NamiCameraManager = Cast<ANamiPlayerCameraManager>(CameraManager);
	if (NamiCameraManager)
	{
		NamiCameraManager->AddCameraModifier(Modifier);
		Modifier->ActivateEffect(false);
		
		// 添加到跟踪映射
		TWeakObjectPtr<APlayerController> PCWeak(PlayerController);
		if (!ActiveModifiersMap.Contains(PCWeak))
		{
			ActiveModifiersMap.Add(PCWeak, TArray<TWeakObjectPtr<UNamiCameraEffectModifierBase>>());
		}
		ActiveModifiersMap[PCWeak].Add(Modifier);
		
		NAMI_LOG_LIBRARY(Log, TEXT("[UNamiCameraEffectLibrary] 激活后处理修改器：%s"), 
			*EffectName.ToString());
		
		return Modifier;
	}
	
	NAMI_LOG_WARNING(TEXT("[UNamiCameraEffectLibrary] ActivatePostProcessModifier: 不是 ANamiPlayerCameraManager"));
	return nullptr;
}

void UNamiCameraEffectLibrary::DeactivateCameraEffect(UNamiCameraEffectModifierBase* Modifier, bool bForceImmediate)
{
	if (!Modifier)
	{
		return;
	}
	
	Modifier->DeactivateEffect(bForceImmediate);
	
	// 如果立即停止，需要通过跟踪映射找到对应的 PlayerController 来移除
	if (bForceImmediate)
	{
		// 从跟踪映射中查找并移除
		for (auto& Pair : ActiveModifiersMap)
		{
			if (!Pair.Key.IsValid())
			{
				continue;
			}
			
			TArray<TWeakObjectPtr<UNamiCameraEffectModifierBase>>& Modifiers = Pair.Value;
			if (Modifiers.Contains(Modifier))
			{
				// 找到对应的 PlayerController，移除修改器
				APlayerController* PC = Pair.Key.Get();
				if (PC && PC->PlayerCameraManager)
				{
					PC->PlayerCameraManager->RemoveCameraModifier(Modifier);
				}
				
				// 从映射中移除
				Modifiers.RemoveAll([Modifier](const TWeakObjectPtr<UNamiCameraEffectModifierBase>& M)
				{
					return M.Get() == Modifier;
				});
				
				// 如果列表为空，移除映射条目
				if (Modifiers.Num() == 0)
				{
					ActiveModifiersMap.Remove(Pair.Key);
				}
				
				break;
			}
		}
	}
	// 非立即停止时，清理会在 GetAllActiveCameraEffects 中自动进行
}

void UNamiCameraEffectLibrary::DeactivateAllCameraEffects(APlayerController* PlayerController, bool bForceImmediate)
{
	if (!PlayerController)
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] DeactivateAllCameraEffects: PlayerController 为空"));
		return;
	}
	
	// 获取所有相机效果
	TArray<UNamiCameraEffectModifierBase*> Effects = GetAllActiveCameraEffects(PlayerController);
	
	if (Effects.Num() == 0)
	{
		NAMI_LOG_LIBRARY(Verbose, TEXT("[UNamiCameraEffectLibrary] DeactivateAllCameraEffects: 没有激活的相机效果"));
		return;
	}
	
	// 停止所有效果
	for (UNamiCameraEffectModifierBase* Effect : Effects)
	{
		if (Effect)
		{
			Effect->DeactivateEffect(bForceImmediate);
		}
	}
	
	// 如果立即停止，清理映射
	if (bForceImmediate)
	{
		TWeakObjectPtr<APlayerController> PCWeak(PlayerController);
		ActiveModifiersMap.Remove(PCWeak);
	}
	
	NAMI_LOG_LIBRARY(Log, TEXT("[UNamiCameraEffectLibrary] 停止所有相机效果，数量：%d，立即：%s"), 
		Effects.Num(), bForceImmediate ? TEXT("是") : TEXT("否"));
}

void UNamiCameraEffectLibrary::DeactivateCameraEffectByName(APlayerController* PlayerController, FName EffectName, bool bForceImmediate)
{
	if (!PlayerController)
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] DeactivateCameraEffectByName: PlayerController 为空"));
		return;
	}
	
	if (EffectName.IsNone())
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] DeactivateCameraEffectByName: EffectName 为空"));
		return;
	}
	
	UNamiCameraEffectModifierBase* Effect = FindCameraEffectByName(PlayerController, EffectName);
	if (Effect)
	{
		Effect->DeactivateEffect(bForceImmediate);
		NAMI_LOG_LIBRARY(Log, TEXT("[UNamiCameraEffectLibrary] 按名称停止相机效果：%s，立即：%s"), 
			*EffectName.ToString(), bForceImmediate ? TEXT("是") : TEXT("否"));
	}
	else
	{
		NAMI_LOG_WARNING( TEXT("[UNamiCameraEffectLibrary] DeactivateCameraEffectByName: 未找到效果：%s"), 
			*EffectName.ToString());
	}
}

TArray<UNamiCameraEffectModifierBase*> UNamiCameraEffectLibrary::GetAllActiveCameraEffects(APlayerController* PlayerController)
{
	TArray<UNamiCameraEffectModifierBase*> Results;
	
	if (!PlayerController)
	{
		return Results;
	}
	
	// 定期清理无效引用（每 10 次调用清理一次，避免性能问题）
	static int32 CleanupCounter = 0;
	if (++CleanupCounter >= 10)
	{
		CleanupCounter = 0;
		CleanupInvalidReferences();
	}
	
	// 从跟踪映射中获取
	TWeakObjectPtr<APlayerController> PCWeak(PlayerController);
	if (ActiveModifiersMap.Contains(PCWeak))
	{
		TArray<TWeakObjectPtr<UNamiCameraEffectModifierBase>>& Modifiers = ActiveModifiersMap[PCWeak];
		
		// 收集有效的激活修改器
		for (const TWeakObjectPtr<UNamiCameraEffectModifierBase>& Modifier : Modifiers)
		{
			if (Modifier.IsValid() && Modifier->IsActive())
			{
				Results.Add(Modifier.Get());
			}
		}
		
		// 清理当前 PlayerController 的无效引用
		Modifiers.RemoveAll([](const TWeakObjectPtr<UNamiCameraEffectModifierBase>& Modifier)
		{
			return !Modifier.IsValid();
		});
		
		// 如果列表为空，移除映射条目
		if (Modifiers.Num() == 0)
		{
			ActiveModifiersMap.Remove(PCWeak);
		}
	}
	
	return Results;
}

UNamiCameraEffectModifierBase* UNamiCameraEffectLibrary::FindCameraEffectByName(APlayerController* PlayerController, FName EffectName)
{
	if (!PlayerController || EffectName.IsNone())
	{
		return nullptr;
	}
	
	TArray<UNamiCameraEffectModifierBase*> Effects = GetAllActiveCameraEffects(PlayerController);
	
	for (UNamiCameraEffectModifierBase* Effect : Effects)
	{
		if (Effect && Effect->EffectName == EffectName)
		{
			return Effect;
		}
	}
	
	return nullptr;
}

bool UNamiCameraEffectLibrary::HasActiveCameraEffects(APlayerController* PlayerController)
{
	TArray<UNamiCameraEffectModifierBase*> Effects = GetAllActiveCameraEffects(PlayerController);
	return Effects.Num() > 0;
}

void UNamiCameraEffectLibrary::SetEffectDuration(UNamiCameraEffectModifierBase* Modifier, float NewDuration)
{
	if (!Modifier)
	{
		return;
	}
	
	Modifier->Duration = NewDuration;
}

void UNamiCameraEffectLibrary::SetEffectBlendTime(UNamiCameraEffectModifierBase* Modifier, float BlendInTime, float BlendOutTime)
{
	if (!Modifier)
	{
		return;
	}
	
	Modifier->BlendInTime = BlendInTime;
	Modifier->BlendOutTime = BlendOutTime;
}

void UNamiCameraEffectLibrary::SetFramingTargets(UNamiCameraStateModifier* StateModifier, const TArray<AActor*>& Targets)
{
	if (!StateModifier)
	{
		return;
	}

	StateModifier->SetFramingTargets(Targets);
}

void UNamiCameraEffectLibrary::UpdateFramingConfig(UNamiCameraStateModifier* StateModifier, const FNamiCameraFramingConfig& Config, bool bEnableFraming)
{
	if (!StateModifier)
	{
		return;
	}

	StateModifier->FramingConfig = Config;
	StateModifier->bEnableFraming = bEnableFraming;
}

