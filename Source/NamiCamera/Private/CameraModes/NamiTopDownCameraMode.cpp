// Copyright Qiu, Inc. All Rights Reserved.

#include "CameraModes/NamiTopDownCameraMode.h"

#include "Calculators/Target/NamiSingleTargetCalculator.h"
#include "Calculators/Position/NamiTopDownPositionCalculator.h"
#include "Calculators/Rotation/NamiTopDownRotationCalculator.h"
#include "Calculators/FOV/NamiStaticFOVCalculator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiTopDownCameraMode)

UNamiTopDownCameraMode::UNamiTopDownCameraMode()
{
	// 创建默认计算器
	CreateDefaultCalculators();
}

void UNamiTopDownCameraMode::Initialize_Implementation(UNamiCameraComponent* InCameraComponent)
{
	// 先确保有计算器实例（CreateDefaultSubobject 在 NewObject 创建时不起作用）
	// 必须在调用 Super 之前创建，这样 InitializeCalculators() 才能正确初始化它们

	if (!TargetCalculator)
	{
		TargetCalculator = NewObject<UNamiSingleTargetCalculator>(this);
	}

	if (!PositionCalculator)
	{
		UNamiTopDownPositionCalculator* TopDownPos = NewObject<UNamiTopDownPositionCalculator>(this);
		TopDownPos->CameraHeight = 1500.0f;
		TopDownPos->ViewAngle = 45.0f;
		TopDownPos->ViewDirectionYaw = 45.0f;  // 默认东北方向
		TopDownPos->FollowSmoothSpeed = 8.0f;
		PositionCalculator = TopDownPos;
	}

	if (!RotationCalculator)
	{
		RotationCalculator = NewObject<UNamiTopDownRotationCalculator>(this);
	}

	if (!FOVCalculator)
	{
		UNamiStaticFOVCalculator* StaticFOV = NewObject<UNamiStaticFOVCalculator>(this);
		StaticFOV->BaseFOV = 80.0f;  // 略小于第三人称，提供更清晰的视野
		FOVCalculator = StaticFOV;
	}

	// 调用父类初始化（会调用 InitializeCalculators）
	Super::Initialize_Implementation(InCameraComponent);
}

UNamiTopDownPositionCalculator* UNamiTopDownCameraMode::GetTopDownPositionCalculator() const
{
	return Cast<UNamiTopDownPositionCalculator>(PositionCalculator);
}

UNamiTopDownRotationCalculator* UNamiTopDownCameraMode::GetTopDownRotationCalculator() const
{
	return Cast<UNamiTopDownRotationCalculator>(RotationCalculator);
}

void UNamiTopDownCameraMode::CreateDefaultCalculators()
{
	// 创建单目标计算器
	if (!TargetCalculator)
	{
		TargetCalculator = CreateDefaultSubobject<UNamiSingleTargetCalculator>(TEXT("TargetCalculator"));
	}

	// 创建 TopDown 位置计算器
	if (!PositionCalculator)
	{
		UNamiTopDownPositionCalculator* TopDownPos =
			CreateDefaultSubobject<UNamiTopDownPositionCalculator>(TEXT("PositionCalculator"));
		TopDownPos->CameraHeight = 1500.0f;
		TopDownPos->ViewAngle = 45.0f;
		TopDownPos->ViewDirectionYaw = 45.0f;  // 默认东北方向
		TopDownPos->FollowSmoothSpeed = 8.0f;
		PositionCalculator = TopDownPos;
	}

	// 创建 TopDown 旋转计算器
	if (!RotationCalculator)
	{
		RotationCalculator = CreateDefaultSubobject<UNamiTopDownRotationCalculator>(TEXT("RotationCalculator"));
	}

	// 创建静态 FOV 计算器
	if (!FOVCalculator)
	{
		UNamiStaticFOVCalculator* StaticFOV =
			CreateDefaultSubobject<UNamiStaticFOVCalculator>(TEXT("FOVCalculator"));
		StaticFOV->BaseFOV = 80.0f;
		FOVCalculator = StaticFOV;
	}
}
