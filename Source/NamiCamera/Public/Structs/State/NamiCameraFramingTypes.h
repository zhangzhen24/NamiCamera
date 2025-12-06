// Copyright Epic Games, Inc. All Rights Reserved.
//
// 多目标构图的配置与计算结果类型

#pragma once

#include "CoreMinimal.h"
#include "NamiCameraFramingTypes.generated.h"

/**
 * 多目标构图配置
 * - 目标：让多个目标保持在屏幕安全区内，自动拉远/侧移
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraFramingConfig
{
	GENERATED_BODY()

	/** 是否启用构图计算 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing")
	bool bEnableFraming = false;

	/** 期望的目标屏幕占比（0~1），用于控制拉远/拉近 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing", meta = (ClampMin = "0.05", ClampMax = "0.8"))
	float DesiredScreenPercentage = 0.35f;

	/** 屏幕安全区（X=左右、Y=上下），超出则尝试侧移/拉远 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	FVector2D ScreenSafeZone = FVector2D(0.2f, 0.2f);

	/** 允许的最大拉远距离（在当前 ArmLength 基础上叠加） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing", meta = (ClampMin = "0.0"))
	float MaxArmExtension = 600.0f;

	/** ArmLength 的最小/最大硬限制 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing", meta = (ClampMin = "0.0"))
	float MinArmLength = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing", meta = (ClampMin = "0.0"))
	float MaxArmLength = 1200.0f;

	/** 侧移偏好：-1 偏左，0 居中，+1 偏右 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float SideBias = 0.0f;

	/** 垂直抬升偏移，用于俯视或保留脚下空间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing")
	float VerticalOffset = 0.0f;

	/** 平滑速度（位置/距离插值） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing", meta = (ClampMin = "0.0"))
	float LerpSpeed = 8.0f;
};

/**
 * 多目标构图计算结果
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraFramingResult
{
	GENERATED_BODY()

	/** 是否有有效结果 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing")
	bool bHasResult = false;

	/** 计算后的目标中心（可作为新的 PivotLocation） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing")
	FVector TargetCenter = FVector::ZeroVector;

	/** 推荐的吊臂长度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing")
	float RecommendedArmLength = 0.0f;

	/** 推荐的吊臂旋转（通常用于侧移/保持视角） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing")
	FRotator RecommendedArmRotation = FRotator::ZeroRotator;

	/** 额外的相机偏移（可选，用于居中/安全区微调） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Framing")
	FVector RecommendedCameraOffset = FVector::ZeroVector;
};

