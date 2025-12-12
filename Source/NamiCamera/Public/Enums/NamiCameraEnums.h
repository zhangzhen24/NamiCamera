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

/**
 * 相机调整混合模式
 * 用于 CameraAdjust 系统，定义参数如何与基础值混合
 */
UENUM(BlueprintType)
enum class ENamiCameraAdjustBlendMode : uint8
{
	/** 叠加模式：在当前值基础上增加/减少
	 *  FOV = BaseFOV + (Offset * Weight)
	 */
	Additive UMETA(DisplayName = "叠加"),

	/** 覆盖模式：按权重插值到目标值
	 *  FOV = Lerp(BaseFOV, TargetFOV, Weight)
	 */
	Override UMETA(DisplayName = "覆盖"),

	/** 乘法模式：乘以系数
	 *  FOV = BaseFOV * Lerp(1.0, Multiplier, Weight)
	 */
	Multiplicative UMETA(DisplayName = "乘法"),
};

/**
 * 相机调整曲线输入源
 * 用于曲线驱动的动态参数变化
 */
UENUM(BlueprintType)
enum class ENamiCameraAdjustInputSource : uint8
{
	/** 不使用曲线，使用固定值 */
	None UMETA(DisplayName = "无"),

	/** 角色移动速度 */
	MoveSpeed UMETA(DisplayName = "移动速度"),

	/** 视角旋转速度 */
	LookSpeed UMETA(DisplayName = "视角速度"),

	/** 激活后经过的时间 */
	Time UMETA(DisplayName = "时间"),

	/** 自定义输入（通过 SetCustomInput 设置） */
	Custom UMETA(DisplayName = "自定义"),
};

/**
 * 相机调整状态
 */
UENUM(BlueprintType)
enum class ENamiCameraAdjustState : uint8
{
	/** 未激活 */
	Inactive UMETA(DisplayName = "未激活"),

	/** 混合进入中 */
	BlendingIn UMETA(DisplayName = "混合进入"),

	/** 完全激活 */
	Active UMETA(DisplayName = "激活"),

	/** 混合退出中 */
	BlendingOut UMETA(DisplayName = "混合退出"),
};
