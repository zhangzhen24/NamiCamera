// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NamiFollowCameraMode.h"
#include "Components/NamiSpringArm.h"
#include "NamiSpringArmCameraMode.generated.h"

/**
 * 带弹簧臂的跟随相机模式
 *
 * 职责：
 * - 继承 FollowCameraMode 的所有跟随逻辑
 * - 添加 SpringArm 碰撞检测和滞后功能
 *
 * 用途：
 * - 第三人称相机
 * - 肩部视角相机
 * - 任何需要碰撞检测和相机滞后的跟随相机
 *
 * 设计理念：
 * - 单一职责：只负责 SpringArm 的管理和应用
 * - 不处理跟随逻辑（由父类 FollowCameraMode 处理）
 * - 不处理特定相机类型逻辑（由子类如 ThirdPersonCamera 处理）
 */
UCLASS(Blueprintable, BlueprintType)
class NAMICAMERA_API UNamiSpringArmCameraMode : public UNamiFollowCameraMode
{
	GENERATED_BODY()

public:
	UNamiSpringArmCameraMode();

	// ========== 生命周期 ==========
	virtual void Initialize_Implementation(UNamiCameraComponent* InCameraComponent) override;
	virtual void Activate_Implementation() override;
	virtual FNamiCameraView CalculateView_Implementation(float DeltaTime) override;

public:
	// ========== SpringArm 配置 ==========

	/**
	 * 弹簧臂配置（碰撞检测 + 相机滞后）
	 * 直接配置 SpringArm 参数，无需通过 Feature
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. SpringArm")
	FNamiSpringArm SpringArm;

protected:
	/** SpringArm 内部状态 */
	bool bSpringArmInitialized = false;
};
