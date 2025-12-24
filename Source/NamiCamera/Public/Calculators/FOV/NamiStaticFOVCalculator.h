// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Calculators/NamiCameraFOVCalculator.h"
#include "NamiStaticFOVCalculator.generated.h"

/**
 * 静态 FOV 计算器
 *
 * 返回固定的 FOV 值
 * 适用于：不需要动态 FOV 调整的相机
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Static FOV"))
class NAMICAMERA_API UNamiStaticFOVCalculator : public UNamiCameraFOVCalculator
{
	GENERATED_BODY()

public:
	UNamiStaticFOVCalculator();

	// ========== 核心接口 ==========

	virtual float CalculateFOV_Implementation(
		const FVector& CameraLocation,
		const FVector& PivotLocation,
		float DeltaTime) override;
};
