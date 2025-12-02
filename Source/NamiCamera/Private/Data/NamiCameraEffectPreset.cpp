// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/NamiCameraEffectPreset.h"
#include "Modifiers/NamiCameraEffectModifier.h"
#include "LogNamiCamera.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraEffectPreset)

void UNamiCameraEffectPreset::ApplyToModifier(UNamiCameraEffectModifier* Modifier) const
{
	if (!Modifier)
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraEffectPreset] ApplyToModifier: Modifier is null"));
		return;
	}
	
	// 应用基础配置
	Modifier->BlendInTime = BlendInTime;
	Modifier->BlendOutTime = BlendOutTime;
	
	// 应用位置偏移
	Modifier->bEnableLocationOffset = bEnableLocationOffset;
	Modifier->LocationOffset = LocationOffset;
	
	// 应用旋转偏移
	Modifier->bEnableRotationOffset = bEnableRotationOffset;
	Modifier->RotationOffset = RotationOffset;
	
	// 应用距离偏移
	Modifier->bEnableDistanceOffset = bEnableDistanceOffset;
	Modifier->DistanceOffset = DistanceOffset;
	
	// 应用 FOV 偏移
	Modifier->bEnableFOVOffset = bEnableFOVOffset;
	Modifier->FOVOffset = FOVOffset;
	
	// 应用相机效果
	Modifier->CameraShake = CameraShake;
	Modifier->ShakeScale = ShakeScale;
	
	// 应用后处理
	Modifier->bEnablePostProcess = bEnablePostProcess;
	Modifier->PostProcessSettings = PostProcessSettings;
	Modifier->PostProcessWeight = PostProcessWeight;
	
	UE_LOG(LogNamiCamera, Log, TEXT("[UNamiCameraEffectPreset] 应用预设到修改器：%s"), *GetName());
}

UNamiCameraEffectModifier* UNamiCameraEffectPreset::CreateModifier(UObject* Outer) const
{
	if (!Outer)
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraEffectPreset] CreateModifier: Outer is null"));
		return nullptr;
	}
	
	// 创建修改器实例
	UNamiCameraEffectModifier* Modifier = NewObject<UNamiCameraEffectModifier>(Outer);
	
	if (Modifier)
	{
		// 应用预设配置
		ApplyToModifier(Modifier);
	}
	
	return Modifier;
}

#if WITH_EDITOR
void UNamiCameraEffectPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	// 编辑器中的预览功能可以在这里实现
}
#endif

