// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "CameraAdjust/NamiCameraAdjustParams.h"
#include "AnimNotifyState_CameraAdjust.generated.h"

class UNamiCameraAdjust;
class UNamiCameraComponent;

/**
 * 相机调整动画通知状态
 *
 * 用于在技能动画中调整相机表现，支持：
 * - FOV 调整（广角/长焦效果）
 * - 相机臂长度调整（拉近/拉远）
 * - 相机位置偏移（震动效果、肩部视角）
 * - 相机旋转偏移
 * - 平滑的 BlendIn/BlendOut 过渡
 *
 * 使用方式：
 * 1. 在动画序列中添加此通知状态
 * 2. 配置需要调整的相机参数
 * 3. 设置混合时间
 */
UCLASS(DisplayName = "Camera Adjust", meta = (Tooltip = "相机调整通知。用于在技能动画中调整相机FOV、距离、位置等参数。"))
class NAMICAMERA_API UAnimNotifyState_CameraAdjust : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UAnimNotifyState_CameraAdjust();

	// ==================== FOV 调整 ====================

	/**
	 * FOV 调整参数
	 * 正值广角，负值长焦。例如：+10 = 更广的视野，-10 = 更窄的视野
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. FOV",
		meta = (Tooltip = "FOV偏移量。正值广角，负值长焦。"))
	FNamiCameraFloatParam FOV;

	// ==================== 相机臂 ====================

	/**
	 * 相机臂长度调整
	 * 正值拉远，负值拉近。例如：+100 = 相机拉远100单位
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 相机臂",
		meta = (Tooltip = "臂长偏移量。正值拉远，负值拉近。"))
	FNamiCameraFloatParam ArmLength;

	/**
	 * 相机臂旋转调整
	 * Additive 模式：相对于当前臂旋转的偏移
	 * Override 模式：相对于角色 Mesh 朝向的目标位置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 相机臂",
		meta = (Tooltip = "臂旋转调整。\nAdditive: 相对于当前臂旋转偏移\nOverride: 相对于角色Mesh朝向的目标位置"))
	FNamiCameraArmRotationParam ArmRotation;

	// ==================== 相机位置 ====================

	/**
	 * 相机位置偏移（相机本地空间）
	 * X=前后，Y=左右，Z=上下
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 相机位置",
		meta = (Tooltip = "相机位置偏移（相机本地空间）。\nX=前后, Y=左右, Z=上下"))
	FNamiCameraVectorParam CameraOffset;

	/**
	 * 相机旋转偏移
	 * 只影响相机朝向，不影响位置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 相机位置",
		meta = (Tooltip = "相机旋转偏移。只影响相机朝向，不影响位置。"))
	FNamiCameraRotatorParam CameraRotation;

	// ==================== Pivot ====================

	/**
	 * Pivot 位置偏移（世界空间）
	 * 调整相机观察的中心点
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4. Pivot",
		meta = (Tooltip = "Pivot位置偏移（世界空间）。调整相机观察的中心点。"))
	FNamiCameraVectorParam PivotOffset;

	// ==================== 混合设置 ====================

	/** 混入时间（秒）。效果从0到1的过渡时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. 混合",
		meta = (ClampMin = "0.0", ClampMax = "2.0",
			Tooltip = "效果从0到1的过渡时间（秒）"))
	float BlendInTime = 0.15f;

	/** 混出时间（秒）。效果从1到0的过渡时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. 混合",
		meta = (ClampMin = "0.0", ClampMax = "2.0",
			Tooltip = "效果从1到0的过渡时间（秒）"))
	float BlendOutTime = 0.2f;

	/** 混合曲线类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. 混合")
	ENamiCameraBlendType BlendType = ENamiCameraBlendType::EaseInOut;

	/** 优先级。数值越高越后处理，可覆盖低优先级的调整 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. 混合",
		meta = (Tooltip = "数值越高优先级越高，可覆盖低优先级的调整"))
	int32 Priority = 100;

	// ==================== 输入控制 ====================

	/**
	 * 是否允许玩家在混合过程中控制相机臂旋转
	 * true: 玩家输入直接控制相机臂，Adjust 不参与 ArmRotation 混合
	 * false: Adjust 控制相机臂，但会检测玩家输入并触发打断
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "6. 输入控制",
		meta = (Tooltip = "是否允许玩家在混合过程中控制相机臂旋转。\ntrue: 玩家自由控制\nfalse: Adjust控制，玩家输入会触发打断"))
	bool bAllowPlayerInput = false;

	/**
	 * 输入打断阈值（鼠标移动超过此值视为有输入）
	 * 仅在 bAllowPlayerInput=false 时生效
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "6. 输入控制",
		meta = (EditCondition = "!bAllowPlayerInput", ClampMin = "0.1",
			Tooltip = "输入打断阈值。鼠标移动超过此值视为玩家有输入。"))
	float InputInterruptThreshold = 1.0f;

	// ==================== AnimNotifyState 接口 ====================

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

#if WITH_EDITOR
	virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override { return true; }
#endif

protected:
	/** 获取相机组件 */
	UNamiCameraComponent* GetCameraComponent(USkeletalMeshComponent* MeshComp) const;

	/** 构建调整参数 */
	FNamiCameraAdjustParams BuildAdjustParams() const;

private:
	/** 激活的调整器实例 */
	UPROPERTY()
	TWeakObjectPtr<UNamiCameraAdjust> ActiveAdjust;

	/** 缓存的相机组件 */
	TWeakObjectPtr<UNamiCameraComponent> CachedCameraComponent;
};
