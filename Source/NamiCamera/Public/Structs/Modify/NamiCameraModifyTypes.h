// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Structs/State/NamiCameraState.h"
#include "NamiCameraModifyTypes.generated.h"

/**
 * 浮点数修改配置
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiFloatModify
{
	GENERATED_BODY()

	/** 是否启用此修改 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modify",
			  meta = (InlineEditConditionToggle))
	bool bEnabled = false;
	
	/** 混合模式 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modify",
			  meta = (EditCondition = "bEnabled"))
	ENamiCameraBlendMode BlendMode = ENamiCameraBlendMode::Additive;
	
	/** 
	 * 修改值
	 * Additive 模式：增量值
	 * Override 模式：目标值
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modify",
			  meta = (EditCondition = "bEnabled"))
	float Value = 0.0f;
	
	/** 应用修改到目标值 */
	float Apply(float InValue, float Weight = 1.0f) const;
};

/**
 * 向量修改配置
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiVectorModify
{
	GENERATED_BODY()

	/** 是否启用此修改 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modify",
			  meta = (InlineEditConditionToggle))
	bool bEnabled = false;
	
	/** 混合模式 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modify",
			  meta = (EditCondition = "bEnabled"))
	ENamiCameraBlendMode BlendMode = ENamiCameraBlendMode::Additive;
	
	/** 
	 * 修改值
	 * Additive 模式：增量向量
	 * Override 模式：目标向量
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modify",
			  meta = (EditCondition = "bEnabled"))
	FVector Value = FVector::ZeroVector;
	
	/** 应用修改到目标向量 */
	FVector Apply(const FVector& InValue, float Weight = 1.0f) const;
};

/**
 * 旋转修改配置
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiRotatorModify
{
	GENERATED_BODY()

	/** 是否启用此修改 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modify",
			  meta = (InlineEditConditionToggle))
	bool bEnabled = false;
	
	/** 混合模式 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modify",
			  meta = (EditCondition = "bEnabled"))
	ENamiCameraBlendMode BlendMode = ENamiCameraBlendMode::Additive;
	
	/** 
	 * 修改值
	 * Additive 模式：增量旋转
	 * Override 模式：目标旋转
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modify",
			  meta = (EditCondition = "bEnabled"))
	FRotator Value = FRotator::ZeroRotator;
	
	/** 应用修改到目标旋转 */
	FRotator Apply(const FRotator& InValue, float Weight = 1.0f) const;
};

