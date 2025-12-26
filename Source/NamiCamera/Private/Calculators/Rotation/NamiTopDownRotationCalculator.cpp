// Copyright Qiu, Inc. All Rights Reserved.

#include "Calculators/Rotation/NamiTopDownRotationCalculator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiTopDownRotationCalculator)

UNamiTopDownRotationCalculator::UNamiTopDownRotationCalculator()
{
	// 默认锁定 Roll
	bLockRoll = true;
	bRotationInitialized = false;
}

void UNamiTopDownRotationCalculator::Activate_Implementation()
{
	Super::Activate_Implementation();

	// 重置旋转初始化标志，确保每次激活时重新计算旋转
	bRotationInitialized = false;
}

FRotator UNamiTopDownRotationCalculator::CalculateCameraRotation_Implementation(
	const FVector& CameraLocation,
	const FVector& PivotLocation,
	const FRotator& ControlRotation,
	float DeltaTime)
{
	// TopDown 相机使用固定旋转，避免角色移动时的轻微旋转抖动
	// 第一帧计算旋转并缓存，之后每帧直接返回缓存值
	if (!bRotationInitialized)
	{
		// 计算从相机到 Pivot 的方向向量（向下看）
		FVector LookAtDirection = (PivotLocation - CameraLocation).GetSafeNormal();
		CachedRotation = LookAtDirection.Rotation();

		// 锁定 Roll（避免相机倾斜）
		if (bLockRoll)
		{
			CachedRotation.Roll = 0.0f;
		}

		bRotationInitialized = true;
	}

	return CachedRotation;
}
