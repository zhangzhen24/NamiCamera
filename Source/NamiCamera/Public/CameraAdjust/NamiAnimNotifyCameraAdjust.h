// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CameraAdjust/NamiCameraAdjust.h"
#include "NamiAnimNotifyCameraAdjust.generated.h"

struct FNamiCameraAdjustParams;

/**
 * AnimNotifyState 专用的相机调整器
 *
 * 接收预配置的参数并应用，由 UAnimNotifyState_CameraAdjust 创建和管理。
 * 不建议直接使用此类，请使用 AnimNotifyState_CameraAdjust。
 */
UCLASS(NotBlueprintable, NotBlueprintType, Hidden)
class NAMICAMERA_API UNamiAnimNotifyCameraAdjust : public UNamiCameraAdjust
{
	GENERATED_BODY()

public:
	UNamiAnimNotifyCameraAdjust();

	/**
	 * 设置预配置的调整参数
	 * @param InParams 要应用的参数
	 */
	void SetAdjustParams(const FNamiCameraAdjustParams& InParams);

	/**
	 * 获取当前配置的参数
	 */
	const FNamiCameraAdjustParams& GetConfiguredParams() const { return ConfiguredParams; }

protected:
	virtual FNamiCameraAdjustParams CalculateAdjustParams_Implementation(float DeltaTime) override;

private:
	/** 预配置的调整参数 */
	FNamiCameraAdjustParams ConfiguredParams;
};
