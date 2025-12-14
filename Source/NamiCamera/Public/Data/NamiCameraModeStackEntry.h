// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NamiCameraModeStackEntry.generated.h"

class UNamiCameraModeBase;

/**
 * 相机模式堆栈条目
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraModeStackEntry
{
	GENERATED_BODY()

	/** 句柄ID */
	int32 HandleId = 0;

	/** 优先级 */
	int32 Priority = 0;

	/** 相机模式 */
	TWeakObjectPtr<UNamiCameraModeBase> CameraMode = nullptr;
};

