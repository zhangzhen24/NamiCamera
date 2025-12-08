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
 * 视角控制模式
 * 
 * 定义当 Modifier 修改玩家视角时，如何处理玩家输入
 */
UENUM(BlueprintType)
enum class ENamiCameraControlMode : uint8
{
	/** 
	 * 建议视角：玩家输入可打断
	 * 当检测到玩家相机输入时，自动退出视角控制
	 * 适用于：技能演出、轻量引导
	 */
	Suggestion UMETA(DisplayName = "建议视角"),
	
	/** 
	 * 强制视角：玩家输入不可打断
	 * 完全控制玩家视角，忽略玩家输入
	 * 适用于：过场动画、QTE、剧情演出
	 */
	Forced UMETA(DisplayName = "强制视角"),
	
	/** 
	 * 混合视角：玩家输入会衰减效果权重
	 * 玩家输入越大，效果权重越低
	 * 适用于：柔和的视角引导
	 */
	Blended UMETA(DisplayName = "混合视角"),
};

/**
 * 相机输入模式
 * 
 * 定义技能相机如何处理玩家输入
 */
UENUM(BlueprintType)
enum class ENamiCameraInputMode : uint8
{
	/** 
	 * 无输入：完全忽略玩家输入
	 * 适用于：过场动画、QTE、剧情演出
	 */
	NoInput UMETA(DisplayName = "无输入"),
	
	/** 
	 * 有输入：允许玩家输入影响相机
	 * 适用于：技能相机、战斗相机
	 */
	WithInput UMETA(DisplayName = "有输入"),
};

