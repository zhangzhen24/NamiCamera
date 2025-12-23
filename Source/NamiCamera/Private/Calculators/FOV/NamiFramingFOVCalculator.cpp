// Copyright Qiu, Inc. All Rights Reserved.

#include "Calculators/FOV/NamiFramingFOVCalculator.h"

#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiFramingFOVCalculator)

UNamiFramingFOVCalculator::UNamiFramingFOVCalculator()
{
	BaseFOV = 80.0f;
	MinFOV = 60.0f;
	MaxFOV = 100.0f;
}

float UNamiFramingFOVCalculator::CalculateFOV_Implementation(
	const FVector& CameraLocation,
	const FVector& PivotLocation,
	float DeltaTime)
{
	float TargetFOV = BaseFOV;

	// 如果有锁定目标且需要保持双方在画面内
	if (bKeepBothInFrame && HasValidLockedTarget())
	{
		FVector PlayerLocation = GetPlayerLocation();
		FVector TargetLocation = GetLockedTargetLocation();

		TargetFOV = CalculateFramingFOV(CameraLocation, PlayerLocation, TargetLocation);
	}

	// 限制范围
	TargetFOV = FMath::Clamp(TargetFOV, MinFOV, MaxFOV);

	// 首帧直接设置
	if (!bFirstFrameProcessed)
	{
		CurrentFOV = TargetFOV;
		bFirstFrameProcessed = true;
		return TargetFOV;
	}

	// 平滑过渡
	if (FOVTransitionSpeed > 0.0f && DeltaTime > 0.0f)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, FOVTransitionSpeed);
	}
	else
	{
		CurrentFOV = TargetFOV;
	}

	return CurrentFOV;
}

void UNamiFramingFOVCalculator::SetPrimaryTarget(AActor* Target)
{
	PrimaryTarget = Target;
}

void UNamiFramingFOVCalculator::SetLockOnProvider(TScriptInterface<INamiLockOnTargetProvider> Provider)
{
	LockOnProvider = Provider;
}

float UNamiFramingFOVCalculator::CalculateFramingFOV_Implementation(
	const FVector& CameraLocation,
	const FVector& PlayerLocation,
	const FVector& TargetLocation)
{
	// 计算相机到玩家和目标的方向
	const FVector ToPlayer = PlayerLocation - CameraLocation;
	const FVector ToTarget = TargetLocation - CameraLocation;

	// 计算两个方向之间的角度
	const float DotProduct = FVector::DotProduct(ToPlayer.GetSafeNormal(), ToTarget.GetSafeNormal());
	const float AngleBetween = FMath::Acos(FMath::Clamp(DotProduct, -1.0f, 1.0f));
	const float AngleDegrees = FMath::RadiansToDegrees(AngleBetween);

	// 考虑边缘留白，计算所需的 FOV
	// FOV 需要大于角度差，加上边缘留白
	const float RequiredFOV = AngleDegrees * (1.0f + FramePadding * 2.0f);

	return RequiredFOV;
}

FVector UNamiFramingFOVCalculator::GetPlayerLocation() const
{
	if (AActor* Target = PrimaryTarget.Get())
	{
		return Target->GetActorLocation();
	}
	return FVector::ZeroVector;
}

FVector UNamiFramingFOVCalculator::GetLockedTargetLocation() const
{
	if (LockOnProvider.GetInterface() && LockOnProvider->HasLockedTarget())
	{
		return LockOnProvider->GetLockedFocusLocation();
	}
	return FVector::ZeroVector;
}

bool UNamiFramingFOVCalculator::HasValidLockedTarget() const
{
	return LockOnProvider.GetInterface() && LockOnProvider->HasLockedTarget();
}
