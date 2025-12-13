// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Enums/NamiCameraEnums.h"
#include "NamiCameraAdjustParams.h"
#include "NamiCameraAdjustCurveBinding.h"
#include "NamiCameraAdjust.generated.h"

class UNamiCameraComponent;

/** 被玩家输入打断时触发的委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCameraAdjustInputInterrupted);

/**
 * 相机调整基类
 *
 * CameraAdjust 是一个可堆叠的相机属性修改器，支持：
 * - 生命周期管理：OnActivate -> Tick -> OnDeactivate
 * - 混合控制：BlendIn/BlendOut 平滑过渡
 * - 混合模式：Additive（叠加）/ Override（覆盖）
 * - 曲线驱动：根据速度、时间等动态调整参数
 *
 * 使用方式：
 * 1. 继承此类并重写 CalculateAdjustParams
 * 2. 通过 UNamiCameraComponent::PushCameraAdjust 激活
 * 3. 通过 UNamiCameraComponent::PopCameraAdjust 停用
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class NAMICAMERA_API UNamiCameraAdjust : public UObject
{
	GENERATED_BODY()

public:
	UNamiCameraAdjust();

	// ========== 生命周期 ==========

	/**
	 * 初始化调整器
	 * @param InOwnerComponent 所属的相机组件
	 */
	virtual void Initialize(UNamiCameraComponent* InOwnerComponent);

	/**
	 * 激活时调用
	 * 在 BlendIn 开始时调用
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Adjust|Lifecycle")
	void OnActivate();

	/**
	 * 每帧更新
	 * @param DeltaTime 帧时间
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Adjust|Lifecycle")
	void Tick(float DeltaTime);

	/**
	 * 停用时调用
	 * 在 BlendOut 完成后调用
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Adjust|Lifecycle")
	void OnDeactivate();

	// ========== 核心接口 ==========

	/**
	 * 计算当前帧的调整参数
	 * 子类需要重写此方法来实现具体的调整逻辑
	 * @param DeltaTime 帧时间
	 * @return 调整参数
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Adjust")
	FNamiCameraAdjustParams CalculateAdjustParams(float DeltaTime);

	/**
	 * 获取经过权重缩放的最终调整参数
	 * @param DeltaTime 帧时间
	 * @return 缩放后的调整参数
	 */
	FNamiCameraAdjustParams GetWeightedAdjustParams(float DeltaTime);

	/**
	 * 请求停用（开始BlendOut）
	 * @param bForceImmediate 是否立即停用（跳过BlendOut）
	 */
	void RequestDeactivate(bool bForceImmediate = false);

	// ========== 混合控制 ==========

	/** 混合进入时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending", meta = (ClampMin = "0.0"))
	float BlendInTime;

	/** 混合退出时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending", meta = (ClampMin = "0.0"))
	float BlendOutTime;

	/** 混合曲线类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending")
	ENamiCameraBlendType BlendType;

	/** 自定义混合曲线（当BlendType为CustomCurve时使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending",
		meta = (EditCondition = "BlendType == ENamiCameraBlendType::CustomCurve"))
	UCurveFloat* BlendCurve;

	// ========== 混合模式 ==========

	/** 参数混合模式 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending")
	ENamiCameraAdjustBlendMode BlendMode;

	// ========== Override 模式参数 ==========

	/**
	 * 臂旋转目标值（Override模式使用）
	 * 相对于激活时刻角色朝向的偏移
	 * 例如：(0, 0, 90) 表示相机移动到角色右侧90度的位置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override Mode",
		meta = (EditCondition = "BlendMode == ENamiCameraAdjustBlendMode::Override"))
	FRotator ArmRotationTarget = FRotator::ZeroRotator;

	/**
	 * 获取缓存的世界空间目标臂旋转
	 * 在激活时刻基于角色朝向计算并缓存
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Adjust|Override")
	FRotator GetCachedWorldArmRotationTarget() const { return CachedWorldArmRotationTarget; }

	// ========== 玩家输入控制 ==========

	/**
	 * 是否允许玩家在混合过程中控制相机臂旋转
	 * true: 玩家输入直接控制相机臂，Adjust 不参与 ArmRotation 混合
	 * false: Adjust 控制相机臂，但会检测玩家输入并触发打断
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Control")
	bool bAllowPlayerInput = false;

	/**
	 * 输入打断阈值（鼠标移动超过此值视为有输入）
	 * 仅在 bAllowPlayerInput=false 时生效
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Control",
		meta = (EditCondition = "!bAllowPlayerInput", ClampMin = "0.1"))
	float InputInterruptThreshold = 1.0f;

	/**
	 * 被玩家输入打断时触发
	 * 可用于蓝图中响应打断事件（如：播放提示动画）
	 */
	UPROPERTY(BlueprintAssignable, Category = "Camera Adjust|Events")
	FOnCameraAdjustInputInterrupted OnInputInterrupted;

	/**
	 * 触发输入打断
	 * 调用后会广播 OnInputInterrupted 事件，并开始 BlendOut
	 */
	void TriggerInputInterrupt();

	/** 是否被玩家输入打断 */
	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	bool IsInputInterrupted() const { return bInputInterrupted; }

	/** 混出时是否已同步 ControlRotation */
	bool IsBlendOutSynced() const { return bBlendOutSynced; }

	/** 标记混出已同步 */
	void MarkBlendOutSynced() { bBlendOutSynced = true; }

	// ========== 优先级 ==========

	/** 优先级（数值越高越后处理，越能覆盖前面的效果） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Priority")
	int32 Priority;

	// ========== 曲线驱动 ==========

	/** 曲线驱动配置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve Driven")
	FNamiCameraAdjustCurveConfig CurveConfig;

	/**
	 * 设置自定义输入值（用于Custom输入源）
	 * @param Value 自定义输入值
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Adjust|Curve")
	void SetCustomInput(float Value);

	/**
	 * 获取当前自定义输入值
	 * @return 自定义输入值
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Adjust|Curve")
	float GetCustomInput() const { return CustomInputValue; }

	// ========== 状态查询 ==========

	/** 获取当前混合权重（0-1） */
	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	float GetCurrentBlendWeight() const { return CurrentBlendWeight; }

	/** 获取当前状态 */
	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	ENamiCameraAdjustState GetState() const { return State; }

	/** 是否处于激活状态（包括BlendingIn和Active） */
	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	bool IsActive() const;

	/** 是否正在混合进入 */
	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	bool IsBlendingIn() const { return State == ENamiCameraAdjustState::BlendingIn; }

	/** 是否正在混合退出 */
	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	bool IsBlendingOut() const { return State == ENamiCameraAdjustState::BlendingOut; }

	/** 是否完全激活（权重为1） */
	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	bool IsFullyActive() const { return State == ENamiCameraAdjustState::Active; }

	/** 是否已完全停用 */
	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	bool IsFullyInactive() const { return State == ENamiCameraAdjustState::Inactive; }

	/** 获取激活后经过的时间 */
	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	float GetActiveTime() const { return ActiveTime; }

	/** 获取所属的相机组件 */
	UFUNCTION(BlueprintPure, Category = "Camera Adjust")
	UNamiCameraComponent* GetOwnerComponent() const;

