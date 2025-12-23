// Copyright Qiu, Inc. All Rights Reserved.

#include "CameraModes/NamiThirdPersonCameraMode.h"

#include "Calculators/Target/NamiSingleTargetCalculator.h"
#include "Calculators/Position/NamiOffsetPositionCalculator.h"
#include "Calculators/Rotation/NamiControlRotationCalculator.h"
#include "Calculators/FOV/NamiStaticFOVCalculator.h"
#include "ModeComponents/NamiCameraModeComponent.h"
#include "ModeComponents/NamiCameraSpringArmComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiThirdPersonCameraMode)

UNamiThirdPersonCameraMode::UNamiThirdPersonCameraMode()
{
	// 创建默认计算器和组件
	CreateDefaultCalculators();
	CreateDefaultComponents();
}

void UNamiThirdPersonCameraMode::Initialize_Implementation(UNamiCameraComponent* InCameraComponent)
{
	// 先确保有计算器实例（CreateDefaultSubobject 在 NewObject 创建时不起作用）
	// 必须在调用 Super 之前创建，这样 InitializeCalculators() 才能正确初始化它们
	if (!TargetCalculator)
	{
		TargetCalculator = NewObject<UNamiSingleTargetCalculator>(this);
	}
	if (!PositionCalculator)
	{
		UNamiOffsetPositionCalculator* OffsetCalculator = NewObject<UNamiOffsetPositionCalculator>(this);
		OffsetCalculator->CameraOffset = FVector(-300.0f, 0.0f, 100.0f);
		OffsetCalculator->bUseControlRotation = true;
		PositionCalculator = OffsetCalculator;
	}
	if (!RotationCalculator)
	{
		RotationCalculator = NewObject<UNamiControlRotationCalculator>(this);
	}
	if (!FOVCalculator)
	{
		UNamiStaticFOVCalculator* StaticFOV = NewObject<UNamiStaticFOVCalculator>(this);
		StaticFOV->BaseFOV = 90.0f;
		FOVCalculator = StaticFOV;
	}

	// 调用父类初始化（会调用 InitializeCalculators）
	Super::Initialize_Implementation(InCameraComponent);
}

UNamiCameraSpringArmComponent* UNamiThirdPersonCameraMode::GetSpringArmComponent() const
{
	return GetComponent<UNamiCameraSpringArmComponent>();
}

void UNamiThirdPersonCameraMode::CreateDefaultCalculators()
{
	// 创建单目标计算器
	if (!TargetCalculator)
	{
		TargetCalculator = CreateDefaultSubobject<UNamiSingleTargetCalculator>(TEXT("TargetCalculator"));
	}

	// 创建偏移位置计算器
	if (!PositionCalculator)
	{
		UNamiOffsetPositionCalculator* OffsetCalculator = CreateDefaultSubobject<UNamiOffsetPositionCalculator>(TEXT("PositionCalculator"));
		OffsetCalculator->CameraOffset = FVector(-300.0f, 0.0f, 100.0f);
		OffsetCalculator->bUseControlRotation = true;
		PositionCalculator = OffsetCalculator;
	}

	// 创建控制旋转计算器
	if (!RotationCalculator)
	{
		RotationCalculator = CreateDefaultSubobject<UNamiControlRotationCalculator>(TEXT("RotationCalculator"));
	}

	// 创建静态 FOV 计算器
	if (!FOVCalculator)
	{
		UNamiStaticFOVCalculator* StaticFOV = CreateDefaultSubobject<UNamiStaticFOVCalculator>(TEXT("FOVCalculator"));
		StaticFOV->BaseFOV = 90.0f;
		FOVCalculator = StaticFOV;
	}
}

void UNamiThirdPersonCameraMode::CreateDefaultComponents()
{
	// 创建弹簧臂组件
	bool bHasSpringArm = false;
	for (UNamiCameraModeComponent* Component : ModeComponents)
	{
		if (Cast<UNamiCameraSpringArmComponent>(Component))
		{
			bHasSpringArm = true;
			break;
		}
	}

	if (!bHasSpringArm)
	{
		UNamiCameraSpringArmComponent* SpringArmComp = CreateDefaultSubobject<UNamiCameraSpringArmComponent>(TEXT("SpringArmComponent"));
		SpringArmComp->SpringArm.SpringArmLength = 350.0f;
		SpringArmComp->SpringArm.bDoCollisionTest = true;
		ModeComponents.Add(SpringArmComp);
	}
}
