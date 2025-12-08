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

bool UNamiCameraSettings::ShouldLogInputInterrupt()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableInputInterruptLog;
}

bool UNamiCameraSettings::ShouldLogCameraInfo()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableCameraInfoLog;
}

// ========== DrawDebug 检查方法 ==========

bool UNamiCameraSettings::ShouldEnableDrawDebug()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableDrawDebug;
}

bool UNamiCameraSettings::ShouldDrawPivotLocation()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableDrawDebug && Settings->bDrawPivotLocation;
}

bool UNamiCameraSettings::ShouldDrawCameraLocation()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableDrawDebug && Settings->bDrawCameraLocation;
}

bool UNamiCameraSettings::ShouldDrawCameraDirection()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableDrawDebug && Settings->bDrawCameraDirection;
}

bool UNamiCameraSettings::ShouldDrawArmInfo()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableDrawDebug && Settings->bDrawArmInfo;
}


float UNamiCameraSettings::GetDrawDebugDuration()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings ? Settings->DrawDebugDuration : 0.0f;
}

float UNamiCameraSettings::GetDrawDebugThickness()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings ? Settings->DrawDebugThickness : 2.0f;
}

// ========== 屏幕日志检查方法 ==========

bool UNamiCameraSettings::ShouldLogEffectOnScreen()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableEffectLogOnScreen;
}

bool UNamiCameraSettings::ShouldLogStateCalculationOnScreen()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableStateCalculationLogOnScreen;
}

bool UNamiCameraSettings::ShouldLogANSOnScreen()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableANSLogOnScreen;
}

bool UNamiCameraSettings::ShouldLogComponentOnScreen()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableComponentLogOnScreen;
}

bool UNamiCameraSettings::ShouldLogModeOnScreen()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableModeLogOnScreen;
}

bool UNamiCameraSettings::ShouldLogLibraryOnScreen()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableLibraryLogOnScreen;
}

bool UNamiCameraSettings::ShouldLogBlendProbeOnScreen()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableBlendProbeLogOnScreen;
}

bool UNamiCameraSettings::ShouldLogWarningOnScreen()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableWarningLogOnScreen;
}

bool UNamiCameraSettings::ShouldLogInputInterruptOnScreen()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableInputInterruptLogOnScreen;
}

bool UNamiCameraSettings::ShouldLogCameraInfoOnScreen()
{
	const UNamiCameraSettings* Settings = Get();
	return Settings && Settings->bEnableCameraInfoLogOnScreen;
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

