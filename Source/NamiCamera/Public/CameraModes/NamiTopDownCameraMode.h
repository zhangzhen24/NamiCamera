// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CameraModes/NamiComposableCameraMode.h"
#include "NamiTopDownCameraMode.generated.h"

class UNamiTopDownPositionCalculator;
class UNamiTopDownRotationCalculator;

/**
 * TopDown 俯视角相机模式
 *
 * 适用于俯视角游戏（ARPG、RTS、MOBA 等）
 *
 * 特点：
 * - 固定俯视角观察角色
 * - 固定观察方向
 * - 平滑跟随角色移动
 *
 * 计算器组合：
 * - TargetCalculator: UNamiSingleTargetCalculator（跟随单个目标）
 * - PositionCalculator: UNamiTopDownPositionCalculator（固定俯视角位置）
 * - RotationCalculator: UNamiTopDownRotationCalculator（向下看向目标）
 * - FOVCalculator: UNamiStaticFOVCalculator（固定 FOV）
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Top Down Camera"))
class NAMICAMERA_API UNamiTopDownCameraMode : public UNamiComposableCameraMode
{
	GENERATED_BODY()

public:
	UNamiTopDownCameraMode();

	// ========== 生命周期 ==========

	virtual void Initialize_Implementation(UNamiCameraComponent* InCameraComponent) override;

	// ========== 计算器访问 ==========

	/**
	 * 获取 TopDown 位置计算器
	 */
	UFUNCTION(BlueprintPure, Category = "Top Down Camera")
	UNamiTopDownPositionCalculator* GetTopDownPositionCalculator() const;

	/**
	 * 获取 TopDown 旋转计算器
	 */
	UFUNCTION(BlueprintPure, Category = "Top Down Camera")
	UNamiTopDownRotationCalculator* GetTopDownRotationCalculator() const;

protected:
	/**
	 * 创建默认计算器
	 */
	void CreateDefaultCalculators();
};
