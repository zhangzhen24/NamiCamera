// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Structs/View/NamiCameraView.h"
#include "NamiCameraDebugLibrary.generated.h"

/**
 * 相机Debug绘制库
 * 提供相机View数据的Debug绘制功能
 */
UCLASS()
class NAMICAMERA_API UNamiCameraDebugLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 绘制相机View数据
	 * @param View 相机View数据
	 * @param World 世界对象
	 * @param Duration 绘制持续时间（秒），0表示只绘制一帧
	 * @param Thickness 线条粗细
	 * @param bDrawPivot 是否绘制观察点
	 * @param bDrawCamera 是否绘制相机位置
	 * @param bDrawDirection 是否绘制相机方向
	 * @param bDrawDistance 是否绘制距离信息
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Debug", CallInEditor)
	static void DrawCameraView(
		const FNamiCameraView& View,
		UWorld* World,
		float Duration = 0.0f,
		float Thickness = 2.0f,
		bool bDrawPivot = true,
		bool bDrawCamera = true,
		bool bDrawDirection = true,
		bool bDrawDistance = true);

	/**
	 * 绘制相机View数据（简化版本）
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Debug", CallInEditor)
	static void DrawCameraViewSimple(
		const FNamiCameraView& View,
		UWorld* World,
		float Duration = 0.0f);

	/**
	 * 获取相机View数据的字符串表示（用于日志输出）
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Debug")
	static FString GetCameraViewString(const FNamiCameraView& View);
};

