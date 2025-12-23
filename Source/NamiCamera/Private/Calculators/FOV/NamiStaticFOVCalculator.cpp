// Copyright Qiu, Inc. All Rights Reserved.

#include "Calculators/FOV/NamiStaticFOVCalculator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiStaticFOVCalculator)

UNamiStaticFOVCalculator::UNamiStaticFOVCalculator()
{
	BaseFOV = 90.0f;
}

float UNamiStaticFOVCalculator::CalculateFOV_Implementation(
	const FVector& CameraLocation,
	const FVector& PivotLocation,
	float DeltaTime)
{
	// 直接返回基础 FOV
	CurrentFOV = BaseFOV;
	return BaseFOV;
}
