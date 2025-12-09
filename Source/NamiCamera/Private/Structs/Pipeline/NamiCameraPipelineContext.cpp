// Copyright Epic Games, Inc. All Rights Reserved.

#include "Structs/Pipeline/NamiCameraPipelineContext.h"
#include "Structs/State/NamiCameraState.h"

void FNamiCameraPipelineContext::FPreservedValues::PreserveFromState(const FNamiCameraState& State)
{
	PivotLocation = State.PivotLocation;
	PivotRotation = State.PivotRotation;
	ArmLength = State.ArmLength;
	ArmRotation = State.ArmRotation;
	ArmOffset = State.ArmOffset;
	CameraLocationOffset = State.CameraLocationOffset;
	CameraRotationOffset = State.CameraRotationOffset;
	FieldOfView = State.FieldOfView;
	CameraLocation = State.CameraLocation;
	CameraRotation = State.CameraRotation;
}

void FNamiCameraPipelineContext::FPreservedValues::RestoreRotationToState(FNamiCameraState& OutState) const
{
	OutState.PivotRotation = PivotRotation;
	OutState.ArmRotation = ArmRotation;
	OutState.CameraRotationOffset = CameraRotationOffset;
	OutState.CameraRotation = CameraRotation;
}

void FNamiCameraPipelineContext::FPreservedValues::RestoreLocationToState(FNamiCameraState& OutState) const
{
	OutState.PivotLocation = PivotLocation;
	OutState.ArmLength = ArmLength;
	OutState.ArmOffset = ArmOffset;
	OutState.CameraLocationOffset = CameraLocationOffset;
	OutState.CameraLocation = CameraLocation;
}

