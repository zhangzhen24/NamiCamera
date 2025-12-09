// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Features/NamiCameraEffectFeature.h"
#include "Structs/Modify/NamiCameraModifyTypes.h"
#include "Structs/State/NamiCameraState.h"
#include "Structs/Pipeline/NamiCameraPipelineContext.h"
#include "Enums/NamiCameraEnums.h"
#include "NamiCameraAdjustFeature.generated.h"

/**
 * 相机调整 Feature（重新设计）
 * 
 * 设计原则：
 * - 使用管线级参数更新控制
 * - 状态机管理生命周期
 * - 简化混入/混出逻辑
 * - 支持 bAllowPlayerInput 不同状态下的打断需求
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class NAMICAMERA_API UNamiCameraAdjustFeature : public UNamiCameraEffectFeature
{
	GENERATED_BODY()

public:
	UNamiCameraAdjustFeature();

	// ========== 重写 ==========

	virtual void Initialize_Implementation(UNamiCameraModeBase* InCameraMode) override;
	virtual void Activate_Implementation() override;
	virtual void Update_Implementation(float DeltaTime) override;
	virtual void ApplyToViewWithContext_Implementation(
		FNamiCameraView& InOutView, 
		float DeltaTime,
		FNamiCameraPipelineContext& Context) override;
	virtual void DeactivateEffect(bool bForceImmediate = false) override;

	// ========== 配置 ==========

	/** 调整配置。包含所有可调整的相机参数（与ANS的StateModify相同）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjust",
			  meta = (ShowInnerProperties, Tooltip = "包含所有可调整的相机参数（与ANS的StateModify相同）。"))
	FNamiCameraStateModify AdjustConfig;

	/** PivotRotation 变化速度限制（度/秒）。用于限制PivotRotation的变化速度，防止鼠标快速输入时导致PivotLocation跳变。0表示不限制（使用最短路径直接变化）。建议值：默认720.0（每秒2圈），快速响应1080.0（每秒3圈），平滑响应360.0（每秒1圈）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjust|Smoothing",
			  meta = (ClampMin = "0.0", UIMin = "0.0",
					  Tooltip = "用于限制 PivotRotation 的变化速度，防止鼠标快速输入时导致 PivotLocation 跳变。\n0 表示不限制（使用最短路径直接变化）。\n建议值：\n- 默认：720.0（每秒2圈，适合大多数情况）\n- 快速响应：1080.0（每秒3圈）\n- 平滑响应：360.0（每秒1圈）"))
	float MaxPivotRotationSpeed = 720.0f;

protected:
	// ========== 状态机 ==========

	enum class EAdjustPhase
	{
		Inactive,      // 未激活
		BlendIn,       // 混入中
		Active,        // 激活中
		BlendOut,      // 混出中（正常）
		BlendOutInterrupted  // 混出中（输入打断）
	};

	EAdjustPhase CurrentPhase = EAdjustPhase::Inactive;

	// ========== 状态快照 ==========

	/** 混入时的基础状态（混出目标） */
	FNamiCameraState BlendInBaseState;
	bool bHasBlendInBaseState = false;

	/** 混出时的起始状态（混出起点） */
	FNamiCameraState BlendOutStartState;
	bool bHasBlendOutStartState = false;

	/** 混出时的相机旋转（用于保持） */
	FRotator BlendOutCameraRotation;
	bool bHasBlendOutCameraRotation = false;

	// ========== 核心方法 ==========

	/**
	 * 更新状态机
	 */
	void UpdatePhase(float Weight, bool bInIsExiting, bool bIsInterrupted);

	/**
	 * 检查并处理输入打断
	 */
	bool CheckAndHandleInterrupt(
		FNamiCameraPipelineContext& Context,
		float DeltaTime);

	/**
	 * 更新参数更新控制
	 */
	void UpdateParamControl(
		FNamiCameraPipelineContext& Context,
		float Weight);

	/**
	 * 处理混入阶段
	 */
	void ProcessBlendIn(
		const FNamiCameraView& BaseView,
		float Weight,
		float DeltaTime,
		FNamiCameraPipelineContext& Context,
		FNamiCameraView& OutView);

	/**
	 * 处理激活阶段
	 */
	void ProcessActive(
		const FNamiCameraView& BaseView,
		float Weight,
		float DeltaTime,
		FNamiCameraPipelineContext& Context,
		FNamiCameraView& OutView);

	/**
	 * 处理混出阶段
	 */
	void ProcessBlendOut(
		const FNamiCameraView& BaseView,
		float Weight,
		float DeltaTime,
		FNamiCameraPipelineContext& Context,
		FNamiCameraView& OutView);

	/**
	 * 从视图构建状态
	 */
	FNamiCameraState BuildStateFromView(
		const FNamiCameraView& View,
		float DeltaTime) const;

	/**
	 * 应用配置到状态
	 */
	void ApplyConfigToState(
		FNamiCameraState& InOutState,
		float Weight,
		const FNamiCameraPipelineContext& Context) const;

	/**
	 * 从状态计算视图
	 */
	void ComputeViewFromState(
		const FNamiCameraState& State,
		FNamiCameraView& OutView) const;

	/**
	 * 混合两个状态
	 */
	void BlendStates(
		const FNamiCameraState& From,
		const FNamiCameraState& To,
		float Weight,
		FNamiCameraState& OutState) const;

	/**
	 * 应用参数更新控制到视图
	 */
	void ApplyParamUpdateControlToView(
		FNamiCameraView& InOutView,
		const FNamiCameraPipelineContext& Context) const;

	// ========== 辅助方法 ==========

	FVector GetPivotLocation(const FNamiCameraView& View) const;
	FRotator GetCharacterRotation() const;

	/** 上一帧的 PivotRotation（用于确保最短路径，避免跳变） */
	mutable FRotator LastPivotRotation;
	mutable bool bHasLastPivotRotation = false;
};
