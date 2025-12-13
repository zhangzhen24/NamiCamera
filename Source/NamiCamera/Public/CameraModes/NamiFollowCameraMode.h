// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CameraModes/NamiCameraModeBase.h"
#include "CameraModes/NamiFollowTarget.h"
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
	virtual FVector CalculatePivotLocation_Implementation(float DeltaTime) override;

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
	 * 获取控制旋转（Control Rotation）
	 * 从 OwnerPawn 或 PlayerController 获取，并自动归一化到 0-360 度
	 *
	 * @return 归一化后的控制旋转
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Follow Camera|Rotation")
	FRotator GetControlRotation() const;

	/**
	 * 应用旋转限制和归一化
	 * 根据配置限制 Pitch、Yaw，锁定 Roll，并归一化旋转
	 *
	 * @param InRotation 输入的旋转
	 * @param bApplyPitchLimit 是否应用俯仰角限制
	 * @param InMinPitch 最小俯仰角
	 * @param InMaxPitch 最大俯仰角
	 * @param bApplyYawLimit 是否应用偏航角限制
	 * @param InMinYaw 最小偏航角
	 * @param InMaxYaw 最大偏航角
	 * @param bApplyRollLock 是否锁定 Roll 为 0
	 * @return 处理后的旋转（已归一化）
	 */
	UFUNCTION(BlueprintCallable, Category = "Follow Camera|Rotation")
	FRotator ApplyRotationConstraints(
		const FRotator& InRotation,
		bool bApplyPitchLimit = false,
		float InMinPitch = -89.0f,
		float InMaxPitch = 89.0f,
		bool bApplyYawLimit = false,
		float InMinYaw = -180.0f,
		float InMaxYaw = 180.0f,
		bool bApplyRollLock = true) const;

	/**
	 * 应用 PivotLocationOffset 到基础 PivotLocation
	 * 根据配置应用偏移，并考虑控制器旋转
	 *
	 * @param InBasePivot 基础枢轴点位置
	 * @return 应用偏移后的枢轴点位置
	 */
	FVector ApplyPivotLocationOffset(const FVector& InBasePivot) const;


public:
	// ========== 配置 ==========

	/** 视点偏移（PivotLocation偏移，相对于目标位置） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Offset",
			  meta = (Tooltip = "视点偏移（PivotLocation偏移，相对于目标位置）"))
	FVector PivotLocationOffset = FVector::ZeroVector;

	/** 是否使用控制器旋转（ControlRotation）来计算视点偏移方向 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Offset",
			  meta = (Tooltip = "是否使用控制器旋转（ControlRotation）来计算视点偏移方向"))
	bool bPivotOffsetUseTargetRotation = true;

	/** 视点偏移是否只使用Yaw旋转（忽略Pitch和Roll） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Offset",
			  meta = (EditCondition = "bPivotOffsetUseTargetRotation",
					  Tooltip = "视点偏移是否只使用Yaw旋转（忽略Pitch和Roll）"))
	bool bPivotOffsetUseYawOnly = true;

	/** 相机偏移（相对于 PivotLocation） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Offset",
			  meta = (Tooltip = "相机偏移（相对于 PivotLocation）"))
	FVector CameraOffset = FVector(-300.0f, 0.0f, 100.0f);

	/** 是否使用主目标的旋转来计算偏移方向 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Offset",
			  meta = (Tooltip = "是否使用主目标的旋转来计算偏移方向"))
	bool bUseTargetRotation = true;

	/** 是否只使用Yaw旋转（忽略Pitch和Roll） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Offset",
			  meta = (EditCondition = "bUseTargetRotation",
					  Tooltip = "是否只使用Yaw旋转（忽略Pitch和Roll）"))
	bool bUseYawOnly = true;

	// ========== 动态FOV配置 ==========

	/** 是否启用动态FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Dynamic FOV",
			  meta = (Tooltip = "是否根据角色速度或其他因素动态调整FOV"))
	bool bEnableDynamicFOV = false;

	/** 基础FOV值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Dynamic FOV",
			  meta = (EditCondition = "bEnableDynamicFOV", ClampMin = "30.0", ClampMax = "120.0",
					  Tooltip = "相机的基础视野角度"))
	float BaseFOV = 90.0f;

	/** 最小FOV值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Dynamic FOV",
			  meta = (EditCondition = "bEnableDynamicFOV", ClampMin = "30.0", ClampMax = "120.0",
					  Tooltip = "动态FOV的最小允许值"))
	float MinDynamicFOV = 60.0f;

	/** 最大FOV值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Dynamic FOV",
			  meta = (EditCondition = "bEnableDynamicFOV", ClampMin = "30.0", ClampMax = "120.0",
					  Tooltip = "动态FOV的最大允许值"))
	float MaxDynamicFOV = 110.0f;

	/** FOV变化速率（每秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Dynamic FOV",
			  meta = (EditCondition = "bEnableDynamicFOV", ClampMin = "0.0", ClampMax = "100.0",
					  Tooltip = "FOV每秒变化的最大速率，用于平滑过渡FOV变化"))
	float DynamicFOVChangeRate = 20.0f;

	/** 速度影响FOV的系数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Camera|Dynamic FOV",
			  meta = (EditCondition = "bEnableDynamicFOV", ClampMin = "0.0", ClampMax = "10.0",
					  Tooltip = "角色速度对FOV的影响程度，值越大FOV随速度变化越明显"))
	float SpeedFOVFactor = 1.0f;

protected:
	/** 目标列表 */
	UPROPERTY()
	TArray<FNamiFollowTarget> Targets;

	/** 上一次有效的控制旋转（用于后备） */
	mutable FRotator LastValidControlRotation;

	/** 自定义枢轴点 */
	FVector CustomPivotLocation = FVector::ZeroVector;
	bool bUseCustomPivotLocation = false;

	/** 当前状态 */
	FVector CurrentPivotLocation = FVector::ZeroVector;
	FVector CurrentCameraLocation = FVector::ZeroVector;
	FRotator CurrentCameraRotation = FRotator::ZeroRotator;

	/** 是否已初始化 */
	bool bInitialized = false;

	// ========== 动态FOV相关 ==========

	/** 当前动态FOV值 */
	float CurrentDynamicFOV = 90.0f;

	/** FOV平滑速度 */
	float DynamicFOVSmoothVelocity = 0.0f;
};
