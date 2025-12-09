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
 * 相机参数更新模式
 * 定义参数在不同阶段如何更新
 */
UENUM(BlueprintType)
enum class ENamiCameraParamUpdateMode : uint8
{
	/** 正常更新：由当前 Feature 控制 */
	Normal UMETA(DisplayName = "正常更新"),
	
	/** 停止更新：参数冻结在当前值 */
	Frozen UMETA(DisplayName = "停止更新"),
	
	/** 维持现状：保持当前值，不混合回基础值 */
	Preserved UMETA(DisplayName = "维持现状"),
	
	/** 玩家输入：由玩家输入控制 */
	PlayerInput UMETA(DisplayName = "玩家输入"),
	
	/** 基础值：使用基础状态的值（来自 Mode） */
	BaseValue UMETA(DisplayName = "基础值"),
};

/**
 * 输入控制状态
 * 管理当前 Feature 的输入控制模式
 */
UENUM(BlueprintType)
enum class ENamiCameraInputControlState : uint8
{
	/** 玩家输入模式：旋转参数由玩家输入控制 */
	PlayerInput UMETA(DisplayName = "玩家输入"),
	
	/** 完全控制模式：CameraAdjust 完全控制所有参数 */
	FullControl UMETA(DisplayName = "完全控制"),
	
	/** 可打断模式：CameraAdjust 控制，但可被玩家输入打断 */
	Interruptible UMETA(DisplayName = "可打断"),
	
	/** 混合模式：CameraAdjust 控制，但玩家输入会衰减权重 */
	Blended UMETA(DisplayName = "混合"),
	
	/** 打断中模式：已触发打断，旋转参数交给玩家 */
	Interrupted UMETA(DisplayName = "打断中"),
};

