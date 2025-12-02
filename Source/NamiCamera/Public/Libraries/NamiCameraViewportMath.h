// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NamiCameraViewportMath.generated.h"

/**
 * 视口约束结果
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiViewportConstraintResult
{
	GENERATED_BODY()

	/** 计算是否成功 */
	UPROPERTY(BlueprintReadOnly, Category = "Viewport Constraint")
	bool bSuccess = false;

	/** 计算出的相机位置 */
	UPROPERTY(BlueprintReadOnly, Category = "Viewport Constraint")
	FVector CameraLocation = FVector::ZeroVector;

	/** 计算出的相机旋转 */
	UPROPERTY(BlueprintReadOnly, Category = "Viewport Constraint")
	FRotator CameraRotation = FRotator::ZeroRotator;

	/** 计算出的FOV */
	UPROPERTY(BlueprintReadOnly, Category = "Viewport Constraint")
	float FOV = 90.0f;

	/** 约束满足程度 (0-1)，越高越好 */
	UPROPERTY(BlueprintReadOnly, Category = "Viewport Constraint")
	float ConstraintSatisfaction = 0.0f;
};

/**
 * 视口目标约束
 * 定义一个目标在视口中的期望位置
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiViewportTargetConstraint
{
	GENERATED_BODY()

	/** 目标Actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Constraint",
		meta = (DisplayName = "目标Actor"))
	TWeakObjectPtr<AActor> Target;

	/** 目标世界位置（如果Target为空则使用此位置） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Constraint",
		meta = (DisplayName = "目标位置"))
	FVector TargetLocation = FVector::ZeroVector;

	/** 
	 * 期望的屏幕位置（归一化 0-1）
	 * (0,0) = 左上角, (1,1) = 右下角
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Constraint",
		meta = (DisplayName = "期望屏幕位置", ClampMin = "0.0", ClampMax = "1.0"))
	FVector2D DesiredScreenPosition = FVector2D(0.5f, 0.5f);

	/** 权重（用于多目标约束求解） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Constraint",
		meta = (DisplayName = "权重", ClampMin = "0.0", ClampMax = "1.0"))
	float Weight = 1.0f;

	/** 是否是硬约束（必须满足） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Constraint",
		meta = (DisplayName = "硬约束"))
	bool bHardConstraint = false;

	/** 获取目标世界位置 */
	FVector GetWorldLocation() const
	{
		if (Target.IsValid())
		{
			return Target->GetActorLocation();
		}
		return TargetLocation;
	}
};

/**
 * 视口边界约束
 * 定义目标在视口中的允许范围
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiViewportBoundsConstraint
{
	GENERATED_BODY()

	/** 目标Actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounds Constraint",
		meta = (DisplayName = "目标Actor"))
	TWeakObjectPtr<AActor> Target;

	/** 最小屏幕位置（归一化） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounds Constraint",
		meta = (DisplayName = "最小位置", ClampMin = "0.0", ClampMax = "1.0"))
	FVector2D MinScreenPosition = FVector2D(0.1f, 0.1f);

	/** 最大屏幕位置（归一化） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounds Constraint",
		meta = (DisplayName = "最大位置", ClampMin = "0.0", ClampMax = "1.0"))
	FVector2D MaxScreenPosition = FVector2D(0.9f, 0.9f);

	/** 是否必须在视口内 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounds Constraint",
		meta = (DisplayName = "必须可见"))
	bool bMustBeVisible = true;

	/** 获取目标世界位置 */
	FVector GetWorldLocation() const
	{
		if (Target.IsValid())
		{
			return Target->GetActorLocation();
		}
		return FVector::ZeroVector;
	}
};

/**
 * 相机视口数学库
 * 提供视口相关的数学计算函数，包括正向投影和反向推理
 */
