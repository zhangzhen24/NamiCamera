// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModeComponents/NamiCameraModeComponent.h"
#include "Interfaces/NamiLockOnTargetProvider.h"
#include "NamiCameraLockOnComponent.generated.h"

/**
 * 锁定目标组件
 *
 * 管理锁定目标的获取和位置平滑：
 * - 通过 INamiLockOnTargetProvider 接口获取锁定目标
 * - 平滑目标位置过渡
 * - 缓存距离信息
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Lock On Component"))
class NAMICAMERA_API UNamiCameraLockOnComponent : public UNamiCameraModeComponent
{
	GENERATED_BODY()

public:
	UNamiCameraLockOnComponent();

	// ========== UNamiCameraModeComponent ==========
	virtual void Activate_Implementation() override;
	virtual void Update_Implementation(float DeltaTime) override;

	// ========== 锁定目标管理 ==========

	/**
	 * 设置锁定目标提供者
	 */
	UFUNCTION(BlueprintCallable, Category = "Lock On")
	void SetLockOnProvider(TScriptInterface<INamiLockOnTargetProvider> Provider);

	/**
	 * 获取锁定目标提供者
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Lock On")
	TScriptInterface<INamiLockOnTargetProvider> GetLockOnProvider() const;

	/**
	 * 是否有有效的锁定目标
	 */
	UFUNCTION(BlueprintPure, Category = "Lock On")
	bool HasValidLockedTarget() const;

	/**
	 * 获取锁定目标位置（原始位置）
	 */
	UFUNCTION(BlueprintPure, Category = "Lock On")
	FVector GetLockedTargetLocation() const;

	/**
	 * 获取锁定目标焦点位置（相机看向的位置）
	 */
	UFUNCTION(BlueprintPure, Category = "Lock On")
	FVector GetLockedFocusLocation() const;

	/**
	 * 获取有效的目标位置（平滑后）
	 */
	UFUNCTION(BlueprintPure, Category = "Lock On")
	FVector GetEffectiveTargetLocation() const;

	/**
	 * 获取到目标的距离（缓存值）
	 */
	UFUNCTION(BlueprintPure, Category = "Lock On")
	float GetDistanceToTarget() const { return CachedDistanceToTarget; }

	/**
	 * 获取锁定目标 Actor
	 */
	UFUNCTION(BlueprintPure, Category = "Lock On")
	AActor* GetLockedTargetActor() const;

public:
	// ========== 配置 ==========

	/**
	 * 目标位置平滑速度
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Smoothing", meta = (ClampMin = "0.0"))
	float TargetLocationSmoothSpeed = 12.0f;

	/**
	 * 是否使用平滑的目标位置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On|Smoothing")
	bool bUseSmoothTargetLocation = true;

protected:
	/** 锁定目标提供者 */
	UPROPERTY()
	TScriptInterface<INamiLockOnTargetProvider> LockOnProvider;

	/** 平滑后的目标位置 */
	UPROPERTY(BlueprintReadOnly, Category = "Lock On")
	FVector SmoothedTargetLocation;

	/** 缓存的到目标的距离 */
	UPROPERTY(BlueprintReadOnly, Category = "Lock On")
	float CachedDistanceToTarget = 0.0f;

	/** 目标位置是否已初始化 */
	bool bTargetLocationInitialized = false;

	/** 更新平滑的目标位置 */
	void UpdateSmoothedTargetLocation(float DeltaTime);
};
