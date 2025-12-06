// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Features/NamiCameraEffectFeature.h"
#include "Structs/Modify/NamiCameraModifyTypes.h"
#include "Structs/State/NamiCameraState.h"
#include "Enums/NamiCameraEnums.h"
#include "Interfaces/NamiCameraInputProvider.h"
#include "NamiCameraAdjustFeature.generated.h"

/**
 * 相机调整 Feature
 * 
 * 功能：
 * - 调整相机参数（吊臂、旋转、枢轴点等）
 * - 支持 BlendIn/BlendOut 混合
 * - 支持输入打断
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
	virtual bool CheckInputInterrupt() const override;

	// ========== 配置 ==========

	/**
	 * 调整配置（与 ANS 的 StateModify 相同）
	 * 包含所有可调整的相机参数
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjust",
			  meta = (ShowInnerProperties, DisplayName = "调整配置"))
	FNamiCameraStateModify AdjustConfig;

	/**
	 * 是否启用输入打断（整段效果）
	 * 当玩家输入超过阈值时，整个效果按打断混出时间退出
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjust|Input",
			  meta = (DisplayName = "允许输入打断整段效果",
					  Tooltip = "开启后，当玩家输入超过阈值时，整个效果按打断混出时间退出"))
	bool bAllowInputInterruptWholeEffect = false;

	/**
	 * 输入触发打断的阈值（整段打断）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjust|Input",
			  meta = (DisplayName = "打断阈值",
					  EditCondition = "bAllowInputInterruptWholeEffect",
					  ClampMin = "0.01", ClampMax = "0.5",
					  Tooltip = "玩家输入超过此值触发整段效果打断；建议与视角控制阈值保持一致，默认 0.1"))
	float InterruptInputThreshold = 0.1f;

	/**
	 * 打断冷却时间，避免抖动反复触发
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjust|Input",
			  meta = (DisplayName = "打断冷却时间",
					  EditCondition = "bAllowInputInterruptWholeEffect",
					  ClampMin = "0.0", ClampMax = "1.0",
					  Tooltip = "触发一次打断后，冷却期内不再重复触发；默认 0.15"))
	float InterruptCooldown = 0.15f;

	/**
	 * 是否启用视角控制打断
	 * 当玩家有相机输入时，是否打断视角控制（ControlRotationOffset）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjust|Input",
			  meta = (DisplayName = "允许视角控制打断",
					  EditCondition = "AdjustConfig.ControlRotationOffset.bEnabled && AdjustConfig.ControlMode != ENamiCameraControlMode::Forced",
					  Tooltip = "开启后，玩家有相机输入时会打断视角控制"))
	bool bAllowViewControlInterrupt = true;

	/**
	 * 视角控制输入阈值
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjust|Input",
			  meta = (DisplayName = "视角控制输入阈值",
					  EditCondition = "bAllowViewControlInterrupt",
					  ClampMin = "0.01", ClampMax = "0.5",
					  Tooltip = "相机输入死区，低于此值不触发视角打断；推荐 0.1（10%）"))
	float ViewControlInputThreshold = 0.1f;

	/**
	 * 设置输入提供者
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Adjust Feature")
	void SetInputProvider(TScriptInterface<INamiCameraInputProvider> InInputProvider);

	/**
	 * 获取输入提供者
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Adjust Feature")
	TScriptInterface<INamiCameraInputProvider> GetInputProvider() const { return InputProvider; }

protected:
	// ========== State 计算相关 ==========

	/**
	 * 从视图构建初始 State
	 */
	FNamiCameraState BuildInitialStateFromView(const FNamiCameraView& View) const;

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

	// ========== 输入打断相关 ==========

	/**
	 * 检查整段效果输入打断
	 */
	void CheckWholeEffectInterrupt(float DeltaTime);

	/**
	 * 检查视角控制输入打断
	 */
	void CheckViewControlInterrupt();

	/**
	 * 获取玩家输入大小
	 */
	float GetPlayerCameraInputMagnitude() const;

	/**
	 * 更新缓存的引用
	 */
	void UpdateCachedReferences();

	// ========== 内部状态 ==========

	/** 输入提供者 */
	UPROPERTY()
	TScriptInterface<INamiCameraInputProvider> InputProvider;

	/** 缓存的 PlayerController */
	TWeakObjectPtr<APlayerController> CachedPlayerController;

	/** 缓存的 Pawn */
	TWeakObjectPtr<APawn> CachedPawn;

	/** 缓存的初始 State */
	FNamiCameraState CachedInitialState;

	/** 是否已缓存初始 State */
	bool bHasCachedInitialState = false;

	/** 输入打断冷却计时器 */
	float InputInterruptCooldownTimer = 0.0f;

	/** 视角控制是否被输入打断 */
	bool bViewControlInterrupted = false;

	/** 上一帧的相机输入大小 */
	float LastCameraInputMagnitude = 0.0f;
};

