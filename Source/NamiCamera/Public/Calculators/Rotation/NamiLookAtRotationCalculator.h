// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Calculators/Rotation/NamiCameraRotationCalculator.h"
#include "NamiLookAtRotationCalculator.generated.h"

/**
 * 看向旋转计算器
 *
 * 相机始终看向 PivotLocation
 * 适用于：双焦点相机、固定视角相机
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Look At"))
class NAMICAMERA_API UNamiLookAtRotationCalculator : public UNamiCameraRotationCalculator
{
	GENERATED_BODY()

public:
	UNamiLookAtRotationCalculator();

	// ========== 核心接口 ==========

	virtual FRotator CalculateCameraRotation_Implementation(
		const FVector& CameraLocation,
		const FVector& PivotLocation,
		const FRotator& ControlRotation,
		float DeltaTime) override;
};
