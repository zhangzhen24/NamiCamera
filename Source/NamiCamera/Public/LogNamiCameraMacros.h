// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LogNamiCamera.h"
#include "Settings/NamiCameraSettings.h"
#include "Engine/Engine.h"

/**
 * Nami相机系统日志宏
 * 根据 Settings 中的开关控制是否打印日志
 * 支持同时输出到 Log 和屏幕
 */

// ========== 内部辅助宏 ==========

/**
 * 统一的日志打印宏
 * 同时输出到 Log 和屏幕（如果启用）
 */
#define NAMI_LOG_INTERNAL(Verbosity, Format, ...) \
	do { \
		/* 输出到 Log */ \
		UE_LOG(LogNamiCamera, Verbosity, Format, ##__VA_ARGS__); \
		/* 输出到屏幕（如果启用） */ \
		if (UNamiCameraSettings::ShouldShowOnScreenLog()) \
		{ \
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
		} \
	} while(0)

// ========== 效果/修改器日志 ==========
#define NAMI_LOG_EFFECT(Verbosity, Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldLogEffect()) \
		{ \
			NAMI_LOG_INTERNAL(Verbosity, Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== State计算日志 ==========
#define NAMI_LOG_STATE(Verbosity, Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldLogStateCalculation()) \
		{ \
			NAMI_LOG_INTERNAL(Verbosity, Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== ANS日志 ==========
#define NAMI_LOG_ANS(Verbosity, Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldLogANS()) \
		{ \
			NAMI_LOG_INTERNAL(Verbosity, Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== 组件日志 ==========
#define NAMI_LOG_COMPONENT(Verbosity, Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldLogComponent()) \
		{ \
			NAMI_LOG_INTERNAL(Verbosity, Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== 模式日志 ==========
#define NAMI_LOG_MODE(Verbosity, Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldLogMode()) \
		{ \
			NAMI_LOG_INTERNAL(Verbosity, Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== 库函数日志 ==========
#define NAMI_LOG_LIBRARY(Verbosity, Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldLogLibrary()) \
		{ \
			NAMI_LOG_INTERNAL(Verbosity, Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== 混合探针日志 ==========
#define NAMI_LOG_BLEND_PROBE(Verbosity, Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldLogBlendProbe()) \
		{ \
			NAMI_LOG_INTERNAL(Verbosity, Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== 警告日志（默认开启）==========
#define NAMI_LOG_WARNING(Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldLogWarning()) \
		{ \
			NAMI_LOG_INTERNAL(Warning, Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== 构图日志（使用现有开关）==========
#define NAMI_LOG_FRAMING(Verbosity, Format, ...) \
	do { \
		const UNamiCameraSettings* FramingLogSettings = UNamiCameraSettings::Get(); \
		if (FramingLogSettings && FramingLogSettings->bEnableFramingLog) \
		{ \
			NAMI_LOG_INTERNAL(Verbosity, Format, ##__VA_ARGS__); \
		} \
	} while(0)

// ========== 堆栈日志（使用现有开关）==========
#define NAMI_LOG_STACK(Verbosity, Format, ...) \
	do { \
		if (UNamiCameraSettings::ShouldEnableStackDebugLog()) \
		{ \
			NAMI_LOG_INTERNAL(Verbosity, Format, ##__VA_ARGS__); \
		} \
	} while(0)

