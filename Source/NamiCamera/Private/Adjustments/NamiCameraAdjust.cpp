// Copyright Qiu, Inc. All Rights Reserved.

#include "Adjustments/NamiCameraAdjust.h"
#include "Components/NamiCameraComponent.h"
#include "Core/LogNamiCamera.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

UNamiCameraAdjust::UNamiCameraAdjust()
	: BlendInTime(0.3f)
	  , BlendOutTime(0.3f)
	  , BlendType(ENamiCameraBlendType::EaseInOut)
	  , BlendCurve(nullptr)
	  , BlendMode(ENamiCameraAdjustBlendMode::Additive)
	  , Priority(0)
	  , CurrentBlendWeight(0.f)
	  , BlendTimer(0.f)
	  , State(ENamiCameraAdjustState::Inactive)
	  , ActiveTime(0.f)
	  , CustomInputValue(0.f)
	  , bInputInterrupted(false)
{
}

void UNamiCameraAdjust::Initialize(UNamiCameraComponent* InOwnerComponent)
{
	OwnerComponent = InOwnerComponent;
	State = ENamiCameraAdjustState::Inactive;
	CurrentBlendWeight = 0.f;
	BlendTimer = 0.f;
	ActiveTime = 0.f;
	bInputInterrupted = false;
}

void UNamiCameraAdjust::OnActivate_Implementation()
{
}

void UNamiCameraAdjust::Tick_Implementation(float DeltaTime)
{
}

void UNamiCameraAdjust::OnDeactivate_Implementation()
{
	bInputInterrupted = false;
}

void UNamiCameraAdjust::SetStaticParams(const FNamiCameraAdjustParams& InParams)
{
	StaticParams = InParams;
	bUseStaticParams = true;
}

FNamiCameraAdjustParams UNamiCameraAdjust::CalculateAdjustParams_Implementation(float DeltaTime)
{
	if (bUseStaticParams)
	{
		return StaticParams;
	}
	return FNamiCameraAdjustParams();
}

FNamiCameraAdjustParams UNamiCameraAdjust::GetWeightedAdjustParams(float DeltaTime)
{
	UpdateBlending(DeltaTime);

	if (State != ENamiCameraAdjustState::Inactive)
	{
		ActiveTime += DeltaTime;
	}

	Tick(DeltaTime);

	FNamiCameraAdjustParams Params = CalculateAdjustParams(DeltaTime);

	ApplyCurveDrivenParams(Params);

	return Params.ScaleAdditiveParamsByWeight(CurrentBlendWeight);
}

void UNamiCameraAdjust::RequestDeactivate(bool bForceImmediate)
{
	if (State == ENamiCameraAdjustState::Inactive)
	{
		return;
	}

	if (bForceImmediate || BlendOutTime <= 0.f)
	{
		State = ENamiCameraAdjustState::Inactive;
		CurrentBlendWeight = 0.f;
		OnDeactivate();
	}
	else
	{
		State = ENamiCameraAdjustState::BlendingOut;
		BlendTimer = BlendOutTime * CurrentBlendWeight;
		bBlendOutSynced = false;
	}
}

void UNamiCameraAdjust::SetCustomInput(float Value)
{
	CustomInputValue = Value;
}

void UNamiCameraAdjust::TriggerInputInterrupt()
{
	if (!bInputInterrupted)
	{
		bInputInterrupted = true;

		OnInputInterrupted.Broadcast();

		RequestDeactivate();

		bBlendOutSynced = true;
	}
}

bool UNamiCameraAdjust::IsActive() const
{
	return State == ENamiCameraAdjustState::BlendingIn ||
		State == ENamiCameraAdjustState::Active ||
		State == ENamiCameraAdjustState::BlendingOut;
}

UNamiCameraComponent* UNamiCameraAdjust::GetOwnerComponent() const
{
	return OwnerComponent.Get();
}

void UNamiCameraAdjust::CacheArmRotationTarget()
{
	UNamiCameraComponent* CameraComp = GetOwnerComponent();
	if (!CameraComp)
	{
		return;
	}

	APawn* OwnerPawn = CameraComp->GetOwnerPawn();
	if (!OwnerPawn)
	{
		return;
	}

	FRotator ActorForwardRotation = OwnerPawn->GetActorRotation();

	CachedWorldArmRotationTarget = ActorForwardRotation + ArmRotationTarget;
	CachedWorldArmRotationTarget.Normalize();

	UE_LOG(LogNamiCamera, Log, TEXT("[CacheArmRotationTarget] ActorForward: P=%.2f Y=%.2f, Target: P=%.2f Y=%.2f, Result: P=%.2f Y=%.2f"),
	       ActorForwardRotation.Pitch, ActorForwardRotation.Yaw,
	       ArmRotationTarget.Pitch, ArmRotationTarget.Yaw,
	       CachedWorldArmRotationTarget.Pitch, CachedWorldArmRotationTarget.Yaw);
}

