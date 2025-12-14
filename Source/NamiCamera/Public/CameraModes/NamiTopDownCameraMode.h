// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CameraModes/NamiSpringArmCameraMode.h"
#include "NamiTopDownCameraMode.generated.h"

/**
 * 俯视角相机模式
 *
 * 特点：
 * - 固定的俯视角度，适合 MOBA、RTS、ARPG 等游戏类型
 * - 锁定 Pitch 和 Roll，只允许 Yaw 旋转
 * - 可选的鼠标位置追踪功能
 * - 禁用弹簧臂碰撞检测，保持稳定的相机距离
 */
UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="顶视角相机模式"))
class NAMICAMERA_API UNamiTopDownCameraMode : public UNamiSpringArmCameraMode
{
	GENERATED_BODY()

public:
	UNamiTopDownCameraMode();

	//~ Begin UNamiCameraModeBase Interface
	virtual FNamiCameraView CalculateView_Implementation(float DeltaTime) override;
	//~ End UNamiCameraModeBase Interface

protected:
	/** 相机俯仰角度（负值表示向下看） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown",
		meta=(ClampMin="-89.0", ClampMax="0.0", UIMin="-89.0", UIMax="0.0",
		Tooltip="相机俯视角度，范围 [-89, 0]，推荐值: -55"))
	float CameraPitch;

	/** 相机偏航角度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown",
		meta=(ClampMin="-180.0", ClampMax="180.0", UIMin="-180.0", UIMax="180.0",
		Tooltip="相机水平旋转角度"))
	float CameraYaw;

	/** 相机横滚角度（通常保持为 0） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown",
		meta=(ClampMin="-180.0", ClampMax="180.0", UIMin="-180.0", UIMax="180.0",
		Tooltip="相机倾斜角度，通常保持为 0"))
	float CameraRoll;

	/** 是否忽略 Pitch 输入（锁定俯仰角） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown",
		meta=(Tooltip="是否锁定俯仰角，防止玩家改变相机上下视角"))
	bool bIgnorePitch;

	/** 是否忽略 Roll 输入（锁定横滚角） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown",
		meta=(Tooltip="是否锁定横滚角，保持相机水平"))
	bool bIgnoreRoll;

	/** 是否启用鼠标位置追踪 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Advanced",
		meta=(Tooltip="是否根据鼠标位置微调相机焦点"))
	bool bEnableMouseTracking;

	/** 鼠标追踪影响强度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Advanced",
		meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0",
		EditCondition="bEnableMouseTracking",
		Tooltip="鼠标位置对相机焦点的影响程度"))
	float MouseTrackingStrength;

	/** 鼠标追踪最大偏移距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Advanced",
		meta=(ClampMin="0.0", UIMin="0.0",
		EditCondition="bEnableMouseTracking",
		Tooltip="鼠标追踪允许的最大偏移距离"))
	float MaxMouseTrackingOffset;

protected:
	/** 追踪鼠标位置（如果启用） */
	FVector TraceMousePosition(const FVector& BasePivot) const;
};
