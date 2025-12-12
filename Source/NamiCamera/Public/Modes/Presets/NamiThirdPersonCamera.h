// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modes/Templates/NamiSpringArmCameraMode.h"
#include "NamiThirdPersonCamera.generated.h"

/**
 * 第三人称相机预设
 *
 * 特点：
 * - 继承 SpringArmCameraMode 的碰撞检测和滞后功能
 * - 可配置的俯仰角限制
 * - 可配置的偏航角限制
 * - Roll 锁定
 *
 * 使用场景：
 * - 标准第三人称游戏
 * - 动作游戏
 * - RPG游戏
 */
UCLASS(Blueprintable, BlueprintType)
class NAMICAMERA_API UNamiThirdPersonCamera : public UNamiSpringArmCameraMode
{
	GENERATED_BODY()

public:
	UNamiThirdPersonCamera();

	// ========== 生命周期 ==========
	virtual void Initialize_Implementation(UNamiCameraComponent* InCameraComponent) override;
	virtual void Activate_Implementation() override;
	virtual FNamiCameraView CalculateView_Implementation(float DeltaTime) override;

protected:
	// ========== 核心计算（重写）==========

	/**
	 * 重写枢轴点计算，使用角色的眼睛位置
	 * 而不是使用Actor的根位置
	 */
	virtual FVector CalculatePivotLocation_Implementation(float DeltaTime) override;

public:
	// ========== 配置 ==========

	// ========== 旋转限制 ==========
	
	/** 是否限制俯仰角 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 旋转限制",
		meta = (InlineEditConditionToggle,
		Tooltip = "是否限制俯仰角"))
	bool bLimitPitch = true;

	/** 最小俯仰角（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 旋转限制",
		meta = (EditCondition = "bLimitPitch", 
		ClampMin = "-89.0", ClampMax = "89.0", UIMin = "-89.0", UIMax = "89.0",
		Tooltip = "相机可以向下看的最大角度，建议范围：-89到0度"))
	float MinPitch = -60.0f;

	/** 最大俯仰角（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 旋转限制",
		meta = (EditCondition = "bLimitPitch",
		ClampMin = "-89.0", ClampMax = "89.0", UIMin = "-89.0", UIMax = "89.0",
		Tooltip = "相机可以向上看的最大角度，建议范围：0到89度"))
	float MaxPitch = 30.0f;

	/** 是否限制偏航角 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 旋转限制",
		meta = (InlineEditConditionToggle,
		Tooltip = "是否限制偏航角"))
	bool bLimitYaw = false;

	/** 最小偏航角（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 旋转限制",
		meta = (EditCondition = "bLimitYaw", 
		ClampMin = "-180.0", ClampMax = "180.0", UIMin = "-180.0", UIMax = "180.0",
		Tooltip = "相机可以向左旋转的最大角度"))
	float MinYaw = -180.0f;

	/** 最大偏航角（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 旋转限制",
		meta = (EditCondition = "bLimitYaw",
		ClampMin = "-180.0", ClampMax = "180.0", UIMin = "-180.0", UIMax = "180.0",
		Tooltip = "相机可以向右旋转的最大角度"))
	float MaxYaw = 180.0f;

	/** 是否锁定Roll旋转 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 旋转限制",
		meta = (Tooltip = "是否将Roll旋转锁定为0度，建议保持开启以避免相机倾斜"))
	bool bLockRoll = true;

	// ========== 输入和行为 ==========

	/** 是否使用相对旋转（相对于目标） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 输入和行为",
		meta = (Tooltip = "相机旋转是否相对于目标旋转，建议开启以获得更自然的跟随效果"))
	bool bUseRelativeRotation = true;

protected:
	/** 上一帧的相机旋转（用于平滑） */
	FRotator LastCameraRotation;
};

