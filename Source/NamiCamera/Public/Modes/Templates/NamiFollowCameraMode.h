// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modes/NamiCameraModeBase.h"
#include "Structs/Follow/NamiFollowTarget.h"
#include "NamiFollowCameraMode.generated.h"

/**
 * 跟随相机模式模板
 *
 * 核心概念：
 * - 相机围绕 PivotLocation（枢轴点）观察
 * - PivotLocation 的计算方式决定了相机的行为
 *
 * 用途：
 * - 第三人称相机
 * - 俯视角相机
 * - 肩部视角相机
 * - 战斗相机
 * - 技能特写相机
 *
 * 继承指南：
 * - 重写 CalculatePivotLocation() 来定义"看哪里"
 * - 重写 CalculateCameraLocation() 来定义"从哪看"
 * - 使用 Feature 来添加弹簧臂、碰撞检测等功能
 *
 * 数据流：
 *   Targets → CalculatePivotLocation() → PivotLocation
 *           → CalculateCameraLocation() → CameraLocation
 *           → CalculateCameraRotation() → CameraRotation
 *           → Feature.ApplyToView()     → FinalView
 */
UCLASS(Blueprintable, BlueprintType)
class NAMICAMERA_API UNamiFollowCameraMode : public UNamiCameraModeBase
{
	GENERATED_BODY()

public:
	UNamiFollowCameraMode();

	// ========== 生命周期 ==========
	virtual void Activate_Implementation() override;
	virtual FNamiCameraView CalculateView_Implementation(float DeltaTime) override;

	// ========== 目标管理 ==========

	/**
	 * 设置主要跟随目标
	 * PivotLocation 将基于此目标计算
	 */
	UFUNCTION(BlueprintCallable, Category = "Follow Camera|Target")
	void SetPrimaryTarget(AActor *Target);

	/**
	 * 获取主要目标
	 */
	UFUNCTION(BlueprintPure, Category = "Follow Camera|Target")
	AActor *GetPrimaryTarget() const;

	/**
	 * 添加辅助目标（参与 PivotLocation 计算）
	 * @param Target 目标Actor
	 * @param Weight 权重
	 * @param TargetType 目标类型
	 */
	UFUNCTION(BlueprintCallable, Category = "Follow Camera|Target")
	void AddTarget(AActor *Target, float Weight = 1.0f, ENamiFollowTargetType TargetType = ENamiFollowTargetType::Secondary);

	/**
	 * 移除目标
	 */
	UFUNCTION(BlueprintCallable, Category = "Follow Camera|Target")
	void RemoveTarget(AActor *Target);

	/**
	 * 清除所有目标
	 */
	UFUNCTION(BlueprintCallable, Category = "Follow Camera|Target")
	void ClearAllTargets();

	/**
	 * 获取所有目标
	 */
	UFUNCTION(BlueprintPure, Category = "Follow Camera|Target")
	const TArray<FNamiFollowTarget> &GetTargets() const { return Targets; }

	/**
	 * 直接设置 PivotLocation（不使用目标计算）
	 */
	UFUNCTION(BlueprintCallable, Category = "Follow Camera|Target")
	void SetCustomPivotLocation(const FVector &Location);

	/**
	 * 清除自定义 PivotLocation（恢复使用目标计算）
	 */
	UFUNCTION(BlueprintCallable, Category = "Follow Camera|Target")
	void ClearCustomPivotLocation();

	/**
	 * 获取当前 PivotLocation
	 */
	UFUNCTION(BlueprintPure, Category = "Follow Camera|Target")
	FVector GetPivotLocation() const { return CurrentPivotLocation; }

protected:
	// ========== 核心计算（子类可重写）==========

