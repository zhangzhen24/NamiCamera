// Copyright Qiu, Inc. All Rights Reserved.

#include "ModeComponents/NamiCameraDynamicFOVComponent.h"

#include "CameraModes/NamiCameraModeBase.h"
#include "CameraModes/NamiComposableCameraMode.h"
#include "Components/NamiCameraComponent.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraDynamicFOVComponent)

UNamiCameraDynamicFOVComponent::UNamiCameraDynamicFOVComponent()
{
	ComponentName = TEXT("DynamicFOV");
	Priority = 10; // 低优先级，在其他组件之后应用
}

void UNamiCameraDynamicFOVComponent::Activate_Implementation()
{
	Super::Activate_Implementation();
	CurrentDynamicFOV = BaseFOV;
}

void UNamiCameraDynamicFOVComponent::ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime)
{
	if (!bEnabled)
	{
		return;
	}

	// 计算目标 FOV
	float TargetFOV = BaseFOV;

	// 获取速度来源
	AActor* SpeedSource = GetSpeedSourceActor();
	if (SpeedSource)
	{
		APawn* PawnSource = Cast<APawn>(SpeedSource);
		if (PawnSource)
		{
			float Speed = PawnSource->GetVelocity().Size();
			TargetFOV += Speed * SpeedFOVFactor;
		}
		else
		{
			// 非 Pawn 也尝试获取速度
			FVector Velocity = SpeedSource->GetVelocity();
			float Speed = Velocity.Size();
			TargetFOV += Speed * SpeedFOVFactor;
		}
	}

	// 限制 FOV 范围
	TargetFOV = FMath::Clamp(TargetFOV, MinDynamicFOV, MaxDynamicFOV);

	// 平滑过渡 FOV
	if (DeltaTime > 0.0f && DynamicFOVChangeRate > 0.0f)
	{
		CurrentDynamicFOV = FMath::FInterpTo(CurrentDynamicFOV, TargetFOV, DeltaTime, DynamicFOVChangeRate);
	}
	else
	{
		CurrentDynamicFOV = TargetFOV;
	}

	// 应用到视图
	InOutView.FOV = CurrentDynamicFOV;
}

AActor* UNamiCameraDynamicFOVComponent::GetSpeedSourceActor_Implementation() const
{
	if (UNamiCameraModeBase* Mode = GetCameraMode())
	{
		// 尝试从 ComposableCameraMode 获取主要目标
		if (UNamiComposableCameraMode* ComposableMode = Cast<UNamiComposableCameraMode>(Mode))
		{
			return ComposableMode->GetPrimaryTarget();
		}

		// 后备：从相机组件获取 Owner
		if (UNamiCameraComponent* CameraComp = Mode->GetCameraComponent())
		{
			return CameraComp->GetOwner();
		}
	}

	return nullptr;
}
