// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Adjusts/NamiCameraAdjustParams.h"
#include "Enums/NamiCameraEnums.h"
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

	/** 是否启用FOV调整 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. FOV")
	bool bEnableFOV = false;

	/** FOV偏移量（Additive模式）。正值广角，负值长焦。例如：+10 = 更广的视野，-10 = 更窄的视野 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. FOV",
		meta = (EditCondition = "bEnableFOV", ClampMin = "-60", ClampMax = "60",
			Tooltip = "FOV偏移量。正值广角，负值长焦。\n例如：+10 = 更广的视野，-10 = 更窄的视野"))
	float FOVOffset = 0.f;

	// ==================== 相机臂长度 ====================

	/** 是否启用臂长调整 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 相机臂")
	bool bEnableArmLength = false;

	/** 臂长偏移量。正值拉远，负值拉近。例如：+100 = 相机拉远100单位 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 相机臂",
		meta = (EditCondition = "bEnableArmLength", ClampMin = "-500", ClampMax = "500",
			Tooltip = "臂长偏移量。正值拉远，负值拉近。"))
	float ArmLengthOffset = 0.f;

	// ==================== 相机臂旋转 ====================

	/** 是否启用臂旋转调整 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 相机臂")
	bool bEnableArmRotation = false;

	/**
	 * 臂旋转值。语义取决于混合模式：
	 * - Additive模式：相对于当前臂旋转的偏移
	 * - Override模式：相对于角色朝向的目标位置（激活时刻锁定）
	 * Pitch=俯仰(正值向上)，Yaw=偏航(正值向右)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 相机臂",
		meta = (EditCondition = "bEnableArmRotation",
			Tooltip = "臂旋转值。\nAdditive模式：相对于当前臂旋转的偏移\nOverride模式：相对于角色朝向的目标位置\n\nPitch = 俯仰（正值向上看）\nYaw = 偏航（正值向右看）"))
	FRotator ArmRotationOffset = FRotator::ZeroRotator;

	// ==================== 相机位置偏移 ====================

	/** 是否启用相机位置偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 相机位置")
	bool bEnableCameraOffset = false;

	/** 相机位置偏移（相机本地空间）。X=前后，Y=左右，Z=上下 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 相机位置",
		meta = (EditCondition = "bEnableCameraOffset",
			Tooltip = "相机位置偏移（相机本地空间）。\nX = 前后（正值向前）\nY = 左右（正值向右）\nZ = 上下（正值向上）"))
	FVector CameraLocationOffset = FVector::ZeroVector;

	// ==================== 相机旋转偏移 ====================

	/** 是否启用相机旋转偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 相机位置")
	bool bEnableCameraRotation = false;

	/** 相机旋转偏移。只影响朝向，不影响位置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. 相机位置",
		meta = (EditCondition = "bEnableCameraRotation",
			Tooltip = "相机旋转偏移。只影响相机朝向，不影响相机位置。"))
	FRotator CameraRotationOffset = FRotator::ZeroRotator;

	// ==================== Pivot 偏移 ====================

	/** 是否启用Pivot偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4. Pivot")
	bool bEnablePivotOffset = false;

	/** Pivot位置偏移（世界空间）。调整相机观察的中心点 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4. Pivot",
		meta = (EditCondition = "bEnablePivotOffset",
			Tooltip = "Pivot位置偏移（世界空间）。调整相机观察的中心点。"))
	FVector PivotOffset = FVector::ZeroVector;

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

	/** 混合模式 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. 混合",
		meta = (Tooltip = "Additive = 在当前值基础上叠加\nOverride = 覆盖到目标值\nMultiplicative = 乘法缩放"))
	ENamiCameraAdjustBlendMode BlendMode = ENamiCameraAdjustBlendMode::Additive;

	/** 优先级。数值越高越后处理，可覆盖低优先级的调整 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "5. 混合",
		meta = (Tooltip = "数值越高优先级越高，可覆盖低优先级的调整"))
	int32 Priority = 100;

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
