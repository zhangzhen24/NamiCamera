// Copyright Epic Games, Inc. All Rights Reserved.

#include "Libraries/NamiCameraEffectLibrary.h"
#include "Modifiers/NamiCameraEffectModifier.h"
#include "Data/NamiCameraEffectPreset.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/NamiPlayerCameraManager.h"
#include "LogNamiCamera.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraEffectLibrary)

// 静态映射：跟踪每个 PlayerController 的修改器
static TMap<TWeakObjectPtr<APlayerController>, TArray<TWeakObjectPtr<UNamiCameraEffectModifier>>> ActiveModifiersMap;

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
		TArray<TWeakObjectPtr<UNamiCameraEffectModifier>>& Modifiers = It.Value();
		Modifiers.RemoveAll([](const TWeakObjectPtr<UNamiCameraEffectModifier>& Modifier)
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

UNamiCameraEffectModifier* UNamiCameraEffectLibrary::ActivateCameraEffect(
	UObject* WorldContextObject,
	APlayerController* PlayerController,
	UNamiCameraEffectPreset* Preset,
	FName EffectName)
{
	if (!PlayerController || !Preset)
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraEffectLibrary] ActivateCameraEffect: PlayerController 或 Preset 为空"));
		return nullptr;
	}
	
	APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
	if (!CameraManager)
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraEffectLibrary] ActivateCameraEffect: 无法获取 PlayerCameraManager"));
		return nullptr;
	}
	
	// 使用预设创建修改器
	UNamiCameraEffectModifier* Modifier = Preset->CreateModifier(CameraManager);
	if (!Modifier)
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraEffectLibrary] ActivateCameraEffect: 创建修改器失败"));
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
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraEffectLibrary] ActivateCameraEffect: 不是 ANamiPlayerCameraManager，无法添加修改器"));
		return nullptr;
	}
	
	// 激活效果
	Modifier->ActivateEffect(false);
	
	// 添加到跟踪映射
	TWeakObjectPtr<APlayerController> PCWeak(PlayerController);
	if (!ActiveModifiersMap.Contains(PCWeak))
	{
		ActiveModifiersMap.Add(PCWeak, TArray<TWeakObjectPtr<UNamiCameraEffectModifier>>());
	}
	ActiveModifiersMap[PCWeak].Add(Modifier);
	
	UE_LOG(LogNamiCamera, Log, TEXT("[UNamiCameraEffectLibrary] 激活相机效果：%s（预设：%s），修改器：%s"), 
		*EffectName.ToString(), *Preset->GetName(), *Modifier->GetName());
	
	return Modifier;
}

UNamiCameraEffectModifier* UNamiCameraEffectLibrary::ActivateCustomCameraEffect(
	UObject* WorldContextObject,
	APlayerController* PlayerController,
	TSubclassOf<UNamiCameraEffectModifier> ModifierClass,
	FName EffectName)
{
	if (!PlayerController || !ModifierClass)
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraEffectLibrary] ActivateCustomCameraEffect: PlayerController 或 ModifierClass 为空"));
		return nullptr;
	}
	
	APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
	if (!CameraManager)
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraEffectLibrary] ActivateCustomCameraEffect: 无法获取 PlayerCameraManager"));
		return nullptr;
	}
	
	// 创建自定义修改器
	UNamiCameraEffectModifier* Modifier = NewObject<UNamiCameraEffectModifier>(CameraManager, ModifierClass);
	if (!Modifier)
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraEffectLibrary] ActivateCustomCameraEffect: 创建修改器失败"));
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
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraEffectLibrary] ActivateCustomCameraEffect: 不是 ANamiPlayerCameraManager，无法添加修改器"));
		return nullptr;
	}
	
	// 激活效果
	Modifier->ActivateEffect(false);
	
	// 添加到跟踪映射
	TWeakObjectPtr<APlayerController> PCWeak(PlayerController);
	if (!ActiveModifiersMap.Contains(PCWeak))
	{
		ActiveModifiersMap.Add(PCWeak, TArray<TWeakObjectPtr<UNamiCameraEffectModifier>>());
	}
	ActiveModifiersMap[PCWeak].Add(Modifier);
	
	UE_LOG(LogNamiCamera, Log, TEXT("[UNamiCameraEffectLibrary] 激活自定义相机效果：%s，修改器类：%s"), 
		*EffectName.ToString(), *ModifierClass->GetName());
	
	return Modifier;
}

