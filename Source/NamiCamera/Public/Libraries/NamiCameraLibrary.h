// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Structs/Mode/NamiCameraModeHandle.h"
#include "NamiCameraLibrary.generated.h"

class AActor;
class UNamiCameraComponent;
class UNamiCameraModeBase;
class UNamiTopDownCamera;

/**
 * 相机工具库
 * 提供便捷的相机操作函数
 */
UCLASS()
class NAMICAMERA_API UNamiCameraLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 为Actor推送相机模式
	 * @param Actor 目标Actor（通常是角色）
	 * @param CameraModeClass 相机模式类
	 * @param Priority 优先级
	 * @return 模式句柄
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library",
		meta = (CallInEditor = "true"))
	static FNamiCameraModeHandle PushCameraModeForActor(
		const AActor* Actor,
		TSubclassOf<UNamiCameraModeBase> CameraModeClass,
		int32 Priority = 0);

	/**
	 * 为Actor推送相机模式（使用实例）
	 * @param Actor 目标Actor（通常是角色）
	 * @param CameraModeInstance 相机模式实例
	 * @param Priority 优先级
	 * @return 模式句柄
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library",
		meta = (CallInEditor = "true"))
	static FNamiCameraModeHandle PushCameraModeForActorUsingInstance(
		const AActor* Actor,
		UNamiCameraModeBase* CameraModeInstance,
		int32 Priority = 0);

	/**
	 * 快速设置俯视角相机（带速度预测）
	 * @param Actor 目标Actor（通常是角色）
	 * @param CameraHeight 相机高度
	 * @param LookDownAngle 俯视角度
	 * @return 模式句柄
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Top Down",
		meta = (CallInEditor = "true", 
		Tooltip = "快速设置俯视角相机，自动添加目标并配置速度预测"))
	static FNamiCameraModeHandle SetupTopDownCameraForActor(
		const AActor* Actor,
		float CameraHeight = 1000.0f,
		float LookDownAngle = 60.0f);

	/**
	 * 通过句柄移除相机模式
	 * @param ModeHandle 模式句柄
	 * @return true 如果成功移除
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library",
		meta = (CallInEditor = "true"))
	static bool PopCameraModeByHandle(UPARAM(ref) FNamiCameraModeHandle& ModeHandle);

	/**
	 * 从Actor移除相机模式实例
	 * @param Actor 目标Actor
	 * @param CameraMode 相机模式实例
	 * @return true 如果成功移除
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library",
		meta = (CallInEditor = "true"))
	static bool PopCameraModeInstanceFromActor(const AActor* Actor, UNamiCameraModeBase* CameraMode);

	/**
	 * 从Actor获取当前激活的相机模式
	 * @param Actor 目标Actor
	 * @return 激活的相机模式，如果没有则返回 nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Library",
		meta = (CallInEditor = "true"))
	static UNamiCameraModeBase* GetActiveCameraModeFromActor(const AActor* Actor);
};

