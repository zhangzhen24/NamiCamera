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
	/** 敌人目标（用于战斗相机） */
	Enemy UMETA(DisplayName = "敌人目标"),
};

/**
 * 相机参数混合模式
 */
UENUM(BlueprintType)
enum class ENamiCameraBlendMode : uint8
{
	/** 叠加模式：在当前值基础上增加/减少 */
	Additive UMETA(DisplayName = "叠加"),
	
	/** 覆盖模式：直接过渡到目标值 */
	Override UMETA(DisplayName = "覆盖"),
};

/**
 * 相机修改阶段
 */
UENUM(BlueprintType)
enum class ENamiCameraModifyStage : uint8
{
	/** 修改输入参数（在计算输出前） */
	Input UMETA(DisplayName = "输入参数"),
	
	/** 修改输出结果（在计算输出后） */
	Output UMETA(DisplayName = "输出结果"),
};

/**
 * 相机效果结束行为
 */
UENUM(BlueprintType)
enum class ENamiCameraEndBehavior : uint8
{
	/** 混合回去：平滑过渡回原始状态 */
	BlendBack UMETA(DisplayName = "混合回去"),
	
	/** 立即结束：无过渡，立即切回 */
	ForceEnd UMETA(DisplayName = "立即结束"),
	
	/** 保持效果：不结束，保持当前状态（需手动结束） */
	Stay UMETA(DisplayName = "保持效果"),
};


