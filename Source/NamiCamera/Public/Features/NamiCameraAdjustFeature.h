// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Features/NamiCameraEffectFeature.h"
#include "Structs/Modify/NamiCameraModifyTypes.h"
#include "Structs/State/NamiCameraState.h"
#include "Enums/NamiCameraEnums.h"
#include "NamiCameraAdjustFeature.generated.h"

/**
 * 相机调整 Feature
 * 
 * 功能：
 * - 调整相机参数（吊臂、旋转、枢轴点等）
 * - 支持 BlendIn/BlendOut 混合
 * - 支持 Additive/Override 混合模式
 * 
 * 使用场景：
 * - ANS 技能镜头
 * - 动态相机调整
 * - 过场动画相机控制
 * 
 * 设计思路：
 * - 作为相机修改层，直接修改 FNamiCameraState 的输入参数
 * - 通过 ComputeOutput() 计算最终视图
 * - 简化状态管理，只缓存初始 State，不缓存中间状态
 * - 直接使用 FNamiCameraStateModify::ApplyToState 应用修改
 * 
 * 与 StateModifier 的区别：
 * - 在阶段一应用，接入平滑混合层
 * - 作为 Feature 添加到 Mode 或全局管理
 * - 生命周期由 Feature 系统管理
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
	virtual void ApplyEffect_Implementation(FNamiCameraView& InOutView, float Weight, float DeltaTime) override;
	virtual void DeactivateEffect(bool bForceImmediate = false) override;

	// ========== 配置 ==========

	/**
	 * 调整配置（与 ANS 的 StateModify 相同）
	 * 包含所有可调整的相机参数
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjust",
			  meta = (ShowInnerProperties, DisplayName = "调整配置"))
	FNamiCameraStateModify AdjustConfig;

	/**
	 * 最大角度变化速度（度/秒）
	 * 用于限制 PivotRotation 的变化速度，防止鼠标快速输入时导致 PivotLocation 跳变
	 * 0 表示不限制（使用最短路径直接变化）
	 * 
	 * 建议值：
	 * - 默认：720.0（每秒2圈，适合大多数情况）
	 * - 快速响应：1080.0（每秒3圈）
	 * - 平滑响应：360.0（每秒1圈）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjust|Smoothing",
			  meta = (ClampMin = "0.0", UIMin = "0.0", DisplayName = "最大角度变化速度（度/秒）"))
	float MaxPivotRotationSpeed = 720.0f;

protected:
	// ========== 内部状态（用于结束/打断流程） ==========
	/** 混出开始时的相机旋转（用于保持旋转） */
	FRotator ExitStartCameraRotation;
	bool bHasExitStartCameraRotation = false;

	/** 混出开始时的旋转状态（bAllowPlayerInput=false 时保存全部旋转参数） */
	FNamiCameraState ExitStartRotationState;
	bool bHasExitStartRotationState = false;

	/** 混出开始时的完整状态（打断后从当前镜头起点混出） */
	FNamiCameraState ExitStartState;
	bool bHasExitStartState = false;

	/** 上一帧输出的 State（打断首帧备用） */
	FNamiCameraState LastOutputState;
	bool bHasLastOutputState = false;

	// ========== 输入打断相关 ==========
	mutable FVector2D CachedInputVector = FVector2D::ZeroVector;
	mutable bool bInputVectorCacheValid = false;
	mutable float CachedInputMagnitude = 0.0f;
	mutable bool bInputMagnitudeCacheValid = false;
	float InputAccumulationTime = 0.0f;
	bool bInputEnabledOnInterrupt = false;

protected:
	// ========== State 计算相关 ==========

	/**
	 * 从视图构建初始 State
	 */
	FNamiCameraState BuildInitialStateFromView(const FNamiCameraView& View);

	/**
	 * 从 View 反推 State 参数（用于参数混合）
	 * 从 CameraLocation 和 PivotLocation 反推出 ArmLength、ArmRotation、ArmOffset
	 * 
	 * @param View 当前视图
	 * @param DeltaTime 帧时间（用于角度变化速度限制）
	 * @return 反推出的 State（包含当前参数）
	 */
	FNamiCameraState BuildCurrentStateFromView(const FNamiCameraView& View, float DeltaTime = 0.0f) const;

	/**
	 * 在 State 层面混合参数
	 * 只混合影响位置的参数（ArmLength、ArmRotation、ArmOffset），保证焦点不变
	 * 
	 * @param CurrentState 当前 State（会被修改）
	 * @param TargetState 目标 State
	 * @param Weight 混合权重（0.0 = 完全使用 Current，1.0 = 完全使用 Target）
	 * @param bFreezeRotations 是否冻结旋转参数（混出时使用）
	 */
	void BlendParametersInState(FNamiCameraState& CurrentState, const FNamiCameraState& TargetState, float Weight, bool bFreezeRotations = false) const;

	/**
	 * 应用调整配置到 State
	 */
	void ApplyAdjustConfigToState(FNamiCameraState& InOutState, float Weight) const;

	/**
	 * 从 State 计算视图
	 */
	void ComputeViewFromState(const FNamiCameraState& State, FNamiCameraView& OutView) const;

	/**
	 * 获取枢轴点位置
	 */
	FVector GetPivotLocation(const FNamiCameraView& View) const;

	/**
	 * 获取角色朝向（带回退逻辑）
	 * @param InState 当前 State（用于回退）
	 * @return 角色朝向，如果无法获取则回退到 PivotRotation
	 */
	FRotator GetCharacterRotation(const FNamiCameraState& InState) const;

	/**
	 * 获取玩家相机输入向量
	 * @return 输入向量 (X=Yaw, Y=Pitch)，范围 [-1.0, 1.0]
	 */
	FVector2D GetPlayerCameraInputVector() const;

	/**
	 * 获取玩家相机输入大小
	 * @return 输入大小（0.0 = 无输入，1.0 = 最大输入）
	 */
	float GetPlayerCameraInputMagnitude() const;

	// ========== 内部状态 ==========

	/** 缓存的初始 State */
	FNamiCameraState CachedInitialState;

	/** 是否已缓存初始 State */
	bool bHasCachedInitialState = false;

	/** 上一帧的 PivotRotation（用于确保最短路径，避免跳变） */
	mutable FRotator LastPivotRotation;

	/** 是否有上一帧的 PivotRotation 数据 */
	mutable bool bHasLastPivotRotation = false;

	/** 上一帧的 CharacterRotation（用于确保最短路径，避免跳变） */
	mutable FRotator LastCharacterRotation;

	/** 是否有上一帧的 CharacterRotation 数据 */
	mutable bool bHasLastCharacterRotation = false;
};

