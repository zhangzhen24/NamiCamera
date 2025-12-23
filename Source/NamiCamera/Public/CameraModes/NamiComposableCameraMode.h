// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NamiCameraModeBase.h"
#include "Calculators/FOV/NamiCameraFOVCalculator.h"
#include "Calculators/Position/NamiCameraPositionCalculator.h"
#include "Calculators/Rotation/NamiCameraRotationCalculator.h"
#include "Calculators/Target/NamiCameraTargetCalculator.h"
#include "NamiComposableCameraMode.generated.h"

/**
 * 组合式相机模式
 *
 * 通过计算器模式组合不同的相机行为：
 * - TargetCalculator: 目标计算器（计算 PivotLocation）
 * - PositionCalculator: 位置计算器（计算 CameraLocation）
 * - RotationCalculator: 旋转计算器（计算 CameraRotation）
 * - FOVCalculator: FOV 计算器（计算 FOV）
 *
 * 使用示例：
 * - 第三人称相机：SingleTarget + Offset + ControlRotation + StaticFOV
 * - 双焦点相机：DualFocusTarget + EllipseOrbit + LookAt + FramingFOV
 * - 软锁定相机：SingleTarget + Offset + SoftLock + DistanceBasedFOV
 *
 * ModeComponent 可用于后处理，如：
 * - SpringArmComponent: 碰撞检测
 * - LockOnComponent: 锁定目标管理
 */
UCLASS(Blueprintable, BlueprintType)
class NAMICAMERA_API UNamiComposableCameraMode : public UNamiCameraModeBase
{
	GENERATED_BODY()

public:
	UNamiComposableCameraMode();

	// ========== 生命周期 ==========

	virtual void Initialize_Implementation(UNamiCameraComponent* InCameraComponent) override;
	virtual void Activate_Implementation() override;
	virtual void Deactivate_Implementation() override;
	virtual FNamiCameraView CalculateView_Implementation(float DeltaTime) override;

	// ========== 目标管理 ==========

	/**
	 * 设置主要目标
	 * 会同步设置到 TargetCalculator
	 */
	UFUNCTION(BlueprintCallable, Category = "Composable Camera|Target")
	void SetPrimaryTarget(AActor* Target);

	/**
	 * 获取主要目标
	 */
	UFUNCTION(BlueprintPure, Category = "Composable Camera|Target")
	AActor* GetPrimaryTarget() const;

	// ========== Calculator API（新统一接口） ==========

	/** 获取目标计算器 */
	UFUNCTION(BlueprintPure, Category = "Composable Camera|Calculators", meta = (DisplayName = "Get Target Calculator"))
	UNamiCameraTargetCalculator* GetTargetCalculator() const { return TargetCalculator; }

	/** 获取位置计算器 */
	UFUNCTION(BlueprintPure, Category = "Composable Camera|Calculators", meta = (DisplayName = "Get Position Calculator"))
	UNamiCameraPositionCalculator* GetPositionCalculator() const { return PositionCalculator; }

	/** 获取旋转计算器 */
	UFUNCTION(BlueprintPure, Category = "Composable Camera|Calculators", meta = (DisplayName = "Get Rotation Calculator"))
	UNamiCameraRotationCalculator* GetRotationCalculator() const { return RotationCalculator; }

	/** 获取 FOV 计算器 */
	UFUNCTION(BlueprintPure, Category = "Composable Camera|Calculators", meta = (DisplayName = "Get FOV Calculator"))
	UNamiCameraFOVCalculator* GetFOVCalculator() const { return FOVCalculator; }

	/** 设置目标计算器 */
	UFUNCTION(BlueprintCallable, Category = "Composable Camera|Calculators", meta = (DisplayName = "Set Target Calculator"))
	void SetTargetCalculator(UNamiCameraTargetCalculator* InCalculator);

	/** 设置位置计算器 */
	UFUNCTION(BlueprintCallable, Category = "Composable Camera|Calculators", meta = (DisplayName = "Set Position Calculator"))
	void SetPositionCalculator(UNamiCameraPositionCalculator* InCalculator);

	/** 设置旋转计算器 */
	UFUNCTION(BlueprintCallable, Category = "Composable Camera|Calculators", meta = (DisplayName = "Set Rotation Calculator"))
	void SetRotationCalculator(UNamiCameraRotationCalculator* InCalculator);

	/** 设置 FOV 计算器 */
	UFUNCTION(BlueprintCallable, Category = "Composable Camera|Calculators", meta = (DisplayName = "Set FOV Calculator"))
	void SetFOVCalculator(UNamiCameraFOVCalculator* InCalculator);

protected:
	// ========== 辅助方法 ==========

	/**
	 * 获取控制旋转
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Composable Camera")
	FRotator GetControlRotation() const;

	/**
	 * 初始化计算器（从 Instanced 属性创建实例）
	 */
	void InitializeCalculators();

public:
	// ========== 计算器配置（编辑器可配置） ==========

	/**
	 * 目标计算器
	 * 负责计算相机跟随的目标位置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "1. Calculators|Target")
	TObjectPtr<UNamiCameraTargetCalculator> TargetCalculator;

	/**
	 * 位置计算器
	 * 负责根据目标位置计算相机位置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "1. Calculators|Position")
	TObjectPtr<UNamiCameraPositionCalculator> PositionCalculator;

	/**
	 * 旋转计算器
	 * 负责计算相机旋转
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "1. Calculators|Rotation")
	TObjectPtr<UNamiCameraRotationCalculator> RotationCalculator;

	/**
	 * FOV 计算器
	 * 负责计算相机视野
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "1. Calculators|FOV")
	TObjectPtr<UNamiCameraFOVCalculator> FOVCalculator;

	// ========== 视点偏移配置 ==========

	/**
	 * 视点偏移（相对于 PivotLocation）
	 * 用于调整相机看向的点
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. Pivot")
	FVector PivotOffset = FVector::ZeroVector;

	/**
	 * 是否使用控制旋转来旋转视点偏移
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. Pivot")
	bool bPivotOffsetUseControlRotation = false;

	/**
	 * 视点偏移只使用 Yaw
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. Pivot", meta = (EditCondition = "bPivotOffsetUseControlRotation"))
	bool bPivotOffsetUseYawOnly = true;

protected:
	/** 当前枢轴位置 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FVector CurrentPivotLocation;

	/** 当前相机位置 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FVector CurrentCameraLocation;

	/** 当前相机旋转 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FRotator CurrentCameraRotation;

	/** 缓存的控制旋转 */
	mutable FRotator CachedControlRotation;

	/** 是否已初始化 */
	bool bCalculatorsInitialized = false;
};
