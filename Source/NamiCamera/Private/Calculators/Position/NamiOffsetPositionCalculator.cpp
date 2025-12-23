// Copyright Qiu, Inc. All Rights Reserved.

#include "Calculators/Position/NamiOffsetPositionCalculator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiOffsetPositionCalculator)

UNamiOffsetPositionCalculator::UNamiOffsetPositionCalculator()
{
	// 默认第三人称偏移
	CameraOffset = FVector(-300.0f, 0.0f, 100.0f);
	bUseControlRotation = true;
	bUseYawOnly = false;
}

void UNamiOffsetPositionCalculator::Activate_Implementation()
{
	Super::Activate_Implementation();
}

FVector UNamiOffsetPositionCalculator::CalculateCameraPosition_Implementation(
	const FVector& PivotLocation,
	const FRotator& ControlRotation,
	float DeltaTime)
{
	// 计算偏移
	FVector Offset = CameraOffset;

	if (bUseControlRotation)
	{
		FRotator RotationToUse = ControlRotation;
		if (bUseYawOnly)
		{
			RotationToUse.Pitch = 0.0f;
			RotationToUse.Roll = 0.0f;
		}
		Offset = RotationToUse.RotateVector(Offset);
	}

	FVector TargetPosition = PivotLocation + Offset;

	// 首帧初始化
	if (!bFirstFrameProcessed)
	{
		CurrentCameraPosition = TargetPosition;
		bFirstFrameProcessed = true;
		return TargetPosition;
	}

	// 应用平滑
	if (PositionSmoothSpeed > 0.0f && DeltaTime > 0.0f)
	{
		CurrentCameraPosition = FMath::VInterpTo(CurrentCameraPosition, TargetPosition, DeltaTime, PositionSmoothSpeed);
	}
	else
	{
		CurrentCameraPosition = TargetPosition;
	}

	return CurrentCameraPosition;
}
