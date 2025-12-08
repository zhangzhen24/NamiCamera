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
class UNamiThirdPersonCamera;
class UNamiFollowCameraMode;
class UNamiToricSpaceFeature;
class UNamiCameraFeature;
struct FGameplayTag;
struct FGameplayTagContainer;

// 前向声明枚举
enum class ENamiThirdPersonCameraPreset : uint8;

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
	
	// ========== 快捷配置方法 ==========
	
	/**
	 * 快速设置第三人称相机距离
	 * @param Actor 目标Actor
	 * @param Distance 相机距离
	 * @return 是否成功设置
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Quick Config",
		meta = (CallInEditor = "true"))
	static bool SetThirdPersonCameraDistance(const AActor* Actor, float Distance);
	
	/**
	 * 快速设置第三人称相机碰撞检测
	 * @param Actor 目标Actor
	 * @param bEnabled 是否启用
	 * @return 是否成功设置
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Quick Config",
		meta = (CallInEditor = "true"))
	static bool SetThirdPersonCameraCollision(const AActor* Actor, bool bEnabled);
	
	/**
	 * 快速设置第三人称相机滞后
	 * @param Actor 目标Actor
	 * @param bEnabled 是否启用
	 * @param LagSpeed 滞后速度
	 * @return 是否成功设置
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Quick Config",
		meta = (CallInEditor = "true"))
	static bool SetThirdPersonCameraLag(const AActor* Actor, bool bEnabled, float LagSpeed = 10.0f);
	
	/**
	 * 应用第三人称相机预设
	 * @param Actor 目标Actor
	 * @param Preset 预设类型
	 * @return 是否成功应用
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Quick Config",
		meta = (CallInEditor = "true"))
	static bool ApplyThirdPersonCameraPreset(const AActor* Actor, ENamiThirdPersonCameraPreset Preset);
	
	// ========== Toric Space Feature 操作方法 ==========
	
	/**
	 * 从Actor获取ToricSpaceFeature
	 * @param Actor 拥有相机组件的Actor（通常是玩家角色）
	 * @param CameraMode 指定的相机模式，如果为null则使用当前激活的相机模式
	 * @return ToricSpaceFeature实例，如果没有找到则返回nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Library|Toric Space",
		meta = (CallInEditor = "true",
		Tooltip = "从相机模式中获取ToricSpaceFeature实例"))
	static UNamiToricSpaceFeature* GetToricSpaceFeatureFromActor(
		const AActor* Actor,
		UNamiCameraModeBase* CameraMode = nullptr);
	
	/**
	 * 设置ToricSpace的水平角度（Theta）
	 * @param Actor 拥有相机组件的Actor
	 * @param Theta 水平角度（0-360度）
	 * @param CameraMode 指定的相机模式，如果为null则使用当前激活的相机模式
	 * @return 是否成功设置
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Toric Space",
		meta = (CallInEditor = "true",
		Tooltip = "设置复曲面空间的水平角度（Theta）"))
	static bool SetToricSpaceTheta(
		const AActor* Actor,
		float Theta,
		UNamiCameraModeBase* CameraMode = nullptr);
	
	/**
	 * 设置ToricSpace的垂直角度（Phi）
	 * @param Actor 拥有相机组件的Actor
	 * @param Phi 垂直角度（-90到90度）
	 * @param CameraMode 指定的相机模式，如果为null则使用当前激活的相机模式
	 * @return 是否成功设置
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Toric Space",
		meta = (CallInEditor = "true",
		Tooltip = "设置复曲面空间的垂直角度（Phi）"))
	static bool SetToricSpacePhi(
		const AActor* Actor,
		float Phi,
		UNamiCameraModeBase* CameraMode = nullptr);
	
	/**
	 * 设置ToricSpace的半径
	 * @param Actor 拥有相机组件的Actor
	 * @param Radius 半径（单位：cm）
	 * @param CameraMode 指定的相机模式，如果为null则使用当前激活的相机模式
	 * @return 是否成功设置
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Toric Space",
		meta = (CallInEditor = "true",
		Tooltip = "设置复曲面空间的半径"))
	static bool SetToricSpaceRadius(
		const AActor* Actor,
		float Radius,
		UNamiCameraModeBase* CameraMode = nullptr);
	
	/**
	 * 设置ToricSpace的高度偏移
	 * @param Actor 拥有相机组件的Actor
	 * @param Height 高度偏移（单位：cm）
	 * @param CameraMode 指定的相机模式，如果为null则使用当前激活的相机模式
	 * @return 是否成功设置
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Toric Space",
		meta = (CallInEditor = "true",
		Tooltip = "设置复曲面空间的高度偏移"))
	static bool SetToricSpaceHeight(
		const AActor* Actor,
		float Height,
		UNamiCameraModeBase* CameraMode = nullptr);
	
	/**
	 * 设置ToricSpace的聚焦目标
	 * @param Actor 拥有相机组件的Actor
	 * @param FocusTarget 聚焦目标（0=目标1，1=目标2，0.5=中点）
	 * @param CameraMode 指定的相机模式，如果为null则使用当前激活的相机模式
	 * @return 是否成功设置
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Toric Space",
		meta = (CallInEditor = "true",
		Tooltip = "设置复曲面空间的聚焦目标"))
	static bool SetToricSpaceFocusTarget(
		const AActor* Actor,
		float FocusTarget,
		UNamiCameraModeBase* CameraMode = nullptr);
	
	/**
	 * 设置ToricSpace是否启用180度规则
	 * @param Actor 拥有相机组件的Actor
	 * @param bEnabled 是否启用
	 * @param CameraMode 指定的相机模式，如果为null则使用当前激活的相机模式
	 * @return 是否成功设置
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Toric Space",
		meta = (CallInEditor = "true",
		Tooltip = "设置是否启用180度规则"))
	static bool SetToricSpace180DegreeRule(
		const AActor* Actor,
		bool bEnabled,
		UNamiCameraModeBase* CameraMode = nullptr);
	
	/**
	 * 设置ToricSpace是否自动计算半径
	 * @param Actor 拥有相机组件的Actor
	 * @param bEnabled 是否启用
	 * @param CameraMode 指定的相机模式，如果为null则使用当前激活的相机模式
	 * @return 是否成功设置
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Toric Space",
		meta = (CallInEditor = "true",
		Tooltip = "设置是否自动计算半径"))
	static bool SetToricSpaceAutoCalculateRadius(
		const AActor* Actor,
		bool bEnabled,
		UNamiCameraModeBase* CameraMode = nullptr);
	
	/**
	 * 获取ToricSpace的水平角度（Theta）
	 * @param Actor 拥有相机组件的Actor
	 * @param OutTheta 输出的水平角度
	 * @param CameraMode 指定的相机模式，如果为null则使用当前激活的相机模式
	 * @return 是否成功获取
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Library|Toric Space",
		meta = (CallInEditor = "true",
		Tooltip = "获取复曲面空间的水平角度（Theta）"))
	static bool GetToricSpaceTheta(
		const AActor* Actor,
		UPARAM(ref) float& OutTheta,
		UNamiCameraModeBase* CameraMode = nullptr);
	
	/**
	 * 获取ToricSpace的垂直角度（Phi）
	 * @param Actor 拥有相机组件的Actor
	 * @param OutPhi 输出的垂直角度
	 * @param CameraMode 指定的相机模式，如果为null则使用当前激活的相机模式
	 * @return 是否成功获取
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Library|Toric Space",
		meta = (CallInEditor = "true",
		Tooltip = "获取复曲面空间的垂直角度（Phi）"))
	static bool GetToricSpacePhi(
		const AActor* Actor,
		UPARAM(ref) float& OutPhi,
		UNamiCameraModeBase* CameraMode = nullptr);
	
	/**
	 * 获取ToricSpace的半径
	 * @param Actor 拥有相机组件的Actor
	 * @param OutRadius 输出的半径
	 * @param CameraMode 指定的相机模式，如果为null则使用当前激活的相机模式
	 * @return 是否成功获取
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Library|Toric Space",
		meta = (CallInEditor = "true",
		Tooltip = "获取复曲面空间的半径"))
	static bool GetToricSpaceRadius(
		const AActor* Actor,
		UPARAM(ref) float& OutRadius,
		UNamiCameraModeBase* CameraMode = nullptr);
	
	// ========== 敌人目标管理方法 ==========
	
	/**
	 * 快速添加敌人目标到相机模式
	 * @param Actor 拥有相机组件的Actor（通常是玩家角色）
	 * @param EnemyTarget 敌人目标Actor
	 * @param Weight 目标权重（默认1.0）
	 * @param CameraMode 指定的相机模式，如果为null则使用当前激活的相机模式
	 * @return 是否成功添加
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Enemy Target",
		meta = (CallInEditor = "true",
		Tooltip = "快速添加敌人目标到相机模式，用于战斗相机等场景"))
	static bool AddEnemyTargetToCamera(
		const AActor* Actor,
		AActor* EnemyTarget,
		float Weight = 1.0f,
		UNamiCameraModeBase* CameraMode = nullptr);
	
	/**
	 * 快速移除敌人目标
	 * @param Actor 拥有相机组件的Actor（通常是玩家角色）
	 * @param EnemyTarget 要移除的敌人目标Actor
	 * @param CameraMode 指定的相机模式，如果为null则使用当前激活的相机模式
	 * @return 是否成功移除
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Enemy Target",
		meta = (CallInEditor = "true",
		Tooltip = "快速移除敌人目标"))
	static bool RemoveEnemyTargetFromCamera(
		const AActor* Actor,
		AActor* EnemyTarget,
		UNamiCameraModeBase* CameraMode = nullptr);
	
	/**
	 * 清除所有敌人目标
	 * @param Actor 拥有相机组件的Actor（通常是玩家角色）
	 * @param CameraMode 指定的相机模式，如果为null则使用当前激活的相机模式
	 * @return 是否成功清除
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Enemy Target",
		meta = (CallInEditor = "true",
		Tooltip = "清除所有敌人目标"))
	static bool ClearAllEnemyTargetsFromCamera(
		const AActor* Actor,
		UNamiCameraModeBase* CameraMode = nullptr);
	
	/**
	 * 获取所有敌人目标
	 * @param Actor 拥有相机组件的Actor（通常是玩家角色）
	 * @param OutEnemyTargets 输出的敌人目标数组
	 * @param CameraMode 指定的相机模式，如果为null则使用当前激活的相机模式
	 * @return 敌人目标数量
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Library|Enemy Target",
		meta = (CallInEditor = "true",
		Tooltip = "获取所有敌人目标"))
	static int32 GetEnemyTargetsFromCamera(
		const AActor* Actor,
		UPARAM(ref) TArray<AActor*>& OutEnemyTargets,
		UNamiCameraModeBase* CameraMode = nullptr);

	// ========== Feature 查找辅助函数 ==========

	/**
	 * 从Actor的相机组件中查找Feature（通过名称）
	 * @param Actor 拥有相机组件的Actor（通常是玩家角色）
	 * @param FeatureName Feature名称
	 * @return 找到的Feature，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Library|Feature",
		meta = (CallInEditor = "true",
		Tooltip = "从Actor的相机组件中查找Feature（通过名称）"))
	static UNamiCameraFeature* GetFeatureByNameFromActor(
		const AActor* Actor,
		FName FeatureName);

	/**
	 * 从Actor的相机组件中查找Feature（通过Tag）
	 * @param Actor 拥有相机组件的Actor（通常是玩家角色）
	 * @param Tag 要匹配的Tag
	 * @return 匹配的Feature列表
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Library|Feature",
		meta = (CallInEditor = "true",
		Tooltip = "从Actor的相机组件中查找Feature（通过Tag）"))
	static TArray<UNamiCameraFeature*> GetFeaturesByTagFromActor(
		const AActor* Actor,
		const FGameplayTag& Tag);

	/**
	 * 从Actor的相机组件中查找Feature（通过Tag容器）
	 * @param Actor 拥有相机组件的Actor（通常是玩家角色）
	 * @param TagContainer Tag容器（匹配任意Tag）
	 * @return 匹配的Feature列表
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Library|Feature",
		meta = (CallInEditor = "true",
		Tooltip = "从Actor的相机组件中查找Feature（通过Tag容器）"))
	static TArray<UNamiCameraFeature*> GetFeaturesByTagsFromActor(
		const AActor* Actor,
		const FGameplayTagContainer& TagContainer);
};

