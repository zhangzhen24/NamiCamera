// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "NamiCameraEffectPreset.generated.h"

/**
 * Nami 相机效果预设
 * 
 * 用于复用常见的相机效果配置
 * 
 * 使用场景：
 * 1. 多个动画需要相同的镜头效果
 * 2. 统一管理某类技能的镜头风格
 * 3. 快速迭代和调整（修改预设影响所有引用）
 * 
 * 示例：
 * - "大招特写"预设：用于所有大招动画
 * - "受击反馈"预设：用于所有受击动画
 * - "处决镜头"预设：用于所有处决动画
 */
UCLASS(BlueprintType, meta = (DisplayName = "Nami Camera Effect Preset"))
class NAMICAMERA_API UNamiCameraEffectPreset : public UDataAsset
{
	GENERATED_BODY()

public:
	// ========== 预设信息 ==========
	
	/** 预设名称 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "预设信息",
			  meta = (DisplayName = "预设名称"))
	FText PresetName;
	
	/** 预设描述 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "预设信息",
			  meta = (DisplayName = "预设描述", MultiLine = true))
	FText Description;
	
	/** 预设分类标签 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "预设信息",
			  meta = (DisplayName = "分类标签"))
	FGameplayTag CategoryTag;
	
	/** 预设图标 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "预设信息",
			  meta = (DisplayName = "预设图标"))
	UTexture2D* Icon = nullptr;
	
	// ========== 基础配置 ==========
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "基础配置",
			  meta = (DisplayName = "进入混合时间", ClampMin = "0.0", ClampMax = "5.0"))
	float BlendInTime = 0.3f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "基础配置",
			  meta = (DisplayName = "退出混合时间", ClampMin = "0.0", ClampMax = "5.0"))
	float BlendOutTime = 0.5f;
	
	// ========== 位置偏移 ==========
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "位置偏移",
			  meta = (DisplayName = "启用位置偏移", InlineEditConditionToggle))
	bool bEnableLocationOffset = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "位置偏移",
			  meta = (DisplayName = "位置偏移", EditCondition = "bEnableLocationOffset"))
	FVector LocationOffset = FVector::ZeroVector;
	
	// ========== 旋转偏移 ==========
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "旋转偏移",
			  meta = (DisplayName = "启用旋转偏移", InlineEditConditionToggle))
	bool bEnableRotationOffset = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "旋转偏移",
			  meta = (DisplayName = "旋转偏移", EditCondition = "bEnableRotationOffset"))
	FRotator RotationOffset = FRotator::ZeroRotator;
	
	// ========== 距离偏移 ==========
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "距离偏移",
			  meta = (DisplayName = "启用距离偏移", InlineEditConditionToggle))
	bool bEnableDistanceOffset = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "距离偏移",
			  meta = (DisplayName = "距离偏移", EditCondition = "bEnableDistanceOffset",
					  ClampMin = "-1000.0", ClampMax = "1000.0"))
	float DistanceOffset = -100.0f;
	
	// ========== FOV 控制 ==========
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FOV 控制",
			  meta = (DisplayName = "启用 FOV 偏移", InlineEditConditionToggle))
	bool bEnableFOVOffset = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FOV 控制",
			  meta = (DisplayName = "FOV 偏移", EditCondition = "bEnableFOVOffset",
					  ClampMin = "-90.0", ClampMax = "90.0"))
	float FOVOffset = -10.0f;
	
	// ========== 相机效果 ==========
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "相机效果",
			  meta = (DisplayName = "相机震动"))
	TSubclassOf<UCameraShakeBase> CameraShake;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "相机效果",
			  meta = (DisplayName = "震动强度", ClampMin = "0.0", ClampMax = "10.0"))
	float ShakeScale = 1.0f;
	
	// ========== 后处理 ==========
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "后处理",
			  meta = (DisplayName = "启用后处理", InlineEditConditionToggle))
	bool bEnablePostProcess = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "后处理",
			  meta = (DisplayName = "后处理设置", EditCondition = "bEnablePostProcess",
					  ShowOnlyInnerProperties))
	FPostProcessSettings PostProcessSettings;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "后处理",
			  meta = (DisplayName = "后处理权重", EditCondition = "bEnablePostProcess",
					  ClampMin = "0.0", ClampMax = "1.0"))
	float PostProcessWeight = 1.0f;
	
	// ========== 工具方法 ==========
	
	/**
	 * 应用到修改器
	 * @param Modifier 目标修改器
	 */
	UFUNCTION(BlueprintCallable, Category = "Preset")
	void ApplyToModifier(class UNamiCameraEffectModifier* Modifier) const;
	
	/**
	 * 创建修改器实例
	 * @param Outer 外部对象
	 * @return 创建的修改器
	 */
	UFUNCTION(BlueprintCallable, Category = "Preset")
	class UNamiCameraEffectModifier* CreateModifier(UObject* Outer) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

