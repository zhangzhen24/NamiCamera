// Copyright Qiu, Inc. All Rights Reserved.

#include "ModeComponents/NamiCameraLockOnComponent.h"

#include "CameraModes/NamiCameraModeBase.h"
#include "Components/NamiCameraComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraLockOnComponent)

UNamiCameraLockOnComponent::UNamiCameraLockOnComponent()
{
	ComponentName = TEXT("LockOn");
	Priority = 50; // 中等优先级

	SmoothedTargetLocation = FVector::ZeroVector;
}

void UNamiCameraLockOnComponent::Activate_Implementation()
{
	Super::Activate_Implementation();

	// 重置状态
	bTargetLocationInitialized = false;
	SmoothedTargetLocation = FVector::ZeroVector;
	CachedDistanceToTarget = 0.0f;
}

void UNamiCameraLockOnComponent::Update_Implementation(float DeltaTime)
{
	Super::Update_Implementation(DeltaTime);

	// 更新平滑的目标位置
	UpdateSmoothedTargetLocation(DeltaTime);

	// 更新缓存的距离
	if (HasValidLockedTarget())
	{
		if (UNamiCameraModeBase* Mode = GetCameraMode())
		{
			if (UNamiCameraComponent* CameraComp = Mode->GetCameraComponent())
			{
				if (AActor* Owner = CameraComp->GetOwner())
				{
					CachedDistanceToTarget = FVector::Dist(Owner->GetActorLocation(), GetEffectiveTargetLocation());
				}
			}
		}
	}
}

void UNamiCameraLockOnComponent::SetLockOnProvider(TScriptInterface<INamiLockOnTargetProvider> Provider)
{
	LockOnProvider = Provider;
	bTargetLocationInitialized = false;
}

TScriptInterface<INamiLockOnTargetProvider> UNamiCameraLockOnComponent::GetLockOnProvider_Implementation() const
{
	return LockOnProvider;
}

bool UNamiCameraLockOnComponent::HasValidLockedTarget() const
{
	const TScriptInterface<INamiLockOnTargetProvider> Provider = GetLockOnProvider();
	return Provider.GetInterface() && Provider->HasLockedTarget();
}

FVector UNamiCameraLockOnComponent::GetLockedTargetLocation() const
{
	const TScriptInterface<INamiLockOnTargetProvider> Provider = GetLockOnProvider();
	if (Provider.GetInterface() && Provider->HasLockedTarget())
	{
		return Provider->GetLockedLocation();
	}
	return FVector::ZeroVector;
}

FVector UNamiCameraLockOnComponent::GetLockedFocusLocation() const
{
	const TScriptInterface<INamiLockOnTargetProvider> Provider = GetLockOnProvider();
	if (Provider.GetInterface() && Provider->HasLockedTarget())
	{
		return Provider->GetLockedFocusLocation();
	}
	return FVector::ZeroVector;
}

FVector UNamiCameraLockOnComponent::GetEffectiveTargetLocation() const
{
	if (bUseSmoothTargetLocation && bTargetLocationInitialized)
	{
		return SmoothedTargetLocation;
	}
	return GetLockedFocusLocation();
}

AActor* UNamiCameraLockOnComponent::GetLockedTargetActor() const
{
	const TScriptInterface<INamiLockOnTargetProvider> Provider = GetLockOnProvider();
	if (Provider.GetInterface())
	{
		return Provider->GetLockedTargetActor();
	}
	return nullptr;
}

void UNamiCameraLockOnComponent::UpdateSmoothedTargetLocation(float DeltaTime)
{
	if (!HasValidLockedTarget())
	{
		return;
	}

	const FVector TargetLocation = GetLockedFocusLocation();

	if (!bTargetLocationInitialized)
	{
		// 第一次初始化，直接设置
		SmoothedTargetLocation = TargetLocation;
		bTargetLocationInitialized = true;
	}
	else if (bUseSmoothTargetLocation)
	{
		// 平滑过渡到目标位置
		SmoothedTargetLocation = FMath::VInterpTo(
			SmoothedTargetLocation,
			TargetLocation,
			DeltaTime,
			TargetLocationSmoothSpeed
		);
	}
	else
	{
		// 直接使用目标位置
		SmoothedTargetLocation = TargetLocation;
	}
}
