// Copyright Epic Games, Inc. All Rights Reserved.

#include "Structs/State/NamiCameraStateFlags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraStateFlags)

FNamiCameraStateFlags::FNamiCameraStateFlags(bool bDefaultValue)
{
	SetAll(bDefaultValue);
}

void FNamiCameraStateFlags::Clear()
{
	SetAll(false);
}

void FNamiCameraStateFlags::SetAll(bool bValue)
{
	SetAllInput(bValue);
	SetAllOutput(bValue);
}

void FNamiCameraStateFlags::SetAllInput(bool bValue)
{
	// 枢轴点参数
	bPivotLocation = bValue;
	bPivotRotation = bValue;
	// 吊臂参数
	bArmLength = bValue;
	bArmRotation = bValue;
	bArmOffset = bValue;
	// 相机参数
	bCameraLocationOffset = bValue;
	bCameraRotationOffset = bValue;
	bFieldOfView = bValue;
}

void FNamiCameraStateFlags::SetAllOutput(bool bValue)
{
	bCameraLocation = bValue;
	bCameraRotation = bValue;
}

bool FNamiCameraStateFlags::HasAnyInputFlag() const
{
	return bPivotLocation || bPivotRotation ||
		bArmLength || bArmRotation || bArmOffset ||
		bCameraLocationOffset || bCameraRotationOffset || bFieldOfView;
}

bool FNamiCameraStateFlags::HasAnyOutputFlag() const
{
	return bCameraLocation || bCameraRotation;
}

bool FNamiCameraStateFlags::HasAnyFlag() const
{
	return HasAnyInputFlag() || HasAnyOutputFlag();
}

FNamiCameraStateFlags& FNamiCameraStateFlags::operator|=(const FNamiCameraStateFlags& Other)
{
	// 枢轴点参数
	bPivotLocation |= Other.bPivotLocation;
	bPivotRotation |= Other.bPivotRotation;
	// 吊臂参数
	bArmLength |= Other.bArmLength;
	bArmRotation |= Other.bArmRotation;
	bArmOffset |= Other.bArmOffset;
	// 相机参数
	bCameraLocationOffset |= Other.bCameraLocationOffset;
	bCameraRotationOffset |= Other.bCameraRotationOffset;
	bFieldOfView |= Other.bFieldOfView;
	// 输出结果
	bCameraLocation |= Other.bCameraLocation;
	bCameraRotation |= Other.bCameraRotation;
	return *this;
}

FNamiCameraStateFlags FNamiCameraStateFlags::operator|(const FNamiCameraStateFlags& Other) const
{
	FNamiCameraStateFlags Result = *this;
	Result |= Other;
	return Result;
}

FNamiCameraStateFlags FNamiCameraStateFlags::operator&(const FNamiCameraStateFlags& Other) const
{
	FNamiCameraStateFlags Result;
	// 枢轴点参数
	Result.bPivotLocation = bPivotLocation && Other.bPivotLocation;
	Result.bPivotRotation = bPivotRotation && Other.bPivotRotation;
	// 吊臂参数
	Result.bArmLength = bArmLength && Other.bArmLength;
	Result.bArmRotation = bArmRotation && Other.bArmRotation;
	Result.bArmOffset = bArmOffset && Other.bArmOffset;
	// 相机参数
	Result.bCameraLocationOffset = bCameraLocationOffset && Other.bCameraLocationOffset;
	Result.bCameraRotationOffset = bCameraRotationOffset && Other.bCameraRotationOffset;
	Result.bFieldOfView = bFieldOfView && Other.bFieldOfView;
	// 输出结果
	Result.bCameraLocation = bCameraLocation && Other.bCameraLocation;
	Result.bCameraRotation = bCameraRotation && Other.bCameraRotation;
	return Result;
}

const FNamiCameraStateFlags& FNamiCameraStateFlags::All()
{
	static FNamiCameraStateFlags AllFlags(true);
	return AllFlags;
}

const FNamiCameraStateFlags& FNamiCameraStateFlags::AllInput()
{
	static FNamiCameraStateFlags InputFlags;
	static bool bInitialized = false;
	if (!bInitialized)
	{
		InputFlags.SetAllInput(true);
		bInitialized = true;
	}
	return InputFlags;
}

const FNamiCameraStateFlags& FNamiCameraStateFlags::AllOutput()
{
	static FNamiCameraStateFlags OutputFlags;
	static bool bInitialized = false;
	if (!bInitialized)
	{
		OutputFlags.SetAllOutput(true);
		bInitialized = true;
	}
	return OutputFlags;
}