protected:
	// ========== 内部状态 ==========

	/** 当前混合权重 */
	float CurrentBlendWeight;

	/** 混合计时器 */
	float BlendTimer;

	/** 当前状态 */
	ENamiCameraAdjustState State;

	/** 激活后经过的时间 */
	float ActiveTime;

	/** 自定义输入值 */
	float CustomInputValue;

	/** 是否被玩家输入打断 */
	bool bInputInterrupted;

	/** 混出时是否已同步 ControlRotation */
	bool bBlendOutSynced = false;

	/** 所属的相机组件 */
	UPROPERTY()
	TWeakObjectPtr<UNamiCameraComponent> OwnerComponent;

	/** 【缓存】激活时刻计算的世界空间目标臂旋转（Override模式使用） */
	FRotator CachedWorldArmRotationTarget;

	// ========== 内部方法 ==========

	/**
	 * 缓存臂旋转目标（Override模式）
	 * 在激活时刻调用，基于角色朝向计算世界空间目标
	 */
	void CacheArmRotationTarget();

	/**
	 * 更新混合状态
	 * @param DeltaTime 帧时间
	 */
	void UpdateBlending(float DeltaTime);

	/**
	 * 根据BlendType计算混合Alpha值
	 * @param LinearAlpha 线性Alpha值（0-1）
	 * @return 经过曲线变换的Alpha值
	 */
	float CalculateBlendAlpha(float LinearAlpha) const;

	/**
	 * 获取指定输入源的当前值
	 * @param Source 输入源类型
	 * @return 输入值
	 */
	float GetInputSourceValue(ENamiCameraAdjustInputSource Source) const;

	/**
	 * 评估曲线绑定
	 * @param Binding 曲线绑定配置
	 * @return 评估结果
	 */
	float EvaluateCurveBinding(const FNamiCameraAdjustCurveBinding& Binding) const;

	/**
	 * 应用曲线驱动的参数到调整参数
	 * @param OutParams 输出参数
	 */
	void ApplyCurveDrivenParams(FNamiCameraAdjustParams& OutParams) const;

	// Blueprint 实现
	virtual void OnActivate_Implementation();
	virtual void Tick_Implementation(float DeltaTime);
	virtual void OnDeactivate_Implementation();
	virtual FNamiCameraAdjustParams CalculateAdjustParams_Implementation(float DeltaTime);
};
