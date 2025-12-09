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
 * 用于定义要修改的相机参数，支持枢轴点、吊臂、相机等所有参数的修改。
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraStateModify
{
	GENERATED_BODY()

	// ==================== 枢轴点参数修改 ====================
	
	/** 枢轴点位置修改。枢轴点是相机围绕旋转的中心点，通常是角色位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Pivot",
			  meta = (Tooltip = "枢轴点是相机围绕旋转的中心点，通常是角色位置。\nAdditive 模式：在当前位置基础上偏移。\nOverride 模式：直接设置目标位置。"))
	FNamiVectorModify PivotLocation;
	
	/** 枢轴点旋转修改。基础旋转，决定相机的基础朝向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Pivot",
			  meta = (Tooltip = "基础旋转，决定相机的基础朝向。\nAdditive 模式：在当前旋转基础上叠加。\nOverride 模式：直接设置目标旋转。"))
	FNamiRotatorModify PivotRotation;
	
	// ==================== 吊臂参数修改 ====================
	
	/** 吊臂长度修改。调整相机与角色之间的距离。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Arm",
			  meta = (Tooltip = "调整相机与角色之间的距离。\nAdditive 模式：正值拉远，负值拉近。\nOverride 模式：直接设置目标距离。"))
	FNamiFloatModify ArmLength;
	
	/** 吊臂旋转修改（围绕枢轴点旋转）。主要影响相机位置（让相机沿以角色为中心的圆弧移动），间接影响相机朝向。与CameraRotationOffset的区别：ArmRotation既影响位置也影响朝向，CameraRotationOffset只影响朝向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Arm",
			  meta = (Tooltip = "调整吊臂方向，以角色位置为中心，以吊臂长度为半径旋转。\n主要影响相机位置（让相机沿以角色为中心的圆弧移动），间接影响相机朝向。\nPitch：俯仰（正值向上，形成俯视角）。\nYaw：偏航（正值向右绕行）。\n与 CameraRotationOffset 的区别：ArmRotation 既影响位置也影响朝向，CameraRotationOffset 只影响朝向。"))
	FNamiRotatorModify ArmRotation;
	
	/** 吊臂偏移修改。在吊臂本地空间中的偏移，常用于肩部视角切换（左右偏移）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Arm",
			  meta = (Tooltip = "在吊臂本地空间中的偏移。\n常用于肩部视角切换（左右偏移）。\nAdditive 模式：在当前偏移基础上叠加。\nOverride 模式：直接设置目标偏移。"))
	FNamiVectorModify ArmOffset;
	
	// ==================== 控制旋转参数修改 ====================
	
	/** 控制旋转偏移修改（玩家视角意图）。修改ControlRotation，代表临时改变玩家"想看哪里"。可被玩家输入打断（根据ControlMode设置）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Control",
			  meta = (Tooltip = "修改 ControlRotation，代表临时改变玩家想看哪里。\n可被玩家输入打断（根据 ControlMode 设置）。\n区别于 CameraRotationOffset：ControlRotationOffset 修改玩家视角意图，CameraRotationOffset 是纯视觉效果。"))
	FNamiRotatorModify ControlRotationOffset;
	
	/** 视角控制模式。决定玩家输入如何影响视角控制。建议视角：玩家输入可打断。强制视角：忽略玩家输入。混合视角：玩家输入衰减效果。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Control",
			  meta = (EditCondition = "ControlRotationOffset.bEnabled",
					  Tooltip = "建议视角：玩家输入可打断（推荐用于技能演出）\n强制视角：忽略玩家输入（用于过场动画、QTE）\n混合视角：玩家输入衰减效果"))
	ENamiCameraControlMode ControlMode = ENamiCameraControlMode::Suggestion;
	
	/** 相机输入检测阈值（死区）。输入值低于此阈值时不触发打断。推荐值0.1（10%）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Control",
			  meta = (EditCondition = "ControlRotationOffset.bEnabled && ControlMode != ENamiCameraControlMode::Forced",
					  ClampMin = "0.01", ClampMax = "0.5",
					  Tooltip = "相机输入死区，推荐 0.1（10%）。输入值低于此阈值时不触发打断。"))
	float CameraInputThreshold = 0.1f;
	
	/** 是否允许玩家输入。false时CameraAdjust完全控制所有参数，玩家输入被忽略。true时玩家输入控制旋转参数，CameraAdjust控制位置参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Control",
			  meta = (Tooltip = "false（不接入输入）：\n- CameraAdjust 完全控制所有参数\n- 玩家输入被忽略\n- 全程由 CameraAdjust 接管，不支持输入打断\n\ntrue（接入输入）：\n- CameraAdjust 只控制位置参数（ArmLength, ArmOffset, PivotLocation）\n- 玩家输入控制旋转参数（ArmRotation, CameraRotationOffset, PivotRotation）\n- 始终允许玩家输入"))
	bool bAllowPlayerInput = false;
	
	
	// ==================== 相机参数修改 ====================
	
	/** 相机位置偏移修改（相机本地空间）。在吊臂计算后应用的额外位置偏移，用于微调相机位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Camera",
			  meta = (Tooltip = "在吊臂计算完成后，额外的相机位置偏移（相机本地空间）。\n用于微调相机位置，如技能特效时的相机抖动。\nAdditive 模式：在当前偏移基础上叠加。\nOverride 模式：直接设置目标偏移。"))
	FNamiVectorModify CameraLocationOffset;
	
	/** 相机旋转偏移修改。只影响相机朝向，不影响相机位置。用于看向特定方向或目标。不受玩家输入影响。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Camera",
			  meta = (Tooltip = "只改变相机朝向，不影响相机位置。\n用于看向特定方向或目标。\nPitch：俯仰（正值向上看）。\nYaw：偏航（正值向右看）。\nRoll：翻滚。\n不受玩家输入影响。"))
	FNamiRotatorModify CameraRotationOffset;
	
	/** FOV修改。调整相机的视野角度（Field of View）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Camera",
			  meta = (Tooltip = "调整相机的视野角度（Field of View）。\nAdditive 模式：正值广角，负值长焦。\nOverride 模式：直接设置目标 FOV。"))
	FNamiFloatModify FieldOfView;

	// ==================== 输出结果修改 ====================
	
	/** 最终相机位置修改（直接修改输出）。直接设置相机的最终位置，跳过所有计算。谨慎使用，通常用于特殊效果。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output",
			  meta = (Tooltip = "直接设置相机的最终位置，跳过所有计算。谨慎使用，通常用于特殊效果。"))
	FNamiVectorModify CameraLocation;
	
	/** 最终相机旋转修改（直接修改输出）。直接设置相机的最终旋转，跳过所有计算。谨慎使用，通常用于特殊效果。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output",
			  meta = (Tooltip = "直接设置相机的最终旋转，跳过所有计算。谨慎使用，通常用于特殊效果。"))
	FNamiRotatorModify CameraRotation;

	// ==================== 混出设置 ====================
	
	/** 混出时保持相机旋转。如果启用，在混出时相机旋转会保持在混出开始时的状态，不会混合回默认值。如果禁用，相机旋转会正常混合回默认值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend Out",
			  meta = (Tooltip = "如果启用，在混出（BlendOut）时，相机旋转会保持在混出开始时的状态，不会混合回默认值。\n如果禁用，相机旋转会正常混合回默认值。\n默认启用，适用于大多数技能镜头场景。"))
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

