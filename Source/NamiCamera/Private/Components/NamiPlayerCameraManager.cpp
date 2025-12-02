// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/NamiPlayerCameraManager.h"
#include "Camera/CameraModifier.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiPlayerCameraManager)

bool ANamiPlayerCameraManager::AddCameraModifier(UCameraModifier* Modifier)
{
	if (!Modifier)
	{
		return false;
	}
	
	return AddCameraModifierToList(Modifier);
}
