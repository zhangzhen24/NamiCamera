// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NamiCameraModeHandle.generated.h"

class UNamiCameraComponent;

/**
 * 相机模式句柄
 * 用于标识和管理相机模式实例
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraModeHandle
{
	GENERATED_BODY()

public:
	/** 是否有效 */
	bool IsValid() const;

	/** 重置句柄 */
	void Reset();

private:
	friend class UNamiCameraComponent;
	friend class UNamiCameraLibrary;

	/** 所有者组件 */
	UPROPERTY()
	TWeakObjectPtr<UNamiCameraComponent> Owner;

	/** 句柄ID */
	UPROPERTY()
	int32 HandleId = 0;
};

