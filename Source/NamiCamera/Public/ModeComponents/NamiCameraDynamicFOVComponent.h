// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModeComponents/NamiCameraModeComponent.h"
#include "NamiCameraDynamicFOVComponent.generated.h"

/**
 * 动态 FOV 组件
 *
 * 根据目标速度动态调整 FOV：
 * - 高速移动时增加 FOV（速度感）
 * - 静止时恢复基础 FOV
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Dynamic FOV Component"))
class NAMICAMERA_API UNamiCameraDynamicFOVComponent : public UNamiCameraModeComponent
{
	GENERATED_BODY()

public:
	UNamiCameraDynamicFOVComponent();

	// ========== UNamiCameraModeComponent ==========
	virtual void Activate_Implementation() override;
	virtual void ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime) override;

public:
	// ========== 配置 ==========

	/**
	 * 基础 FOV
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic FOV", meta = (ClampMin = "30.0", ClampMax = "120.0"))
	float BaseFOV = 90.0f;

	/**
	 * 最小 FOV
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic FOV", meta = (ClampMin = "30.0", ClampMax = "120.0"))
	float MinDynamicFOV = 60.0f;

	/**
	 * 最大 FOV
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic FOV", meta = (ClampMin = "30.0", ClampMax = "120.0"))
	float MaxDynamicFOV = 100.0f;

	/**
	 * 速度对 FOV 的影响因子
	 * FOV = BaseFOV + Speed * SpeedFOVFactor
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic FOV", meta = (ClampMin = "0.0"))
	float SpeedFOVFactor = 0.01f;

	/**
	 * FOV 变化速度
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic FOV", meta = (ClampMin = "0.0"))
	float DynamicFOVChangeRate = 5.0f;

protected:
	/** 当前动态 FOV */
	UPROPERTY(BlueprintReadOnly, Category = "Dynamic FOV")
	float CurrentDynamicFOV;

	/**
	 * 获取速度来源 Actor
	 * 默认从相机模式获取主要目标
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Dynamic FOV")
	AActor* GetSpeedSourceActor() const;
};
