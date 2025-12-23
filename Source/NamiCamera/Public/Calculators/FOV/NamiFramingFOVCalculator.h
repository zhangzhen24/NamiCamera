// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Calculators/FOV/NamiCameraFOVCalculator.h"
#include "Interfaces/NamiLockOnTargetProvider.h"
#include "NamiFramingFOVCalculator.generated.h"

/**
 * 构图 FOV 计算器
 *
 * 动态调整 FOV 以保持玩家和锁定目标都在画面内
 * 适用于：双焦点相机、多人同框相机
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Framing FOV"))
class NAMICAMERA_API UNamiFramingFOVCalculator : public UNamiCameraFOVCalculator
{
	GENERATED_BODY()

public:
	UNamiFramingFOVCalculator();

	// ========== 核心接口 ==========

	virtual float CalculateFOV_Implementation(
		const FVector& CameraLocation,
		const FVector& PivotLocation,
		float DeltaTime) override;

	// ========== 目标设置 ==========

	/**
	 * 设置主要目标（玩家）
	 */
	UFUNCTION(BlueprintCallable, Category = "Framing FOV")
	void SetPrimaryTarget(AActor* Target);

	/**
	 * 设置锁定目标提供者
	 */
	UFUNCTION(BlueprintCallable, Category = "Framing FOV")
	void SetLockOnProvider(TScriptInterface<INamiLockOnTargetProvider> Provider);

public:
	// ========== 配置 ==========

	/**
	 * 是否保持双方在画面内
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing FOV")
	bool bKeepBothInFrame = true;

	/**
	 * 画面边缘留白比例
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing FOV", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float FramePadding = 0.15f;

protected:
	/**
	 * 计算需要的 FOV 来包含两个目标
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Framing FOV")
	float CalculateFramingFOV(
		const FVector& CameraLocation,
		const FVector& PlayerLocation,
		const FVector& TargetLocation);

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
};
