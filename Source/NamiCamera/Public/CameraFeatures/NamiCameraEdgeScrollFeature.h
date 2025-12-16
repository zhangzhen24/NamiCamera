// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CameraFeatures/NamiCameraFeature.h"
#include "NamiCameraEdgeScrollFeature.generated.h"

/**
 * 边缘滚动Feature
 *
 * 当鼠标移动到屏幕边缘时自动平移相机
 * 适用于MOBA、RTS、ARPG等游戏
 */
UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="边缘滚动"))
class NAMICAMERA_API UNamiCameraEdgeScrollFeature : public UNamiCameraFeature
{
	GENERATED_BODY()

public:
	UNamiCameraEdgeScrollFeature();

	//~ Begin UNamiCameraFeature Interface
	virtual void Update_Implementation(float DeltaTime) override;
	//~ End UNamiCameraFeature Interface

	/** 边缘检测阈值（像素） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Edge Scroll",
		meta=(ClampMin="1.0", UIMin="1.0",
		Tooltip="鼠标距离屏幕边缘多少像素时触发滚动"))
	float EdgeThreshold;

	/** 滚动速度（单位/秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Edge Scroll",
		meta=(ClampMin="0.0", UIMin="0.0",
		Tooltip="边缘滚动的速度"))
	float ScrollSpeed;

	/** 是否启用水平滚动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Edge Scroll",
		meta=(Tooltip="是否允许在屏幕左右边缘触发滚动"))
	bool bEnableHorizontalScroll;

	/** 是否启用垂直滚动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Edge Scroll",
		meta=(Tooltip="是否允许在屏幕上下边缘触发滚动"))
	bool bEnableVerticalScroll;

	/** 检查鼠标是否在屏幕边缘 */
	UFUNCTION(BlueprintPure, Category="Edge Scroll")
	bool IsMouseAtScreenEdge(FVector2D& OutDirection) const;

protected:
	/** 获取边缘滚动方向 */
	FVector2D GetEdgeScrollDirection() const;
};
