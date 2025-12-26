// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Calculators/NamiCameraRotationCalculator.h"
#include "NamiTopDownRotationCalculator.generated.h"

/**
 * TopDown 俯视角旋转计算器
 *
 * 计算相机朝向，始终向下看向目标点（Pivot）
 * 适用于俯视角相机模式
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Top Down Rotation"))
class NAMICAMERA_API UNamiTopDownRotationCalculator : public UNamiCameraRotationCalculator
{
	GENERATED_BODY()

public:
	UNamiTopDownRotationCalculator();

	// ========== 生命周期重写 ==========

	virtual void Activate_Implementation() override;

	// ========== 核心接口重写 ==========

	/**
	 * 计算 TopDown 相机旋转
	 * 始终向下看向 Pivot 位置
	 * @param CameraLocation 相机位置
	 * @param PivotLocation 枢轴点位置（看向的目标）
	 * @param ControlRotation 控制旋转（TopDown 模式下不使用）
	 * @param DeltaTime 帧时间
	 * @return 计算后的相机旋转
	 */
	virtual FRotator CalculateCameraRotation_Implementation(
		const FVector& CameraLocation,
		const FVector& PivotLocation,
		const FRotator& ControlRotation,
		float DeltaTime) override;

protected:
	// 缓存的固定旋转（第一帧计算后保持不变）
	FRotator CachedRotation;

	// 是否已初始化旋转
	bool bRotationInitialized = false;
};
