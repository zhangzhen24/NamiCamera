// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modes/Templates/NamiFollowCameraMode.h"
#include "Components/NamiSpringArm.h"
#include "NamiThirdPersonCamera.generated.h"

/**
 * 第三人称相机预设类型
 */
UENUM(BlueprintType)
enum class ENamiThirdPersonCameraPreset : uint8
{
	/** 自定义配置 */
	Custom UMETA(DisplayName = "自定义"),
	
	/** 近距离战斗（150-250单位，快速响应） */
	CloseCombat UMETA(DisplayName = "近距离战斗"),
	
	/** 标准第三人称（300-400单位，平衡） */
	Standard UMETA(DisplayName = "标准第三人称"),
	
	/** 远距离探索（500-800单位，广阔视野） */
	Exploration UMETA(DisplayName = "远距离探索"),
	
	/** 电影感（400-600单位，平滑滞后） */
	Cinematic UMETA(DisplayName = "电影感"),
	
	/** 竞技（200-300单位，精确控制） */
	Competitive UMETA(DisplayName = "竞技")
};

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

	// ========== 快捷配置（常用设置）==========
	
	/**
	 * 相机预设
	 * 选择预设配置可以快速设置相机参数
	 * 选择"自定义"后可以手动调整所有参数
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 快捷配置",
		meta = (DisplayName = "相机预设"))
	ENamiThirdPersonCameraPreset CameraPreset = ENamiThirdPersonCameraPreset::Standard;
	
	/** 
	 * 相机距离（快捷配置）
	 * 控制相机与角色的距离，值越大距离越远
	 * 这是最常用的配置项，直接暴露在顶层方便调整
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 快捷配置", 
		meta = (DisplayName = "相机距离", ClampMin = "50.0", ClampMax = "2000.0", UIMin = "50.0", UIMax = "2000.0",
		Tooltip = "相机与角色的距离。推荐值：近距离战斗 150-250，标准第三人称 300-400，远距离探索 500-800"))
	float CameraDistance = 350.0f;
	
	/**
	 * 是否启用碰撞检测（快捷配置）
	 * 开启后相机会避免穿透墙壁和障碍物
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 快捷配置",
		meta = (DisplayName = "启用碰撞检测", Tooltip = "开启后相机会自动避免穿透墙壁和障碍物，建议保持开启"))
	bool bEnableCollision = true;
	
	/**
	 * 是否启用相机滞后（快捷配置）
	 * 开启后相机移动会有延迟，让画面更平滑
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 快捷配置",
		meta = (DisplayName = "启用相机滞后", Tooltip = "开启后相机移动会有延迟效果，让画面更平滑。适合慢节奏游戏"))
	bool bEnableCameraLag = false;
	
	/**
	 * 相机滞后速度（快捷配置）
	 * 值越大相机跟随越快，值越小延迟越明显
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 快捷配置",
		meta = (DisplayName = "相机滞后速度", EditCondition = "bEnableCameraLag", 
		ClampMin = "0.0", ClampMax = "100.0", UIMin = "1.0", UIMax = "20.0",
		Tooltip = "控制相机跟随的速度。推荐值：快速响应 15-20，平衡 8-12，慢速平滑 3-6"))
	float CameraLagSpeed = 10.0f;

	// ========== SpringArm Feature ==========
	
	/**
	 * 获取 SpringArmFeature（便捷方法）
	 * SpringArm 功能现在通过 Feature 系统实现
	 * 在 Initialize 时会自动创建 SpringArmFeature
	 */
	UFUNCTION(BlueprintPure, Category = "Third Person|SpringArm")
	class UNamiSpringArmFeature* GetSpringArmFeature() const;
	
	/**
	 * 设置相机距离（便捷方法）
	 * 直接设置相机与角色的距离
	 */
	UFUNCTION(BlueprintCallable, Category = "Third Person|Quick Config")
	void SetCameraDistance(float NewDistance);
	
	/**
	 * 设置碰撞检测（便捷方法）
	 */
	UFUNCTION(BlueprintCallable, Category = "Third Person|Quick Config")
	void SetCollisionEnabled(bool bEnabled);
	
	/**
	 * 设置相机滞后（便捷方法）
	 */
	UFUNCTION(BlueprintCallable, Category = "Third Person|Quick Config")
	void SetCameraLag(bool bEnabled, float LagSpeed = 10.0f);
	
	/**
	 * 应用相机预设（便捷方法）
	 * 根据预设类型快速配置相机参数
	 */
	UFUNCTION(BlueprintCallable, Category = "Third Person|Quick Config")
	void ApplyCameraPreset(ENamiThirdPersonCameraPreset Preset);

	// ========== 旋转限制 ==========
	
	/** 是否限制俯仰角 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 旋转限制",
		meta = (DisplayName = "限制俯仰角", InlineEditConditionToggle))
	bool bLimitPitch = true;

	/** 最小俯仰角（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 旋转限制",
		meta = (DisplayName = "最小俯仰角", EditCondition = "bLimitPitch", 
		ClampMin = "-89.0", ClampMax = "89.0", UIMin = "-89.0", UIMax = "89.0",
		Tooltip = "相机可以向下看的最大角度，建议范围：-89到0度"))
	float MinPitch = -60.0f;

	/** 最大俯仰角（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 旋转限制",
		meta = (DisplayName = "最大俯仰角", EditCondition = "bLimitPitch",
		ClampMin = "-89.0", ClampMax = "89.0", UIMin = "-89.0", UIMax = "89.0",
		Tooltip = "相机可以向上看的最大角度，建议范围：0到89度"))
	float MaxPitch = 30.0f;

	/** 是否限制偏航角 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 旋转限制",
		meta = (DisplayName = "限制偏航角", InlineEditConditionToggle))
	bool bLimitYaw = false;

	/** 最小偏航角（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 旋转限制",
		meta = (DisplayName = "最小偏航角", EditCondition = "bLimitYaw", 
		ClampMin = "-180.0", ClampMax = "180.0", UIMin = "-180.0", UIMax = "180.0",
		Tooltip = "相机可以向左旋转的最大角度"))
	float MinYaw = -180.0f;

	/** 最大偏航角（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 旋转限制",
		meta = (DisplayName = "最大偏航角", EditCondition = "bLimitYaw",
		ClampMin = "-180.0", ClampMax = "180.0", UIMin = "-180.0", UIMax = "180.0",
		Tooltip = "相机可以向右旋转的最大角度"))
	float MaxYaw = 180.0f;

	/** 是否锁定Roll旋转 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 旋转限制",
		meta = (DisplayName = "锁定Roll", Tooltip = "是否将Roll旋转锁定为0度，建议保持开启以避免相机倾斜"))
	bool bLockRoll = true;

	// ========== 输入和行为 ==========

	/** 鼠标灵敏度缩放 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 输入和行为",
		meta = (DisplayName = "鼠标灵敏度", ClampMin = "0.1", ClampMax = "5.0", UIMin = "0.1", UIMax = "5.0",
		Tooltip = "调整鼠标控制相机的灵敏度，值越大相机旋转越快"))
	float MouseSensitivity = 1.0f;

	/** 是否使用相对旋转（相对于目标） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 输入和行为",
		meta = (DisplayName = "使用相对旋转", Tooltip = "相机旋转是否相对于目标旋转，建议开启以获得更自然的跟随效果"))
	bool bUseRelativeRotation = true;

	/** 相机距离缩放因子 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 输入和行为",
		meta = (DisplayName = "相机距离缩放", ClampMin = "0.5", ClampMax = "2.0", UIMin = "0.5", UIMax = "2.0",
		Tooltip = "调整相机与目标的距离，值越大距离越远"))
	float CameraDistanceScale = 1.0f;

protected:
	/** 获取控制旋转（从PlayerController获取） */
	FRotator GetControlRotation() const;
	
	/** 同步快捷配置到 SpringArmFeature */
	void SyncQuickConfigToFeature();

protected:
	/** 上一帧的相机旋转（用于平滑） */
	FRotator LastCameraRotation;
	
	/** 上一次有效的控制旋转（用于后备） */
	mutable FRotator LastValidControlRotation;
};

