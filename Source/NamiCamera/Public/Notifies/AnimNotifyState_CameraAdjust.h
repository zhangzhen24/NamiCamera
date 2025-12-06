// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "Structs/Modify/NamiCameraModifyTypes.h"
#include "Enums/NamiCameraEnums.h"
#include "AnimNotifyState_CameraAdjust.generated.h"

class UNamiCameraAdjustFeature;
class UNamiCameraModeBase;

/**
 * 相机调整动画通知状态
 * 
 * 用于在动画中控制相机参数调整
 * 
 * 核心特点：
 * - 直接使用 FNamiCameraStateModify 配置（与 AdjustFeature 一致）
 * - 支持 BlendIn/BlendOut 混合
 * - 自动使用动画总时长作为 Duration
 * 
 * 使用场景：
 * - 技能动画中的相机调整
 * - 过场动画的相机控制
 * - 任何需要基于动画时长的相机效果
 */
UCLASS(meta = (DisplayName = "Camera Adjust"))
class NAMICAMERA_API UAnimNotifyState_CameraAdjust : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UAnimNotifyState_CameraAdjust();

	// ==================== 相机调整配置 ====================

	/**
	 * 相机调整配置
	 * 包含所有可调整的相机参数（吊臂、旋转、枢轴点等）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Adjust",
			  meta = (ShowInnerProperties, DisplayName = "调整配置"))
	FNamiCameraStateModify AdjustConfig;

	// ==================== 混合设置 ====================

	/** 混入时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending",
			  meta = (DisplayName = "混入时间",
					  ClampMin = "0.0", ClampMax = "2.0",
					  Tooltip = "效果从 0 到 1 的混合时间"))
	float BlendInTime = 0.15f;

	/** 混出时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending",
			  meta = (DisplayName = "混出时间",
					  ClampMin = "0.0", ClampMax = "2.0",
					  Tooltip = "效果从 1 到 0 的混合时间"))
	float BlendOutTime = 0.2f;

	/** 结束行为 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending",
			  meta = (DisplayName = "结束行为",
					  Tooltip = "混合回去：平滑过渡回原状态\n立即结束：无过渡直接切回\n保持效果：不结束，需手动停止"))
	ENamiCameraEndBehavior EndBehavior = ENamiCameraEndBehavior::BlendBack;

	// ==================== GameplayTags ====================

	/**
	 * 手动清理 Tag（用于标记需要手动清理的 Feature）
	 * 当 EndBehavior 为 Stay 时，会自动添加此 Tag 到 Feature
	 * 留空则使用默认值 "Camera.Feature.ManualCleanup"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayTags",
			  meta = (DisplayName = "手动清理 Tag",
					  Tooltip = "用于标记需要手动清理的 Feature。留空则使用默认值 Camera.Feature.ManualCleanup。"))
	FGameplayTag StayTag;

	/**
	 * 来源标识 Tag（用于标识此 Feature 的来源）
	 * 当 EndBehavior 为 Stay 时，会自动添加此 Tag 到 Feature
	 * 留空则使用默认值 "Camera.Feature.Source.ANS.CameraAdjust"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayTags",
			  meta = (DisplayName = "来源 Tag",
					  Tooltip = "用于标识此 Feature 的来源。留空则使用默认值 Camera.Feature.Source.ANS.CameraAdjust。"))
	FGameplayTag SourceTag;

	// ==================== AnimNotifyState 接口 ====================

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, 
							float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
						  const FAnimNotifyEventReference& EventReference) override;
	
	virtual FString GetNotifyName_Implementation() const override;

protected:
	/**
	 * 创建并配置 Adjust Feature
	 */
	UNamiCameraAdjustFeature* CreateAdjustFeature(UNamiCameraComponent* CameraComponent, const FName& EffectName, float TotalDuration);

	/**
	 * 停止 Adjust Feature
	 */
	void StopAdjustFeature();

	/**
	 * 生成唯一的效果名称（基于动画和实例）
	 */
	FName GenerateUniqueEffectName(const UAnimSequenceBase* Animation) const;

private:
	/** 激活的相机调整 Feature 实例 */
	TWeakObjectPtr<UNamiCameraAdjustFeature> ActiveAdjustFeature;
};

