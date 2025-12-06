// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modifiers/NamiCameraEffectModifierBase.h"
#include "NamiCameraPostProcessModifier.generated.h"

/**
 * Nami 相机后处理修改器
 * 
 * 专门用于应用后处理效果
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Nami Camera PostProcess Modifier"))
class NAMICAMERA_API UNamiCameraPostProcessModifier : public UNamiCameraEffectModifierBase
{
	GENERATED_BODY()

public:
	UNamiCameraPostProcessModifier();

	// ========== UNamiCameraEffectModifierBase 接口 ==========
	
	virtual bool ApplyEffect(struct FMinimalViewInfo& InOutPOV, float Weight, float DeltaTime) override;

	// ========== 配置 ==========
	
	/** 后处理设置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|PostProcess",
			  meta = (ShowOnlyInnerProperties))
	FPostProcessSettings PostProcessSettings;
	
	/** 后处理权重 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|PostProcess",
			  meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PostProcessWeight = 1.0f;
};

