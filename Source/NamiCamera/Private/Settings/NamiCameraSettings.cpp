// Copyright Epic Games, Inc. All Rights Reserved.

#include "Settings/NamiCameraSettings.h"

const UNamiCameraSettings* UNamiCameraSettings::Get()
{
	return GetDefault<UNamiCameraSettings>();
}

UNamiCameraSettings::UNamiCameraSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("NamiCamera");
}

bool UNamiCameraSettings::ShouldEnableStackDebugLog()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableStackDebugLog;
}

bool UNamiCameraSettings::ShouldEnableFramingDebug()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableFramingDebug;
}

// ========== 日志开关检查方法 ==========

bool UNamiCameraSettings::ShouldLogEffect()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableEffectLog;
}

bool UNamiCameraSettings::ShouldLogStateCalculation()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableStateCalculationLog;
}

bool UNamiCameraSettings::ShouldLogANS()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableANSLog;
}

bool UNamiCameraSettings::ShouldLogComponent()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableComponentLog;
}

bool UNamiCameraSettings::ShouldLogMode()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableModeLog;
}

bool UNamiCameraSettings::ShouldLogLibrary()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableLibraryLog;
}

bool UNamiCameraSettings::ShouldLogBlendProbe()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableBlendProbeLog;
}

bool UNamiCameraSettings::ShouldLogWarning()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableWarningLog;
}

// ========== 屏幕日志检查方法 ==========

bool UNamiCameraSettings::ShouldShowOnScreenLog()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableOnScreenLog;
}

float UNamiCameraSettings::GetOnScreenLogDuration()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings ? Settings->OnScreenLogDuration : 0.0f;
}

FLinearColor UNamiCameraSettings::GetOnScreenLogTextColor()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings ? Settings->OnScreenLogTextColor : FLinearColor::Green;
}

