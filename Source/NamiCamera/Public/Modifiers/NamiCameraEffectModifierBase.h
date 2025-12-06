// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "Camera/PlayerCameraManager.h"
#include "NamiCameraEffectModifierBase.generated.h"

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
 * Nami 相机效果修改器基类
 * 
 * 提供通用的生命周期管理和混合控制
 * 所有功能型 Modifier 应继承此类
 * 
 * 核心功能：
 * - 生命周期管理（激活/停用/暂停/恢复）
 * - 混合权重计算
 * - 持续时间管理
 * - 基础接口
 */
UCLASS(Abstract, Blueprintable, BlueprintType, meta = (DisplayName = "Nami Camera Effect Modifier Base"))
class NAMICAMERA_API UNamiCameraEffectModifierBase : public UCameraModifier
{
	GENERATED_BODY()

public:
	UNamiCameraEffectModifierBase();

	// ========== UCameraModifier 接口 ==========
	
	virtual bool ModifyCamera(float DeltaTime, struct FMinimalViewInfo& InOutPOV) override;
	virtual void AddedToCamera(APlayerCameraManager* Camera) override;

	// ========== 公共接口 ==========
	
	/**
	 * 激活效果
	 * @param bResetTimer 是否重置计时器
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect")
	virtual void ActivateEffect(bool bResetTimer = false);
	
	/**
	 * 停止效果
	 * @param bForceImmediate 是否立即停止
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect")
	virtual void DeactivateEffect(bool bForceImmediate = false);
	
	/**
	 * 暂停效果
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect")
	void PauseEffect();
	
	/**
	 * 恢复效果
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect")
	void ResumeEffect();
	
	/**
	 * 打断效果（使用打断混合时间）
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect")
	void InterruptEffect();
	
	/**
	 * 获取当前混合权重
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Effect")
	float GetCurrentBlendWeight() const { return CurrentBlendWeight; }
	
	/**
	 * 获取已激活时间
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Effect")
	float GetActiveTime() const { return ActiveTime; }
	
	/**
	 * 是否正在激活
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Effect")
	bool IsActive() const { return bIsActive; }
	
	/**
	 * 获取 CameraOwner（用于检查是否已正确添加到 CameraManager）
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Effect")
	APlayerCameraManager* GetOwningCameraManager() const { return CameraOwner; }

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
	
	/** 混合函数类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Blending")
	TEnumAsByte<EViewTargetBlendFunction> BlendFunction = VTBlend_Cubic;
	
	/** 
	 * 结束行为
	 * 控制效果结束时的处理方式
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Behavior",
			  meta = (DisplayName = "结束行为",
					  Tooltip = "混合回去：平滑过渡回原状态\n立即结束：无过渡直接切回\n保持效果：不结束，需手动停止"))
	ENamiCameraEndBehavior EndBehavior = ENamiCameraEndBehavior::BlendBack;
	
	/** 
	 * 允许输入打断
	 * 当玩家有输入时是否打断效果
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Behavior",
			  meta = (DisplayName = "允许输入打断",
					  Tooltip = "启用后，玩家有移动/攻击等输入时会打断效果"))
	bool bAllowInputInterrupt = false;
	
	/** 打断时的混合时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Behavior",
			  meta = (DisplayName = "打断混合时间",
					  EditCondition = "bAllowInputInterrupt",
					  ClampMin = "0.0", ClampMax = "2.0",
					  Tooltip = "被打断时的混合退出时间"))
	float InterruptBlendTime = 0.15f;

protected:
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
	
	/** 原始视图信息 */
	FMinimalViewInfo OriginalPOV;
	
	/** 计算混合权重 */
	float CalculateBlendWeight(float DeltaTime);
	
	/**
	 * 应用效果（子类实现）
	 * @param InOutPOV 输入输出的视图
	 * @param Weight 混合权重
	 * @param DeltaTime 帧时间
	 * @return 是否应用了效果
	 */
	virtual bool ApplyEffect(struct FMinimalViewInfo& InOutPOV, float Weight, float DeltaTime) { return false; }
	
	/**
	 * 检查是否应该继续运行（子类可以重写）
	 * 用于检查是否有激活的震动等需要保持 Modifier 激活的情况
	 */
	virtual bool ShouldKeepActive(float Weight) const { return false; }
};

