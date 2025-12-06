// Copyright Epic Games, Inc. All Rights Reserved.

#include "Features/NamiCameraFramingFeature.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraFramingFeature)

UNamiCameraFramingFeature::UNamiCameraFramingFeature()
{
	FeatureName = TEXT("Framing");
	Priority = 40; // 在 SpringArm 之前执行，提供目标中心与距离
}

void UNamiCameraFramingFeature::Activate_Implementation()
{
	Super::Activate_Implementation();
	LastCenter = FVector::ZeroVector;
	LastDistance = 300.0f;
}

void UNamiCameraFramingFeature::Deactivate_Implementation()
{
	Super::Deactivate_Implementation();
	Targets.Reset();
}

void UNamiCameraFramingFeature::SetTargets(const TArray<AActor*>& InTargets)
{
	Targets.Reset();
	for (AActor* Target : InTargets)
	{
		if (IsValid(Target))
		{
			Targets.Add(Target);
		}
	}
}

void UNamiCameraFramingFeature::ClearTargets()
{
	Targets.Reset();
}

void UNamiCameraFramingFeature::GatherTargets(TArray<FVector>& OutPositions) const
{
	for (const TWeakObjectPtr<AActor>& Target : Targets)
	{
		if (Target.IsValid())
		{
			OutPositions.Add(Target->GetActorLocation());
		}
	}
}

void UNamiCameraFramingFeature::ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime)
{
	Super::ApplyToView_Implementation(InOutView, DeltaTime);

	if (!IsEnabled() || !FramingConfig.bEnableFraming)
	{
		return;
	}

	TArray<FVector> Positions;
	GatherTargets(Positions);
	if (Positions.Num() == 0)
	{
		return;
	}

	// 计算中心与包围球半径
	FVector Center = FVector::ZeroVector;
	for (const FVector& Pos : Positions)
	{
		Center += Pos;
	}
	Center /= Positions.Num();

	float MaxRadius = 0.0f;
	for (const FVector& Pos : Positions)
	{
		MaxRadius = FMath::Max(MaxRadius, FVector::Dist(Pos, Center));
	}
	MaxRadius += ExtraRadiusPadding;

	// 使用当前 FOV 估算需要的距离以满足 DesiredScreenPercentage
	const float DesiredPercent = FMath::Clamp(FramingConfig.DesiredScreenPercentage, 0.05f, 0.8f);
	const float HalfFovRadians = FMath::DegreesToRadians(FMath::Max(1.0f, InOutView.FOV * 0.5f));
	float NeededDistance = MaxRadius / FMath::Tan(HalfFovRadians) / DesiredPercent;

	// 限制距离
	NeededDistance = FMath::Clamp(NeededDistance, FramingConfig.MinArmLength, FramingConfig.MaxArmLength);
	float TargetDistance = NeededDistance;

	// 插值平滑
	const float LerpAlpha = (FramingConfig.LerpSpeed > 0.0f) ? FMath::Clamp(DeltaTime * FramingConfig.LerpSpeed, 0.0f, 1.0f) : 1.0f;
	FVector SmoothedCenter = FMath::Lerp(LastCenter, Center + FVector(0, 0, FramingConfig.VerticalOffset), LerpAlpha);
	float SmoothedDistance = FMath::Lerp(LastDistance, TargetDistance, LerpAlpha);

	// 使用当前视角方向放置相机
	const FRotator CameraRot = InOutView.CameraRotation;
	const FVector Forward = CameraRot.Vector();
	InOutView.PivotLocation = SmoothedCenter;
	InOutView.CameraLocation = SmoothedCenter - Forward * SmoothedDistance;

	// 侧移偏好：沿右向量偏移
	if (!FMath::IsNearlyZero(FramingConfig.SideBias))
	{
		const FVector Right = FRotationMatrix(CameraRot).GetScaledAxis(EAxis::Y);
		InOutView.CameraLocation += Right * (SmoothedDistance * 0.1f * FramingConfig.SideBias);
	}

	LastCenter = SmoothedCenter;
	LastDistance = SmoothedDistance;
}

