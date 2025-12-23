// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Calculators/NamiCameraCalculatorBase.h"
#include "NamiCameraPositionCalculator.generated.h"

class UNamiCameraModeBase;

/**
 * 位置计算器基类
 *
 * 负责根据 PivotLocation 计算相机位置
 *
 * 实现类型：
 * - OffsetPositionCalculator: 简单偏移（第三人称）
 * - EllipseOrbitPositionCalculator: 椭圆轨道（双焦点）
 */
UCLASS(Abstract, Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew)
class NAMICAMERA_API UNamiCameraPositionCalculator : public UNamiCameraCalculatorBase
{
	GENERATED_BODY()

public:
	UNamiCameraPositionCalculator();

	// ========== 生命周期重写 ==========

	virtual void Activate_Implementation() override;

	// ========== 核心接口 ==========

	/**
	 * 计算相机位置
	 * @param PivotLocation 枢轴点位置
	 * @param ControlRotation 控制旋转（来自玩家输入或其他源）
	 * @param DeltaTime 帧时间
	 * @return 计算后的相机位置
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Calculator|Position")
	FVector CalculateCameraPosition(const FVector& PivotLocation, const FRotator& ControlRotation, float DeltaTime);

public:
	// ========== 通用配置 ==========

	/** 相机偏移（相对于 PivotLocation） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position")
	FVector CameraOffset = FVector(-300.0f, 0.0f, 100.0f);

	/** 是否使用控制旋转来旋转偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position")
	bool bUseControlRotation = true;

	/** 只使用 Yaw（忽略 Pitch 和 Roll） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position", meta = (EditCondition = "bUseControlRotation"))
	bool bUseYawOnly = false;

protected:
	/** 当前相机位置（用于平滑） */
	UPROPERTY(BlueprintReadOnly, Category = "Position")
	FVector CurrentCameraPosition;

	/** 是否已处理首帧（用于平滑初始化） */
	UPROPERTY(BlueprintReadOnly, Category = "Position")
	uint8 bFirstFrameProcessed : 1;
};
