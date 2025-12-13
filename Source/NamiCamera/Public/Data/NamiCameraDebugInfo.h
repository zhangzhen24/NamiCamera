// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/NamiCameraView.h"
#include "Data/NamiCameraState.h"
#include "NamiCameraDebugInfo.generated.h"

/**
 * 相机调试信息结构体
 * 用于收集和格式化相机系统的关键信息
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraDebugInfo
{
	GENERATED_BODY()

	// ========== 视图信息 ==========
	
	/** 相机位置 */
	UPROPERTY(BlueprintReadOnly, Category = "Camera|View")
	FVector CameraLocation = FVector::ZeroVector;

	/** 相机旋转 */
	UPROPERTY(BlueprintReadOnly, Category = "Camera|View")
	FRotator CameraRotation = FRotator::ZeroRotator;

	/** 枢轴点位置 */
	UPROPERTY(BlueprintReadOnly, Category = "Camera|View")
	FVector PivotLocation = FVector::ZeroVector;

	/** 控制旋转 */
	UPROPERTY(BlueprintReadOnly, Category = "Camera|View")
	FRotator ControlRotation = FRotator::ZeroRotator;

	/** 视野角度 */
	UPROPERTY(BlueprintReadOnly, Category = "Camera|View")
	float FieldOfView = 90.0f;

	// ========== 状态信息 ==========

	/** 吊臂长度 */
	UPROPERTY(BlueprintReadOnly, Category = "Camera|State")
	float ArmLength = 0.0f;

	/** 吊臂旋转 */
	UPROPERTY(BlueprintReadOnly, Category = "Camera|State")
	FRotator ArmRotation = FRotator::ZeroRotator;

	/** 吊臂偏移 */
	UPROPERTY(BlueprintReadOnly, Category = "Camera|State")
	FVector ArmOffset = FVector::ZeroVector;

	/** 相机位置偏移 */
	UPROPERTY(BlueprintReadOnly, Category = "Camera|State")
	FVector CameraLocationOffset = FVector::ZeroVector;

	/** 相机旋转偏移 */
	UPROPERTY(BlueprintReadOnly, Category = "Camera|State")
	FRotator CameraRotationOffset = FRotator::ZeroRotator;

	// ========== 计算信息 ==========

	/** 相机到枢轴点的距离 */
	UPROPERTY(BlueprintReadOnly, Category = "Camera|Calculated")
	float DistanceToPivot = 0.0f;

	/** 从 View 构建调试信息 */
	void FromView(const FNamiCameraView& View);

	/** 从 State 构建调试信息 */
	void FromState(const FNamiCameraState& State);

	/** 格式化输出为字符串（用于日志） */
	FString ToString() const;

	/** 格式化输出为多行字符串（用于屏幕显示） */
	FString ToMultiLineString() const;
};

