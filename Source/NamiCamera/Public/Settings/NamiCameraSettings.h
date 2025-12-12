// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NamiCameraSettings.generated.h"

/**
 * Nami相机系统设置
 * 可以在 Project Settings > Plugins > Nami Camera Settings 中配置
 */
UCLASS(Config = Game, DefaultConfig, meta = (Tooltip = "Nami相机系统设置，可以在 Project Settings > Plugins > Nami Camera Settings 中配置"))
class NAMICAMERA_API UNamiCameraSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UNamiCameraSettings(const FObjectInitializer& ObjectInitializer);

	/** 是否启用相机模式堆栈Debug日志。勾选后会在屏幕上显示每个模式的混合权重、状态等信息，用于调试相机模式切换和混合过程。发布时建议关闭。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug",
		meta = (ToolTip = "是否在每帧打印相机模式堆栈的混合信息\n• 勾选后会在屏幕上显示每个模式的混合权重、状态等信息\n• 用于调试相机模式切换和混合过程\n• 发布时建议关闭"))
	bool bEnableStackDebugLog{false};

	/** Debug日志显示时长（秒，0表示持续显示）。0表示持续显示直到关闭，大于0表示显示指定秒数后自动消失。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug",
		meta = (EditCondition = "bEnableStackDebugLog",
			ToolTip = "Debug日志在屏幕上的显示时长（秒）\n• 0表示持续显示直到关闭\n• 大于0表示显示指定秒数后自动消失"))
	float DebugLogDuration{0.0f};

	/** Debug日志文本颜色。Debug日志在屏幕上显示的颜色。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug",
		meta = (EditCondition = "bEnableStackDebugLog",
			ToolTip = "Debug日志在屏幕上显示的颜色"))
	FLinearColor DebugLogTextColor{FLinearColor::Green};

	// ========== 日志分类开关 ==========

	/** 是否启用效果/修改器日志输出到Log（激活/停用/混合等） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Logs",
		meta = (
			ToolTip = "将效果激活/停用、混合权重等日志输出到Log窗口"))
	bool bEnableEffectLog{false};

	/** 是否启用效果/修改器日志输出到屏幕 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|LogScreen",
		meta = (
			ToolTip = "将效果激活/停用、混合权重等日志输出到屏幕"))
	bool bEnableEffectLogOnScreen{false};

	/** 是否启用State计算日志输出到Log（详细的计算过程） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Logs",
		meta = (
			ToolTip = "将State计算过程、混合过程等详细日志输出到Log窗口（VeryVerbose级别）"))
	bool bEnableStateCalculationLog{false};

	/** 是否启用State计算日志输出到屏幕 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|LogScreen",
		meta = (
			ToolTip = "将State计算过程、混合过程等详细日志输出到屏幕"))
	bool bEnableStateCalculationLogOnScreen{false};

	/** 是否启用组件日志输出到Log（Component相关） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Logs",
		meta = (
			ToolTip = "将相机组件相关日志输出到Log窗口"))
	bool bEnableComponentLog{false};

	/** 是否启用组件日志输出到屏幕 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|LogScreen",
		meta = (
			ToolTip = "将相机组件相关日志输出到屏幕"))
	bool bEnableComponentLogOnScreen{false};

	/** 是否启用警告日志输出到Log（Warning级别） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Logs",
		meta = (
			ToolTip = "将警告级别日志输出到Log窗口\n• 按需开启，用于调试警告信息"))
	bool bEnableWarningLog{false};

	/** 是否启用警告日志输出到屏幕 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|LogScreen",
		meta = (
			ToolTip = "将警告级别日志输出到屏幕\n• 按需开启，用于调试警告信息"))
	bool bEnableWarningLogOnScreen{false};

	/** 是否启用相机信息日志输出到Log（关键相机参数） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Logs",
		meta = (
			ToolTip = "将关键相机信息日志输出到Log窗口\n• 包括相机位置、旋转、枢轴点、吊臂参数等\n• 用于调试相机状态和参数变化\n• 建议在调试相机行为时开启"))
	bool bEnableCameraInfoLog{false};

	/** 是否启用相机信息日志输出到屏幕 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|LogScreen",
		meta = (
			ToolTip = "将关键相机信息日志输出到屏幕\n• 包括相机位置、旋转、枢轴点、吊臂参数等\n• 用于调试相机状态和参数变化\n• 建议在调试相机行为时开启"))
	bool bEnableCameraInfoLogOnScreen{false};

	/** 是否启用模式混合日志输出到Log（模式切换和混合过程） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|Logs",
		meta = (
			ToolTip = "将模式混合日志输出到Log窗口\n• 包括 PushCameraMode、SetBlendWeight、UpdateBlending 等\n• 用于调试相机模式切换和混合过程\n• 建议在调试模式切换问题时开启"))
	bool bEnableModeBlendLog{false};

	/** 是否启用模式混合日志输出到屏幕 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|LogScreen",
		meta = (
			ToolTip = "将模式混合日志输出到屏幕\n• 包括 PushCameraMode、SetBlendWeight、UpdateBlending 等\n• 用于调试相机模式切换和混合过程\n• 建议在调试模式切换问题时开启"))
	bool bEnableModeBlendLogOnScreen{false};

	// ========== DrawDebug 可视化 ==========

	/** 是否启用 DrawDebug 绘制（全局开关） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|DrawDebug",
		meta = (
			ToolTip = "全局开关，开启后允许绘制相机相关的可视化信息\n• 包括 PivotLocation、CameraLocation、ArmLength 等\n• 用于调试相机位置和参数\n• 发布时建议关闭"))
	bool bEnableDrawDebug{false};

	/** 是否绘制 PivotLocation（枢轴点位置） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|DrawDebug",
		meta = (
			EditCondition = "bEnableDrawDebug",
			ToolTip = "绘制 PivotLocation（绿色球体）\n• 枢轴点是相机围绕旋转的中心点\n• 通常是角色位置\n• 按需开启"))
	bool bDrawPivotLocation{false};

	/** 是否绘制 CameraLocation（相机位置） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|DrawDebug",
		meta = (
			EditCondition = "bEnableDrawDebug",
			ToolTip = "绘制 CameraLocation（蓝色球体）\n• 默认关闭，避免遮挡画面"))
	bool bDrawCameraLocation{false};

	/** 是否绘制相机方向（Forward/Right/Up 箭头） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|DrawDebug",
		meta = (
			EditCondition = "bEnableDrawDebug",
			ToolTip = "绘制相机的前、右、上方向箭头\n• 红色：前方向（Forward）\n• 绿色：右方向（Right）\n• 蓝色：上方向（Up）\n• 按需开启"))
	bool bDrawCameraDirection{false};

	/** 是否绘制吊臂信息（ArmLength、ArmRotation） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|DrawDebug",
		meta = (
			EditCondition = "bEnableDrawDebug",
			ToolTip = "绘制吊臂相关信息\n• 从 PivotLocation 到 CameraLocation 的连线\n• 吊臂长度和旋转信息\n• 按需开启"))
	bool bDrawArmInfo{false};

	/** DrawDebug 绘制持续时间（秒，0=仅一帧） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|DrawDebug",
		meta = (
			EditCondition = "bEnableDrawDebug",
			ToolTip = "绘制持续时间（秒）\n• 0 表示只绘制一帧\n• >0 表示持续绘制指定秒数"))
	float DrawDebugDuration{0.0f};

	/** DrawDebug 线宽 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|DrawDebug",
		meta = (
			EditCondition = "bEnableDrawDebug",
			ClampMin = "0.5", ClampMax = "8.0",
			ToolTip = "DrawDebug 绘制的线宽"))
	float DrawDebugThickness{2.0f};

	// ========== 屏幕日志配置 ==========

	/** 屏幕日志显示时长（秒，0表示持续显示） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|OnScreen",
		meta = (
			ToolTip = "屏幕日志在屏幕上的显示时长（秒）\n• 0表示持续显示（每帧刷新）\n• 大于0表示显示指定秒数后自动消失"))
	float OnScreenLogDuration{0.0f};

	/** 屏幕日志文本颜色 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug|OnScreen",
		meta = (
			ToolTip = "屏幕日志在屏幕上显示的颜色"))
	FLinearColor OnScreenLogTextColor{FLinearColor::Green};

	/** 获取设置实例 */
	static const UNamiCameraSettings* Get();

	/** 检查是否应该启用堆栈Debug日志 */
	static bool ShouldEnableStackDebugLog();

	// ========== 日志开关检查方法 ==========

	/** 检查是否应该打印效果日志 */
	static bool ShouldLogEffect();

	/** 检查是否应该打印State计算日志 */
	static bool ShouldLogStateCalculation();

	/** 检查是否应该打印组件日志 */
	static bool ShouldLogComponent();

	/** 检查是否应该打印警告日志 */
	static bool ShouldLogWarning();

	/** 检查是否应该打印相机信息日志 */
	static bool ShouldLogCameraInfo();

	/** 检查是否应该打印模式混合日志 */
	static bool ShouldLogModeBlend();

	// ========== 屏幕日志检查方法 ==========

	/** 检查是否应该将效果日志输出到屏幕 */
	static bool ShouldLogEffectOnScreen();

	/** 检查是否应该将State计算日志输出到屏幕 */
	static bool ShouldLogStateCalculationOnScreen();

	/** 检查是否应该将组件日志输出到屏幕 */
	static bool ShouldLogComponentOnScreen();

	/** 检查是否应该将警告日志输出到屏幕 */
	static bool ShouldLogWarningOnScreen();

	/** 检查是否应该将相机信息日志输出到屏幕 */
	static bool ShouldLogCameraInfoOnScreen();

	/** 检查是否应该将模式混合日志输出到屏幕 */
	static bool ShouldLogModeBlendOnScreen();

	// ========== DrawDebug 检查方法 ==========

	/** 检查是否应该启用 DrawDebug 绘制 */
	static bool ShouldEnableDrawDebug();

	/** 检查是否应该绘制 PivotLocation */
	static bool ShouldDrawPivotLocation();

	/** 检查是否应该绘制 CameraLocation */
	static bool ShouldDrawCameraLocation();

	/** 检查是否应该绘制相机方向 */
	static bool ShouldDrawCameraDirection();

	/** 检查是否应该绘制吊臂信息 */
	static bool ShouldDrawArmInfo();


	/** 获取 DrawDebug 绘制持续时间 */
	static float GetDrawDebugDuration();

	/** 获取 DrawDebug 线宽 */
	static float GetDrawDebugThickness();


	/** 获取屏幕日志显示时长 */
	static float GetOnScreenLogDuration();

	/** 获取屏幕日志文本颜色 */
	static FLinearColor GetOnScreenLogTextColor();
};


