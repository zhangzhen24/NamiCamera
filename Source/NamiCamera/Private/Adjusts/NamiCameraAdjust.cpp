// Copyright Epic Games, Inc. All Rights Reserved.

#include "Adjusts/NamiCameraAdjust.h"
#include "Components/NamiCameraComponent.h"
#include "LogNamiCamera.h"
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
	// 默认实现为空，子类可重写
}

void UNamiCameraAdjust::Tick_Implementation(float DeltaTime)
{
	// 默认实现为空，子类可重写
}

void UNamiCameraAdjust::OnDeactivate_Implementation()
{
	// 重置输入打断状态
	bInputInterrupted = false;
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

	// 按每个参数的 BlendMode 进行缩放
	// Additive 模式的参数会被权重缩放
	// Override 模式的参数保持不变（delta 在 CalculateCombinedAdjustParams 中计算）
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
		bBlendOutSynced = false; // 重置同步标记，第一帧需要同步 ControlRotation
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

		// 广播蓝图事件
		OnInputInterrupted.Broadcast();

		// 开始混出（保留玩家对相机臂的控制权）
		RequestDeactivate();

		// 标记混出已同步，因为打断时已经同步了 ControlRotation
		// 这样混出第一帧不会再次同步，避免瞬切
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
	// 总是缓存目标臂旋转
	// 只有当 ArmRotation 参数的 BlendMode 为 Override 时才会使用
	// 在 CalculateCombinedAdjustParams 中会根据每个参数的 BlendMode 决定是否使用此值

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

	// 使用 Actor 的朝向作为基准
	// Actor 的前方是游戏逻辑中角色的朝向（+X 方向）
	// 不使用 Mesh 的朝向，因为 Mesh 可能有 -90° 或 180° 的本地旋转偏移
	FRotator ActorForwardRotation = OwnerPawn->GetActorRotation();

	// 计算世界空间的目标臂旋转
	// ArmRotationTarget 是相对于角色朝向的偏移：
	// - Yaw = 0°：角色正前方
	// - Yaw = 45°：角色右前方
	// - Yaw = -45°：角色左前方
	// - Yaw = 180°：角色正后方
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
