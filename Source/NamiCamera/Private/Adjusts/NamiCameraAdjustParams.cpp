// Copyright Epic Games, Inc. All Rights Reserved.

#include "Adjusts/NamiCameraAdjustParams.h"

FNamiCameraAdjustParams FNamiCameraAdjustParams::Lerp(
	const FNamiCameraAdjustParams& A,
	const FNamiCameraAdjustParams& B,
	float Alpha)
{
	FNamiCameraAdjustParams Result;

	// 视图参数插值
	Result.FOVOffset = FMath::Lerp(A.FOVOffset, B.FOVOffset, Alpha);
	Result.FOVMultiplier = FMath::Lerp(A.FOVMultiplier, B.FOVMultiplier, Alpha);
	Result.FOVTarget = FMath::Lerp(A.FOVTarget, B.FOVTarget, Alpha);
	Result.CameraLocationOffset = FMath::Lerp(A.CameraLocationOffset, B.CameraLocationOffset, Alpha);
	Result.CameraRotationOffset = FMath::Lerp(A.CameraRotationOffset, B.CameraRotationOffset, Alpha);
	Result.PivotOffset = FMath::Lerp(A.PivotOffset, B.PivotOffset, Alpha);

	// SpringArm参数插值
	Result.TargetArmLengthOffset = FMath::Lerp(A.TargetArmLengthOffset, B.TargetArmLengthOffset, Alpha);
	Result.TargetArmLengthMultiplier = FMath::Lerp(A.TargetArmLengthMultiplier, B.TargetArmLengthMultiplier, Alpha);
	Result.TargetArmLengthTarget = FMath::Lerp(A.TargetArmLengthTarget, B.TargetArmLengthTarget, Alpha);
	Result.ArmRotationOffset = FMath::Lerp(A.ArmRotationOffset, B.ArmRotationOffset, Alpha);
	Result.SocketOffsetDelta = FMath::Lerp(A.SocketOffsetDelta, B.SocketOffsetDelta, Alpha);
	Result.TargetOffsetDelta = FMath::Lerp(A.TargetOffsetDelta, B.TargetOffsetDelta, Alpha);

	// 合并修改标志
	Result.ModifiedFlags = A.ModifiedFlags | B.ModifiedFlags;

	return Result;
}

FNamiCameraAdjustParams FNamiCameraAdjustParams::ScaleByWeight(float Weight) const
{
	FNamiCameraAdjustParams Result;

	// 视图参数缩放
	Result.FOVOffset = FOVOffset * Weight;
	Result.FOVMultiplier = FMath::Lerp(1.f, FOVMultiplier, Weight);
	Result.FOVTarget = FOVTarget;  // Target不缩放
	Result.CameraLocationOffset = CameraLocationOffset * Weight;
	Result.CameraRotationOffset = CameraRotationOffset * Weight;
	Result.PivotOffset = PivotOffset * Weight;

	// SpringArm参数缩放
	Result.TargetArmLengthOffset = TargetArmLengthOffset * Weight;
	Result.TargetArmLengthMultiplier = FMath::Lerp(1.f, TargetArmLengthMultiplier, Weight);
	Result.TargetArmLengthTarget = TargetArmLengthTarget;  // Target不缩放
	Result.ArmRotationOffset = ArmRotationOffset * Weight;
	Result.SocketOffsetDelta = SocketOffsetDelta * Weight;
	Result.TargetOffsetDelta = TargetOffsetDelta * Weight;

	// 保持修改标志
	Result.ModifiedFlags = ModifiedFlags;

	return Result;
}

FNamiCameraAdjustParams FNamiCameraAdjustParams::Combine(
	const FNamiCameraAdjustParams& A,
	const FNamiCameraAdjustParams& B)
{
	FNamiCameraAdjustParams Result;

	// 视图参数叠加
	Result.FOVOffset = A.FOVOffset + B.FOVOffset;
	Result.FOVMultiplier = A.FOVMultiplier * B.FOVMultiplier;
	Result.FOVTarget = B.FOVTarget;  // Override使用后者
	Result.CameraLocationOffset = A.CameraLocationOffset + B.CameraLocationOffset;
	Result.CameraRotationOffset = A.CameraRotationOffset + B.CameraRotationOffset;
	Result.PivotOffset = A.PivotOffset + B.PivotOffset;

	// SpringArm参数叠加
	Result.TargetArmLengthOffset = A.TargetArmLengthOffset + B.TargetArmLengthOffset;
	Result.TargetArmLengthMultiplier = A.TargetArmLengthMultiplier * B.TargetArmLengthMultiplier;
	Result.TargetArmLengthTarget = B.TargetArmLengthTarget;  // Override使用后者
	Result.ArmRotationOffset = A.ArmRotationOffset + B.ArmRotationOffset;
	Result.SocketOffsetDelta = A.SocketOffsetDelta + B.SocketOffsetDelta;
	Result.TargetOffsetDelta = A.TargetOffsetDelta + B.TargetOffsetDelta;

	// 合并修改标志
	Result.ModifiedFlags = A.ModifiedFlags | B.ModifiedFlags;

	return Result;
}
