// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_NamiCameraEffect.generated.h"

/**
 * LookAt 目标类型
 */
UENUM(BlueprintType)
enum ELookAtTargetType
{
	/** 动画通知所在的 Actor */
	AnimNotifyActor UMETA(DisplayName = "动画 Actor"),
	
	/** 玩家控制的 Pawn */
	PlayerPawn UMETA(DisplayName = "玩家 Pawn"),
	
	/** 自定义 Actor */
	CustomActor UMETA(DisplayName = "自定义 Actor"),
	
	/** 不覆盖 */
	None UMETA(DisplayName = "不覆盖")
};

/**
 * Nami 相机效果动画通知状态
 * 
 * 在动画蒙太奇中使用，自动管理相机效果的激活和退出
 * 支持预设和自定义配置
 */
UCLASS(DisplayName = "Nami Camera Effect", meta = (DisplayName = "Nami Camera Effect"))
class NAMICAMERA_API UAnimNotifyState_NamiCameraEffect : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	// ========== 预设配置（可选）==========
	
	/** 
	 * 相机效果预设（可选）
	 * 
	 * 用途：
	 * - 当多个动画需要相同的镜头效果时，使用预设可以统一管理
	 * - 修改预设会影响所有引用它的动画
	 * - 留空则使用下方的直接配置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Effect",
			  meta = (DisplayName = "效果预设（可选）"))
	class UNamiCameraEffectPreset* EffectPreset = nullptr;
	
	// ========== 基础配置 ==========
	
	/** 效果名称（用于调试） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 基础配置",
			  meta = (DisplayName = "效果名称"))
	FName EffectName = TEXT("CameraEffect");
	
	/** 进入混合时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 基础配置",
			  meta = (DisplayName = "进入混合时间",
					  ClampMin = "0.0", ClampMax = "5.0",
					  Tooltip = "相机效果淡入的时间。如果使用预设，此值会覆盖预设配置"))
	float BlendInTime = 0.3f;
	
	/** 退出混合时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 基础配置",
			  meta = (DisplayName = "退出混合时间",
					  ClampMin = "0.0", ClampMax = "5.0",
					  Tooltip = "相机效果淡出的时间。如果使用预设，此值会覆盖预设配置"))
	float BlendOutTime = 0.5f;
	
	/** 是否覆盖预设的混合时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 基础配置",
			  meta = (DisplayName = "覆盖混合时间",
					  EditCondition = "EffectPreset != nullptr",
					  Tooltip = "是否使用上方的混合时间覆盖预设中的配置"))
	bool bOverrideBlendTime = false;
	
	// ========== 位置偏移 ==========
	
	/** 启用位置偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 位置偏移",
			  meta = (DisplayName = "启用位置偏移",
					  InlineEditConditionToggle))
	bool bEnableLocationOffset = false;
	
	/** 位置偏移（局部空间） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 位置偏移",
			  meta = (DisplayName = "位置偏移",
					  EditCondition = "bEnableLocationOffset",
					  Tooltip = "相机位置偏移（局部空间）：\n• X: 左右（正值向右）\n• Y: 前后（正值向前）\n• Z: 上下（正值向上）"))
	FVector LocationOffset = FVector::ZeroVector;
	
	/** 是否覆盖预设的位置偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 位置偏移",
			  meta = (DisplayName = "覆盖预设",
					  EditCondition = "EffectPreset != nullptr && bEnableLocationOffset"))
	bool bOverrideLocationOffset = false;
	
	// ========== 旋转偏移 ==========
	
	/** 启用旋转偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 旋转偏移",
			  meta = (DisplayName = "启用旋转偏移",
					  InlineEditConditionToggle))
	bool bEnableRotationOffset = false;
	
	/** 旋转偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 旋转偏移",
			  meta = (DisplayName = "旋转偏移",
					  EditCondition = "bEnableRotationOffset",
					  Tooltip = "相机旋转偏移：\n• Pitch: 俯仰（正值向上看）\n• Yaw: 偏航（正值向右转）\n• Roll: 翻滚（正值顺时针）"))
	FRotator RotationOffset = FRotator::ZeroRotator;
	
	/** 是否覆盖预设的旋转偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 旋转偏移",
			  meta = (DisplayName = "覆盖预设",
					  EditCondition = "EffectPreset != nullptr && bEnableRotationOffset"))
	bool bOverrideRotationOffset = false;
	
	// ========== 距离偏移 ==========
	
	/** 启用距离偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4. 距离偏移",
			  meta = (DisplayName = "启用距离偏移",
					  InlineEditConditionToggle))
	bool bEnableDistanceOffset = false;
	
	/** 距离偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4. 距离偏移",
			  meta = (DisplayName = "距离偏移",
					  EditCondition = "bEnableDistanceOffset",
					  ClampMin = "-1000.0", ClampMax = "1000.0",
					  Tooltip = "相机距离偏移：\n• 负值：拉近镜头\n• 正值：拉远镜头\n推荐值：-150（特写），-100（近景），0（正常）"))
	float DistanceOffset = -100.0f;
	
	/** 是否覆盖预设的距离偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4. 距离偏移",
			  meta = (DisplayName = "覆盖预设",
					  EditCondition = "EffectPreset != nullptr && bEnableDistanceOffset"))
	bool bOverrideDistanceOffset = false;
	
	// ========== FOV 控制 ==========
	
	/** 启用 FOV 偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. FOV 控制",
			  meta = (DisplayName = "启用 FOV 偏移",
					  InlineEditConditionToggle))
	bool bEnableFOVOffset = false;
	
	/** FOV 偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. FOV 控制",
			  meta = (DisplayName = "FOV 偏移",
					  EditCondition = "bEnableFOVOffset",
					  ClampMin = "-90.0", ClampMax = "90.0",
					  Tooltip = "FOV 偏移：\n• 负值：更聚焦（长焦效果）\n• 正值：更广角\n推荐值：-15（特写），-10（近景），0（正常）"))
	float FOVOffset = -10.0f;
	
	/** 是否覆盖预设的 FOV 偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. FOV 控制",
			  meta = (DisplayName = "覆盖预设",
					  EditCondition = "EffectPreset != nullptr && bEnableFOVOffset"))
	bool bOverrideFOVOffset = false;
	
	// ========== LookAt 控制 ==========
	
	/** 覆盖 LookAt 目标 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "6. LookAt 控制",
			  meta = (DisplayName = "覆盖 LookAt 目标",
					  InlineEditConditionToggle))
	bool bOverrideLookAtTarget = false;
	
	/** LookAt 目标类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "6. LookAt 控制",
			  meta = (DisplayName = "目标类型",
					  EditCondition = "bOverrideLookAtTarget"))
	TEnumAsByte<ELookAtTargetType> LookAtTargetType = ELookAtTargetType::AnimNotifyActor;
	
	/** 自定义 LookAt 目标 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "6. LookAt 控制",
			  meta = (DisplayName = "自定义目标",
					  EditCondition = "bOverrideLookAtTarget && LookAtTargetType == ELookAtTargetType::CustomActor"))
	TSoftObjectPtr<AActor> CustomLookAtTarget;
	
	/** LookAt 偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "6. LookAt 控制",
			  meta = (DisplayName = "LookAt 偏移",
					  EditCondition = "bOverrideLookAtTarget",
					  Tooltip = "相对于目标的偏移（世界空间）"))
	FVector LookAtOffset = FVector::ZeroVector;
	
	/** LookAt 权重 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "6. LookAt 控制",
			  meta = (DisplayName = "LookAt 权重",
					  EditCondition = "bOverrideLookAtTarget",
					  ClampMin = "0.0", ClampMax = "1.0",
					  Tooltip = "LookAt 的混合权重（0=不看向目标，1=完全看向目标）"))
	float LookAtWeight = 1.0f;
	
	// ========== 相机效果 ==========
	
	/** 相机震动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "7. 相机效果",
			  meta = (DisplayName = "相机震动"))
	TSubclassOf<UCameraShakeBase> CameraShake;
	
	/** 震动强度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "7. 相机效果",
			  meta = (DisplayName = "震动强度",
					  ClampMin = "0.0", ClampMax = "10.0"))
	float ShakeScale = 1.0f;
	
	/** 是否覆盖预设的相机震动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "7. 相机效果",
			  meta = (DisplayName = "覆盖预设震动",
					  EditCondition = "EffectPreset != nullptr"))
	bool bOverrideCameraShake = false;
	
	// ========== AnimNotifyState 接口 ==========
	
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, 
							float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
						  const FAnimNotifyEventReference& EventReference) override;
	
	virtual FString GetNotifyName_Implementation() const override;

#if WITH_EDITOR
	virtual void ValidateAssociatedAssets() override;
#endif

private:
	/** 激活的修改器实例 */
	TWeakObjectPtr<class UNamiCameraEffectModifier> ActiveModifier;
	
	/** 创建修改器（合并预设和直接配置） */
	class UNamiCameraEffectModifier* CreateModifier(USkeletalMeshComponent* MeshComp);
	
	/** 应用预设配置 */
	void ApplyPresetToModifier(class UNamiCameraEffectModifier* Modifier);
	
	/** 应用直接配置（覆盖预设） */
	void ApplyDirectConfigToModifier(class UNamiCameraEffectModifier* Modifier);
	
	/** 解析 LookAt 目标 */
	AActor* ResolveLookAtTarget(USkeletalMeshComponent* MeshComp) const;
	
	/** 获取 PlayerCameraManager */
	class APlayerCameraManager* GetPlayerCameraManager(USkeletalMeshComponent* MeshComp) const;
};

