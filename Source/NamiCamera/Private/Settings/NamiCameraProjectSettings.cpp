// Copyright Epic Games, Inc. All Rights Reserved.

#include "Settings/NamiCameraProjectSettings.h"

const UNamiCameraProjectSettings* UNamiCameraProjectSettings::Get()
{
	return GetDefault<UNamiCameraProjectSettings>();
}

UNamiCameraProjectSettings::UNamiCameraProjectSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("NamiCamera");
}

bool UNamiCameraProjectSettings::ShouldEnableStackDebugLog()
{
	const UNamiCameraProjectSettings* Settings = Get();
	return Settings && Settings->bEnableStackDebugLog;
}

