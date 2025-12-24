// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Settings/NamiCameraSettings.h"
#include "Engine/Engine.h"
#include "Core/LogNamiCamera.h"

/**
 * Nami相机系统日志宏
 * 根据 Settings 中的开关控制是否打印日志
 * 支持同时输出到 Log 和屏幕
 */

// ========== 内部辅助宏 ==========

/**
 * 统一的日志打印宏（Log输出）
 * 输出到 Log 窗口
 */
#define NAMI_LOG_TO_LOG(Verbosity, Format, ...) \
	do { \
		UE_LOG(LogNamiCamera, Verbosity, Format, ##__VA_ARGS__); \
	} while(0)

/**
 * 统一的日志打印宏（屏幕输出）
 * 输出到屏幕
 */
#define NAMI_LOG_TO_SCREEN(Format, ...) \
	do { \
		const float OnScreenDuration = UNamiCameraSettings::GetOnScreenLogDuration(); \
		const FLinearColor OnScreenColor = UNamiCameraSettings::GetOnScreenLogTextColor(); \
		if (GEngine) \
		{ \
			/* 格式化消息用于屏幕显示 */ \
			FString OnScreenMessage = FString::Printf(Format, ##__VA_ARGS__); \
			/* 使用 -1 作为 Key 表示每次都是新消息（会覆盖同 Key 的消息） */ \
			/* Duration = 0 表示持续显示（每帧刷新） */ \
			GEngine->AddOnScreenDebugMessage(-1, OnScreenDuration, OnScreenColor.ToFColor(true), *OnScreenMessage); \
		} \
	} while(0)

// ========== 效果/修改器日志 ==========
#define NAMI_LOG_EFFECT(Verbosity, Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldLogEffect()) \
		{ \
			NAMI_LOG_TO_LOG(Verbosity, Format, ##__VA_ARGS__); \
		} \
		if (UNamiCameraSettings::ShouldLogEffectOnScreen()) \
		{ \
			NAMI_LOG_TO_SCREEN(Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== State计算日志 ==========
#define NAMI_LOG_STATE(Verbosity, Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldLogStateCalculation()) \
		{ \
			NAMI_LOG_TO_LOG(Verbosity, Format, ##__VA_ARGS__); \
		} \
		if (UNamiCameraSettings::ShouldLogStateCalculationOnScreen()) \
		{ \
			NAMI_LOG_TO_SCREEN(Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== 组件日志 ==========
#define NAMI_LOG_COMPONENT(Verbosity, Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldLogComponent()) \
		{ \
			NAMI_LOG_TO_LOG(Verbosity, Format, ##__VA_ARGS__); \
		} \
		if (UNamiCameraSettings::ShouldLogComponentOnScreen()) \
		{ \
			NAMI_LOG_TO_SCREEN(Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== 警告日志 ==========
#define NAMI_LOG_WARNING(Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldLogWarning()) \
		{ \
			NAMI_LOG_TO_LOG(Warning, Format, ##__VA_ARGS__); \
		} \
		if (UNamiCameraSettings::ShouldLogWarningOnScreen()) \
		{ \
			NAMI_LOG_TO_SCREEN(Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== 堆栈日志（使用现有开关，暂不支持屏幕输出）==========
#define NAMI_LOG_STACK(Verbosity, Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldEnableStackDebugLog()) \
		{ \
			NAMI_LOG_TO_LOG(Verbosity, Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== 相机信息日志（关键相机参数）==========
#define NAMI_LOG_CAMERA_INFO(Verbosity, Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldLogCameraInfo()) \
		{ \
			NAMI_LOG_TO_LOG(Verbosity, Format, ##__VA_ARGS__); \
		} \
		if (UNamiCameraSettings::ShouldLogCameraInfoOnScreen()) \
		{ \
			NAMI_LOG_TO_SCREEN(Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== 模式混合日志（模式切换和混合过程）==========
#define NAMI_LOG_MODE_BLEND(Verbosity, Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldLogModeBlend()) \
		{ \
			NAMI_LOG_TO_LOG(Verbosity, Format, ##__VA_ARGS__); \
		} \
		if (UNamiCameraSettings::ShouldLogModeBlendOnScreen()) \
		{ \
			NAMI_LOG_TO_SCREEN(Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== 输入打断日志（CameraAdjust 输入打断和混出同步）==========
#define NAMI_LOG_INPUT_INTERRUPT(Verbosity, Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldLogInputInterrupt()) \
		{ \
			NAMI_LOG_TO_LOG(Verbosity, Format, ##__VA_ARGS__); \
		} \
		if (UNamiCameraSettings::ShouldLogInputInterruptOnScreen()) \
		{ \
			NAMI_LOG_TO_SCREEN(Format, ##__VA_ARGS__); \
		} \
	} while(0)