	/**
	 * 计算 PivotLocation
	 * 子类可重写此方法来自定义枢轴点计算逻辑
	 *
	 * 默认实现：
	 * - 如果有自定义 PivotLocation，使用它
	 * - 如果只有主目标，使用主目标位置
	 * - 如果有多个目标，计算加权中心
	 *
	 * @return 枢轴点位置
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Follow Camera|Calculation")
	FVector CalculatePivotLocation() const;

	/**
	 * 计算相机位置
	 * 基于 PivotLocation 计算相机应该在哪里
	 *
	 * 默认实现：PivotLocation + CameraOffset（考虑旋转）
	 *
	 * @param InPivotLocation 枢轴点位置
	 * @return 相机位置
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Follow Camera|Calculation")
	FVector CalculateCameraLocation(const FVector &InPivotLocation) const;

	/**
	 * 计算相机朝向
	 * 默认实现：看向 PivotLocation
	 *
	 * @param InCameraLocation 相机位置
	 * @param InPivotLocation 枢轴点位置
	 * @return 相机旋转
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Follow Camera|Calculation")
	FRotator CalculateCameraRotation(const FVector &InCameraLocation, const FVector &InPivotLocation) const;

	/**
	 * 获取主目标的旋转
	 * 用于计算相机偏移方向
	 */
	FRotator GetPrimaryTargetRotation() const;

	/**
	 * 应用平滑
	 */
	void ApplySmoothing(float DeltaTime);

public:
	// ========== 配置 ==========

