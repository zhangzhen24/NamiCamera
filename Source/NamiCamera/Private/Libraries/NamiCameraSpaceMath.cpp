// Copyright Epic Games, Inc. All Rights Reserved.

#include "Libraries/NamiCameraSpaceMath.h"
#include "Structs/State/NamiCameraState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraSpaceMath)

bool UNamiCameraSpaceMath::GetSpaceTransform(
	const FNamiCameraState& State,
	ENamiCameraSpace Space,
	const FTransform& PawnTransform,
	FTransform& OutTransform)
{
	switch (Space)
	{
	case ENamiCameraSpace::World:
		OutTransform = FTransform::Identity;
		return true;
		
	case ENamiCameraSpace::Camera:
		OutTransform = FTransform(State.CameraRotation, State.CameraLocation);
		return true;
		
	case ENamiCameraSpace::Pivot:
		OutTransform = FTransform(State.PivotRotation, State.PivotLocation);
		return true;
		
	case ENamiCameraSpace::Pawn:
		OutTransform = PawnTransform;
		return true;
		
	default:
		OutTransform = FTransform::Identity;
		return false;
	}
}

FVector UNamiCameraSpaceMath::OffsetPositionInSpace(
	const FNamiCameraState& State,
	const FVector& WorldPosition,
	const FVector& Offset,
	ENamiCameraSpace Space,
	const FTransform& PawnTransform)
{
	if (Offset.IsNearlyZero())
	{
		return WorldPosition;
	}
	
	FTransform SpaceTransform;
	if (!GetSpaceTransform(State, Space, PawnTransform, SpaceTransform))
	{
		return WorldPosition;
	}
	
	// 将偏移转换到世界空间
	FVector WorldOffset;
	if (Space == ENamiCameraSpace::World)
	{
		WorldOffset = Offset;
	}
	else
	{
		WorldOffset = SpaceTransform.TransformVectorNoScale(Offset);
	}
	
	return WorldPosition + WorldOffset;
}

FRotator UNamiCameraSpaceMath::OffsetRotationInSpace(
	const FNamiCameraState& State,
	const FRotator& WorldRotation,
	const FRotator& Offset,
	ENamiCameraSpace Space,
	const FTransform& PawnTransform)
{
	if (Offset.IsNearlyZero())
	{
		return WorldRotation;
	}
	
	FTransform SpaceTransform;
	if (!GetSpaceTransform(State, Space, PawnTransform, SpaceTransform))
	{
		return WorldRotation;
	}
	
	// 根据空间类型应用旋转
	FRotator ResultRotation;
	if (Space == ENamiCameraSpace::World)
	{
		// 世界空间：直接叠加
		ResultRotation = WorldRotation + Offset;
	}
	else
	{
		// 本地空间：在空间变换的基础上叠加
		FQuat SpaceQuat = SpaceTransform.GetRotation();
		FQuat OffsetQuat = Offset.Quaternion();
		FQuat WorldQuat = WorldRotation.Quaternion();
		
		// 将偏移转换到世界空间然后应用
		FQuat WorldOffset = SpaceQuat * OffsetQuat * SpaceQuat.Inverse();
		ResultRotation = (WorldQuat * WorldOffset).Rotator();
	}
	
	ResultRotation.Normalize();
	return ResultRotation;
}

FVector UNamiCameraSpaceMath::WorldToSpace(
	const FNamiCameraState& State,
	const FVector& WorldPosition,
	ENamiCameraSpace Space,
	const FTransform& PawnTransform)
{
	FTransform SpaceTransform;
	if (!GetSpaceTransform(State, Space, PawnTransform, SpaceTransform))
	{
		return WorldPosition;
	}
	
	if (Space == ENamiCameraSpace::World)
	{
		return WorldPosition;
	}
	
	return SpaceTransform.InverseTransformPosition(WorldPosition);
}

FVector UNamiCameraSpaceMath::SpaceToWorld(
	const FNamiCameraState& State,
	const FVector& LocalPosition,
	ENamiCameraSpace Space,
	const FTransform& PawnTransform)
{
	FTransform SpaceTransform;
	if (!GetSpaceTransform(State, Space, PawnTransform, SpaceTransform))
	{
		return LocalPosition;
	}
	
	if (Space == ENamiCameraSpace::World)
	{
		return LocalPosition;
	}
	
	return SpaceTransform.TransformPosition(LocalPosition);
}

FVector UNamiCameraSpaceMath::CalculateArmEndLocation(
	const FVector& PivotLocation,
	const FRotator& PivotRotation,
	float ArmLength,
	const FRotator& ArmRotation,
	const FVector& ArmOffset)
{
	// 计算最终旋转
	FRotator FinalRotation = PivotRotation + ArmRotation;
	FinalRotation.Normalize();
	
	// 计算吊臂方向（相机看向枢轴点，所以吊臂方向是旋转向量的反方向）
	FVector ArmDirection = FinalRotation.Vector();
	
	// 计算基础位置
	FVector EndLocation = PivotLocation - ArmDirection * ArmLength;
	
	// 应用本地偏移
	if (!ArmOffset.IsNearlyZero())
	{
		EndLocation += FinalRotation.RotateVector(ArmOffset);
	}
	
	return EndLocation;
}

FVector UNamiCameraSpaceMath::CalculateArmDirection(
	const FRotator& PivotRotation,
	const FRotator& ArmRotation)
{
	FRotator FinalRotation = PivotRotation + ArmRotation;
	FinalRotation.Normalize();
	return FinalRotation.Vector();
}

void UNamiCameraSpaceMath::CalculateArmParameters(
	const FVector& CameraLocation,
	const FVector& PivotLocation,
	float& OutArmLength,
	FRotator& OutArmRotation)
{
	// 计算从枢轴点到相机的向量
	FVector ToPivot = PivotLocation - CameraLocation;
	
	// 吊臂长度
	OutArmLength = ToPivot.Size();
	
	// 吊臂旋转（相机看向枢轴点的方向）
	if (OutArmLength > KINDA_SMALL_NUMBER)
	{
		OutArmRotation = ToPivot.Rotation();
	}
	else
	{
		OutArmRotation = FRotator::ZeroRotator;
	}
}

