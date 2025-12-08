// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "Structs/Modify/NamiCameraModifyTypes.h"
#include "Enums/NamiCameraEnums.h"
#include "Camera/CameraShakeBase.h"
#include "AnimNotifyState_CameraAdjust.generated.h"

class UNamiCameraAdjustFeature;
class UNamiCameraShakeFeature;
class UNamiCameraModeBase;
class UNamiCameraComponent;

/**
 * 相机调整动画通知状态
 * 
 * 用于在动画中控制相机参数调整和效果
 * 
 * 核心特点：
 * - 支持修改所有相机参数（吊臂、旋转、枢轴点等）
 * - 支持相机震动效果
 * - 支持输入打断
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

	// ==================== 1. 枢轴点参数 ====================

	/** 枢轴点位置修改 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 枢轴点",
			  meta = (DisplayName = "枢轴位置",
					  Tooltip = "枢轴点是相机围绕旋转的中心点，通常是角色位置。\nAdditive 模式：在当前位置基础上偏移。\nOverride 模式：直接设置目标位置。"))
	FNamiVectorModify PivotLocation;

	/** 枢轴点旋转修改 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 枢轴点",
			  meta = (DisplayName = "枢轴旋转",
					  Tooltip = "基础旋转，决定相机的基础朝向。\nAdditive 模式：在当前旋转基础上叠加。\nOverride 模式：直接设置目标旋转。"))
	FNamiRotatorModify PivotRotation;

	// ==================== 2. 吊臂参数 ====================

	/** 吊臂长度调整 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 吊臂",
			  meta = (DisplayName = "吊臂长度",
					  Tooltip = "调整相机与角色之间的距离。\nAdditive 模式：正值拉远，负值拉近。\nOverride 模式：直接设置目标距离。"))
	FNamiFloatModify ArmLength;

	/** 吊臂旋转调整 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 吊臂",
			  meta = (DisplayName = "吊臂旋转",
					  Tooltip = "调整吊臂方向，以角色位置为中心，以吊臂长度为半径旋转。\n主要影响相机位置（让相机沿以角色为中心的圆弧移动），间接影响相机朝向。\nPitch：俯仰（正值向上，形成俯视角）。\nYaw：偏航（正值向右绕行）。\n可通过控制模式设置是否允许玩家输入打断。"))
	FNamiRotatorModify ArmRotation;

	/** 吊臂偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 吊臂",
			  meta = (DisplayName = "吊臂偏移",
					  Tooltip = "在吊臂本地空间中的偏移。\n常用于肩部视角切换（左右偏移）。\nAdditive 模式：在当前偏移基础上叠加。\nOverride 模式：直接设置目标偏移。"))
	FNamiVectorModify ArmOffset;

	// ==================== 3. 视角控制设置 ====================

	/** 
	 * 是否允许玩家输入
	 * 
	 * false（不接入输入）：
	 *   - CameraAdjust 完全控制所有参数
	 *   - 玩家输入被忽略
	 *   - 全程由 CameraAdjust 接管，不支持输入打断
	 * 
	 * true（接入输入）：
	 *   - CameraAdjust 只控制位置参数（ArmLength, ArmOffset, PivotLocation）
	 *   - 玩家输入控制旋转参数（ArmRotation, CameraRotationOffset, PivotRotation）
	 *   - 始终允许玩家输入
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 视角控制",
			  meta = (DisplayName = "允许玩家输入",
					  Tooltip = "开启后，玩家输入控制旋转参数，CameraAdjust 控制位置参数"))
	bool bAllowPlayerInput = false;

	/** 视角控制模式 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 视角控制",
			  meta = (DisplayName = "控制模式",
					  EditCondition = "ArmRotation.bEnabled && !bAllowPlayerInput",
					  Tooltip = "建议视角：玩家输入可打断（推荐用于技能演出）\n强制视角：忽略玩家输入（用于过场动画、QTE）\n混合视角：玩家输入衰减效果\n注意：仅在允许玩家输入关闭时生效"))
	ENamiCameraControlMode ControlMode = ENamiCameraControlMode::Suggestion;

	/** 
	 * 相机输入检测阈值（死区）
	 * 输入值低于此阈值时不触发打断
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 视角控制",
			  meta = (DisplayName = "输入阈值",
					  EditCondition = "ArmRotation.bEnabled && !bAllowPlayerInput && ControlMode != ENamiCameraControlMode::Forced",
					  ClampMin = "0.01", ClampMax = "0.5",
					  Tooltip = "相机输入死区，推荐 0.1（10%）。输入值低于此阈值时不触发打断。"))
	float CameraInputThreshold = 0.1f;

	// ==================== 4. 相机参数 ====================

	/** 相机位置偏移（相机本地空间） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4. 相机",
			  meta = (DisplayName = "相机位置偏移",
					  Tooltip = "在吊臂计算完成后，额外的相机位置偏移（相机本地空间）。\n用于微调相机位置，如技能特效时的相机抖动。\nAdditive 模式：在当前偏移基础上叠加。\nOverride 模式：直接设置目标偏移。"))
	FNamiVectorModify CameraLocationOffset;

	/** 相机旋转偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4. 相机",
			  meta = (DisplayName = "相机旋转偏移",
					  Tooltip = "只改变相机朝向，不影响相机位置。\n用于看向特定方向或目标。\nPitch：俯仰（正值向上看）。\nYaw：偏航（正值向右看）。\nRoll：翻滚。\n不受玩家输入影响。"))
	FNamiRotatorModify CameraRotationOffset;

	/** FOV 调整 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4. 相机",
			  meta = (DisplayName = "视野角度",
					  Tooltip = "调整相机的视野角度（Field of View）。\nAdditive 模式：正值广角，负值长焦。\nOverride 模式：直接设置目标 FOV。"))
	FNamiFloatModify FieldOfView;

	// ==================== 5. 输出结果 ====================

	/** 最终相机位置修改（直接修改输出） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. 输出结果",
			  meta = (DisplayName = "最终相机位置",
					  Tooltip = "直接设置相机的最终位置，跳过所有计算。谨慎使用，通常用于特殊效果。"))
	FNamiVectorModify CameraLocation;

	/** 最终相机旋转修改（直接修改输出） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. 输出结果",
			  meta = (DisplayName = "最终相机旋转",
					  Tooltip = "直接设置相机的最终旋转，跳过所有计算。谨慎使用，通常用于特殊效果。"))
	FNamiRotatorModify CameraRotation;

	// ==================== 6. 效果 ====================

	/** 相机震动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "6. 效果",
			  meta = (DisplayName = "相机震动",
					  Tooltip = "相机震动效果类。\n选择继承自 UCameraShakeBase 的震动类，用于在动画期间产生相机震动效果。\n震动效果会在动画开始时激活，在动画结束时停止（根据结束行为）。"))
	TSubclassOf<UCameraShakeBase> CameraShake;

	/** 震动强度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "6. 效果",
			  meta = (DisplayName = "震动强度",
					  EditCondition = "CameraShake != nullptr",
					  ClampMin = "0.0", ClampMax = "5.0",
					  Tooltip = "震动效果的强度倍数。\n1.0 为原始强度，大于 1.0 增强震动，小于 1.0 减弱震动。\n建议范围：0.0 - 5.0。"))
	float ShakeScale = 1.0f;

	/** 激活前是否强制停止已有技能镜头 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "6. 效果",
			  meta = (DisplayName = "激活前清理已有镜头",
					  Tooltip = "开启后，在创建本次镜头前会停止当前玩家上的所有相机效果，避免技能镜头叠加。"))
	bool bStopExistingEffects = false;

	// ==================== 7. 混合设置 ====================

	/** 混入时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "7. 混合",
			  meta = (DisplayName = "混入时间",
					  ClampMin = "0.0", ClampMax = "2.0",
					  Tooltip = "效果从 0 到 1 的混合时间"))
	float BlendInTime = 0.15f;

	/** 混出时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "7. 混合",
			  meta = (DisplayName = "混出时间",
					  ClampMin = "0.0", ClampMax = "2.0",
					  Tooltip = "效果从 1 到 0 的混合时间"))
	float BlendOutTime = 0.2f;

	/** 结束行为 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "7. 混合",
			  meta = (DisplayName = "结束行为",
					  Tooltip = "混合回去：平滑过渡回原状态\n立即结束：无过渡直接切回\n保持效果：不结束，需手动停止"))
	ENamiCameraEndBehavior EndBehavior = ENamiCameraEndBehavior::BlendBack;

	/** 混出时保持相机旋转 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "7. 混合",
			  meta = (DisplayName = "混出时保持相机旋转",
					  Tooltip = "如果启用，在混出（BlendOut）时，相机旋转会保持在混出开始时的状态，不会混合回默认值。\n如果禁用，相机旋转会正常混合回默认值。\n默认启用，适用于大多数技能镜头场景。"))
	bool bPreserveCameraRotationOnExit = true;

	/** 最大角度变化速度（度/秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "7. 混合",
			  meta = (DisplayName = "最大角度变化速度（度/秒）",
					  ClampMin = "0.0", UIMin = "0.0",
					  Tooltip = "用于限制 PivotRotation 的变化速度，防止鼠标快速输入时导致 PivotLocation 跳变。\n0 表示不限制（使用最短路径直接变化）。\n建议值：\n- 默认：720.0（每秒2圈，适合大多数情况）\n- 快速响应：1080.0（每秒3圈）\n- 平滑响应：360.0（每秒1圈）"))
	float MaxPivotRotationSpeed = 720.0f;

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
	/** 清理现有同类效果（可选整清） */
	void ClearExistingEffects(UNamiCameraComponent* CameraComponent, UNamiCameraModeBase* ActiveMode, const FName& StateEffectName, const FName& ShakeEffectName) const;

	/** 创建并配置 Adjust Feature */
	UNamiCameraAdjustFeature* CreateAdjustFeature(UNamiCameraComponent* CameraComponent, const FName& StateEffectName, float TotalDuration);

	/** 创建并配置 Shake Feature */
	UNamiCameraShakeFeature* CreateShakeFeature(UNamiCameraModeBase* ActiveMode, const FName& ShakeEffectName);

	/** 停止 Adjust Feature */
	void StopAdjustFeature();

	/** 停止 Shake Feature */
	void StopShakeFeature();

	/** 构建修改配置 */
	FNamiCameraStateModify BuildStateModify() const;

	/** 生成唯一的效果名称（基于动画和实例） */
	FName GenerateUniqueEffectName(const UAnimSequenceBase* Animation, const FString& Suffix = TEXT("")) const;

#if WITH_EDITOR
	virtual void ValidateAssociatedAssets() override;
#endif

private:
	/** 激活的相机调整 Feature 实例 */
	TWeakObjectPtr<UNamiCameraAdjustFeature> ActiveAdjustFeature;

	/** 激活的震动 Feature 实例 */
	TWeakObjectPtr<UNamiCameraShakeFeature> ActiveShakeFeature;
};

