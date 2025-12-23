// Copyright Qiu, Inc. All Rights Reserved.

#include "Calculators/Position/NamiEllipseOrbitPositionCalculator.h"

#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiEllipseOrbitPositionCalculator)

UNamiEllipseOrbitPositionCalculator::UNamiEllipseOrbitPositionCalculator()
{
	// 禁用基类的控制旋转
	bUseControlRotation = false;
}

void UNamiEllipseOrbitPositionCalculator::Activate_Implementation()
{
	Super::Activate_Implementation();

	CurrentOrbitAngle = DefaultOrbitAngle;
	TargetOrbitAngle = DefaultOrbitAngle;
}

FVector UNamiEllipseOrbitPositionCalculator::CalculateCameraPosition_Implementation(
	const FVector& PivotLocation,
	const FRotator& ControlRotation,
	float DeltaTime)
{
	// 获取玩家和目标位置
	FVector PlayerLocation = GetPlayerLocation();
	FVector TargetLocation = HasValidLockedTarget() ? GetLockedTargetLocation() : PlayerLocation;

	// 计算椭圆轨道位置
	FVector TargetPosition = CalculateEllipsePosition(PivotLocation, PlayerLocation, TargetLocation, DeltaTime);

	// 首帧初始化
	if (!bFirstFrameProcessed)
	{
		CurrentCameraPosition = TargetPosition;
		bFirstFrameProcessed = true;
		return TargetPosition;
	}

	// 平滑过渡
	if (PositionSmoothSpeed > 0.0f && DeltaTime > 0.0f)
	{
		CurrentCameraPosition = FMath::VInterpTo(CurrentCameraPosition, TargetPosition, DeltaTime, PositionSmoothSpeed);
	}
	else
	{
		CurrentCameraPosition = TargetPosition;
	}

	return CurrentCameraPosition;
}

void UNamiEllipseOrbitPositionCalculator::SetPrimaryTarget(AActor* Target)
{
	PrimaryTarget = Target;
}

void UNamiEllipseOrbitPositionCalculator::SetLockOnProvider(TScriptInterface<INamiLockOnTargetProvider> Provider)
{
	LockOnProvider = Provider;
}

void UNamiEllipseOrbitPositionCalculator::AddOrbitInput(float DeltaAngle)
{
	if (bEnablePlayerInput)
	{
		TargetOrbitAngle += DeltaAngle * InputSensitivity;

		if (bClampOrbitAngle)
		{
			TargetOrbitAngle = FMath::Clamp(TargetOrbitAngle, -MaxOrbitAngle, MaxOrbitAngle);
		}
	}
}

void UNamiEllipseOrbitPositionCalculator::SetTargetOrbitAngle(float Angle)
{
	TargetOrbitAngle = Angle;
	if (bClampOrbitAngle)
	{
		TargetOrbitAngle = FMath::Clamp(TargetOrbitAngle, -MaxOrbitAngle, MaxOrbitAngle);
	}
}

FVector UNamiEllipseOrbitPositionCalculator::CalculateEllipsePosition_Implementation(
	const FVector& PivotLocation,
	const FVector& PlayerLocation,
	const FVector& TargetLocation,
	float DeltaTime)
{
	// 平滑轨道角度
	if (OrbitAngleSmoothSpeed > 0.0f && DeltaTime > 0.0f)
	{
		CurrentOrbitAngle = FMath::FInterpTo(CurrentOrbitAngle, TargetOrbitAngle, DeltaTime, OrbitAngleSmoothSpeed);
	}
	else
	{
		CurrentOrbitAngle = TargetOrbitAngle;
	}

	// 计算玩家到目标的方向（水平面）
	FVector PlayerToTarget = TargetLocation - PlayerLocation;
	PlayerToTarget.Z = 0.0f;
	const float CharacterDistance = PlayerToTarget.Size();
	PlayerToTarget.Normalize();

	// 计算侧向方向
	FVector SideDirection = FVector::CrossProduct(FVector::UpVector, PlayerToTarget);
	SideDirection.Normalize();

	// 计算相机方向
	const float AngleRad = FMath::DegreesToRadians(CurrentOrbitAngle);
	FVector CameraDirection = PlayerToTarget * FMath::Cos(AngleRad) + SideDirection * FMath::Sin(AngleRad);

	// 计算椭圆参数
	float a = EllipseMajorRadius;
	float b = EllipseMinorRadius;

	// 自适应缩放
	if (bEnableAdaptiveDistance && CharacterDistance > KINDA_SMALL_NUMBER)
	{
		const float EllipseScale = FMath::Clamp(
			CharacterDistance / AdaptiveDistanceBase * EllipseScaleFactor,
			MinEllipseScale,
			MaxEllipseScale
		);
		a *= EllipseScale;
		b *= EllipseScale;
	}

	// 椭圆公式计算距离
	// r(θ) = a * b / sqrt((b*sin(θ))² + (a*cos(θ))²)
	const float CosTheta = FMath::Cos(AngleRad);
	const float SinTheta = FMath::Sin(AngleRad);
	const float Denominator = FMath::Sqrt(
		FMath::Square(b * SinTheta) + FMath::Square(a * CosTheta)
	);
	float EllipseDistance = (a * b) / FMath::Max(Denominator, KINDA_SMALL_NUMBER);

	// 限制距离范围
	EllipseDistance = FMath::Clamp(EllipseDistance, MinCameraDistance, MaxCameraDistance);

	// 计算相机位置
	FVector CameraPosition = PivotLocation - CameraDirection * EllipseDistance;
	CameraPosition.Z = PivotLocation.Z + HeightOffset;

	return CameraPosition;
}

FVector UNamiEllipseOrbitPositionCalculator::GetPlayerLocation() const
{
	if (AActor* Target = PrimaryTarget.Get())
	{
		return Target->GetActorLocation();
	}
	return FVector::ZeroVector;
}

FVector UNamiEllipseOrbitPositionCalculator::GetLockedTargetLocation() const
{
	if (LockOnProvider.GetInterface() && LockOnProvider->HasLockedTarget())
	{
		return LockOnProvider->GetLockedFocusLocation();
	}
	return FVector::ZeroVector;
}

bool UNamiEllipseOrbitPositionCalculator::HasValidLockedTarget() const
{
	return LockOnProvider.GetInterface() && LockOnProvider->HasLockedTarget();
}
