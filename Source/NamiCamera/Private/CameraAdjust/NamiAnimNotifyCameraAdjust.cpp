// Copyright Epic Games, Inc. All Rights Reserved.

#include "CameraAdjust/NamiAnimNotifyCameraAdjust.h"

UNamiAnimNotifyCameraAdjust::UNamiAnimNotifyCameraAdjust()
{
	// 默认配置
	BlendInTime = 0.15f;
	BlendOutTime = 0.2f;
	BlendType = ENamiCameraBlendType::EaseInOut;
	BlendMode = ENamiCameraAdjustBlendMode::Additive;
	Priority = 100;
}

void UNamiAnimNotifyCameraAdjust::SetAdjustParams(const FNamiCameraAdjustParams& InParams)
{
	ConfiguredParams = InParams;
}

FNamiCameraAdjustParams UNamiAnimNotifyCameraAdjust::CalculateAdjustParams_Implementation(float DeltaTime)
{
	// 直接返回预配置的参数
	return ConfiguredParams;
}
