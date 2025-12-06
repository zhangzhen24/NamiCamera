// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modifiers/NamiCameraEffectModifierBase.h"
#include "Structs/State/NamiCameraState.h"
#include "Structs/Modify/NamiCameraModifyTypes.h"
#include "Interfaces/NamiCameraInputProvider.h"
#include "Structs/State/NamiCameraFramingTypes.h"
#include "NamiCameraStateModifier.generated.h"

/**
 * Nami 相机状态修改器
 *
 * 使用 State 系统修改相机参数
 * 支持吊臂旋转、长度、视角控制等功能
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Nami Camera State Modifier"))
class NAMICAMERA_API UNamiCameraStateModifier : public UNamiCameraEffectModifierBase
{
	GENERATED_BODY()

public:
	UNamiCameraStateModifier();

	// ========== UNamiCameraEffectModifierBase 接口 ==========

	virtual bool ApplyEffect(struct FMinimalViewInfo &InOutPOV, float Weight, float DeltaTime) override;
	virtual void AddedToCamera(APlayerCameraManager *Camera) override;

	// ========== 公共接口 ==========

	/**
	 * 相机状态修改配置
	 * 使用 State 系统计算相机视图
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Camera Effect|State")
	FNamiCameraStateModify StateModify;

	/**
	 * 设置输入提供者
	 * 用于解耦输入系统
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect")
	void SetInputProvider(TScriptInterface<INamiCameraInputProvider> InInputProvider);

	/**
	 * 获取输入提供者
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Effect")
	TScriptInterface<INamiCameraInputProvider> GetInputProvider() const { return InputProvider; }

	/** 输入触发整段打断的阈值（整段打断专用） */
	UPROPERTY(BlueprintReadWrite, Category = "Camera Effect|Interrupt")
	float InputInterruptThreshold = 0.1f;

	/** 整段打断的冷却时间，避免抖动反复触发 */
	UPROPERTY(BlueprintReadWrite, Category = "Camera Effect|Interrupt")
	float InputInterruptCooldown = 0.15f;

	// ========== 多目标构图 ==========

	/** 是否启用多目标构图（默认关闭） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Effect|Framing")
	bool bEnableFraming = false;

	/**
	 * 是否允许在 Modifier 内执行构图计算
	 * 默认关闭，以避免与模式层构图重复；需要时显式开启
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Effect|Framing")
	bool bUseModifierFraming = false;

	/** 构图配置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Effect|Framing")
	FNamiCameraFramingConfig FramingConfig;

	/** 构图调试可视化（仅在本 Modifier 构图时生效） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Effect|Framing")
	bool bEnableFramingDebug = false;

	/** 设置构图目标（Actor 列表） */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect|Framing")
	void SetFramingTargets(const TArray<AActor *> &InTargets);

	/** 清空构图目标 */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect|Framing")
	void ClearFramingTargets();

protected:
	/** 输入提供者（用于解耦输入系统） */
	UPROPERTY(BlueprintReadWrite, Category = "Camera Effect|Input")
	TScriptInterface<INamiCameraInputProvider> InputProvider;

	/** 缓存的 PlayerController（避免重复查找） */
	TWeakObjectPtr<APlayerController> CachedPlayerController;

	/** 缓存的 Pawn（避免重复查找） */
	TWeakObjectPtr<APawn> CachedPawn;

	/** 缓存的初始 State（避免每帧重新构建） */
	FNamiCameraState CachedInitialState;

	/** 是否已缓存初始 State */
	bool bHasCachedInitialState = false;

	/** 上一次应用后的 State（用于 Additive/Override 模式，确保从上一次结果继续） */
	FNamiCameraState LastAppliedState;

	/** 是否已有上一次应用的状态 */
	bool bHasLastAppliedState = false;

	/** 初始状态构建时的 PivotLocation（用于检测位置突变） */
	FVector InitialPivotLocation = FVector::ZeroVector;

	/** 缓存的输入大小（避免重复计算） */
	float CachedInputMagnitude = 0.0f;

	/** 输入大小缓存是否有效 */
	bool bInputMagnitudeCacheValid = false;

	/** 上一帧的相机输入大小 */
	float LastCameraInputMagnitude = 0.0f;

	/** 视角控制是否已被打断 */
	bool bViewControlInterrupted = false;

	// ========== 退出阶段缓存 ==========
	/** 退出混出时，ArmRotation 的起始（角色空间）缓存，避免每帧重算导致空间漂移 */
	FRotator ExitStartCharacterSpaceArmRotation = FRotator::ZeroRotator;
	/** 退出混出时，缓存的 BasePivotQuat（退出瞬间的 PivotRotation + ControlRotationOffset） */
	FQuat ExitStartBasePivotQuat = FQuat::Identity;
	/** 退出混出时，缓存的 CharacterQuat（退出瞬间的角色朝向） */
	FQuat ExitStartCharacterQuat = FQuat::Identity;
	/** 是否已缓存退出混出的 ArmRotation 起点 */
	bool bHasExitArmStart = false;

	float ExitStartArmLength = 0.0f;
	FVector ExitStartArmOffset = FVector::ZeroVector;
	bool bHasExitArmLengthStart = false;
	bool bHasExitArmOffsetStart = false;

	/** 冷却计时（内部使用） */
	float InterruptCooldownRemaining = 0.0f;

	/** 最终输出是否强制瞬切（跳过输出层混合） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Effect|Output")
	bool bSnapOutput = false;

	// ========== 多目标构图状态 ==========
	/** 目标列表（弱引用，避免持有生命周期） */
	TArray<TWeakObjectPtr<AActor>> FramingTargets;
	/** 上一帧的构图中心，用于平滑 */
	FVector LastFramingCenter = FVector::ZeroVector;
	/** 上一帧的构图距离，用于平滑 */
	float LastFramingDistance = 300.0f;
	/** 构图告警是否已输出（避免刷屏） */
	bool bFramingWarningLogged = false;

	/**
	 * 从当前相机视图构建初始 State
	 * 用于将 FMinimalViewInfo 转换为 FNamiCameraState
	 */
	FNamiCameraState BuildInitialStateFromPOV(const FMinimalViewInfo &POV, const FVector &PivotLocation) const;

	/**
	 * 应用 State 修改并计算目标视图
	 * bForTargetMix=true 时使用满权重计算目标，不执行退出混出逻辑
	 */
	void ApplyStateModifyAndComputeView(FMinimalViewInfo &InOutPOV, float Weight, bool bForTargetMix = false);

	/**
	 * 获取枢轴点位置（通常是角色位置）
	 * 使用缓存避免重复查找
	 */
	FVector GetPivotLocation(const FMinimalViewInfo &InOutPOV) const;

	/** 更新缓存的 PlayerController 和 Pawn 引用 */
	void UpdateCachedReferences();

	/**
	 * 检测玩家是否有相机输入
	 * @return 相机输入的大小（0 = 无输入，1 = 最大输入）
	 */
	float GetPlayerCameraInputMagnitude() const;

	/** 检查是否应该被玩家输入打断 */
	void CheckViewControlInterrupt();

	/** 检查是否需要整段打断（玩家输入触发） */
	void CheckWholeEffectInterrupt(float DeltaTime);

	/** 计算多目标构图结果（返回是否成功） */
	bool ComputeFramingResult(float DeltaTime, const FMinimalViewInfo &InOutPOV, FNamiCameraFramingResult &OutResult);
};
