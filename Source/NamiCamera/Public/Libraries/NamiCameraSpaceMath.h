// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NamiCameraSpaceMath.generated.h"

struct FNamiCameraState;

/**
 * 相机空间类型
 */
UENUM(BlueprintType)
enum class ENamiCameraSpace : uint8
{
	/** 世界空间 */
	World UMETA(DisplayName = "世界"),
	
	/** 相机本地空间 */
	Camera UMETA(DisplayName = "相机"),
	
	/** 枢轴点空间（跟随目标空间） */
	Pivot UMETA(DisplayName = "枢轴点"),
	
	/** 角色空间 */
	Pawn UMETA(DisplayName = "角色"),
};

/**
 * 相机空间变换数学工具库
 * 
 * 提供在不同坐标空间中进行位置和旋转计算的工具方法
 */
UCLASS()
class NAMICAMERA_API UNamiCameraSpaceMath : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ==================== 空间变换 ====================
	
	/**
	 * 获取指定空间的变换矩阵
	 * 
	 * @param State 相机状态
	 * @param Space 目标空间
	 * @param PawnTransform 角色变换（仅当 Space == Pawn 时使用）
	 * @param OutTransform 输出的变换矩阵
	 * @return 是否成功获取
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Math")
	static bool GetSpaceTransform(
		const FNamiCameraState& State,
		ENamiCameraSpace Space,
		const FTransform& PawnTransform,
		FTransform& OutTransform);

	/**
	 * 在指定空间中应用位置偏移
	 * 
	 * @param State 相机状态
	 * @param WorldPosition 世界位置
	 * @param Offset 偏移量
	 * @param Space 偏移空间
	 * @param PawnTransform 角色变换（仅当 Space == Pawn 时使用）
	 * @return 偏移后的世界位置
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Math")
	static FVector OffsetPositionInSpace(
		const FNamiCameraState& State,
		const FVector& WorldPosition,
		const FVector& Offset,
		ENamiCameraSpace Space,
		const FTransform& PawnTransform);

	/**
	 * 在指定空间中应用旋转偏移
	 * 
	 * @param State 相机状态
	 * @param WorldRotation 世界旋转
	 * @param Offset 旋转偏移
	 * @param Space 偏移空间
	 * @param PawnTransform 角色变换（仅当 Space == Pawn 时使用）
	 * @return 偏移后的世界旋转
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Math")
	static FRotator OffsetRotationInSpace(
		const FNamiCameraState& State,
		const FRotator& WorldRotation,
		const FRotator& Offset,
		ENamiCameraSpace Space,
		const FTransform& PawnTransform);

	/**
	 * 将世界空间位置转换到指定空间
	 * 
	 * @param State 相机状态
	 * @param WorldPosition 世界位置
	 * @param Space 目标空间
	 * @param PawnTransform 角色变换（仅当 Space == Pawn 时使用）
	 * @return 在目标空间中的位置
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Math")
	static FVector WorldToSpace(
		const FNamiCameraState& State,
		const FVector& WorldPosition,
		ENamiCameraSpace Space,
		const FTransform& PawnTransform);

	/**
	 * 将指定空间中的位置转换到世界空间
	 * 
	 * @param State 相机状态
	 * @param LocalPosition 本地位置
	 * @param Space 本地空间
	 * @param PawnTransform 角色变换（仅当 Space == Pawn 时使用）
	 * @return 世界位置
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Math")
	static FVector SpaceToWorld(
		const FNamiCameraState& State,
		const FVector& LocalPosition,
		ENamiCameraSpace Space,
		const FTransform& PawnTransform);

	// ==================== 吊臂计算 ====================
	
	/**
	 * 计算吊臂末端位置
	 * 
	 * @param PivotLocation 枢轴点位置
	 * @param PivotRotation 枢轴点旋转
	 * @param ArmLength 吊臂长度
	 * @param ArmRotation 吊臂旋转偏移
	 * @param ArmOffset 吊臂末端偏移
	 * @return 吊臂末端的世界位置（相机位置）
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Math")
	static FVector CalculateArmEndLocation(
		const FVector& PivotLocation,
		const FRotator& PivotRotation,
		float ArmLength,
		const FRotator& ArmRotation = FRotator::ZeroRotator,
		const FVector& ArmOffset = FVector::ZeroVector);

	/**
	 * 计算吊臂方向
	 * 
	 * @param PivotRotation 枢轴点旋转
	 * @param ArmRotation 吊臂旋转偏移
	 * @return 吊臂方向（指向相机的方向）
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Math")
	static FVector CalculateArmDirection(
		const FRotator& PivotRotation,
		const FRotator& ArmRotation = FRotator::ZeroRotator);

	// ==================== 辅助方法 ====================
	
	/**
	 * 计算两点之间的吊臂参数
	 * 
	 * @param CameraLocation 相机位置
	 * @param PivotLocation 枢轴点位置
	 * @param OutArmLength 输出吊臂长度
	 * @param OutArmRotation 输出吊臂旋转
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Math")
	static void CalculateArmParameters(
		const FVector& CameraLocation,
		const FVector& PivotLocation,
		float& OutArmLength,
		FRotator& OutArmRotation);
};

