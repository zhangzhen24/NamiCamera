// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Features/NamiCameraFeature.h"
#include "NamiVelocityPredictionFeature.generated.h"

class UCurveFloat;

/**
 * 速度预测 Feature
 * 
 * 根据目标移动速度动态调整相机观察位置，实现类似《逃离塔科夫》的相机效果
 * 
 * 工作原理：
 * - 在 Update() 中获取目标速度
 * - 在 ApplyToView() 中修改 PivotLocation，使其提前观察目标移动方向
 * - 快速移动时，相机提前观察；静止时，相机回到目标正上方
 * 
 * 适用场景：
 * - 俯视角相机
 * - 第三人称相机
 * - 任何需要速度预测的跟随相机
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class NAMICAMERA_API UNamiVelocityPredictionFeature : public UNamiCameraFeature
{
	GENERATED_BODY()

public:
	UNamiVelocityPredictionFeature();

	// ========== Feature 生命周期 ==========
	virtual void Update_Implementation(float DeltaTime) override;
	virtual void ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime) override;

	// ========== 配置 ==========

	/** 是否启用速度预测 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Prediction",
		meta = (DisplayName = "启用速度预测"))
	bool bEnablePrediction = true;

	/** 预测时间（秒）- 相机提前观察的时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Prediction",
		meta = (DisplayName = "预测时间", ClampMin = "0.0", ClampMax = "2.0",
		Tooltip = "相机提前观察的时间（秒）。值越大，相机越提前观察目标移动方向。"))
	float PredictionTime = 0.3f;

	/** 最小速度阈值（cm/s）- 低于此速度不进行预测 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Prediction",
		meta = (DisplayName = "最小速度阈值", ClampMin = "0.0",
		Tooltip = "目标速度低于此值时，不进行预测。避免静止时相机抖动。"))
	float MinVelocityThreshold = 50.0f;

	/** 最大速度阈值（cm/s）- 达到此速度时预测强度最大 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Prediction",
		meta = (DisplayName = "最大速度阈值", ClampMin = "0.0",
		Tooltip = "目标速度达到此值时，预测强度达到最大。"))
	float MaxVelocityThreshold = 600.0f;

	/** 预测强度曲线 - 根据速度大小调整预测强度（X轴：速度归一化，Y轴：预测强度0-1） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Prediction",
		meta = (DisplayName = "预测强度曲线",
		Tooltip = "根据速度大小调整预测强度。X轴：速度归一化（0-1），Y轴：预测强度（0-1）。"))
	TObjectPtr<UCurveFloat> PredictionStrengthCurve = nullptr;

	/** 是否只使用水平速度（忽略垂直速度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Prediction",
		meta = (DisplayName = "只用水平速度",
		Tooltip = "只使用水平速度（X, Y），忽略垂直速度（Z）。适合俯视角相机。"))
	bool bUseHorizontalVelocityOnly = true;

	/** 是否保持相机 Yaw 不变（只更新 Pitch） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Prediction",
		meta = (DisplayName = "保持水平旋转",
		Tooltip = "如果启用，相机在速度预测时保持 Yaw（水平旋转）不变，只更新 Pitch（俯仰角）。适合俯视角相机，避免相机水平旋转。"))
	bool bPreserveYawRotation = true;

protected:
	/** 当前目标速度（每帧更新） */
	FVector CurrentVelocity = FVector::ZeroVector;

	/** 当前速度大小 */
	float CurrentSpeed = 0.0f;

	/** 获取目标速度 */
	FVector GetTargetVelocity() const;
};

