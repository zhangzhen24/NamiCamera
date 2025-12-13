// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NamiCameraView.generated.h"

/**
 * 相机视图数据
 * 包含相机的完整状态信息
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraView
{
	GENERATED_BODY()

	FNamiCameraView();

	/**
	 * 混合两个视图
	 * @param Other 另一个视图
	 * @param OtherWeight 另一个视图的权重
	 */
	void Blend(const FNamiCameraView& Other, float OtherWeight);

	/** 枢轴位置（相机观察的焦点） */
	UPROPERTY(BlueprintReadWrite, Category = "Camera View")
	FVector PivotLocation;

	/** 相机位置 */
	UPROPERTY(BlueprintReadWrite, Category = "Camera View")
	FVector CameraLocation;

	/** 相机旋转 */
	UPROPERTY(BlueprintReadWrite, Category = "Camera View")
	FRotator CameraRotation;

	/** 控制器位置 */
	UPROPERTY(BlueprintReadWrite, Category = "Camera View")
	FVector ControlLocation;

	/** 控制器旋转 */
	UPROPERTY(BlueprintReadWrite, Category = "Camera View")
	FRotator ControlRotation;

	/** 视野角度 */
	UPROPERTY(BlueprintReadWrite, Category = "Camera View")
	float FOV = 80.0f;
};

