// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Calculators/NamiCameraPositionCalculator.h"
#include "NamiTopDownPositionCalculator.generated.h"

/**
 * TopDown 俯视角位置计算器
 *
 * 计算固定俯视角的相机位置，特点：
 * - 固定高度和俯视角度
 * - 固定观察方向
 * - 平滑跟随角色移动
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Top Down Position"))
class NAMICAMERA_API UNamiTopDownPositionCalculator : public UNamiCameraPositionCalculator
{
	GENERATED_BODY()

public:
	UNamiTopDownPositionCalculator();

	// ========== 生命周期重写 ==========

	virtual void Activate_Implementation() override;

	// ========== 核心接口重写 ==========

	/**
	 * 计算 TopDown 相机位置
	 * @param PivotLocation 枢轴点位置（角色位置）
	 * @param ControlRotation 控制旋转（TopDown 模式下不使用）
	 * @param DeltaTime 帧时间
	 * @return 计算后的相机位置
	 */
	virtual FVector CalculateCameraPosition_Implementation(
		const FVector& PivotLocation,
		const FRotator& ControlRotation,
		float DeltaTime) override;

	// ========== 辅助方法 ==========

	/**
	 * 获取主要目标 Actor
	 */
	UFUNCTION(BlueprintPure, Category = "Top Down Position")
	AActor* GetPrimaryTarget() const;

public:
	// ========== 相机配置 ==========

	/**
	 * 相机高度（单位：厘米）
	 * 值越大，相机越高，视野越广
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down",
		meta = (ClampMin = "100.0", UIMin = "500.0", UIMax = "5000.0"))
	float CameraHeight = 1500.0f;

	/**
	 * 俯视角度（相对于水平面，单位：度）
	 * 0° = 水平看，90° = 垂直向下看
	 * 推荐范围：30° - 60°
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down",
		meta = (ClampMin = "1.0", ClampMax = "89.0", UIMin = "30.0", UIMax = "60.0"))
	float ViewAngle = 45.0f;

	/**
	 * 观察方向 Yaw 角度（单位：度）
	 * 0° = 正北，90° = 正东，180° = 正南，270° = 正西
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down",
		meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float ViewDirectionYaw = 45.0f;

	// ========== 跟随平滑配置 ==========

	/**
	 * 跟随平滑速度
	 * 0 = 瞬间跟随，值越大跟随越快
	 * 推荐值：5 - 10
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down|Smoothing",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "20.0"))
	float FollowSmoothSpeed = 8.0f;

protected:
	/**
	 * 计算基础偏移向量（未旋转）
	 * 根据高度和俯视角计算相对于 Pivot 的偏移
	 */
	FVector CalculateBaseOffset() const;
};
