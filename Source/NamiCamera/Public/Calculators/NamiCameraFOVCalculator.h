// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Calculators/NamiCameraCalculatorBase.h"
#include "NamiCameraFOVCalculator.generated.h"

class UNamiCameraModeBase;

/**
 * FOV 计算器基类
 *
 * 负责计算相机 FOV
 *
 * 实现类型：
 * - StaticFOVCalculator: 固定 FOV
 * - SpeedBasedFOVCalculator: 基于目标速度
 * - DistanceBasedFOVCalculator: 基于与目标距离
 * - FramingFOVCalculator: 构图同框（保持多个目标在画面内）
 */
UCLASS(Abstract, Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew)
class NAMICAMERA_API UNamiCameraFOVCalculator : public UNamiCameraCalculatorBase
{
	GENERATED_BODY()

public:
	UNamiCameraFOVCalculator();

	// ========== 生命周期重写 ==========

	virtual void Activate_Implementation() override;

	// ========== 核心接口 ==========

	/**
	 * 计算 FOV
	 * @param CameraLocation 相机位置
	 * @param PivotLocation 枢轴点位置
	 * @param DeltaTime 帧时间
	 * @return 计算后的 FOV
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Calculator|FOV")
	float CalculateFOV(const FVector& CameraLocation, const FVector& PivotLocation, float DeltaTime);

public:
	// ========== 通用配置 ==========

	/** 基础 FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FOV", meta = (ClampMin = "30.0", ClampMax = "120.0"))
	float BaseFOV = 90.0f;

	/** 最小 FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FOV", meta = (ClampMin = "30.0", ClampMax = "120.0"))
	float MinFOV = 60.0f;

	/** 最大 FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FOV", meta = (ClampMin = "30.0", ClampMax = "120.0"))
	float MaxFOV = 100.0f;

	/** FOV 过渡速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FOV", meta = (ClampMin = "0.0"))
	float FOVTransitionSpeed = 5.0f;

protected:
	/** 当前 FOV（用于平滑） */
	UPROPERTY(BlueprintReadOnly, Category = "FOV")
	float CurrentFOV;

	/** 是否已处理首帧（用于 FOV 初始化） */
	UPROPERTY(BlueprintReadOnly, Category = "FOV")
	uint8 bFirstFrameProcessed : 1;
};
