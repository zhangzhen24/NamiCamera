// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Calculators/Target/NamiCameraTargetCalculator.h"
#include "Interfaces/NamiLockOnTargetProvider.h"
#include "NamiDualFocusTargetCalculator.generated.h"

/**
 * 双焦点目标计算器
 *
 * 计算玩家和锁定目标之间的加权焦点
 * 适用于：鬼泣风格战斗相机、双人同框相机
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Dual Focus Target"))
class NAMICAMERA_API UNamiDualFocusTargetCalculator : public UNamiCameraTargetCalculator
{
	GENERATED_BODY()

public:
	UNamiDualFocusTargetCalculator();

	virtual void Activate_Implementation() override;

	// ========== 核心接口 ==========

	virtual bool CalculateTargetLocation_Implementation(float DeltaTime, FVector& OutLocation) override;
	virtual bool HasValidTarget_Implementation() const override;

	// ========== 锁定目标管理 ==========

	/**
	 * 设置锁定目标提供者
	 */
	UFUNCTION(BlueprintCallable, Category = "Dual Focus")
	void SetLockOnProvider(TScriptInterface<INamiLockOnTargetProvider> Provider);

	/**
	 * 获取锁定目标提供者
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Dual Focus")
	TScriptInterface<INamiLockOnTargetProvider> GetLockOnProvider() const;

	/**
	 * 是否有有效的锁定目标
	 */
	UFUNCTION(BlueprintPure, Category = "Dual Focus")
	bool HasValidLockedTarget() const;

	/**
	 * 获取锁定目标位置
	 */
	UFUNCTION(BlueprintPure, Category = "Dual Focus")
	FVector GetLockedTargetLocation() const;

	/**
	 * 获取有效的锁定目标位置（平滑后）
	 */
	UFUNCTION(BlueprintPure, Category = "Dual Focus")
	FVector GetEffectiveLockedLocation() const;

public:
	// ========== 配置 ==========

	/**
	 * 玩家权重（焦点计算）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Focus", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PlayerFocusWeight = 0.6f;

	/**
	 * 目标权重（焦点计算）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Focus", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TargetFocusWeight = 0.4f;

	/**
	 * 焦点位置平滑速度
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Focus|Smoothing", meta = (ClampMin = "0.0"))
	float FocusPointSmoothSpeed = 8.0f;

	/**
	 * 锁定目标位置平滑速度
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Focus|Smoothing", meta = (ClampMin = "0.0"))
	float LockedTargetSmoothSpeed = 12.0f;

protected:
	/**
	 * 计算双焦点位置
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Dual Focus")
	FVector CalculateDualFocusPoint(const FVector& PlayerLocation, const FVector& TargetLocation);

	/** 锁定目标提供者 */
	UPROPERTY()
	TScriptInterface<INamiLockOnTargetProvider> LockOnProvider;

	/** 平滑后的锁定目标位置 */
	FVector SmoothedLockedLocation;

	/** 平滑后的焦点位置 */
	FVector SmoothedFocusPoint;

	/** 锁定目标位置是否已初始化 */
	bool bLockedLocationInitialized = false;

	/** 焦点位置是否已初始化 */
	bool bFocusPointInitialized = false;
};