UCLASS()
class NAMICAMERA_API UNamiCameraViewportMath : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ==================== 坐标转换 ====================

	/**
	 * 世界坐标转屏幕坐标（归一化）
	 * @param WorldLocation 世界坐标
	 * @param CameraLocation 相机位置
	 * @param CameraRotation 相机旋转
	 * @param FOV 视野角度
	 * @param AspectRatio 宽高比
	 * @param OutScreenPosition 输出的屏幕位置（归一化 0-1）
	 * @return 是否在屏幕内
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Viewport Math")
	static bool WorldToScreenNormalized(
		const FVector& WorldLocation,
		const FVector& CameraLocation,
		const FRotator& CameraRotation,
		float FOV,
		float AspectRatio,
		FVector2D& OutScreenPosition);

	/**
	 * 屏幕坐标转世界射线
	 * @param ScreenPosition 屏幕位置（归一化 0-1）
	 * @param CameraLocation 相机位置
	 * @param CameraRotation 相机旋转
	 * @param FOV 视野角度
	 * @param AspectRatio 宽高比
	 * @param OutRayOrigin 输出的射线起点
	 * @param OutRayDirection 输出的射线方向
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Viewport Math")
	static void ScreenToWorldRay(
		const FVector2D& ScreenPosition,
		const FVector& CameraLocation,
		const FRotator& CameraRotation,
		float FOV,
		float AspectRatio,
		FVector& OutRayOrigin,
		FVector& OutRayDirection);

	// ==================== 反向推理：计算相机位置 ====================

	/**
	 * 根据单个目标的期望屏幕位置计算相机位置
	 * 
	 * @param TargetWorldLocation 目标世界位置
	 * @param DesiredScreenPosition 期望的屏幕位置（归一化）
	 * @param CameraDistance 相机到目标的距离
	 * @param CameraHeight 相机相对于目标的高度偏移
	 * @param FOV 视野角度
	 * @param AspectRatio 宽高比
	 * @param BaseDirection 基础方向（相机朝向的参考方向）
	 * @param OutCameraLocation 输出的相机位置
	 * @param OutCameraRotation 输出的相机旋转
	 * @return 是否计算成功
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Viewport Math|Inverse")
	static bool CalculateCameraForSingleTarget(
		const FVector& TargetWorldLocation,
		const FVector2D& DesiredScreenPosition,
		float CameraDistance,
		float CameraHeight,
		float FOV,
		float AspectRatio,
		const FVector& BaseDirection,
		FVector& OutCameraLocation,
		FRotator& OutCameraRotation);

	/**
	 * 根据两个目标的期望屏幕位置计算相机位置（战斗相机核心算法）
	 * 
	 * @param Target1Location 目标1世界位置（通常是玩家）
	 * @param Target1ScreenPosition 目标1期望屏幕位置（如左下 0.25, 0.65）
	 * @param Target2Location 目标2世界位置（通常是敌人）
	 * @param Target2ScreenPosition 目标2期望屏幕位置（如右上 0.75, 0.35）
	 * @param CameraHeight 相机高度偏移
	 * @param FOVRange FOV范围（用于调整）
	 * @param AspectRatio 宽高比
	 * @return 约束结果
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Viewport Math|Inverse")
	static FNamiViewportConstraintResult CalculateCameraForTwoTargets(
		const FVector& Target1Location,
		const FVector2D& Target1ScreenPosition,
		const FVector& Target2Location,
		const FVector2D& Target2ScreenPosition,
		float CameraHeight,
		FVector2D FOVRange,
		float AspectRatio);

	/**
	 * 根据多个目标约束计算最优相机位置（迭代优化）
	 * 
	 * @param Constraints 目标约束列表
	 * @param InitialCameraLocation 初始相机位置（优化起点）
	 * @param InitialFOV 初始FOV
	 * @param CameraHeightRange 相机高度范围
	 * @param FOVRange FOV范围
	 * @param AspectRatio 宽高比
	 * @param MaxIterations 最大迭代次数
	 * @return 约束结果
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Viewport Math|Inverse")
	static FNamiViewportConstraintResult CalculateCameraForMultipleTargets(
		const TArray<FNamiViewportTargetConstraint>& Constraints,
		const FVector& InitialCameraLocation,
		float InitialFOV,
		FVector2D CameraHeightRange,
		FVector2D FOVRange,
		float AspectRatio,
		int32 MaxIterations = 50);

	// ==================== 视口边界约束 ====================

	/**
	 * 检查目标是否在视口边界内
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Viewport Math|Bounds")
	static bool IsTargetWithinBounds(
		const FVector& TargetLocation,
		const FVector& CameraLocation,
		const FRotator& CameraRotation,
		float FOV,
		float AspectRatio,
		const FNamiViewportBoundsConstraint& BoundsConstraint);

	/**
	 * 计算让所有目标都在指定边界内的最小FOV
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Viewport Math|Bounds")
	static float CalculateMinFOVForTargets(
		const TArray<FVector>& Targets,
		const FVector& CameraLocation,
		const FRotator& CameraRotation,
		FVector2D BoundsMin,
		FVector2D BoundsMax,
		float AspectRatio,
		FVector2D FOVRange);

	/**
	 * 计算包含所有目标的理想相机距离
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Viewport Math|Bounds")
	static float CalculateIdealCameraDistance(
		const TArray<FVector>& Targets,
		const FRotator& CameraRotation,
		float FOV,
		float AspectRatio,
		FVector2D ScreenPadding,
		FVector2D DistanceRange);

	// ==================== 辅助函数 ====================

	/**
	 * 计算两个目标在屏幕上的距离
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Viewport Math|Helpers")
	static float CalculateScreenDistance(
		const FVector& Location1,
		const FVector& Location2,
		const FVector& CameraLocation,
		const FRotator& CameraRotation,
		float FOV,
		float AspectRatio);

	/**
	 * 计算目标列表的屏幕边界框
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Viewport Math|Helpers")
	static FBox2D CalculateScreenBoundingBox(
		const TArray<FVector>& Targets,
		const FVector& CameraLocation,
		const FRotator& CameraRotation,
		float FOV,
		float AspectRatio);

	/**
	 * 计算目标列表的世界中心点
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Viewport Math|Helpers")
	static FVector CalculateTargetsCenter(const TArray<FVector>& Targets);

	/**
	 * 计算加权中心点
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Viewport Math|Helpers")
	static FVector CalculateWeightedCenter(const TArray<FNamiViewportTargetConstraint>& Constraints);

	/**
	 * 评估当前相机位置的构图质量
	 * @return 质量分数 (0-1)，越高越好
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Viewport Math|Helpers")
	static float EvaluateFramingQuality(
		const TArray<FNamiViewportTargetConstraint>& Constraints,
		const FVector& CameraLocation,
		const FRotator& CameraRotation,
		float FOV,
		float AspectRatio);

	/**
	 * 计算相机注视方向（从相机位置看向目标中心）
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Viewport Math|Helpers")
	static FRotator CalculateLookAtRotation(
		const FVector& CameraLocation,
		const FVector& TargetLocation);

	/**
	 * 获取默认宽高比
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Viewport Math|Helpers")
	static float GetDefaultAspectRatio();
};

