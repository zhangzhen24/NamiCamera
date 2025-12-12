// Copyright Epic Games, Inc. All Rights Reserved.

#include "Adjusts/NamiCameraAdjust.h"
#include "Components/NamiCameraComponent.h"
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
{
}

void UNamiCameraAdjust::Initialize(UNamiCameraComponent* InOwnerComponent)
{
	OwnerComponent = InOwnerComponent;
	State = ENamiCameraAdjustState::Inactive;
	CurrentBlendWeight = 0.f;
	BlendTimer = 0.f;
	ActiveTime = 0.f;
}

void UNamiCameraAdjust::OnActivate_Implementation()
{
	// 默认实现为空，子类可重写
}

void UNamiCameraAdjust::Tick_Implementation(float DeltaTime)
{
	// 默认实现为空，子类可重写
}

void UNamiCameraAdjust::OnDeactivate_Implementation()
{
	// 默认实现为空，子类可重写
}

FNamiCameraAdjustParams UNamiCameraAdjust::CalculateAdjustParams_Implementation(float DeltaTime)
{
	// 默认返回空参数，子类应重写
	return FNamiCameraAdjustParams();
}

FNamiCameraAdjustParams UNamiCameraAdjust::GetWeightedAdjustParams(float DeltaTime)
{
	// 更新混合状态
	UpdateBlending(DeltaTime);

	// 更新激活时间
	if (State != ENamiCameraAdjustState::Inactive)
	{
		ActiveTime += DeltaTime;
	}

	// 调用 Tick
	Tick(DeltaTime);

	// 获取原始参数
	FNamiCameraAdjustParams Params = CalculateAdjustParams(DeltaTime);

	// 应用曲线驱动的参数
	ApplyCurveDrivenParams(Params);

	// 根据混合模式和权重处理参数
	switch (BlendMode)
	{
	case ENamiCameraAdjustBlendMode::Additive:
	case ENamiCameraAdjustBlendMode::Multiplicative:
		// Additive和Multiplicative模式：缩放偏移值
		return Params.ScaleByWeight(CurrentBlendWeight);

	case ENamiCameraAdjustBlendMode::Override:
		// Override模式：参数不变，权重在应用时处理
		return Params;

	default:
		return Params.ScaleByWeight(CurrentBlendWeight);
	}
}

void UNamiCameraAdjust::RequestDeactivate(bool bForceImmediate)
{
	if (State == ENamiCameraAdjustState::Inactive)
	{
		return;
	}

	if (bForceImmediate || BlendOutTime <= 0.f)
	{
		// 立即停用
		State = ENamiCameraAdjustState::Inactive;
		CurrentBlendWeight = 0.f;
		OnDeactivate();
	}
	else
	{
		// 开始混合退出
		State = ENamiCameraAdjustState::BlendingOut;
		BlendTimer = BlendOutTime * CurrentBlendWeight; // 从当前权重开始退出
	}
}

void UNamiCameraAdjust::SetCustomInput(float Value)
{
	CustomInputValue = Value;
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
	// 只在 Override 模式下缓存目标臂旋转
	if (BlendMode != ENamiCameraAdjustBlendMode::Override)
	{
		return;
	}

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

	// 获取角色 Mesh 朝向（而不是 Actor 朝向）
	// Override 模式的目标是相对于角色 Mesh 空间的：
	// - Yaw = 0°：角色正前方
	// - Yaw = 180°：角色后方（默认相机位置）
	// - Yaw = 45°：角色右前方
	// - Pitch > 0：俯视
	FRotator MeshRotation = FRotator::ZeroRotator;
	if (ACharacter* Character = Cast<ACharacter>(OwnerPawn))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			MeshRotation = Mesh->GetComponentRotation();
		}
		else
		{
			MeshRotation = OwnerPawn->GetActorRotation();
		}
	}
	else
	{
		MeshRotation = OwnerPawn->GetActorRotation();
	}

	// 计算世界空间的目标臂旋转
	// ArmRotationTarget 是相对于角色 Mesh 朝向的偏移
	CachedWorldArmRotationTarget = MeshRotation + ArmRotationTarget;
	CachedWorldArmRotationTarget.Normalize();
}

void UNamiCameraAdjust::UpdateBlending(float DeltaTime)
{
	switch (State)
	{
	case ENamiCameraAdjustState::Inactive:
		// 开始激活
		State = ENamiCameraAdjustState::BlendingIn;
		BlendTimer = 0.f;
		ActiveTime = 0.f;
		CacheArmRotationTarget();  // 在激活时刻缓存目标臂旋转
		OnActivate();
		// Fall through to BlendingIn
		[[fallthrough]];

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

			// 尝试获取Character的MovementComponent
			if (ACharacter* Character = Cast<ACharacter>(OwnerPawn))
			{
				if (UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement())
				{
					return MovementComp->Velocity.Size();
				}
			}

			// 回退到直接获取速度
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

			// 获取输入旋转速度
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
	// FOV曲线
	if (CurveConfig.FOVBinding.IsValid())
	{
		OutParams.FOVOffset += EvaluateCurveBinding(CurveConfig.FOVBinding);
		OutParams.MarkFOVModified();
	}

	// 臂长曲线
	if (CurveConfig.ArmLengthBinding.IsValid())
	{
		OutParams.TargetArmLengthOffset += EvaluateCurveBinding(CurveConfig.ArmLengthBinding);
		OutParams.MarkTargetArmLengthModified();
	}

	// 相机位置偏移曲线
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
