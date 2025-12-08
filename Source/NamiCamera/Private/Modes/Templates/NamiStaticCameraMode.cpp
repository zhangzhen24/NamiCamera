// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modes/Templates/NamiStaticCameraMode.h"
#include "Libraries/NamiCameraMath.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiStaticCameraMode)

UNamiStaticCameraMode::UNamiStaticCameraMode()
{
	SmoothedRotation = FRotator::ZeroRotator;
}

void UNamiStaticCameraMode::Activate_Implementation()
{
	Super::Activate_Implementation();

	// 初始化平滑旋转
	if (bUseLookAt)
	{
		SmoothedRotation = CalculateLookAtRotation();
	}
	else
	{
		SmoothedRotation = CameraRotation;
	}

	YawVelocity = 0.0f;
	PitchVelocity = 0.0f;
}

FNamiCameraView UNamiStaticCameraMode::CalculateView_Implementation(float DeltaTime)
{
	FNamiCameraView View;

	// 设置位置
	View.CameraLocation = CameraLocation;

	// 计算旋转
	FRotator TargetRotation;
	if (bUseLookAt)
	{
		TargetRotation = CalculateLookAtRotation();
	}
	else
	{
		TargetRotation = CameraRotation;
	}

	// 平滑旋转
	if (LookAtSmoothTime > 0.0f && DeltaTime > 0.0f)
	{
		SmoothedRotation = FNamiCameraMath::SmoothDampRotator(
			SmoothedRotation,
			TargetRotation,
			YawVelocity,
			PitchVelocity,
			LookAtSmoothTime,
			LookAtSmoothTime,
			DeltaTime);
	}
	else
	{
		SmoothedRotation = TargetRotation;
	}

	// 使用0-360度归一化，避免180/-180跳变问题
	View.CameraRotation = FNamiCameraMath::NormalizeRotatorTo360(SmoothedRotation);

	// 设置枢轴点
	if (bUseLookAt)
	{
		if (LookAtTarget.IsValid())
		{
			View.PivotLocation = LookAtTarget->GetActorLocation();
		}
		else
		{
			View.PivotLocation = LookAtLocation;
		}
	}
	else
	{
		// 没有注视目标时，枢轴点在相机前方
		View.PivotLocation = CameraLocation + SmoothedRotation.Vector() * 500.0f;
	}

	View.FOV = DefaultFOV;

	return View;
}

void UNamiStaticCameraMode::SetCameraLocation(const FVector& Location)
{
	CameraLocation = Location;
}

void UNamiStaticCameraMode::SetCameraRotation(const FRotator& Rotation)
{
	CameraRotation = Rotation;
	if (!bUseLookAt)
	{
		SmoothedRotation = Rotation;
	}
}

void UNamiStaticCameraMode::SetCameraTransform(const FTransform& Transform)
{
	CameraLocation = Transform.GetLocation();
	CameraRotation = Transform.GetRotation().Rotator();
	if (!bUseLookAt)
	{
		SmoothedRotation = CameraRotation;
	}
}

void UNamiStaticCameraMode::SetCameraFromActor(AActor* Actor)
{
	if (IsValid(Actor))
	{
		SetCameraTransform(Actor->GetActorTransform());
	}
}

void UNamiStaticCameraMode::SetLookAtTarget(AActor* Target)
{
	LookAtTarget = Target;
	bUseLookAt = IsValid(Target);
}

void UNamiStaticCameraMode::SetLookAtLocation(const FVector& Location)
{
	LookAtLocation = Location;
	LookAtTarget.Reset();
	bUseLookAt = true;
}

void UNamiStaticCameraMode::ClearLookAtTarget()
{
	LookAtTarget.Reset();
	LookAtLocation = FVector::ZeroVector;
	bUseLookAt = false;
}

FRotator UNamiStaticCameraMode::CalculateLookAtRotation() const
{
	FVector TargetLocation;
	if (LookAtTarget.IsValid())
	{
		TargetLocation = LookAtTarget->GetActorLocation();
	}
	else
	{
		TargetLocation = LookAtLocation;
	}

	FVector Direction = TargetLocation - CameraLocation;
	if (Direction.IsNearlyZero())
	{
		return CameraRotation;
	}

	// 使用0-360度归一化，避免180/-180跳变问题
	FRotator Rotation = Direction.Rotation();
	return FNamiCameraMath::NormalizeRotatorTo360(Rotation);
}

