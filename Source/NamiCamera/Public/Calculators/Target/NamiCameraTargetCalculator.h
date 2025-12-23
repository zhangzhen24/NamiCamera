// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Calculators/NamiCameraCalculatorBase.h"
#include "NamiCameraTargetCalculator.generated.h"

class UNamiCameraModeBase;

/**
 * 目标计算器基类
 *
 * 负责计算相机跟随的目标位置（PivotLocation）
 *
 * 实现类型：
 * - SingleTargetCalculator: 单目标跟随
 * - MultiTargetCalculator: 多目标加权平均
 * - DualFocusTargetCalculator: 双焦点（玩家+锁定目标）
 */
UCLASS(Abstract, Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew)
class NAMICAMERA_API UNamiCameraTargetCalculator : public UNamiCameraCalculatorBase
{
	GENERATED_BODY()

public:
	UNamiCameraTargetCalculator();

	// ========== 核心接口 ==========

	/**
	 * 计算主要目标位置（PivotLocation）
	 * @param DeltaTime 帧时间
	 * @param OutLocation 输出的目标位置
	 * @return 是否有有效目标
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Calculator|Target")
	bool CalculateTargetLocation(float DeltaTime, FVector& OutLocation);

	/**
	 * 获取主要目标 Actor
	 * @return 主要目标 Actor（可能为空）
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Camera Calculator|Target")
	AActor* GetPrimaryTargetActor() const;

	/**
	 * 获取主要目标的旋转
	 * @return 主要目标的旋转
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Camera Calculator|Target")
	FRotator GetPrimaryTargetRotation() const;

	/**
	 * 是否有有效目标
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Camera Calculator|Target")
	bool HasValidTarget() const;

	// ========== 目标设置 ==========

	/**
	 * 设置主要目标
	 * @param Target 目标 Actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Calculator|Target")
	virtual void SetPrimaryTarget(AActor* Target);

	/**
	 * 获取主要目标
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Calculator|Target")
	AActor* GetPrimaryTarget() const { return PrimaryTarget.Get(); }

protected:
	/** 主要目标 */
	UPROPERTY(BlueprintReadWrite, Category = "Target")
	TWeakObjectPtr<AActor> PrimaryTarget;

	/** 当前计算的目标位置（缓存） */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	FVector CurrentTargetLocation;
};
