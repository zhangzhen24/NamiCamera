// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Calculators/NamiCameraCalculatorBase.h"
#include "NamiCameraRotationCalculator.generated.h"

class UNamiCameraModeBase;

/**
 * 旋转计算器基类
 *
 * 负责计算相机旋转
 *
 * 实现类型：
 * - ControlRotationCalculator: 使用控制器旋转
 * - LookAtRotationCalculator: 看向 PivotLocation
 * - SoftLockRotationCalculator: 软锁定（偏向目标）
 * - HardLockRotationCalculator: 硬锁定（始终看向目标）
 */
UCLASS(Abstract, Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew)
class NAMICAMERA_API UNamiCameraRotationCalculator : public UNamiCameraCalculatorBase
{
	GENERATED_BODY()

public:
	UNamiCameraRotationCalculator();

	// ========== 生命周期重写 ==========

	virtual void Activate_Implementation() override;

	// ========== 核心接口 ==========

	/**
	 * 计算相机旋转
	 * @param CameraLocation 相机位置
	 * @param PivotLocation 枢轴点位置
	 * @param ControlRotation 控制旋转
	 * @param DeltaTime 帧时间
	 * @return 计算后的相机旋转
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Calculator|Rotation")
	FRotator CalculateCameraRotation(
		const FVector& CameraLocation,
		const FVector& PivotLocation,
		const FRotator& ControlRotation,
		float DeltaTime);

	/**
	 * 获取控制旋转
	 * 子类可以重写此方法提供不同的控制旋转源
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Camera Calculator|Rotation")
	FRotator GetControlRotation() const;

public:
	// ========== 通用配置 ==========

	/** 旋转平滑速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (ClampMin = "0.0"))
	float RotationSmoothSpeed = 10.0f;

	/** 是否锁定 Roll */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	bool bLockRoll = true;

protected:
	/** 当前相机旋转（用于平滑） */
	UPROPERTY(BlueprintReadOnly, Category = "Rotation")
	FRotator CurrentCameraRotation;

	/** 是否已处理首帧（用于平滑初始化） */
	UPROPERTY(BlueprintReadOnly, Category = "Rotation")
	uint8 bFirstFrameProcessed : 1;
};
