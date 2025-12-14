// Copyright Qiu, Inc. All Rights Reserved.

#include "CameraAdjust/NamiCameraAdjustParams.h"

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

	// 保持每参数混合模式
	Result.FOVBlendMode = FOVBlendMode;
	Result.ArmLengthBlendMode = ArmLengthBlendMode;
	Result.ArmRotationBlendMode = ArmRotationBlendMode;
	Result.CameraOffsetBlendMode = CameraOffsetBlendMode;
	Result.CameraRotationBlendMode = CameraRotationBlendMode;
	Result.PivotOffsetBlendMode = PivotOffsetBlendMode;

	// 保持修改标志
	Result.ModifiedFlags = ModifiedFlags;

	return Result;
}

FNamiCameraAdjustParams FNamiCameraAdjustParams::ScaleAdditiveParamsByWeight(float Weight) const
{
	FNamiCameraAdjustParams Result;

	// FOV - 只在 Additive 模式下缩放
	if (FOVBlendMode == ENamiCameraAdjustBlendMode::Additive)
	{
		Result.FOVOffset = FOVOffset * Weight;
		Result.FOVMultiplier = FMath::Lerp(1.f, FOVMultiplier, Weight);
	}
	else
	{
		Result.FOVOffset = FOVOffset;
		Result.FOVMultiplier = FOVMultiplier;
	}
	Result.FOVTarget = FOVTarget;
	Result.FOVBlendMode = FOVBlendMode;

	// ArmLength - 只在 Additive 模式下缩放
	if (ArmLengthBlendMode == ENamiCameraAdjustBlendMode::Additive)
	{
		Result.TargetArmLengthOffset = TargetArmLengthOffset * Weight;
		Result.TargetArmLengthMultiplier = FMath::Lerp(1.f, TargetArmLengthMultiplier, Weight);
	}
	else
	{
		Result.TargetArmLengthOffset = TargetArmLengthOffset;
		Result.TargetArmLengthMultiplier = TargetArmLengthMultiplier;
	}
	Result.TargetArmLengthTarget = TargetArmLengthTarget;
	Result.ArmLengthBlendMode = ArmLengthBlendMode;

	// ArmRotation - 只在 Additive 模式下缩放
	if (ArmRotationBlendMode == ENamiCameraAdjustBlendMode::Additive)
	{
		Result.ArmRotationOffset = ArmRotationOffset * Weight;
	}
	else
	{
		// Override 模式：不缩放，delta 在 CalculateCombinedAdjustParams 中计算
		Result.ArmRotationOffset = ArmRotationOffset;
	}
	Result.ArmRotationBlendMode = ArmRotationBlendMode;

	// CameraOffset - 只在 Additive 模式下缩放
	if (CameraOffsetBlendMode == ENamiCameraAdjustBlendMode::Additive)
	{
		Result.CameraLocationOffset = CameraLocationOffset * Weight;
	}
	else
	{
		Result.CameraLocationOffset = CameraLocationOffset;
	}
	Result.CameraOffsetBlendMode = CameraOffsetBlendMode;

	// CameraRotation - 只在 Additive 模式下缩放
	if (CameraRotationBlendMode == ENamiCameraAdjustBlendMode::Additive)
	{
		Result.CameraRotationOffset = CameraRotationOffset * Weight;
	}
	else
	{
		Result.CameraRotationOffset = CameraRotationOffset;
	}
	Result.CameraRotationBlendMode = CameraRotationBlendMode;

	// PivotOffset - 只在 Additive 模式下缩放
	if (PivotOffsetBlendMode == ENamiCameraAdjustBlendMode::Additive)
	{
		Result.PivotOffset = PivotOffset * Weight;
	}
	else
	{
		Result.PivotOffset = PivotOffset;
	}
	Result.PivotOffsetBlendMode = PivotOffsetBlendMode;

	// SocketOffset 和 TargetOffset - 这些通常是 Additive，按权重缩放
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
