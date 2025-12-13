// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NamiCameraModeStack.generated.h"

class UNamiCameraModeBase;
struct FNamiCameraView;

/**
 * 相机模式混合堆栈
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraModeStack
{
	GENERATED_BODY()

public:
	/**
	 * 推送相机模式到堆栈顶部
	 */
	void PushCameraMode(UNamiCameraModeBase* CameraModeInstance);

	/**
	 * 评估堆栈并混合视图
	 * @param DeltaTime 帧时间
	 * @param OutCameraModeView 输出的混合视图
	 * @return 是否有有效的相机模式
	 */
	bool EvaluateStack(float DeltaTime, FNamiCameraView& OutCameraModeView);

	/**
	 * 打印相机模式堆栈信息
	 * @param bPrintToScreen 是否打印到屏幕
	 * @param bPrintToLog 是否打印到日志
	 * @param TextColor 文本颜色
	 * @param Duration 显示时长
	 */
	void DumpCameraModeStack(bool bPrintToScreen = true, bool bPrintToLog = true, 
		FLinearColor TextColor = FLinearColor::Green, float Duration = 0.2f) const;

protected:
	/**
	 * 更新模式堆栈
	 * @param DeltaTime 帧时间
	 * @return 是否有有效的相机模式
	 */
	bool UpdateStack(float DeltaTime);

	/**
	 * 混合所有模式的视图
	 * @param OutCameraModeView 输出的混合视图
	 * @param DeltaTime 帧时间（用于PivotLocation计算）
	 */
	void BlendStack(FNamiCameraView& OutCameraModeView, float DeltaTime = 0.0f) const;

private:
	/** 模式堆栈 */
	UPROPERTY()
	TArray<TObjectPtr<UNamiCameraModeBase>> CameraModeStack;
};

