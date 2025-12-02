// Fill out your copyright notice in the Description page of Project Settings.

#include "Libraries/NamiSpringArmLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiSpringArmLibrary)

void UNamiSpringArmLibrary::InitializeSpringArm(FNamiSpringArm& SpringArm)
{
	SpringArm.Initialize();
}

void UNamiSpringArmLibrary::TickSpringArm(
	FNamiSpringArm& SpringArm,
	UObject* WorldContext,
	float DeltaTime,
	AActor* IgnoreActor,
	const FTransform& InitialTransform,
	const FVector OffsetLocation)
{
	SpringArm.Tick(WorldContext, DeltaTime, IgnoreActor, InitialTransform, OffsetLocation);
}

void UNamiSpringArmLibrary::TickSpringArmMultiple(
	FNamiSpringArm& SpringArm,
	UObject* WorldContext,
	float DeltaTime,
	const TArray<AActor*>& IgnoreActors,
	const FTransform& InitialTransform,
	const FVector OffsetLocation)
{
	SpringArm.Tick(WorldContext, DeltaTime, IgnoreActors, InitialTransform, OffsetLocation);
}

FTransform UNamiSpringArmLibrary::GetCameraTransform(const FNamiSpringArm& SpringArm)
{
	return SpringArm.GetCameraTransform();
}

FVector UNamiSpringArmLibrary::GetSpringArmUnfixedCameraPosition(const FNamiSpringArm& SpringArm)
{
	return SpringArm.GetUnfixedCameraPosition();
}

bool UNamiSpringArmLibrary::IsSpringArmCollisionFixApplied(const FNamiSpringArm& SpringArm)
{
	return SpringArm.IsCollisionFixApplied();
}

