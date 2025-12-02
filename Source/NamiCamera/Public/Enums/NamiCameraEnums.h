// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NamiCameraEnums.generated.h"

/**
 * 相机模式状态
 */
UENUM(BlueprintType)
enum class ENamiCameraModeState : uint8
{
	/** 未初始化 */
	None UMETA(DisplayName = "未初始化"),
	/** 已初始化 */
	Initialized UMETA(DisplayName = "已初始化"),
	/** 激活中 */
	Active UMETA(DisplayName = "激活中"),
	/** 已停用 */
	Inactive UMETA(DisplayName = "已停用"),
};

/**
 * 相机混合类型
 */
UENUM(BlueprintType)
enum class ENamiCameraBlendType : uint8
{
	/** 线性混合 */
	Linear UMETA(DisplayName = "线性"),
	/** 缓入 */
	EaseIn UMETA(DisplayName = "缓入"),
	/** 缓出 */
	EaseOut UMETA(DisplayName = "缓出"),
	/** 缓入缓出 */
	EaseInOut UMETA(DisplayName = "缓入缓出"),
	/** 自定义曲线 */
	CustomCurve UMETA(DisplayName = "自定义曲线"),
};

/**
 * 跟随目标类型
 */
UENUM(BlueprintType)
enum class ENamiFollowTargetType : uint8
{
	/** 主要目标（通常是玩家） */
	Primary UMETA(DisplayName = "主要目标"),
	/** 辅助目标（用于构图计算） */
	Secondary UMETA(DisplayName = "辅助目标"),
};

