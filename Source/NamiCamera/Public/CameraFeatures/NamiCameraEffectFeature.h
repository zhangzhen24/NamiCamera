// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CameraFeatures/NamiCameraFeature.h"
#include "Data/NamiCameraEnums.h"
#include "NamiCameraEffectFeature.generated.h"

/**
 * 相机效果 Feature 基类
 * 提供生命周期管理和混合控制，用于需要 BlendIn/BlendOut、Duration、输入打断等功能的效果 Feature。
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class NAMICAMERA_API UNamiCameraEffectFeature : public UNamiCameraFeature
{
	GENERATED_BODY()

public:
	UNamiCameraEffectFeature();

	// ========== 生命周期管理 ==========

	/**
	 * 激活效果
	 * @param bResetTimer 是否重置计时器
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect Feature")
	virtual void ActivateEffect(bool bResetTimer = false);

	/**
	 * 停止效果
	 * @param bForceImmediate 是否立即停止
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect Feature")
	virtual void DeactivateEffect(bool bForceImmediate = false);

	/**
	 * 暂停效果
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect Feature")
	void PauseEffect();

	/**
	 * 恢复效果
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect Feature")
	void ResumeEffect();

	/**
	 * 获取当前混合权重
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Effect Feature")
	float GetCurrentBlendWeight() const { return CurrentBlendWeight; }

	/**
	 * 获取已激活时间
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Effect Feature")
	float GetActiveTime() const { return ActiveTime; }

	/**
	 * 是否正在激活
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Effect Feature")
	bool IsActive() const { return bIsActive; }

	/**
	 * 是否正在退出
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Effect Feature")
	bool IsExiting() const { return bIsExiting; }

	// ========== 配置 ==========

	/** 效果名称（用于调试和识别） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FName EffectName = NAME_None;

	/** 效果持续时间（0 = 无限，需要手动结束） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	float Duration = 2.0f;

	/** 进入混合时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Blending")
	float BlendInTime = 0.3f;

	/** 退出混合时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Blending")
	float BlendOutTime = 0.5f;

	/** 混合曲线（可选） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Blending")
	UCurveFloat* BlendCurve = nullptr;

	/** 结束行为。混合回去：平滑过渡回原状态。立即结束：无过渡直接切回。保持效果：不结束，需手动停止。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Behavior",
			  meta = (Tooltip = "混合回去：平滑过渡回原状态\n立即结束：无过渡直接切回\n保持效果：不结束，需手动停止"))
	ENamiCameraEndBehavior EndBehavior = ENamiCameraEndBehavior::BlendBack;


protected:
	// ========== 重写 Feature 生命周期 ==========

	virtual void Activate_Implementation() override;
	virtual void Deactivate_Implementation() override;
	virtual void Update_Implementation(float DeltaTime) override;
	virtual void ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime) override;

	// ========== 内部状态 ==========

	/** 当前混合权重 */
	float CurrentBlendWeight = 0.0f;

	/** 已激活时间 */
	float ActiveTime = 0.0f;

	/** 是否正在激活 */
	bool bIsActive = false;

	/** 是否正在退出 */
	bool bIsExiting = false;

	/** 混出起始权重（用于在混入过程中提前混出时，从当前权重开始混出） */
	float BlendOutStartWeight = 1.0f;

	/** 是否暂停 */
	bool bIsPaused = false;

	/** 原始视图信息（用于退出时混合回去） */
	FNamiCameraView OriginalView;

	/** 是否已保存原始视图 */
	bool bHasSavedOriginalView = false;

	// ========== 内部方法 ==========

	/**
	 * 计算混合权重
	 */
	float CalculateBlendWeight(float DeltaTime);

	/**
	 * 应用效果（子类实现，接收混合权重）
	 * @param InOutView 输入输出的视图
	 * @param Weight 混合权重
	 * @param DeltaTime 帧时间
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Effect Feature")
	void ApplyEffect(UPARAM(ref) FNamiCameraView& InOutView, float Weight, float DeltaTime);

	/**
	 * 检查是否应该继续运行（子类可以重写）
	 * 用于检查是否有激活的震动等需要保持 Feature 激活的情况
	 */
	virtual bool ShouldKeepActive(float Weight) const { return false; }
};

