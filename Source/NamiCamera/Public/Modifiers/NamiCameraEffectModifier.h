// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "Camera/PlayerCameraManager.h"
#include "NamiCameraEffectModifier.generated.h"

/**
 * Nami 相机效果修改器
 * 
 * 通用的相机视图修改器，用于临时调整相机参数
 * 不改变相机模式，只修改最终的相机视图
 * 
 * 适用场景：
 * - 技能释放时的镜头特写
 * - 过场动画的镜头调整
 * - 剧情对话的镜头切换
 * - 环境交互的镜头效果
 * - 事件触发的镜头变化
 * - QTE 时刻的镜头强调
 * - 任何需要临时调整相机的场景
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Nami Camera Effect Modifier"))
class NAMICAMERA_API UNamiCameraEffectModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	UNamiCameraEffectModifier();

	// ========== 基础配置 ==========
	
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
	
	// ========== 相机变换偏移 ==========
	
	/** 是否启用位置偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Transform",
			  meta = (InlineEditConditionToggle))
	bool bEnableLocationOffset = false;
	
	/** 相机位置偏移（局部空间） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Transform",
			  meta = (EditCondition = "bEnableLocationOffset"))
	FVector LocationOffset = FVector::ZeroVector;
	
	/** 位置偏移空间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Transform",
			  meta = (EditCondition = "bEnableLocationOffset"))
	TEnumAsByte<ERelativeTransformSpace> LocationOffsetSpace = RTS_Actor;
	
	/** 是否启用旋转偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Transform",
			  meta = (InlineEditConditionToggle))
	bool bEnableRotationOffset = false;
	
	/** 相机旋转偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Transform",
			  meta = (EditCondition = "bEnableRotationOffset"))
	FRotator RotationOffset = FRotator::ZeroRotator;
	
	/** 是否启用距离偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Transform",
			  meta = (InlineEditConditionToggle))
	bool bEnableDistanceOffset = false;
	
	/** 相机距离偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Transform",
			  meta = (EditCondition = "bEnableDistanceOffset"))
	float DistanceOffset = 0.0f;
	
	// ========== FOV 控制 ==========
	
	/** 是否启用 FOV 偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|FOV",
			  meta = (InlineEditConditionToggle))
	bool bEnableFOVOffset = false;
	
	/** FOV 偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|FOV",
			  meta = (EditCondition = "bEnableFOVOffset"))
	float FOVOffset = 0.0f;
	
	/** 是否覆盖 FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|FOV",
			  meta = (EditCondition = "bEnableFOVOffset"))
	bool bOverrideFOV = false;
	
	/** 目标 FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|FOV",
			  meta = (EditCondition = "bEnableFOVOffset && bOverrideFOV",
					  ClampMin = "5.0", ClampMax = "170.0"))
	float TargetFOV = 90.0f;
	
	// ========== LookAt 控制 ==========
	
	/** 是否覆盖 LookAt 目标 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|LookAt",
			  meta = (InlineEditConditionToggle))
	bool bOverrideLookAtTarget = false;
	
	/** LookAt 目标 Actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|LookAt",
			  meta = (EditCondition = "bOverrideLookAtTarget"))
	TWeakObjectPtr<AActor> LookAtTarget;
	
	/** LookAt 目标位置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|LookAt",
			  meta = (EditCondition = "bOverrideLookAtTarget"))
	FVector LookAtLocation = FVector::ZeroVector;
	
	/** 是否使用 Actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|LookAt",
			  meta = (EditCondition = "bOverrideLookAtTarget"))
	bool bUseLookAtActor = true;
	
	/** LookAt 权重 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|LookAt",
			  meta = (EditCondition = "bOverrideLookAtTarget",
					  ClampMin = "0.0", ClampMax = "1.0"))
	float LookAtWeight = 1.0f;
	
	/** LookAt 偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|LookAt",
			  meta = (EditCondition = "bOverrideLookAtTarget"))
	FVector LookAtOffset = FVector::ZeroVector;
	
	// ========== 相机效果 ==========
	
	/** 相机震动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Effects")
	TSubclassOf<UCameraShakeBase> CameraShake;
	
	/** 震动强度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Effects")
	float ShakeScale = 1.0f;
	
	/** 是否在效果开始时播放震动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Effects")
	bool bPlayShakeOnStart = true;
	
	/** 是否循环播放震动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Effects")
	bool bLoopShake = false;
	
	// ========== 后处理效果 ==========
	
	/** 是否启用后处理效果 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|PostProcess",
			  meta = (InlineEditConditionToggle))
	bool bEnablePostProcess = false;
	
	/** 后处理设置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|PostProcess",
			  meta = (EditCondition = "bEnablePostProcess", ShowOnlyInnerProperties))
	FPostProcessSettings PostProcessSettings;
	
	/** 后处理权重 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|PostProcess",
			  meta = (EditCondition = "bEnablePostProcess",
					  ClampMin = "0.0", ClampMax = "1.0"))
	float PostProcessWeight = 1.0f;
	
	// ========== UCameraModifier 接口 ==========
	
	virtual bool ModifyCamera(float DeltaTime, struct FMinimalViewInfo& InOutPOV) override;
	virtual void AddedToCamera(APlayerCameraManager* Camera) override;
	
	// ========== 公共接口 ==========
	
	/**
	 * 激活效果
	 * @param bResetTimer 是否重置计时器
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect")
	void ActivateEffect(bool bResetTimer = false);
	
	/**
	 * 停止效果
	 * @param bForceImmediate 是否立即停止
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect")
	void DeactivateEffect(bool bForceImmediate = false);
	
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
	 * 设置 LookAt 目标
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect")
	void SetLookAtTarget(AActor* Target, FVector Offset = FVector::ZeroVector);
	
	/**
	 * 设置 LookAt 位置
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Effect")
	void SetLookAtLocation(FVector Location);
	
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
	
protected:
	/** 当前混合权重 */
	float CurrentBlendWeight = 0.0f;
	
	/** 已激活时间 */
	float ActiveTime = 0.0f;
	
	/** 是否正在激活 */
	bool bIsActive = false;
	
	/** 是否正在退出 */
	bool bIsExiting = false;
	
	/** 是否暂停 */
	bool bIsPaused = false;
	
	/** 原始视图信息 */
	FMinimalViewInfo OriginalPOV;
	
	/** 相机震动实例 */
	TWeakObjectPtr<UCameraShakeBase> ActiveShakeInstance;
	
	/** 计算混合权重 */
	float CalculateBlendWeight(float DeltaTime);
	
	/** 应用位置偏移 */
	void ApplyLocationOffset(FMinimalViewInfo& InOutPOV, float Weight);
	
	/** 应用旋转偏移 */
	void ApplyRotationOffset(FMinimalViewInfo& InOutPOV, float Weight);
	
	/** 应用 FOV 偏移 */
	void ApplyFOVOffset(FMinimalViewInfo& InOutPOV, float Weight);
	
	/** 应用 LookAt */
	void ApplyLookAt(FMinimalViewInfo& InOutPOV, float Weight);
	
	/** 应用后处理 */
	void ApplyPostProcess(FMinimalViewInfo& InOutPOV, float Weight);
};

