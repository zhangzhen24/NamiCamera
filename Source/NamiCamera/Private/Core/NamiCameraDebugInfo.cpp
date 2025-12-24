// Copyright Qiu, Inc. All Rights Reserved.

#include "Core/NamiCameraDebugInfo.h"
#include "Core/NamiCameraView.h"
#include "Core/NamiCameraState.h"

void FNamiCameraDebugInfo::FromView(const FNamiCameraView& View)
{
	CameraLocation = View.CameraLocation;
	CameraRotation = View.CameraRotation;
	PivotLocation = View.PivotLocation;
	ControlRotation = View.ControlRotation;
	FieldOfView = View.FOV;

	// 计算距离
	DistanceToPivot = FVector::Dist(CameraLocation, PivotLocation);
}

void FNamiCameraDebugInfo::FromState(const FNamiCameraState& State)
{
	ArmLength = State.ArmLength;
	ArmRotation = State.ArmRotation;
	ArmOffset = State.ArmOffset;
	CameraLocationOffset = State.CameraLocationOffset;
	CameraRotationOffset = State.CameraRotationOffset;

	// 从 State 获取 View 信息（State 已经计算过输出）
	CameraLocation = State.CameraLocation;
	CameraRotation = State.CameraRotation;
	PivotLocation = State.PivotLocation;
	ControlRotation = State.PivotRotation;
	FieldOfView = State.FieldOfView;

	// 计算距离
	DistanceToPivot = FVector::Dist(CameraLocation, PivotLocation);
}

FString FNamiCameraDebugInfo::ToString() const
{
	return FString::Printf(
		TEXT("CameraInfo: Loc=%s Rot=%s Pivot=%s CtrlRot=%s FOV=%.1f ArmLen=%.1f ArmRot=%s Dist=%.1f"),
		*CameraLocation.ToString(),
		*CameraRotation.ToString(),
		*PivotLocation.ToString(),
		*ControlRotation.ToString(),
		FieldOfView,
		ArmLength,
		*ArmRotation.ToString(),
		DistanceToPivot
	);
}

FString FNamiCameraDebugInfo::ToMultiLineString() const
{
	FString Result;
	Result += FString::Printf(TEXT("=== Camera Debug Info ===\n"));
	Result += FString::Printf(TEXT("Camera Location: %s\n"), *CameraLocation.ToString());
	Result += FString::Printf(TEXT("Camera Rotation: P=%.1f Y=%.1f R=%.1f\n"),
		CameraRotation.Pitch, CameraRotation.Yaw, CameraRotation.Roll);
	Result += FString::Printf(TEXT("Pivot Location: %s\n"), *PivotLocation.ToString());
	Result += FString::Printf(TEXT("Control Rotation: P=%.1f Y=%.1f R=%.1f\n"),
		ControlRotation.Pitch, ControlRotation.Yaw, ControlRotation.Roll);
	Result += FString::Printf(TEXT("Field of View: %.1f°\n"), FieldOfView);
	Result += FString::Printf(TEXT("Arm Length: %.1f cm\n"), ArmLength);
	Result += FString::Printf(TEXT("Arm Rotation: P=%.1f Y=%.1f R=%.1f\n"),
		ArmRotation.Pitch, ArmRotation.Yaw, ArmRotation.Roll);
	Result += FString::Printf(TEXT("Arm Offset: %s\n"), *ArmOffset.ToString());
	Result += FString::Printf(TEXT("Distance to Pivot: %.1f cm\n"), DistanceToPivot);
	return Result;
}
