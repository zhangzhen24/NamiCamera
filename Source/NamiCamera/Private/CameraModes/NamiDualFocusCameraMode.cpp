// Copyright Qiu, Inc. All Rights Reserved.

#include "CameraModes/NamiDualFocusCameraMode.h"

#include "Calculators/Target/NamiDualFocusTargetCalculator.h"
#include "Calculators/Position/NamiEllipseOrbitPositionCalculator.h"
#include "Calculators/Rotation/NamiLookAtRotationCalculator.h"
#include "Calculators/FOV/NamiFramingFOVCalculator.h"
#include "ModeComponents/NamiCameraLockOnComponent.h"
#include "Components/NamiCameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiDualFocusCameraMode)

UNamiDualFocusCameraMode::UNamiDualFocusCameraMode()
{
	// 设置混合配置
	BlendConfig.BlendTime = 0.5f;
	BlendConfig.BlendType = ENamiCameraBlendType::EaseInOut;

	// 创建默认计算器
	CreateDefaultCalculators();
}

void UNamiDualFocusCameraMode::Initialize_Implementation(UNamiCameraComponent* InCameraComponent)
{
	// 先确保有计算器实例（CreateDefaultSubobject 在 NewObject 创建时不起作用）
	// 必须在调用 Super 之前创建，这样 InitializeCalculators() 才能正确初始化它们
	if (!TargetCalculator)
	{
		UNamiDualFocusTargetCalculator* DualFocusCalculator = NewObject<UNamiDualFocusTargetCalculator>(this);
		DualFocusCalculator->PlayerFocusWeight = 0.6f;
		DualFocusCalculator->TargetFocusWeight = 0.4f;
		TargetCalculator = DualFocusCalculator;
	}
	if (!PositionCalculator)
	{
		UNamiEllipseOrbitPositionCalculator* EllipseCalculator = NewObject<UNamiEllipseOrbitPositionCalculator>(this);
		EllipseCalculator->EllipseMajorRadius = 800.0f;
		EllipseCalculator->EllipseMinorRadius = 500.0f;
		EllipseCalculator->HeightOffset = 150.0f;
		EllipseCalculator->bEnablePlayerInput = true;
		PositionCalculator = EllipseCalculator;
	}
	if (!RotationCalculator)
	{
		UNamiLookAtRotationCalculator* LookAtCalculator = NewObject<UNamiLookAtRotationCalculator>(this);
		LookAtCalculator->RotationSmoothSpeed = 8.0f;
		RotationCalculator = LookAtCalculator;
	}
	if (!FOVCalculator)
	{
		UNamiFramingFOVCalculator* FramingFOV = NewObject<UNamiFramingFOVCalculator>(this);
		FramingFOV->BaseFOV = 80.0f;
		FramingFOV->MinFOV = 60.0f;
		FramingFOV->MaxFOV = 100.0f;
		FramingFOV->bKeepBothInFrame = true;
		FOVCalculator = FramingFOV;
	}

	// 调用父类初始化（会调用 InitializeStrategies）
	Super::Initialize_Implementation(InCameraComponent);

	// 创建并注册 LockOn 组件
	SetupLockOnComponent();

	SyncLockOnProviderToCalculators();
}

void UNamiDualFocusCameraMode::Activate_Implementation()
{
	Super::Activate_Implementation();

	SyncLockOnProviderToCalculators();
}

FNamiCameraView UNamiDualFocusCameraMode::CalculateView_Implementation(float DeltaTime)
{
	// 处理玩家输入
	ProcessPlayerInput(DeltaTime);

	// 调用父类计算视图
	return Super::CalculateView_Implementation(DeltaTime);
}

void UNamiDualFocusCameraMode::SetLockOnProvider(TScriptInterface<INamiLockOnTargetProvider> Provider)
{
	CachedLockOnProvider = Provider;
	SyncLockOnProviderToCalculators();

	// 同步到 LockOnComponent
	if (UNamiCameraLockOnComponent* LockOnComp = GetLockOnComponent())
	{
		LockOnComp->SetLockOnProvider(Provider);
	}
}

TScriptInterface<INamiLockOnTargetProvider> UNamiDualFocusCameraMode::GetLockOnProvider() const
{
	return CachedLockOnProvider;
}

bool UNamiDualFocusCameraMode::HasValidLockedTarget() const
{
	return CachedLockOnProvider.GetInterface() && CachedLockOnProvider->HasLockedTarget();
}

void UNamiDualFocusCameraMode::AddOrbitInput(float DeltaAngle)
{
	if (UNamiEllipseOrbitPositionCalculator* EllipseCalculator = GetEllipseOrbitPositionCalculator())
	{
		EllipseCalculator->AddOrbitInput(DeltaAngle);
	}
}

UNamiDualFocusTargetCalculator* UNamiDualFocusCameraMode::GetDualFocusTargetCalculator() const
{
	return Cast<UNamiDualFocusTargetCalculator>(TargetCalculator);
}

UNamiEllipseOrbitPositionCalculator* UNamiDualFocusCameraMode::GetEllipseOrbitPositionCalculator() const
{
	return Cast<UNamiEllipseOrbitPositionCalculator>(PositionCalculator);
}

