// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "NamiPlayerCameraManager.generated.h"

/**
 * Nami Player Camera Manager
 */
UCLASS()
class NAMICAMERA_API ANamiPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

	friend class UNamiCameraComponent;
};