	/** 相机偏移（相对于 PivotLocation） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Offset",
			  meta = (DisplayName = "相机偏移"))
	FVector CameraOffset = FVector(-300.0f, 0.0f, 100.0f);

	/** 是否使用主目标的旋转来计算偏移方向 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Offset",
			  meta = (DisplayName = "使用目标旋转"))
	bool bUseTargetRotation = true;

	/** 是否只使用Yaw旋转（忽略Pitch和Roll） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Offset",
			  meta = (DisplayName = "只用Yaw", EditCondition = "bUseTargetRotation"))
	bool bUseYawOnly = true;

	/** 枢轴点额外高度偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Offset",
			  meta = (DisplayName = "枢轴点高度偏移"))
	float PivotHeightOffset = 0.0f;

	/**
	 * 是否启用平滑
	 * 如果禁用，相机将立即跟随目标，无延迟
	 * 如果启用，相机会根据平滑时间参数平滑跟随目标
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Smoothing",
			  meta = (DisplayName = "启用平滑", InlineEditConditionToggle))
	bool bEnableSmoothing = true;

	/** 是否启用观察点平滑 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Smoothing",
			  meta = (DisplayName = "启用观察点平滑", EditCondition = "bEnableSmoothing",
					  Tooltip = "是否启用观察点（PivotLocation）的平滑跟随"))
	bool bEnablePivotSmoothing = true;

	/** 是否启用相机位置平滑 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Smoothing",
			  meta = (DisplayName = "启用相机位置平滑", EditCondition = "bEnableSmoothing",
					  Tooltip = "是否启用相机位置的平滑移动"))
	bool bEnableCameraLocationSmoothing = true;

	/** 是否启用相机旋转平滑 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Smoothing",
			  meta = (DisplayName = "启用相机旋转平滑", EditCondition = "bEnableSmoothing",
					  Tooltip = "是否启用相机旋转的平滑过渡"))
	bool bEnableCameraRotationSmoothing = true;

	/**
	 * 枢轴点平滑强度
	 * 控制相机观察点（PivotLocation）跟随目标的平滑程度
	 * - 配置范围：0.0-1.0
	 * - 应用范围：映射到0.0-2.0的实际平滑时间
	 * - 值越小：跟随越快，几乎无延迟，适合快速移动的游戏
	 * - 值越大：跟随越慢但更平滑，适合慢节奏游戏
	 *
	 * 建议值：
	 * - 俯视角/快节奏：0.0-0.2
	 * - 第三人称/平衡：0.2-0.5
	 * - 电影感/慢节奏：0.5-0.8
	 * - 极致平滑：0.8-1.0
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Smoothing",
			  meta = (DisplayName = "观察点平滑强度", ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableSmoothing",
					  Tooltip = "控制相机观察点跟随目标的平滑程度。配置范围0.0-1.0，实际映射到0.0-2.0的平滑时间。值越小跟随越快，值越大越平滑但延迟越大。"))
	float PivotSmoothIntensity = 0.0f;

	/**
	 * 相机位置平滑强度
	 * 控制相机位置移动的平滑程度
	 * - 配置范围：0.0-1.0
	 * - 应用范围：映射到0.0-2.0的实际平滑时间
	 * - 值越小：相机移动越快，响应更及时
	 * - 值越大：相机移动越慢，但更平滑
	 * - 通常应该略大于 PivotSmoothIntensity，以获得更自然的跟随效果
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Smoothing",
			  meta = (DisplayName = "相机位置平滑强度", ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableSmoothing",
					  Tooltip = "控制相机位置移动的平滑程度。配置范围0.0-1.0，实际映射到0.0-2.0的平滑时间。值越小移动越快，值越大越平滑。"))
	float CameraLocationSmoothIntensity = 0.0f;

	/**
	 * 相机旋转平滑强度
	 * 控制相机旋转的平滑程度
	 * - 配置范围：0.0-1.0
	 * - 应用范围：映射到0.0-2.0的实际平滑时间
	 * - 值越小：旋转越快，响应更及时
	 * - 值越大：旋转越慢，但更平滑
	 * - 通常可以与 PivotSmoothIntensity 相同或略小
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Smoothing",
			  meta = (DisplayName = "相机旋转平滑强度", ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableSmoothing",
					  Tooltip = "控制相机旋转的平滑程度。配置范围0.0-1.0，实际映射到0.0-2.0的平滑时间。值越小旋转越快，值越大越平滑。"))
	float CameraRotationSmoothIntensity = 0.0f;

	// ========== 动态FOV配置 ==========

	/** 是否启用动态FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Dynamic FOV",
			  meta = (DisplayName = "启用动态FOV", Tooltip = "是否根据角色速度或其他因素动态调整FOV"))
	bool bEnableDynamicFOV = false;

	/** 基础FOV值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Dynamic FOV",
			  meta = (DisplayName = "基础FOV", EditCondition = "bEnableDynamicFOV", ClampMin = "30.0", ClampMax = "120.0",
					  Tooltip = "相机的基础视野角度"))
	float BaseFOV = 90.0f;

	/** 最小FOV值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Dynamic FOV",
			  meta = (DisplayName = "最小FOV", EditCondition = "bEnableDynamicFOV", ClampMin = "30.0", ClampMax = "120.0",
					  Tooltip = "动态FOV的最小允许值"))
	float MinDynamicFOV = 60.0f;

	/** 最大FOV值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Dynamic FOV",
			  meta = (DisplayName = "最大FOV", EditCondition = "bEnableDynamicFOV", ClampMin = "30.0", ClampMax = "120.0",
					  Tooltip = "动态FOV的最大允许值"))
	float MaxDynamicFOV = 110.0f;

	/** FOV变化速率（每秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Dynamic FOV",
			  meta = (DisplayName = "FOV变化速率", EditCondition = "bEnableDynamicFOV", ClampMin = "0.0", ClampMax = "100.0",
					  Tooltip = "FOV每秒变化的最大速率，用于平滑过渡FOV变化"))
	float DynamicFOVChangeRate = 20.0f;

	/** 速度影响FOV的系数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Dynamic FOV",
			  meta = (DisplayName = "速度影响系数", EditCondition = "bEnableDynamicFOV", ClampMin = "0.0", ClampMax = "10.0",
					  Tooltip = "角色速度对FOV的影响程度，值越大FOV随速度变化越明显"))
	float SpeedFOVFactor = 1.0f;

protected:
	/** 目标列表 */
	UPROPERTY()
	TArray<FNamiFollowTarget> Targets;

	/** 自定义枢轴点 */
	FVector CustomPivotLocation = FVector::ZeroVector;
	bool bUseCustomPivotLocation = false;

	/** 当前状态（平滑后） */
	FVector CurrentPivotLocation = FVector::ZeroVector;
	FVector CurrentCameraLocation = FVector::ZeroVector;
	FRotator CurrentCameraRotation = FRotator::ZeroRotator;

	/** 平滑速度 */
	FVector PivotVelocity = FVector::ZeroVector;
	FVector CameraVelocity = FVector::ZeroVector;
	float YawVelocity = 0.0f;
	float PitchVelocity = 0.0f;

	/** 是否已初始化 */
	bool bInitialized = false;

	// ========== 动态FOV相关 ==========

	/** 当前动态FOV值 */
	float CurrentDynamicFOV = 90.0f;

	/** FOV平滑速度 */
	float DynamicFOVSmoothVelocity = 0.0f;
};
