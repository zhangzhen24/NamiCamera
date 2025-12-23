// Copyright Qiu, Inc. All Rights Reserved.

#include "Calculators/Target/NamiDualFocusTargetCalculator.h"

#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiDualFocusTargetCalculator)

UNamiDualFocusTargetCalculator::UNamiDualFocusTargetCalculator()
{
	SmoothedLockedLocation = FVector::ZeroVector;
	SmoothedFocusPoint = FVector::ZeroVector;
}

void UNamiDualFocusTargetCalculator::Activate_Implementation()
{
	Super::Activate_Implementation();

	bLockedLocationInitialized = false;
	bFocusPointInitialized = false;
	SmoothedLockedLocation = FVector::ZeroVector;
	SmoothedFocusPoint = FVector::ZeroVector;
}

bool UNamiDualFocusTargetCalculator::CalculateTargetLocation_Implementation(float DeltaTime, FVector& OutLocation)
{
	// 获取玩家位置
	FVector PlayerLocation = FVector::ZeroVector;
	if (AActor* Target = PrimaryTarget.Get())
	{
		PlayerLocation = Target->GetActorLocation();
	}
	else
	{
		OutLocation = CurrentTargetLocation;
		return false;
	}

	// 获取锁定目标位置
	FVector LockedLocation = PlayerLocation;
	if (HasValidLockedTarget())
	{
		LockedLocation = GetLockedTargetLocation();

		// 平滑锁定目标位置
		if (!bLockedLocationInitialized)
		{
			SmoothedLockedLocation = LockedLocation;
			bLockedLocationInitialized = true;
		}
		else if (LockedTargetSmoothSpeed > 0.0f)
		{
			SmoothedLockedLocation = FMath::VInterpTo(SmoothedLockedLocation, LockedLocation, DeltaTime, LockedTargetSmoothSpeed);
		}
		else
		{
			SmoothedLockedLocation = LockedLocation;
		}
		LockedLocation = SmoothedLockedLocation;
	}
	else
	{
		bLockedLocationInitialized = false;
	}

	// 计算双焦点
	FVector FocusPoint = CalculateDualFocusPoint(PlayerLocation, LockedLocation);

	// 平滑焦点位置
	if (!bFocusPointInitialized)
	{
		SmoothedFocusPoint = FocusPoint;
		bFocusPointInitialized = true;
	}
	else if (FocusPointSmoothSpeed > 0.0f)
	{
		SmoothedFocusPoint = FMath::VInterpTo(SmoothedFocusPoint, FocusPoint, DeltaTime, FocusPointSmoothSpeed);
	}
	else
	{
		SmoothedFocusPoint = FocusPoint;
	}

	CurrentTargetLocation = SmoothedFocusPoint;
	OutLocation = CurrentTargetLocation;
	return true;
}

bool UNamiDualFocusTargetCalculator::HasValidTarget_Implementation() const
{
	return PrimaryTarget.IsValid();
}

void UNamiDualFocusTargetCalculator::SetLockOnProvider(TScriptInterface<INamiLockOnTargetProvider> Provider)
{
	LockOnProvider = Provider;
	bLockedLocationInitialized = false;
}

TScriptInterface<INamiLockOnTargetProvider> UNamiDualFocusTargetCalculator::GetLockOnProvider_Implementation() const
{
	return LockOnProvider;
}

bool UNamiDualFocusTargetCalculator::HasValidLockedTarget() const
{
	const TScriptInterface<INamiLockOnTargetProvider> Provider = GetLockOnProvider();
	return Provider.GetInterface() && Provider->HasLockedTarget();
}

FVector UNamiDualFocusTargetCalculator::GetLockedTargetLocation() const
{
	const TScriptInterface<INamiLockOnTargetProvider> Provider = GetLockOnProvider();
	if (Provider.GetInterface() && Provider->HasLockedTarget())
	{
		return Provider->GetLockedFocusLocation();
	}
	return FVector::ZeroVector;
}

FVector UNamiDualFocusTargetCalculator::GetEffectiveLockedLocation() const
{
	if (bLockedLocationInitialized)
	{
		return SmoothedLockedLocation;
	}
	return GetLockedTargetLocation();
}

FVector UNamiDualFocusTargetCalculator::CalculateDualFocusPoint_Implementation(const FVector& PlayerLocation, const FVector& TargetLocation)
{
	// 归一化权重
	const float TotalWeight = PlayerFocusWeight + TargetFocusWeight;
	const float NormalizedPlayerWeight = TotalWeight > 0.0f ? PlayerFocusWeight / TotalWeight : 0.5f;

	// 在玩家和目标之间计算加权焦点
	return FMath::Lerp(TargetLocation, PlayerLocation, NormalizedPlayerWeight);
}
