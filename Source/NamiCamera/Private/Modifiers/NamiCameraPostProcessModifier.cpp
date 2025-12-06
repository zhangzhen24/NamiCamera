// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modifiers/NamiCameraPostProcessModifier.h"
#include "LogNamiCamera.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraPostProcessModifier)

UNamiCameraPostProcessModifier::UNamiCameraPostProcessModifier()
{
}

bool UNamiCameraPostProcessModifier::ApplyEffect(FMinimalViewInfo& InOutPOV, float Weight, float DeltaTime)
{
	float FinalWeight = PostProcessWeight * Weight;
	
	// 混合后处理设置
	InOutPOV.PostProcessBlendWeight = FinalWeight;
	InOutPOV.PostProcessSettings = PostProcessSettings;
	
	return FinalWeight > 0.0f;
}

