// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "Structs/Modify/NamiCameraModifyTypes.h"
#include "Enums/NamiCameraEnums.h"
#include "Camera/CameraShakeBase.h"
#include "AnimNotifyState_BattleCamera.generated.h"

class UNamiCameraAdjustFeature;
class UNamiCameraShakeFeature;

/**
 * 相机效果结束行为
 */
UENUM(BlueprintType)
enum class ENamiBattleCameraEndBehavior : uint8
{
	/** 混合回去：平滑过渡回原状态 */
	BlendBack UMETA(DisplayName = "混合回去"),
	
	/** 立即结束：无过渡直接切回 */
	ForceEnd UMETA(DisplayName = "立即结束"),
	
	/** 保持效果：不结束，需手动停止 */
	Stay UMETA(DisplayName = "保持效果"),
};

/**
 * 战斗相机动画通知状态
 * 
 * 用于在技能动画中控制相机表现
 * 
 * 核心特点：
 * - 支持修改"输入参数"（吊臂长度、旋转等）
 * - 支持修改"输出结果"（相机位置、旋转）
 * - 支持 Additive/Override 混合模式
 * - 自动追踪修改的参数
 * 
 * 使用场景：
 * - 技能释放时的镜头拉近/拉远
 * - 大招的俯视角切换
 * - 处决动画的特写镜头
 * - 受击反馈
 */
