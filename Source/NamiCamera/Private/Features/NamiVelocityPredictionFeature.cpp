// Copyright Epic Games, Inc. All Rights Reserved.

#include "Features/NamiVelocityPredictionFeature.h"
#include "Modes/Templates/NamiFollowCameraMode.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PrimitiveComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiVelocityPredictionFeature)

UNamiVelocityPredictionFeature::UNamiVelocityPredictionFeature()
{
	FeatureName = TEXT("VelocityPrediction");
	Priority = 100;  // 较高优先级，在其他 Feature 之前执行
}

void UNamiVelocityPredictionFeature::Update_Implementation(float DeltaTime)
{
	Super::Update_Implementation(DeltaTime);

	if (!IsEnabled() || !bEnablePrediction)
	{
		CurrentVelocity = FVector::ZeroVector;
		CurrentSpeed = 0.0f;
		return;
	}

	// 获取目标速度
	CurrentVelocity = GetTargetVelocity();
	
	// 如果只使用水平速度，移除垂直分量
	if (bUseHorizontalVelocityOnly)
	{
		CurrentVelocity.Z = 0.0f;
	}
	
	CurrentSpeed = CurrentVelocity.Size();
}

void UNamiVelocityPredictionFeature::ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime)
{
	Super::ApplyToView_Implementation(InOutView, DeltaTime);

	if (!IsEnabled() || !bEnablePrediction)
	{
		return;
	}

	// 如果速度低于阈值，不进行预测
	if (CurrentSpeed < MinVelocityThreshold)
	{
		return;
	}

	// 计算预测强度（0-1）
	float PredictionStrength = 1.0f;
	
	if (MaxVelocityThreshold > MinVelocityThreshold)
	{
		// 归一化速度（0-1）
		float NormalizedSpeed = FMath::Clamp(
			(CurrentSpeed - MinVelocityThreshold) / (MaxVelocityThreshold - MinVelocityThreshold),
			0.0f, 1.0f);

		// 如果有曲线，使用曲线调整强度
		if (IsValid(PredictionStrengthCurve))
		{
			PredictionStrength = PredictionStrengthCurve->GetFloatValue(NormalizedSpeed);
		}
		else
		{
			// 默认线性映射
			PredictionStrength = NormalizedSpeed;
		}
	}

	// 计算预测偏移
	FVector PredictedOffset = CurrentVelocity * PredictionTime * PredictionStrength;

	// 修改 PivotLocation（提前观察目标移动方向）
	InOutView.PivotLocation += PredictedOffset;

	// 更新相机旋转，让它看向新的 PivotLocation
	FVector ToNewPivot = InOutView.PivotLocation - InOutView.CameraLocation;
	if (!ToNewPivot.IsNearlyZero())
	{
		FRotator NewRotation = ToNewPivot.Rotation();
		
		// 如果启用了"保持水平旋转"，保持原有的 Yaw 和 Roll，只更新 Pitch
		if (bPreserveYawRotation)
		{
			FRotator CurrentRotation = InOutView.CameraRotation;
			NewRotation.Yaw = CurrentRotation.Yaw;
			NewRotation.Roll = CurrentRotation.Roll;
		}
		// 如果禁用，允许相机自由旋转（适合第三人称相机）
		
		InOutView.CameraRotation = NewRotation;
	}
}

FVector UNamiVelocityPredictionFeature::GetTargetVelocity() const
{
	UNamiCameraModeBase* OwnerMode = GetCameraMode();
	if (!IsValid(OwnerMode))
	{
		return FVector::ZeroVector;
	}

	// 尝试从 FollowCameraMode 获取主目标
	UNamiFollowCameraMode* FollowMode = Cast<UNamiFollowCameraMode>(OwnerMode);
	if (!IsValid(FollowMode))
	{
		return FVector::ZeroVector;
	}

	AActor* PrimaryTarget = FollowMode->GetPrimaryTarget();
	if (!IsValid(PrimaryTarget))
	{
		return FVector::ZeroVector;
	}

	// 尝试从 Character 获取速度
	if (ACharacter* Character = Cast<ACharacter>(PrimaryTarget))
	{
		if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
		{
			return MovementComp->Velocity;
		}
	}

	// 如果不是 Character，尝试从 RootComponent 获取速度
	if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(PrimaryTarget->GetRootComponent()))
	{
		return PrimitiveComp->GetComponentVelocity();
	}

	return FVector::ZeroVector;
}

