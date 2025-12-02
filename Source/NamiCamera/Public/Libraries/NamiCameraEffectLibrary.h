// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NamiCameraEffectLibrary.generated.h"

/**
 * Nami 相机效果 Blueprint 函数库
 * 
 * 提供便捷的 Blueprint 接口来控制相机效果
 */
UCLASS()
class NAMICAMERA_API UNamiCameraEffectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ========== 基础操作 ==========
	
	/**
	 * 激活相机效果（使用预设）
	 * 
	 * @param WorldContextObject 世界上下文对象
	 * @param PlayerController 玩家控制器
	 * @param Preset 效果预设
	 * @param EffectName 效果名称（用于调试和管理）
	 * @return 创建的修改器实例
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Effect",
			  meta = (WorldContext = "WorldContextObject", DisplayName = "激活相机效果（预设）"))
	static class UNamiCameraEffectModifier* ActivateCameraEffect(
		UObject* WorldContextObject,
		class APlayerController* PlayerController,
		class UNamiCameraEffectPreset* Preset,
		FName EffectName = NAME_None);
	
	/**
	 * 激活自定义相机效果
	 * 
	 * @param WorldContextObject 世界上下文对象
	 * @param PlayerController 玩家控制器
	 * @param ModifierClass 自定义修改器类
	 * @param EffectName 效果名称
	 * @return 创建的修改器实例
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Effect",
			  meta = (WorldContext = "WorldContextObject", DisplayName = "激活相机效果（自定义）"))
	static class UNamiCameraEffectModifier* ActivateCustomCameraEffect(
		UObject* WorldContextObject,
		class APlayerController* PlayerController,
		TSubclassOf<class UNamiCameraEffectModifier> ModifierClass,
		FName EffectName = NAME_None);
	
	/**
	 * 停止相机效果
	 * 
	 * @param Modifier 要停止的修改器
	 * @param bForceImmediate 是否立即停止（跳过 BlendOut）
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Effect",
			  meta = (DisplayName = "停止相机效果"))
	static void DeactivateCameraEffect(
		class UNamiCameraEffectModifier* Modifier,
		bool bForceImmediate = false);
	
	/**
	 * 停止所有相机效果
	 * 
	 * @param PlayerController 玩家控制器
	 * @param bForceImmediate 是否立即停止
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Effect",
			  meta = (DisplayName = "停止所有相机效果"))
	static void DeactivateAllCameraEffects(
		class APlayerController* PlayerController,
		bool bForceImmediate = false);
	
	/**
	 * 按名称停止相机效果
	 * 
	 * @param PlayerController 玩家控制器
	 * @param EffectName 效果名称
	 * @param bForceImmediate 是否立即停止
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Effect",
			  meta = (DisplayName = "按名称停止相机效果"))
	static void DeactivateCameraEffectByName(
		class APlayerController* PlayerController,
		FName EffectName,
		bool bForceImmediate = false);
	
	// ========== 查询操作 ==========
	
	/**
	 * 获取所有激活的相机效果
	 * 
	 * @param PlayerController 玩家控制器
	 * @return 所有激活的修改器
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Effect",
			  meta = (DisplayName = "获取所有相机效果"))
	static TArray<class UNamiCameraEffectModifier*> GetAllActiveCameraEffects(
		class APlayerController* PlayerController);
	
	/**
	 * 按名称查找相机效果
	 * 
	 * @param PlayerController 玩家控制器
	 * @param EffectName 效果名称
	 * @return 找到的修改器（如果有）
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Effect",
			  meta = (DisplayName = "查找相机效果"))
	static class UNamiCameraEffectModifier* FindCameraEffectByName(
		class APlayerController* PlayerController,
		FName EffectName);
	
	/**
	 * 检查是否有激活的相机效果
	 * 
	 * @param PlayerController 玩家控制器
	 * @return 是否有激活的效果
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Effect",
			  meta = (DisplayName = "是否有激活的相机效果"))
	static bool HasActiveCameraEffects(class APlayerController* PlayerController);
	
	// ========== 参数调整 ==========
	
	/**
	 * 设置效果的 LookAt 目标
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Effect",
			  meta = (DisplayName = "设置 LookAt 目标"))
	static void SetEffectLookAtTarget(
		class UNamiCameraEffectModifier* Modifier,
		AActor* Target,
		FVector Offset = FVector::ZeroVector);
	
	/**
	 * 设置效果的 LookAt 位置
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Effect",
			  meta = (DisplayName = "设置 LookAt 位置"))
	static void SetEffectLookAtLocation(
		class UNamiCameraEffectModifier* Modifier,
		FVector Location);
	
	/**
	 * 设置效果的持续时间
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Effect",
			  meta = (DisplayName = "设置效果持续时间"))
	static void SetEffectDuration(
		class UNamiCameraEffectModifier* Modifier,
		float NewDuration);
	
	/**
	 * 设置效果的混合时间
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Effect",
			  meta = (DisplayName = "设置效果混合时间"))
	static void SetEffectBlendTime(
		class UNamiCameraEffectModifier* Modifier,
		float BlendInTime,
		float BlendOutTime);
};

