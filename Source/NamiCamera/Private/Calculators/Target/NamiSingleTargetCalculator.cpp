// Copyright Qiu, Inc. All Rights Reserved.

#include "Calculators/Target/NamiSingleTargetCalculator.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiSingleTargetCalculator)

UNamiSingleTargetCalculator::UNamiSingleTargetCalculator()
{
	SmoothedLocation = FVector::ZeroVector;
}

bool UNamiSingleTargetCalculator::CalculateTargetLocation_Implementation(float DeltaTime, FVector& OutLocation)
{
	AActor* Target = PrimaryTarget.Get();
	if (!Target)
	{
		OutLocation = CurrentTargetLocation;
		return false;
	}

	// 获取目标位置
	FVector TargetLocation;
	if (bUseTargetEyesLocation)
	{
		if (APawn* TargetPawn = Cast<APawn>(Target))
		{
			TargetLocation = TargetPawn->GetPawnViewLocation();
		}
		else
		{
			TargetLocation = Target->GetActorLocation();
		}
	}
	else
	{
		TargetLocation = Target->GetActorLocation();
	}

	// 应用偏移
	if (!TargetOffset.IsNearlyZero())
	{
		FVector Offset = TargetOffset;
		if (bUseTargetRotation)
		{
			FRotator TargetRotation = Target->GetActorRotation();
			if (bUseYawOnly)
			{
				TargetRotation.Pitch = 0.0f;
				TargetRotation.Roll = 0.0f;
			}
			Offset = TargetRotation.RotateVector(Offset);
		}
		TargetLocation += Offset;
	}

	// 应用平滑
	if (LocationSmoothSpeed > 0.0f)
	{
		if (!bLocationInitialized)
		{
			SmoothedLocation = TargetLocation;
			bLocationInitialized = true;
		}
		else
		{
			SmoothedLocation = FMath::VInterpTo(SmoothedLocation, TargetLocation, DeltaTime, LocationSmoothSpeed);
		}
		CurrentTargetLocation = SmoothedLocation;
	}
	else
	{
		CurrentTargetLocation = TargetLocation;
	}

	OutLocation = CurrentTargetLocation;
	return true;
}
