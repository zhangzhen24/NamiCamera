// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modes/Templates/NamiFollowCameraMode.h"
#include "Components/NamiSpringArm.h"
#include "NamiThirdPersonCamera.generated.h"

/**
 * 第三人称相机预设
 * 
 * 特点：
 * - 使用SpringArm进行碰撞检测
 * - 支持相机滞后（Camera Lag）
 * - 支持旋转滞后（Rotation Lag）
 * - 可配置的俯仰角限制
 * 
 * 使用场景：
 * - 标准第三人称游戏
 * - 动作游戏
 * - RPG游戏
 */
UCLASS(Blueprintable, BlueprintType)
class NAMICAMERA_API UNamiThirdPersonCamera : public UNamiFollowCameraMode
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
	virtual FVector CalculateCameraLocation_Implementation(const FVector& InPivotLocation) const override;
	virtual FRotator CalculateCameraRotation_Implementation(
		const FVector& InCameraLocation, 
		const FVector& InPivotLocation) const override;

public:
	// ========== 配置 ==========

	/** SpringArm配置（弹簧臂） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person|SpringArm", 
		meta = (ShowInnerProperties, Tooltip = "弹簧臂配置，用于碰撞检测和相机距离控制"))
	FNamiSpringArm SpringArm;

	// ========== 旋转配置 ==========
	
	/** 是否限制俯仰角 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person|Rotation",
		meta = (DisplayName = "限制俯仰角", InlineEditConditionToggle))
	bool bLimitPitch = true;

	/** 最小俯仰角（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person|Rotation",
		meta = (DisplayName = "最小俯仰角", EditCondition = "bLimitPitch", 
		ClampMin = "-89.0", ClampMax = "89.0", UIMin = "-89.0", UIMax = "89.0",
		Tooltip = "相机可以向下看的最大角度，建议范围：-89到0度"))
	float MinPitch = -60.0f;

	/** 最大俯仰角（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person|Rotation",
		meta = (DisplayName = "最大俯仰角", EditCondition = "bLimitPitch",
		ClampMin = "-89.0", ClampMax = "89.0", UIMin = "-89.0", UIMax = "89.0",
		Tooltip = "相机可以向上看的最大角度，建议范围：0到89度"))
	float MaxPitch = 30.0f;

	/** 是否限制偏航角 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person|Rotation",
		meta = (DisplayName = "限制偏航角", InlineEditConditionToggle))
	bool bLimitYaw = false;

	/** 最小偏航角（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person|Rotation",
		meta = (DisplayName = "最小偏航角", EditCondition = "bLimitYaw", 
		ClampMin = "-180.0", ClampMax = "180.0", UIMin = "-180.0", UIMax = "180.0",
		Tooltip = "相机可以向左旋转的最大角度"))
	float MinYaw = -180.0f;

	/** 最大偏航角（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person|Rotation",
		meta = (DisplayName = "最大偏航角", EditCondition = "bLimitYaw",
		ClampMin = "-180.0", ClampMax = "180.0", UIMin = "-180.0", UIMax = "180.0",
		Tooltip = "相机可以向右旋转的最大角度"))
	float MaxYaw = 180.0f;

	/** 是否锁定Roll旋转 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person|Rotation",
		meta = (DisplayName = "锁定Roll", Tooltip = "是否将Roll旋转锁定为0度，建议保持开启以避免相机倾斜"))
	bool bLockRoll = true;

	// ========== 相机行为配置 ==========
	


	/** 鼠标灵敏度缩放 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person|Input",
		meta = (DisplayName = "鼠标灵敏度", ClampMin = "0.1", ClampMax = "5.0", UIMin = "0.1", UIMax = "5.0",
		Tooltip = "调整鼠标控制相机的灵敏度，值越大相机旋转越快"))
	float MouseSensitivity = 1.0f;

	/** 是否使用相对旋转（相对于目标） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person|Behavior",
		meta = (DisplayName = "使用相对旋转", Tooltip = "相机旋转是否相对于目标旋转，建议开启以获得更自然的跟随效果"))
	bool bUseRelativeRotation = true;

	/** 相机距离缩放因子 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person|Behavior",
		meta = (DisplayName = "相机距离缩放", ClampMin = "0.5", ClampMax = "2.0", UIMin = "0.5", UIMax = "2.0",
		Tooltip = "调整相机与目标的距离，值越大距离越远"))
	float CameraDistanceScale = 1.0f;

protected:
	/** 获取控制旋转（从PlayerController获取） */
	FRotator GetControlRotation() const;

protected:
	/** 上一帧的相机旋转（用于平滑） */
	FRotator LastCameraRotation;
};

