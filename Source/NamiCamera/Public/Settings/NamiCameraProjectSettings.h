// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NamiCameraProjectSettings.generated.h"

/**
 * Nami相机系统设置
 * 可以在 Project Settings > Plugins > Nami Camera Settings 中配置
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Nami Camera Settings"))
class NAMICAMERA_API UNamiCameraProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UNamiCameraProjectSettings(const FObjectInitializer& ObjectInitializer);

	/** 是否启用相机模式堆栈Debug日志 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug",
		meta = (DisplayName = "启用堆栈Debug日志",
			ToolTip = "是否在每帧打印相机模式堆栈的混合信息\n• 勾选后会在屏幕上显示每个模式的混合权重、状态等信息\n• 用于调试相机模式切换和混合过程\n• 发布时建议关闭"))
	bool bEnableStackDebugLog{false};

	/** Debug日志显示时长（秒，0表示持续显示） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug",
		meta = (DisplayName = "日志显示时长",
			EditCondition = "bEnableStackDebugLog",
			ToolTip = "Debug日志在屏幕上的显示时长（秒）\n• 0表示持续显示直到关闭\n• 大于0表示显示指定秒数后自动消失"))
	float DebugLogDuration{0.0f};

	/** Debug日志文本颜色 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug",
		meta = (DisplayName = "日志文本颜色",
			EditCondition = "bEnableStackDebugLog",
			ToolTip = "Debug日志在屏幕上显示的颜色"))
	FLinearColor DebugLogTextColor{FLinearColor::Green};

	/** 获取设置实例 */
	static const UNamiCameraProjectSettings* Get();

	/** 检查是否应该启用堆栈Debug日志 */
	static bool ShouldEnableStackDebugLog();
};

