// Copyright Qiu, Inc. All Rights Reserved.

#include "Calculators/NamiCameraTargetCalculator.h"
#include "CameraModes/NamiCameraModeBase.h"

UNamiCameraTargetCalculator::UNamiCameraTargetCalculator()
{
	CurrentTargetLocation = FVector::ZeroVector;
}

bool UNamiCameraTargetCalculator::CalculateTargetLocation_Implementation(float DeltaTime, FVector& OutLocation)
{
	// 默认实现：使用主要目标的位置
	if (AActor* Target = PrimaryTarget.Get())
	{
		OutLocation = Target->GetActorLocation();
		CurrentTargetLocation = OutLocation;
		return true;
	}
	OutLocation = CurrentTargetLocation;
	return false;
}

AActor* UNamiCameraTargetCalculator::GetPrimaryTargetActor_Implementation() const
{
	return PrimaryTarget.Get();
}

FRotator UNamiCameraTargetCalculator::GetPrimaryTargetRotation_Implementation() const
{
	if (AActor* Target = PrimaryTarget.Get())
	{
		return Target->GetActorRotation();
	}
	return FRotator::ZeroRotator;
}

bool UNamiCameraTargetCalculator::HasValidTarget_Implementation() const
{
	return PrimaryTarget.IsValid();
}

void UNamiCameraTargetCalculator::SetPrimaryTarget(AActor* Target)
{
	PrimaryTarget = Target;
}
