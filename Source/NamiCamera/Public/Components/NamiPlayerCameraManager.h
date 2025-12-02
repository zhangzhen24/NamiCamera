// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "NamiPlayerCameraManager.generated.h"

class UCameraModifier;

/**
 * Nami Player Camera Manager
 */
UCLASS()
class NAMICAMERA_API ANamiPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

	friend class UNamiCameraComponent;

public:
	/**
	 * 添加相机修改器（公共接口）
	 * 
	 * 注意：获取所有修改器请使用 UNamiCameraEffectLibrary::GetAllActiveCameraEffects
	 * 
	 * @param Modifier 要添加的修改器
	 * @return 是否成功添加
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera",
			  meta = (Tooltip = "添加相机修改器到相机管理器。获取所有修改器请使用 UNamiCameraEffectLibrary::GetAllActiveCameraEffects"))
	bool AddCameraModifier(UCameraModifier* Modifier);
};