/**
 * 相机状态修改配置
 * 
 * 用于 ANS 和 Modifier 中定义要修改的相机参数
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraStateModify
{
	GENERATED_BODY()

	// ==================== 枢轴点参数修改 ====================
	
	/** 枢轴点位置修改 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Pivot",
			  meta = (DisplayName = "枢轴位置"))
	FNamiVectorModify PivotLocation;
	
	/** 枢轴点旋转修改 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Pivot",
			  meta = (DisplayName = "枢轴旋转"))
	FNamiRotatorModify PivotRotation;
	
	// ==================== 吊臂参数修改 ====================
	
	/** 吊臂长度修改 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Arm",
			  meta = (DisplayName = "吊臂长度"))
	FNamiFloatModify ArmLength;
	
	/** 
	 * 吊臂旋转修改（围绕枢轴点旋转）
	 * 
	 * 影响范围：
	 * - 主要影响：相机位置（让相机沿以角色为中心的圆弧移动）
	 * - 间接影响：相机朝向（相机朝向会跟随吊臂方向）
	 * 
	 * 与 CameraRotationOffset 的区别：
	 * - ArmRotation：调整吊臂方向，既影响位置也影响朝向（间接）
	 * - CameraRotationOffset：只调整相机朝向，不影响位置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Arm",
			  meta = (DisplayName = "吊臂旋转"))
	FNamiRotatorModify ArmRotation;
	
	/** 吊臂偏移修改 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Arm",
			  meta = (DisplayName = "吊臂偏移"))
	FNamiVectorModify ArmOffset;
	
	// ==================== 控制旋转参数修改 ====================
	
	/** 
	 * 控制旋转偏移修改（玩家视角意图）
	 * 
	 * 修改 ControlRotation，代表临时改变玩家"想看哪里"
	 * 可被玩家输入打断（根据 ControlMode 设置）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Control",
			  meta = (DisplayName = "视角控制"))
	FNamiRotatorModify ControlRotationOffset;
	
	/** 
	 * 视角控制模式
	 * 决定玩家输入如何影响视角控制
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Control",
			  meta = (DisplayName = "控制模式",
					  EditCondition = "ControlRotationOffset.bEnabled",
					  Tooltip = "建议视角：玩家输入可打断\n强制视角：忽略玩家输入\n混合视角：玩家输入衰减效果"))
	ENamiCameraControlMode ControlMode = ENamiCameraControlMode::Suggestion;
	
	/** 
	 * 相机输入检测阈值（死区）
	 * 输入值低于此阈值时不触发打断
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Control",
			  meta = (DisplayName = "输入阈值",
					  EditCondition = "ControlRotationOffset.bEnabled && ControlMode != ENamiCameraControlMode::Forced",
					  ClampMin = "0.01", ClampMax = "0.5",
					  Tooltip = "相机输入死区，推荐 0.1（10%）"))
	float CameraInputThreshold = 0.1f;
	
	/** 
	 * 是否允许玩家输入
	 * 
	 * false（不接入输入）：
	 *   - CameraAdjust 完全控制所有参数
	 *   - 玩家输入被忽略
	 *   - 全程由 CameraAdjust 接管，不支持输入打断
	 * 
	 * true（接入输入）：
	 *   - CameraAdjust 只控制位置参数（ArmLength, ArmOffset, PivotLocation）
	 *   - 玩家输入控制旋转参数（ArmRotation, CameraRotationOffset, PivotRotation）
	 *   - 始终允许玩家输入
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Control",
			  meta = (DisplayName = "允许玩家输入",
					  Tooltip = "开启后，玩家输入控制旋转参数，CameraAdjust 控制位置参数"))
	bool bAllowPlayerInput = false;
	
	
	// ==================== 相机参数修改 ====================
	
	/** 
	 * 相机位置偏移修改（相机本地空间）
	 * 在吊臂计算后应用的额外位置偏移
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Camera",
			  meta = (DisplayName = "相机位置偏移"))
	FNamiVectorModify CameraLocationOffset;
	
	/** 
	 * 相机旋转偏移修改
	 * 只影响相机朝向，不影响相机位置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Camera",
			  meta = (DisplayName = "相机旋转偏移"))
	FNamiRotatorModify CameraRotationOffset;
	
	/** FOV 修改 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Camera",
			  meta = (DisplayName = "视野角度"))
	FNamiFloatModify FieldOfView;

	// ==================== 输出结果修改 ====================
	
	/** 最终相机位置修改（直接修改输出） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output",
			  meta = (DisplayName = "最终相机位置"))
	FNamiVectorModify CameraLocation;
	
	/** 最终相机旋转修改（直接修改输出） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output",
			  meta = (DisplayName = "最终相机旋转"))
	FNamiRotatorModify CameraRotation;

	// ==================== 混出设置 ====================
	
	/** 混出时保持相机旋转 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend Out",
			  meta = (DisplayName = "混出时保持相机旋转",
					  Tooltip = "如果启用，在混出（BlendOut）时，相机旋转会保持在混出开始时的状态，不会混合回默认值。\n如果禁用，相机旋转会正常混合回默认值。"))
	bool bPreserveCameraRotationOnExit = true;

	// ==================== 方法 ====================
	
	/**
	 * 应用所有启用的修改到相机状态
	 * 
	 * @param InOutState 要修改的相机状态
	 * @param Weight 混合权重
	 */
	void ApplyToState(FNamiCameraState& InOutState, float Weight = 1.0f) const;
	
	/**
	 * 仅应用输入参数修改
	 */
	void ApplyInputToState(FNamiCameraState& InOutState, float Weight = 1.0f) const;
	
	/**
	 * 仅应用输出结果修改
	 */
	void ApplyOutputToState(FNamiCameraState& InOutState, float Weight = 1.0f) const;
	
	/**
	 * 是否有任何修改启用
	 */
	bool HasAnyModification() const;
	
	/**
	 * 是否有输入参数修改启用
	 */
	bool HasInputModification() const;
	
	/**
	 * 是否有输出结果修改启用
	 */
	bool HasOutputModification() const;
};

