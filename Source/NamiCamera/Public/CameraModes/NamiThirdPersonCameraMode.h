// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NamiComposableCameraMode.h"
#include "NamiThirdPersonCameraMode.generated.h"

class UNamiCameraSpringArmComponent;

/**
 * 第三人称相机模式
 *
 * 预配置的组合式相机，包含：
 * - SingleTargetCalculator: 跟随单个目标
 * - OffsetPositionCalculator: 使用偏移计算位置
 * - ControlRotationCalculator: 使用控制器旋转
 * - StaticFOVCalculator: 固定 FOV
 * - SpringArmComponent: 碰撞检测
 *
 * 适用于：标准第三人称游戏
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Third Person Camera"))
class NAMICAMERA_API UNamiThirdPersonCameraMode : public UNamiComposableCameraMode
{
	GENERATED_BODY()

public:
	UNamiThirdPersonCameraMode();

	// ========== 生命周期 ==========

	virtual void Initialize_Implementation(UNamiCameraComponent* InCameraComponent) override;

	// ========== 快捷配置 ==========

	/**
	 * 获取弹簧臂组件
	 */
	UFUNCTION(BlueprintPure, Category = "Third Person Camera")
	UNamiCameraSpringArmComponent* GetSpringArmComponent() const;

protected:
	/** 创建默认计算器 */
	void CreateDefaultCalculators();

	/** 创建默认组件 */
	void CreateDefaultComponents();
};