void UNamiCameraAdjust::UpdateBlending(float DeltaTime)
{
	switch (State)
	{
	case ENamiCameraAdjustState::Inactive:
		State = ENamiCameraAdjustState::BlendingIn;
		BlendTimer = 0.f;
		ActiveTime = 0.f;
		CacheArmRotationTarget();
		OnActivate();

		if (BlendInTime <= 0.f)
		{
			CurrentBlendWeight = 1.f;
			State = ENamiCameraAdjustState::Active;
		}
		else
		{
			BlendTimer += DeltaTime;
			float LinearAlpha = FMath::Clamp(BlendTimer / BlendInTime, 0.f, 1.f);
			CurrentBlendWeight = CalculateBlendAlpha(LinearAlpha);

			if (LinearAlpha >= 1.f)
			{
				State = ENamiCameraAdjustState::Active;
			}
		}
		break;
	case ENamiCameraAdjustState::BlendingIn:
		if (BlendInTime <= 0.f)
		{
			CurrentBlendWeight = 1.f;
			State = ENamiCameraAdjustState::Active;
		}
		else
		{
			BlendTimer += DeltaTime;
			float LinearAlpha = FMath::Clamp(BlendTimer / BlendInTime, 0.f, 1.f);
			CurrentBlendWeight = CalculateBlendAlpha(LinearAlpha);

			if (LinearAlpha >= 1.f)
			{
				State = ENamiCameraAdjustState::Active;
			}
		}
		break;

	case ENamiCameraAdjustState::Active:
		CurrentBlendWeight = 1.f;
		break;

	case ENamiCameraAdjustState::BlendingOut:
		if (BlendOutTime <= 0.f)
		{
			CurrentBlendWeight = 0.f;
			State = ENamiCameraAdjustState::Inactive;
			OnDeactivate();
		}
		else
		{
			BlendTimer -= DeltaTime;
			float LinearAlpha = FMath::Clamp(BlendTimer / BlendOutTime, 0.f, 1.f);
			CurrentBlendWeight = CalculateBlendAlpha(LinearAlpha);

			if (LinearAlpha <= 0.f)
			{
				State = ENamiCameraAdjustState::Inactive;
				OnDeactivate();
			}
		}
		break;
	}
}

float UNamiCameraAdjust::CalculateBlendAlpha(float LinearAlpha) const
{
	switch (BlendType)
	{
	case ENamiCameraBlendType::Linear:
		return LinearAlpha;

	case ENamiCameraBlendType::EaseIn:
		return FMath::InterpEaseIn(0.f, 1.f, LinearAlpha, 2.f);

	case ENamiCameraBlendType::EaseOut:
		return FMath::InterpEaseOut(0.f, 1.f, LinearAlpha, 2.f);

	case ENamiCameraBlendType::EaseInOut:
		return FMath::InterpEaseInOut(0.f, 1.f, LinearAlpha, 2.f);

	case ENamiCameraBlendType::CustomCurve:
		if (BlendCurve)
		{
			return BlendCurve->GetFloatValue(LinearAlpha);
		}
		return LinearAlpha;

	default:
		return LinearAlpha;
	}
}

float UNamiCameraAdjust::GetInputSourceValue(ENamiCameraAdjustInputSource Source) const
{
	switch (Source)
	{
	case ENamiCameraAdjustInputSource::None:
		return 0.f;

	case ENamiCameraAdjustInputSource::MoveSpeed:
		{
			UNamiCameraComponent* CameraComp = GetOwnerComponent();
			if (!CameraComp)
			{
				return 0.f;
			}

			APawn* OwnerPawn = CameraComp->GetOwnerPawn();
			if (!OwnerPawn)
			{
				return 0.f;
			}

			if (ACharacter* Character = Cast<ACharacter>(OwnerPawn))
			{
				if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
				{
					return MovementComp->Velocity.Size();
				}
			}

			return OwnerPawn->GetVelocity().Size();
		}

	case ENamiCameraAdjustInputSource::LookSpeed:
		{
			UNamiCameraComponent* CameraComp = GetOwnerComponent();
			if (!CameraComp)
			{
				return 0.f;
			}

			APlayerController* PC = CameraComp->GetOwnerPlayerController();
			if (!PC)
			{
				return 0.f;
			}

			float TurnInput = 0.f;
			float LookInput = 0.f;
			PC->GetInputMouseDelta(TurnInput, LookInput);

			return FMath::Sqrt(TurnInput * TurnInput + LookInput * LookInput);
		}

	case ENamiCameraAdjustInputSource::Time:
		return ActiveTime;

	case ENamiCameraAdjustInputSource::Custom:
		return CustomInputValue;

	default:
		return 0.f;
	}
}

float UNamiCameraAdjust::EvaluateCurveBinding(const FNamiCameraAdjustCurveBinding& Binding) const
{
	if (!Binding.IsValid())
	{
		return Binding.OutputOffset;
	}

	float InputValue = GetInputSourceValue(Binding.InputSource);
	return Binding.Evaluate(InputValue);
}

void UNamiCameraAdjust::ApplyCurveDrivenParams(FNamiCameraAdjustParams& OutParams) const
{
	if (CurveConfig.FOVBinding.IsValid())
	{
		OutParams.FOVOffset += EvaluateCurveBinding(CurveConfig.FOVBinding);
		OutParams.MarkFOVModified();
	}

	if (CurveConfig.ArmLengthBinding.IsValid())
	{
		OutParams.TargetArmLengthOffset += EvaluateCurveBinding(CurveConfig.ArmLengthBinding);
		OutParams.MarkTargetArmLengthModified();
	}

	if (CurveConfig.CameraOffsetXBinding.IsValid())
	{
		OutParams.CameraLocationOffset.X += EvaluateCurveBinding(CurveConfig.CameraOffsetXBinding);
		OutParams.MarkCameraLocationOffsetModified();
	}

	if (CurveConfig.CameraOffsetYBinding.IsValid())
	{
		OutParams.CameraLocationOffset.Y += EvaluateCurveBinding(CurveConfig.CameraOffsetYBinding);
		OutParams.MarkCameraLocationOffsetModified();
	}

	if (CurveConfig.CameraOffsetZBinding.IsValid())
	{
		OutParams.CameraLocationOffset.Z += EvaluateCurveBinding(CurveConfig.CameraOffsetZBinding);
		OutParams.MarkCameraLocationOffsetModified();
	}
}
