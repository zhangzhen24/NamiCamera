// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Calculators/NamiCameraRotationCalculator.h"
#include "NamiControlRotationCalculator.generated.h"

/**
 * 控制旋转计算器
 *
 * 使用控制器旋转作为相机旋转
 * 适用于：标准第三人称相机（玩家控制视角）
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Control Rotation"))
class NAMICAMERA_API UNamiControlRotationCalculator : public UNamiCameraRotationCalculator
{
	GENERATED_BODY()

public:
	UNamiControlRotationCalculator();

	// ========== 核心接口 ==========

	virtual FRotator CalculateCameraRotation_Implementation(
		const FVector& CameraLocation,
		const FVector& PivotLocation,
		const FRotator& ControlRotation,
		float DeltaTime) override;

public:
	// ========== 配置 ==========

	/**
	 * Pitch 限制
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation|Limits")
	bool bLimitPitch = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation|Limits", meta = (EditCondition = "bLimitPitch", ClampMin = "-89", ClampMax = "89"))
	float MinPitch = -89.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation|Limits", meta = (EditCondition = "bLimitPitch", ClampMin = "-89", ClampMax = "89"))
	float MaxPitch = 89.0f;
};
