// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Calculators/Position/NamiCameraPositionCalculator.h"
#include "Interfaces/NamiLockOnTargetProvider.h"
#include "NamiEllipseOrbitPositionCalculator.generated.h"

/**
 * 椭圆轨道位置计算器
 *
 * 相机围绕双焦点中心的椭圆轨道移动
 * 支持玩家输入控制轨道角度
 *
 * 适用于：鬼泣风格战斗相机
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Ellipse Orbit Position"))
class NAMICAMERA_API UNamiEllipseOrbitPositionCalculator : public UNamiCameraPositionCalculator
{
	GENERATED_BODY()

public:
	UNamiEllipseOrbitPositionCalculator();

	virtual void Activate_Implementation() override;

	// ========== 核心接口 ==========

	virtual FVector CalculateCameraPosition_Implementation(
		const FVector& PivotLocation,
		const FRotator& ControlRotation,
		float DeltaTime) override;

	// ========== 锁定目标 ==========

	/**
	 * 设置主要目标（玩家）
	 */
	UFUNCTION(BlueprintCallable, Category = "Ellipse Orbit")
	void SetPrimaryTarget(AActor* Target);

	/**
	 * 设置锁定目标提供者
	 */
	UFUNCTION(BlueprintCallable, Category = "Ellipse Orbit")
	void SetLockOnProvider(TScriptInterface<INamiLockOnTargetProvider> Provider);

	// ========== 轨道控制 ==========

	/**
	 * 添加输入到轨道角度
	 */
	UFUNCTION(BlueprintCallable, Category = "Ellipse Orbit|Input")
	void AddOrbitInput(float DeltaAngle);

	/**
	 * 设置目标轨道角度
	 */
	UFUNCTION(BlueprintCallable, Category = "Ellipse Orbit|Input")
	void SetTargetOrbitAngle(float Angle);

	/**
	 * 获取当前轨道角度
	 */
	UFUNCTION(BlueprintPure, Category = "Ellipse Orbit|Input")
	float GetCurrentOrbitAngle() const { return CurrentOrbitAngle; }

public:
	// ========== 椭圆配置 ==========

	/** 椭圆长轴半径（玩家-目标方向） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit", meta = (ClampMin = "100.0"))
	float EllipseMajorRadius = 800.0f;

	/** 椭圆短轴半径（侧向） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit", meta = (ClampMin = "100.0"))
	float EllipseMinorRadius = 500.0f;

	/** 最小相机距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit", meta = (ClampMin = "100.0"))
	float MinCameraDistance = 400.0f;

	/** 最大相机距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit", meta = (ClampMin = "100.0"))
	float MaxCameraDistance = 1200.0f;

	/** 高度偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit")
	float HeightOffset = 150.0f;

	// ========== 输入配置 ==========

	/** 是否启用玩家输入 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit|Input")
	bool bEnablePlayerInput = true;

	/** 输入灵敏度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit|Input", meta = (EditCondition = "bEnablePlayerInput", ClampMin = "0.0"))
	float InputSensitivity = 1.0f;

	/** 轨道角度平滑速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit|Input", meta = (ClampMin = "0.0"))
	float OrbitAngleSmoothSpeed = 5.0f;

	/** 是否限制轨道角度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit|Input")
	bool bClampOrbitAngle = true;

	/** 最大轨道角度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit|Input", meta = (EditCondition = "bClampOrbitAngle", ClampMin = "0.0", ClampMax = "180.0"))
	float MaxOrbitAngle = 120.0f;

	/** 默认轨道角度（激活时使用，禁用玩家输入时的固定角度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit|Input", meta = (ClampMin = "-180.0", ClampMax = "180.0"))
	float DefaultOrbitAngle = 0.0f;

	// ========== 自适应配置 ==========

	/** 是否启用自适应距离（根据角色距离调整椭圆大小） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit|Adaptive")
	bool bEnableAdaptiveDistance = true;

	/** 自适应距离基准（角色距离） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit|Adaptive", meta = (EditCondition = "bEnableAdaptiveDistance", ClampMin = "100.0"))
	float AdaptiveDistanceBase = 500.0f;

	/** 椭圆缩放因子 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit|Adaptive", meta = (EditCondition = "bEnableAdaptiveDistance", ClampMin = "0.1"))
	float EllipseScaleFactor = 1.0f;

	/** 最小椭圆缩放 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit|Adaptive", meta = (EditCondition = "bEnableAdaptiveDistance", ClampMin = "0.1"))
	float MinEllipseScale = 0.5f;

	/** 最大椭圆缩放 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit|Adaptive", meta = (EditCondition = "bEnableAdaptiveDistance", ClampMin = "0.1"))
	float MaxEllipseScale = 2.0f;

	// ========== 位置平滑 ==========

	/** 位置平滑速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ellipse Orbit|Smoothing", meta = (ClampMin = "0.0"))
	float PositionSmoothSpeed = 8.0f;

protected:
	/** 计算椭圆轨道位置 */
	UFUNCTION(BlueprintNativeEvent, Category = "Ellipse Orbit")
	FVector CalculateEllipsePosition(
		const FVector& PivotLocation,
		const FVector& PlayerLocation,
		const FVector& TargetLocation,
		float DeltaTime);

	/** 获取玩家位置 */
	FVector GetPlayerLocation() const;

	/** 获取锁定目标位置 */
	FVector GetLockedTargetLocation() const;

	/** 是否有有效锁定目标 */
	bool HasValidLockedTarget() const;

	/** 主要目标 */
	UPROPERTY()
	TWeakObjectPtr<AActor> PrimaryTarget;

	/** 锁定目标提供者 */
	UPROPERTY()
	TScriptInterface<INamiLockOnTargetProvider> LockOnProvider;

	/** 当前轨道角度 */
	UPROPERTY(BlueprintReadOnly, Category = "Ellipse Orbit")
	float CurrentOrbitAngle = 0.0f;

	/** 目标轨道角度 */
	UPROPERTY(BlueprintReadOnly, Category = "Ellipse Orbit")
	float TargetOrbitAngle = 0.0f;
};
