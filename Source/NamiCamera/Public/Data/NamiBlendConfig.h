// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/NamiCameraEnums.h"
#include "NamiBlendConfig.generated.h"

/**
 * 混合配置
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiBlendConfig
{
	GENERATED_BODY()

	/** 混合时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending",
		meta = (ClampMin = "0.0"))
	float BlendTime = 0.5f;

	/** 混合类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending")
	ENamiCameraBlendType BlendType = ENamiCameraBlendType::Linear;

	/** 自定义混合曲线 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending",
		meta = (EditCondition = "BlendType == ENamiCameraBlendType::CustomCurve"))
	TObjectPtr<UCurveFloat> BlendCurve;
};

