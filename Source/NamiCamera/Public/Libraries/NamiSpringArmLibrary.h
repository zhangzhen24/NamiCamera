// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/NamiSpringArm.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NamiSpringArmLibrary.generated.h"

/**
 * 弹簧臂工具库
 * 提供便捷的蓝图函数用于操作FNamiSpringArm结构体
 */
UCLASS(meta = (BlueprintThreadSafe, ScriptName = "SpringArmLibrary"))
class NAMICAMERA_API UNamiSpringArmLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 初始化弹簧臂
	 * 重置弹簧臂的动态状态，应在使用前调用
	 * 
	 * @param SpringArm 要初始化的弹簧臂（引用，会被修改）
	 * 
	 * @note 此函数会重置弹簧臂的内部状态，包括碰撞检测状态和位置缓存
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Spring Arm",
		meta = (DisplayName = "Initialize Spring Arm",
		Tooltip = "初始化弹簧臂，重置其动态状态。应在使用前调用。"))
	static void InitializeSpringArm(UPARAM(ref) FNamiSpringArm& SpringArm);

	/**
	 * 更新弹簧臂位置
	 * 根据当前变换和偏移计算相机位置，并进行碰撞检测
	 * 
	 * @param SpringArm 要更新的弹簧臂（引用，会被修改）
	 * @param WorldContext 世界上下文对象，用于碰撞检测
	 * @param DeltaTime 帧时间间隔（秒）
	 * @param IgnoreActor 碰撞检测时忽略的Actor
	 * @param InitialTransform 初始变换（通常为目标位置和旋转）
	 * @param OffsetLocation 偏移位置（局部空间）
	 * 
	 * @note 此函数会更新弹簧臂的相机位置，考虑碰撞检测和弹簧效果
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Spring Arm",
		meta = (DisplayName = "Tick Spring Arm (Single Actor)",
		Tooltip = "更新弹簧臂位置。根据当前变换和偏移计算相机位置，并进行碰撞检测。"))
	static void TickSpringArm(
		UPARAM(ref) FNamiSpringArm& SpringArm,
		UObject* WorldContext,
		float DeltaTime,
		AActor* IgnoreActor,
		const FTransform& InitialTransform,
		const FVector OffsetLocation);

	/**
	 * 更新弹簧臂位置
	 * 根据当前变换和偏移计算相机位置，并进行碰撞检测
	 * 
	 * @param SpringArm 要更新的弹簧臂（引用，会被修改）
	 * @param WorldContext 世界上下文对象，用于碰撞检测
	 * @param DeltaTime 帧时间间隔（秒）
	 * @param IgnoreActors 碰撞检测时忽略的Actor数组
	 * @param InitialTransform 初始变换（通常为目标位置和旋转）
	 * @param OffsetLocation 偏移位置（局部空间）
	 * 
	 * @note 此函数会更新弹簧臂的相机位置，考虑碰撞检测和弹簧效果
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Spring Arm",
		meta = (DisplayName = "Tick Spring Arm (Multiple Actors)",
		Tooltip = "更新弹簧臂位置。根据当前变换和偏移计算相机位置，并进行碰撞检测。"))
	static void TickSpringArmMultiple(
		UPARAM(ref) FNamiSpringArm& SpringArm,
		UObject* WorldContext,
		float DeltaTime,
		const TArray<AActor*>& IgnoreActors,
		const FTransform& InitialTransform,
		const FVector OffsetLocation);

	/**
	 * 获取相机变换
	 * 返回弹簧臂计算出的最终相机位置和旋转
	 * 
	 * @param SpringArm 弹簧臂结构体（只读）
	 * @return 相机的变换（位置和旋转）
	 * 
	 * @note 此函数返回的是经过碰撞检测和弹簧效果处理后的最终相机位置
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nami Camera|Spring Arm",
		meta = (DisplayName = "Get Camera Transform",
		Tooltip = "获取弹簧臂计算出的最终相机位置和旋转。返回经过碰撞检测和弹簧效果处理后的最终相机位置。"))
	static FTransform GetCameraTransform(const FNamiSpringArm& SpringArm);

	/**
	 * 获取未修正的相机位置
	 * 返回不考虑碰撞检测的理想相机位置
	 * 
	 * @param SpringArm 弹簧臂结构体（只读）
	 * @return 未修正的相机位置（理想位置）
	 * 
	 * @note 此位置是弹簧臂的理想位置，未经过碰撞检测修正
	 *       可用于调试或计算相机移动距离
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nami Camera|Spring Arm|Collision",
		meta = (DisplayName = "Get Unfixed Camera Position",
		Tooltip = "获取未修正的相机位置。返回不考虑碰撞检测的理想相机位置，可用于调试或计算相机移动距离。"))
	static FVector GetSpringArmUnfixedCameraPosition(const FNamiSpringArm& SpringArm);

	/**
	 * 检查碰撞修正是否已应用
	 * 判断相机位置是否因为碰撞检测而被修正
	 * 
	 * @param SpringArm 弹簧臂结构体（只读）
	 * @return true 如果碰撞修正已应用（相机位置被调整），false 如果相机处于理想位置
	 * 
	 * @note 当相机与场景发生碰撞时，此值会返回true
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nami Camera|Spring Arm|Collision",
		meta = (DisplayName = "Is Collision Fix Applied",
		Tooltip = "检查碰撞修正是否已应用。判断相机位置是否因为碰撞检测而被修正。"))
	static bool IsSpringArmCollisionFixApplied(const FNamiSpringArm& SpringArm);
};

