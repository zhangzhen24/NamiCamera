// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Calculators/Target/NamiCameraTargetCalculator.h"
#include "NamiSingleTargetCalculator.generated.h"

/**
 * 单目标计算器
 *
 * 最基本的目标计算器，跟随单个目标 Actor
 * 适用于：第三人称相机、肩部视角、锁定相机等
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Single Target"))
class NAMICAMERA_API UNamiSingleTargetCalculator : public UNamiCameraTargetCalculator
{
	GENERATED_BODY()

public:
	UNamiSingleTargetCalculator();

	// ========== 核心接口 ==========

	virtual bool CalculateTargetLocation_Implementation(float DeltaTime, FVector& OutLocation) override;

public:
	// ========== 配置 ==========

	/**
	 * 是否使用目标眼睛位置
	 * 启用时使用 Pawn 的 GetPawnViewLocation，而非 Actor 的根位置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	bool bUseTargetEyesLocation = true;

	/**
	 * 目标位置偏移（相对于目标 Actor）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	FVector TargetOffset = FVector::ZeroVector;

	/**
	 * 是否使用目标旋转来旋转偏移
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
	bool bUseTargetRotation = false;

	/**
	 * 只使用目标的 Yaw 旋转
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (EditCondition = "bUseTargetRotation"))
	bool bUseYawOnly = true;

	/**
	 * 位置平滑速度（0 = 无平滑）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target|Smoothing", meta = (ClampMin = "0.0"))
	float LocationSmoothSpeed = 0.0f;

protected:
	/** 平滑后的目标位置 */
	FVector SmoothedLocation;

	/** 是否已初始化平滑位置 */
	bool bLocationInitialized = false;
};
