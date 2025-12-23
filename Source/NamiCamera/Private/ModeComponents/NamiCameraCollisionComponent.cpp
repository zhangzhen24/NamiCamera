// Copyright Qiu, Inc. All Rights Reserved.

#include "ModeComponents/NamiCameraCollisionComponent.h"

#include "CameraModes/NamiCameraModeBase.h"
#include "Components/NamiCameraComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraCollisionComponent)

UNamiCameraCollisionComponent::UNamiCameraCollisionComponent()
{
	ComponentName = TEXT("Collision");
	Priority = 100; // 高优先级，在其他组件之前应用

	// 配置默认值
	SpringArm.SpringArmLength = 350.0f;
	SpringArm.bDoCollisionTest = true;
	SpringArm.bEnableCameraLag = false;
	SpringArm.CameraLagSpeed = 10.0f;
	SpringArm.ProbeSize = 12.0f;
	SpringArm.ProbeChannel = ECC_Camera;
}

void UNamiCameraCollisionComponent::Initialize_Implementation(UNamiCameraModeBase* InCameraMode)
{
	Super::Initialize_Implementation(InCameraMode);

	SpringArm.Initialize();
	bSpringArmInitialized = true;
}

void UNamiCameraCollisionComponent::Activate_Implementation()
{
	Super::Activate_Implementation();

	if (!bSpringArmInitialized)
	{
		SpringArm.Initialize();
		bSpringArmInitialized = true;
	}
}

void UNamiCameraCollisionComponent::ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime)
{
	if (!bSpringArmInitialized || !bEnabled)
	{
		return;
	}

	// 构建初始 Transform
	FTransform InitialTransform;
	InitialTransform.SetLocation(InOutView.PivotLocation);
	InitialTransform.SetRotation(InOutView.CameraRotation.Quaternion());

	// 收集忽略 Actor
	TArray<AActor*> IgnoreActors = GetIgnoreActors();

	// 执行 SpringArm 计算
	SpringArm.Tick(this, DeltaTime, IgnoreActors, InitialTransform, FVector::ZeroVector);

	// 获取结果并更新 View
	const FTransform& CameraTransform = SpringArm.GetCameraTransform();
	InOutView.CameraLocation = CameraTransform.GetLocation();
	InOutView.CameraRotation = CameraTransform.Rotator();
}

TArray<AActor*> UNamiCameraCollisionComponent::GetIgnoreActors_Implementation() const
{
	TArray<AActor*> IgnoreActors;

	if (UNamiCameraModeBase* Mode = GetCameraMode())
	{
		if (UNamiCameraComponent* CameraComp = Mode->GetCameraComponent())
		{
			if (AActor* Owner = CameraComp->GetOwner())
			{
				IgnoreActors.Add(Owner);
			}
		}
	}

	return IgnoreActors;
}
