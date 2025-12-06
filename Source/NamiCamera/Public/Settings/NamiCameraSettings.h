// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NamiCameraSettings.generated.h"

/**
 * Nami相机系统设置
 * 可以在 Project Settings > Plugins > Nami Camera Settings 中配置
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Nami Camera Settings"))
class NAMICAMERA_API UNamiCameraSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UNamiCameraSettings(const FObjectInitializer& ObjectInitializer);

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

	// ========== 构图/可视化 Debug ==========

	/** 是否启用全局构图可视化（需要局部开关配合） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Framing",
		meta = (DisplayName = "启用构图可视化",
			ToolTip = "全局开关，开启后允许绘制构图框/安全区等可视化；\n需局部开关（如 Modifier 上的 bEnableFramingDebug）同时为真才会绘制"))
	bool bEnableFramingDebug{false};

	/** 构图可视化的持续时间（秒，0=仅一帧） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Framing",
		meta = (DisplayName = "构图可视化时长",
			EditCondition = "bEnableFramingDebug",
			ToolTip = "绘制持续时间（秒）；0 表示只绘制一帧"))
	float FramingDebugDuration{0.0f};

	/** 构图可视化线宽 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Framing",
		meta = (DisplayName = "构图线宽",
			EditCondition = "bEnableFramingDebug",
			ClampMin = "0.5", ClampMax = "8.0"))
	float FramingDebugThickness{2.0f};

	/** 是否启用构图日志（一次提示后不刷屏） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Framing",
		meta = (DisplayName = "构图日志",
			ToolTip = "打印构图中心/距离等日志（谨慎开启，避免刷屏）"))
	bool bEnableFramingLog{false};

	// ========== 日志分类开关 ==========

	/** 是否启用效果/修改器日志（激活/停用/混合等） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Logs",
		meta = (DisplayName = "效果日志",
			ToolTip = "打印效果激活/停用、混合权重等日志"))
	bool bEnableEffectLog{false};

	/** 是否启用State计算日志（详细的计算过程） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Logs",
		meta = (DisplayName = "State计算日志",
			ToolTip = "打印State计算过程、混合过程等详细日志（VeryVerbose级别）"))
	bool bEnableStateCalculationLog{false};

	/** 是否启用ANS日志（动画通知相关） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Logs",
		meta = (DisplayName = "ANS日志",
			ToolTip = "打印动画通知状态（AnimNotifyState）相关日志"))
	bool bEnableANSLog{false};

	/** 是否启用组件日志（Component相关） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Logs",
		meta = (DisplayName = "组件日志",
			ToolTip = "打印相机组件相关日志"))
	bool bEnableComponentLog{false};

	/** 是否启用模式日志（Mode相关） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Logs",
		meta = (DisplayName = "模式日志",
			ToolTip = "打印相机模式相关日志"))
	bool bEnableModeLog{false};

	/** 是否启用库函数日志（Library相关） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Logs",
		meta = (DisplayName = "库函数日志",
			ToolTip = "打印库函数调用相关日志"))
	bool bEnableLibraryLog{false};

	/** 是否启用混合探针日志（调试混合过程） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Logs",
		meta = (DisplayName = "混合探针日志",
			ToolTip = "打印混合权重探针日志（EffectBlendProbe/StateBlendProbe），用于调试混合过程"))
	bool bEnableBlendProbeLog{false};

	/** 是否启用警告日志（Warning级别） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Logs",
		meta = (DisplayName = "警告日志",
			ToolTip = "打印警告级别日志（默认开启，建议保持开启）"))
	bool bEnableWarningLog{true};

	// ========== 屏幕日志输出 ==========

	/** 是否启用屏幕日志输出 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|OnScreen",
		meta = (DisplayName = "启用屏幕日志",
			ToolTip = "是否在屏幕上显示日志信息\n• 勾选后会在屏幕上显示相机系统日志\n• 用于实时调试相机状态\n• 发布时建议关闭"))
	bool bEnableOnScreenLog{false};

	/** 屏幕日志显示时长（秒，0表示持续显示） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|OnScreen",
		meta = (DisplayName = "屏幕日志显示时长",
			EditCondition = "bEnableOnScreenLog",
			ToolTip = "屏幕日志在屏幕上的显示时长（秒）\n• 0表示持续显示（每帧刷新）\n• 大于0表示显示指定秒数后自动消失"))
	float OnScreenLogDuration{0.0f};

	/** 屏幕日志文本颜色 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|OnScreen",
		meta = (DisplayName = "屏幕日志文本颜色",
			EditCondition = "bEnableOnScreenLog",
			ToolTip = "屏幕日志在屏幕上显示的颜色"))
	FLinearColor OnScreenLogTextColor{FLinearColor::Green};

	/** 获取设置实例 */
	static const UNamiCameraSettings* Get();

	/** 检查是否应该启用堆栈Debug日志 */
	static bool ShouldEnableStackDebugLog();

	/** 检查是否全局启用构图可视化 */
	static bool ShouldEnableFramingDebug();

	// ========== 日志开关检查方法 ==========

	/** 检查是否应该打印效果日志 */
	static bool ShouldLogEffect();

	/** 检查是否应该打印State计算日志 */
	static bool ShouldLogStateCalculation();

	/** 检查是否应该打印ANS日志 */
	static bool ShouldLogANS();

	/** 检查是否应该打印组件日志 */
	static bool ShouldLogComponent();

	/** 检查是否应该打印模式日志 */
	static bool ShouldLogMode();

	/** 检查是否应该打印库函数日志 */
	static bool ShouldLogLibrary();

	/** 检查是否应该打印混合探针日志 */
	static bool ShouldLogBlendProbe();

	/** 检查是否应该打印警告日志 */
	static bool ShouldLogWarning();

	// ========== 屏幕日志检查方法 ==========

	/** 检查是否应该显示屏幕日志 */
	static bool ShouldShowOnScreenLog();

	/** 获取屏幕日志显示时长 */
	static float GetOnScreenLogDuration();

	/** 获取屏幕日志文本颜色 */
	static FLinearColor GetOnScreenLogTextColor();
};


