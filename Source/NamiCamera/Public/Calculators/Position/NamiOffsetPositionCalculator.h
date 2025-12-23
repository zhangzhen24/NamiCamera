// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Calculators/Position/NamiCameraPositionCalculator.h"
#include "NamiOffsetPositionCalculator.generated.h"

/**
 * 偏移位置计算器
 *
 * 使用固定偏移计算相机位置，支持控制器旋转
 * 适用于：第三人称相机、肩部视角等
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Offset Position"))
class NAMICAMERA_API UNamiOffsetPositionCalculator : public UNamiCameraPositionCalculator
{
	GENERATED_BODY()

public:
	UNamiOffsetPositionCalculator();

	virtual void Activate_Implementation() override;

	// ========== 核心接口 ==========

	virtual FVector CalculateCameraPosition_Implementation(
		const FVector& PivotLocation,
		const FRotator& ControlRotation,
		float DeltaTime) override;

public:
	// ========== 配置 ==========

	/**
	 * 位置平滑速度（0 = 无平滑）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position|Smoothing", meta = (ClampMin = "0.0"))
	float PositionSmoothSpeed = 0.0f;
};