UCLASS(meta = (DisplayName = "战斗相机"))
class NAMICAMERA_API UAnimNotifyState_BattleCamera : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UAnimNotifyState_BattleCamera();

	// ==================== 1. 枢轴点参数 ====================
	
	/** 
	 * 枢轴点位置修改
	 * 
	 * 枢轴点是相机围绕旋转的中心点，通常是角色位置
	 * 
	 * Additive 模式：在当前位置基础上偏移
	 * Override 模式：直接设置目标位置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 枢轴点",
			  meta = (DisplayName = "枢轴位置"))
	FNamiVectorModify PivotLocation;
	
	/** 
	 * 枢轴点旋转修改
	 * 
	 * 基础旋转，决定相机的基础朝向
	 * 
	 * Additive 模式：在当前旋转基础上叠加
	 * Override 模式：直接设置目标旋转
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 枢轴点",
			  meta = (DisplayName = "枢轴旋转"))
	FNamiRotatorModify PivotRotation;

	// ==================== 2. 吊臂参数 ====================
	
	/** 
	 * 吊臂长度调整
	 * 
	 * Additive 模式：正值拉远，负值拉近
	 * Override 模式：直接设置目标距离
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 吊臂",
			  meta = (DisplayName = "吊臂长度"))
	FNamiFloatModify ArmLength;
	
	/** 
	 * 吊臂旋转调整（在控制旋转基础上额外旋转）
	 * 
	 * 旋转原理：
	 * - 以角色位置为中心
	 * - 以吊臂长度为半径
	 * - 在当前控制旋转基础上额外旋转
	 * - 使用四元数组合（PivotQuat * ArmQuat）
	 * 
	 * 影响范围：
	 * - 主要影响：相机位置（让相机沿以角色为中心的圆弧移动）
	 * - 间接影响：相机朝向（相机朝向会跟随吊臂方向）
	 * 
	 * Pitch：俯仰（正值向上，形成俯视角）
	 * Yaw：偏航（正值向右绕行）
	 * 
	 * 与其他旋转参数的区别：
	 * - 吊臂旋转：调整吊臂方向，既影响位置也影响朝向（间接），不受玩家输入打断
	 * - 控制旋转偏移：调整玩家视角意图，可被玩家输入打断
	 * - 相机旋转偏移：只调整相机朝向，不影响位置，不受玩家输入影响
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 吊臂",
			  meta = (DisplayName = "吊臂旋转"))
	FNamiRotatorModify ArmRotation;
	
	/** 
	 * 吊臂末端偏移
	 * 
	 * 在吊臂本地空间中的偏移
	 * 常用于肩部视角切换（左右偏移）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 吊臂",
			  meta = (DisplayName = "吊臂偏移"))
	FNamiVectorModify ArmOffset;

	// ==================== 3. 控制旋转参数 ====================
	
	/** 
	 * 控制旋转偏移（玩家视角意图）
	 * 
	 * 临时改变玩家"想看哪里"
	 * 可被玩家输入打断（根据 ControlMode 设置）
	 * 
	 * 与"相机旋转偏移"的区别：
	 * - 控制旋转偏移：修改玩家视角意图，可被玩家输入打断
	 * - 相机旋转偏移：纯视觉效果，不受玩家输入影响
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 控制旋转",
			  meta = (DisplayName = "控制旋转偏移"))
	FNamiRotatorModify ControlRotationOffset;
	
	/** 
	 * 视角控制模式
	 * 决定玩家输入如何影响视角控制
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 控制旋转",
			  meta = (DisplayName = "控制模式",
					  EditCondition = "ControlRotationOffset.bEnabled",
					  Tooltip = "建议视角：玩家输入可打断（推荐用于技能演出）\n强制视角：忽略玩家输入（用于过场动画、QTE）\n混合视角：玩家输入衰减效果"))
	ENamiCameraControlMode ControlMode = ENamiCameraControlMode::Suggestion;
	
	/** 
	 * 输入检测阈值（死区）
	 * 输入值低于此阈值时不触发打断
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 控制旋转",
			  meta = (DisplayName = "输入阈值",
					  EditCondition = "ControlRotationOffset.bEnabled && ControlMode != ENamiCameraControlMode::Forced",
					  ClampMin = "0.01", ClampMax = "0.5",
					  Tooltip = "相机输入死区，低于此值不触发视角打断。\n输入来源优先 INamiCameraInputProvider::GetCameraInputVector()\n无接口则回退 PC 轴 Turn/LookUp/TurnRate/LookUpRate。\n推荐值：0.1（10%）。"))
	float CameraInputThreshold = 0.1f;
	
	/** 
	 * 输入打断时的混合时间
	 * 当玩家输入打断视角控制时的退出混合时间
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 控制旋转",
			  meta = (DisplayName = "打断混合时间",
					  EditCondition = "ControlRotationOffset.bEnabled && ControlMode != ENamiCameraControlMode::Forced",
					  ClampMin = "0.0", ClampMax = "2.0",
					  Tooltip = "被打断时的混合退出时间"))
	float InputInterruptBlendTime = 0.15f;

	// ==================== 3.1 整段输入打断 ====================
	
	/** 是否允许玩家输入打断整个技能镜头（整段 BlendOut） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 控制旋转",
			  meta = (DisplayName = "允许输入打断技能镜头",
					  Tooltip = "开启后，当玩家输入超过阈值时，整个技能镜头按打断混出时间退出；默认关闭以兼容旧行为"))
	bool bAllowInputInterrupt = false;
	
	/** 输入触发打断的阈值（整段打断） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 控制旋转",
			  meta = (DisplayName = "打断阈值",
					  EditCondition = "bAllowInputInterrupt",
					  ClampMin = "0.01", ClampMax = "0.5",
					  Tooltip = "玩家输入超过此值触发整段技能镜头打断；建议与视角控制阈值保持一致，默认 0.1"))
	float InterruptInputThreshold = 0.1f;
	
	/** 打断冷却时间，避免抖动反复触发 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 控制旋转",
			  meta = (DisplayName = "打断冷却时间",
					  EditCondition = "bAllowInputInterrupt",
					  ClampMin = "0.0", ClampMax = "1.0",
					  Tooltip = "触发一次打断后，冷却期内不再重复触发；默认 0.15"))
	float InterruptCooldown = 0.15f;

	// ==================== 4. 相机参数 ====================
	
	/** 
	 * 相机位置偏移（相机本地空间）
	 * 
	 * 在吊臂计算完成后，额外的相机位置偏移
	 * 用于微调相机位置，如技能特效时的相机抖动
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4. 相机",
			  meta = (DisplayName = "相机位置偏移"))
	FNamiVectorModify CameraLocationOffset;

	/** 
	 * 相机旋转偏移
	 * 
	 * 只改变相机朝向，不影响相机位置
	 * 用于看向特定方向或目标
	 * 
	 * Pitch：俯仰（正值向上看）
	 * Yaw：偏航（正值向右看）
	 * Roll：翻滚
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4. 相机",
			  meta = (DisplayName = "相机旋转偏移"))
	FNamiRotatorModify CameraRotationOffset;
	
	/** 
	 * FOV 调整
	 * 
	 * Additive 模式：正值广角，负值长焦
	 * Override 模式：直接设置目标 FOV
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4. 相机",
			  meta = (DisplayName = "视野角度"))
	FNamiFloatModify FieldOfView;

	// ==================== 5. 输出结果（直接修改） ====================
	
	/** 
	 * 最终相机位置修改（直接修改输出）
	 * 
	 * 直接设置相机的最终位置，跳过所有计算
	 * 谨慎使用，通常用于特殊效果
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. 输出结果",
			  meta = (DisplayName = "最终相机位置"))
	FNamiVectorModify CameraLocation;
	
	/** 
	 * 最终相机旋转修改（直接修改输出）
	 * 
	 * 直接设置相机的最终旋转，跳过所有计算
	 * 谨慎使用，通常用于特殊效果
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. 输出结果",
			  meta = (DisplayName = "最终相机旋转"))
	FNamiRotatorModify CameraRotation;
	
	// ==================== 5. 效果 ====================
	
	/** 相机震动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. 效果",
			  meta = (DisplayName = "相机震动"))
	TSubclassOf<UCameraShakeBase> CameraShake;
	
	/** 震动强度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. 效果",
			  meta = (DisplayName = "震动强度",
					  EditCondition = "CameraShake != nullptr",
					  ClampMin = "0.0", ClampMax = "5.0"))
	float ShakeScale = 1.0f;

	// ==================== 5.1 互斥策略 ====================
	
	/** 激活前是否强制停止已有技能镜头（State/Shake/PostProcess） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. 效果",
			  meta = (DisplayName = "激活前清理已有镜头",
					  Tooltip = "开启后，在创建本次镜头前会停止当前玩家上的所有相机效果，避免技能镜头叠加。"))
	bool bStopExistingEffects = true;

	// ==================== 6. 混合设置 ====================
	
	/** 混入时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "6. 混合",
			  meta = (DisplayName = "混入时间",
					  ClampMin = "0.0", ClampMax = "2.0"))
	float BlendInTime = 0.15f;
	
	/** 混出时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "6. 混合",
			  meta = (DisplayName = "混出时间",
					  ClampMin = "0.0", ClampMax = "2.0"))
	float BlendOutTime = 0.2f;
	
	/** 结束行为 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "6. 混合",
			  meta = (DisplayName = "结束行为"))
	ENamiBattleCameraEndBehavior EndBehavior = ENamiBattleCameraEndBehavior::BlendBack;

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
	 * 留空则使用默认值 "Camera.Feature.Source.ANS.BattleCamera"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayTags",
			  meta = (DisplayName = "来源 Tag",
					  Tooltip = "用于标识此 Feature 的来源。留空则使用默认值 Camera.Feature.Source.ANS.BattleCamera。"))
	FGameplayTag SourceTag;

	// ==================== AnimNotifyState 接口 ====================
	
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, 
							float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
						  const FAnimNotifyEventReference& EventReference) override;
	
	virtual FString GetNotifyName_Implementation() const override;

#if WITH_EDITOR
	virtual void ValidateAssociatedAssets() override;
#endif

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
	FName GenerateUniqueEffectName(const UAnimSequenceBase* Animation, const FString& Suffix) const;
	
	/** 转换结束行为枚举 */
	ENamiCameraEndBehavior ConvertEndBehavior(ENamiBattleCameraEndBehavior InBehavior) const;

private:
	/** 激活的相机调整 Feature 实例 */
	TWeakObjectPtr<UNamiCameraAdjustFeature> ActiveAdjustFeature;
	
	/** 激活的震动 Feature 实例 */
	TWeakObjectPtr<UNamiCameraShakeFeature> ActiveShakeFeature;
};

