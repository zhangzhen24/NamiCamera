// Copyright Qiu, Inc. All Rights Reserved.

#include "Calculators/Rotation/NamiControlRotationCalculator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiControlRotationCalculator)

UNamiControlRotationCalculator::UNamiControlRotationCalculator()
{
	// 控制旋转计算器不需要平滑（玩家输入应该即时响应）
	RotationSmoothSpeed = 0.0f;
}

FRotator UNamiControlRotationCalculator::CalculateCameraRotation_Implementation(
	const FVector& CameraLocation,
	const FVector& PivotLocation,
	const FRotator& ControlRotation,
	float DeltaTime)
{
	FRotator ResultRotation = ControlRotation;

	// 应用 Pitch 限制
	if (bLimitPitch)
	{
		ResultRotation.Pitch = FMath::Clamp(ResultRotation.Pitch, MinPitch, MaxPitch);
	}

	// 锁定 Roll
	if (bLockRoll)
	{
		ResultRotation.Roll = 0.0f;
	}

	CurrentCameraRotation = ResultRotation;
	return ResultRotation;
}