UNamiLookAtRotationCalculator* UNamiDualFocusCameraMode::GetLookAtRotationCalculator() const
{
	return Cast<UNamiLookAtRotationCalculator>(RotationCalculator);
}

UNamiFramingFOVCalculator* UNamiDualFocusCameraMode::GetFramingFOVCalculator() const
{
	return Cast<UNamiFramingFOVCalculator>(FOVCalculator);
}

UNamiCameraLockOnComponent* UNamiDualFocusCameraMode::GetLockOnComponent() const
{
	return LockOnComponent;
}

void UNamiDualFocusCameraMode::CreateDefaultCalculators()
{
	// 创建双焦点目标计算器
	if (!TargetCalculator)
	{
		UNamiDualFocusTargetCalculator* DualFocusCalculator = CreateDefaultSubobject<UNamiDualFocusTargetCalculator>(TEXT("TargetCalculator"));
		DualFocusCalculator->PlayerFocusWeight = 0.6f;
		DualFocusCalculator->TargetFocusWeight = 0.4f;
		TargetCalculator = DualFocusCalculator;
	}

	// 创建椭圆轨道位置计算器
	if (!PositionCalculator)
	{
		UNamiEllipseOrbitPositionCalculator* EllipseCalculator = CreateDefaultSubobject<UNamiEllipseOrbitPositionCalculator>(TEXT("PositionCalculator"));
		EllipseCalculator->EllipseMajorRadius = 800.0f;
		EllipseCalculator->EllipseMinorRadius = 500.0f;
		EllipseCalculator->HeightOffset = 150.0f;
		EllipseCalculator->bEnablePlayerInput = true;
		PositionCalculator = EllipseCalculator;
	}

	// 创建看向旋转计算器
	if (!RotationCalculator)
	{
		UNamiLookAtRotationCalculator* LookAtCalculator = CreateDefaultSubobject<UNamiLookAtRotationCalculator>(TEXT("RotationCalculator"));
		LookAtCalculator->RotationSmoothSpeed = 8.0f;
		RotationCalculator = LookAtCalculator;
	}

	// 创建构图 FOV 计算器
	if (!FOVCalculator)
	{
		UNamiFramingFOVCalculator* FramingFOV = CreateDefaultSubobject<UNamiFramingFOVCalculator>(TEXT("FOVCalculator"));
		FramingFOV->BaseFOV = 80.0f;
		FramingFOV->MinFOV = 60.0f;
		FramingFOV->MaxFOV = 100.0f;
		FramingFOV->bKeepBothInFrame = true;
		FOVCalculator = FramingFOV;
	}
}

void UNamiDualFocusCameraMode::SetupLockOnComponent()
{
	// 检查是否已有 LockOn 组件
	if (LockOnComponent)
	{
		return;
	}

	// 先尝试从 Mode 的 ModeComponents 获取已有的 LockOnComponent
	LockOnComponent = GetComponent<UNamiCameraLockOnComponent>();
	if (LockOnComponent)
	{
		return;
	}

	// 创建并注册新的 LockOnComponent 到 Mode
	LockOnComponent = NewObject<UNamiCameraLockOnComponent>(this);
	LockOnComponent->TargetLocationSmoothSpeed = 12.0f;
	LockOnComponent->bUseSmoothTargetLocation = true;
	AddComponent(LockOnComponent);
}

void UNamiDualFocusCameraMode::SyncLockOnProviderToCalculators()
{
	// 同步到 DualFocusTargetCalculator
	if (UNamiDualFocusTargetCalculator* DualFocusCalculator = GetDualFocusTargetCalculator())
	{
		DualFocusCalculator->SetLockOnProvider(CachedLockOnProvider);
	}

	// 同步到 EllipseOrbitPositionCalculator
	if (UNamiEllipseOrbitPositionCalculator* EllipseCalculator = GetEllipseOrbitPositionCalculator())
	{
		EllipseCalculator->SetLockOnProvider(CachedLockOnProvider);
		// 同步主要目标
		if (AActor* Target = GetPrimaryTarget())
		{
			EllipseCalculator->SetPrimaryTarget(Target);
		}
	}

	// 同步到 FramingFOVCalculator
	if (UNamiFramingFOVCalculator* FramingFOV = GetFramingFOVCalculator())
	{
		FramingFOV->SetLockOnProvider(CachedLockOnProvider);
		if (AActor* Target = GetPrimaryTarget())
		{
			FramingFOV->SetPrimaryTarget(Target);
		}
	}
}

void UNamiDualFocusCameraMode::ProcessPlayerInput(float DeltaTime)
{
	if (UNamiEllipseOrbitPositionCalculator* EllipseCalculator = GetEllipseOrbitPositionCalculator())
	{
		if (EllipseCalculator->bEnablePlayerInput)
		{
			// 获取 PlayerController
			if (UNamiCameraComponent* CameraComp = GetCameraComponent())
			{
				if (APlayerController* PC = CameraComp->GetOwnerPlayerController())
				{
					float MouseDeltaX, MouseDeltaY;
					PC->GetInputMouseDelta(MouseDeltaX, MouseDeltaY);
					EllipseCalculator->AddOrbitInput(MouseDeltaX);
				}
			}
		}
	}
}