void UNamiCameraEffectLibrary::DeactivateCameraEffect(UNamiCameraEffectModifier* Modifier, bool bForceImmediate)
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
			
			TArray<TWeakObjectPtr<UNamiCameraEffectModifier>>& Modifiers = Pair.Value;
			if (Modifiers.Contains(Modifier))
			{
				// 找到对应的 PlayerController，移除修改器
				APlayerController* PC = Pair.Key.Get();
				if (PC && PC->PlayerCameraManager)
				{
					PC->PlayerCameraManager->RemoveCameraModifier(Modifier);
				}
				
				// 从映射中移除
				Modifiers.RemoveAll([Modifier](const TWeakObjectPtr<UNamiCameraEffectModifier>& M)
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
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraEffectLibrary] DeactivateAllCameraEffects: PlayerController 为空"));
		return;
	}
	
	// 获取所有相机效果
	TArray<UNamiCameraEffectModifier*> Effects = GetAllActiveCameraEffects(PlayerController);
	
	if (Effects.Num() == 0)
	{
		UE_LOG(LogNamiCamera, Verbose, TEXT("[UNamiCameraEffectLibrary] DeactivateAllCameraEffects: 没有激活的相机效果"));
		return;
	}
	
	// 停止所有效果
	for (UNamiCameraEffectModifier* Effect : Effects)
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
	
	UE_LOG(LogNamiCamera, Log, TEXT("[UNamiCameraEffectLibrary] 停止所有相机效果，数量：%d，立即：%s"), 
		Effects.Num(), bForceImmediate ? TEXT("是") : TEXT("否"));
}

void UNamiCameraEffectLibrary::DeactivateCameraEffectByName(APlayerController* PlayerController, FName EffectName, bool bForceImmediate)
{
	if (!PlayerController)
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraEffectLibrary] DeactivateCameraEffectByName: PlayerController 为空"));
		return;
	}
	
	if (EffectName.IsNone())
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraEffectLibrary] DeactivateCameraEffectByName: EffectName 为空"));
		return;
	}
	
	UNamiCameraEffectModifier* Effect = FindCameraEffectByName(PlayerController, EffectName);
	if (Effect)
	{
		Effect->DeactivateEffect(bForceImmediate);
		UE_LOG(LogNamiCamera, Log, TEXT("[UNamiCameraEffectLibrary] 按名称停止相机效果：%s，立即：%s"), 
			*EffectName.ToString(), bForceImmediate ? TEXT("是") : TEXT("否"));
	}
	else
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraEffectLibrary] DeactivateCameraEffectByName: 未找到效果：%s"), 
			*EffectName.ToString());
	}
}

TArray<UNamiCameraEffectModifier*> UNamiCameraEffectLibrary::GetAllActiveCameraEffects(APlayerController* PlayerController)
{
	TArray<UNamiCameraEffectModifier*> Results;
	
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
		TArray<TWeakObjectPtr<UNamiCameraEffectModifier>>& Modifiers = ActiveModifiersMap[PCWeak];
		
		// 收集有效的激活修改器
		for (const TWeakObjectPtr<UNamiCameraEffectModifier>& Modifier : Modifiers)
		{
			if (Modifier.IsValid() && Modifier->IsActive())
			{
				Results.Add(Modifier.Get());
			}
		}
		
		// 清理当前 PlayerController 的无效引用
		Modifiers.RemoveAll([](const TWeakObjectPtr<UNamiCameraEffectModifier>& Modifier)
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

UNamiCameraEffectModifier* UNamiCameraEffectLibrary::FindCameraEffectByName(APlayerController* PlayerController, FName EffectName)
{
	if (!PlayerController || EffectName.IsNone())
	{
		return nullptr;
	}
	
	TArray<UNamiCameraEffectModifier*> Effects = GetAllActiveCameraEffects(PlayerController);
	
	for (UNamiCameraEffectModifier* Effect : Effects)
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
	TArray<UNamiCameraEffectModifier*> Effects = GetAllActiveCameraEffects(PlayerController);
	return Effects.Num() > 0;
}

void UNamiCameraEffectLibrary::SetEffectLookAtTarget(UNamiCameraEffectModifier* Modifier, AActor* Target, FVector Offset)
{
	if (!Modifier)
	{
		return;
	}
	
	Modifier->SetLookAtTarget(Target, Offset);
}

void UNamiCameraEffectLibrary::SetEffectLookAtLocation(UNamiCameraEffectModifier* Modifier, FVector Location)
{
	if (!Modifier)
	{
		return;
	}
	
	Modifier->SetLookAtLocation(Location);
}

void UNamiCameraEffectLibrary::SetEffectDuration(UNamiCameraEffectModifier* Modifier, float NewDuration)
{
	if (!Modifier)
	{
		return;
	}
	
	Modifier->Duration = NewDuration;
}

void UNamiCameraEffectLibrary::SetEffectBlendTime(UNamiCameraEffectModifier* Modifier, float BlendInTime, float BlendOutTime)
{
	if (!Modifier)
	{
		return;
	}
	
	Modifier->BlendInTime = BlendInTime;
	Modifier->BlendOutTime = BlendOutTime;
}

