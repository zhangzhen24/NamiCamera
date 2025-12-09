// Copyright Epic Games, Inc. All Rights Reserved.

#include "Features/NamiCameraAdjustFeature.h"
#include "Modes/NamiCameraModeBase.h"
#include "Components/NamiCameraComponent.h"
#include "Interfaces/NamiCameraInputProvider.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "LogNamiCamera.h"
#include "Libraries/NamiCameraMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraAdjustFeature)

UNamiCameraAdjustFeature::UNamiCameraAdjustFeature()
	: Super()
	, MaxPivotRotationSpeed(720.0f)
	, CurrentPhase(EAdjustPhase::Inactive)
{
}

void UNamiCameraAdjustFeature::Initialize_Implementation(UNamiCameraModeBase* InCameraMode)
{
	Super::Initialize_Implementation(InCameraMode);
	CurrentPhase = EAdjustPhase::Inactive;
}

void UNamiCameraAdjustFeature::Activate_Implementation()
{
	Super::Activate_Implementation();
	CurrentPhase = EAdjustPhase::BlendIn;
	bHasBlendInBaseState = false;
	bHasBlendOutStartState = false;
	bHasBlendOutCameraRotation = false;
}

void UNamiCameraAdjustFeature::Update_Implementation(float DeltaTime)
{
	Super::Update_Implementation(DeltaTime);
}

void UNamiCameraAdjustFeature::ApplyToViewWithContext_Implementation(
	FNamiCameraView& InOutView, 
	float DeltaTime,
	FNamiCameraPipelineContext& Context)
{
	if (!IsActive())
	{
		return;
	}

	// 获取基础视图（来自 Mode Stack）
	const FNamiCameraView BaseView = InOutView;

	// 更新基础状态（用于混出目标）
	if (Context.bHasBaseState && !bHasBlendInBaseState)
	{
		BlendInBaseState = Context.BaseState;
		bHasBlendInBaseState = true;
	}

	// 计算混合权重
	const float Weight = GetCurrentBlendWeight();
	const bool bIsCurrentlyExiting = IsExiting();

	// 检查并处理输入打断
	const bool bIsInterrupted = CheckAndHandleInterrupt(Context, DeltaTime);

	// 更新状态机
	UpdatePhase(Weight, bIsCurrentlyExiting, bIsInterrupted);

	// 更新参数更新控制
	UpdateParamControl(Context, Weight);

	// 根据阶段处理
	switch (CurrentPhase)
	{
	case EAdjustPhase::BlendIn:
		ProcessBlendIn(BaseView, Weight, DeltaTime, Context, InOutView);
		break;
	case EAdjustPhase::Active:
		ProcessActive(BaseView, Weight, DeltaTime, Context, InOutView);
		break;
	case EAdjustPhase::BlendOut:
	case EAdjustPhase::BlendOutInterrupted:
		ProcessBlendOut(BaseView, Weight, DeltaTime, Context, InOutView);
		break;
	default:
		break;
	}

	// 应用参数更新控制到最终视图
	ApplyParamUpdateControlToView(InOutView, Context);
}

void UNamiCameraAdjustFeature::DeactivateEffect(bool bForceImmediate)
{
	Super::DeactivateEffect(bForceImmediate);
	CurrentPhase = EAdjustPhase::Inactive;
}

void UNamiCameraAdjustFeature::UpdatePhase(float Weight, bool bInIsExiting, bool bIsInterrupted)
{
	if (bInIsExiting)
	{
		if (bIsInterrupted)
		{
			CurrentPhase = EAdjustPhase::BlendOutInterrupted;
		}
		else
		{
			CurrentPhase = EAdjustPhase::BlendOut;
		}
	}
	else if (Weight >= 1.0f - KINDA_SMALL_NUMBER)
	{
		CurrentPhase = EAdjustPhase::Active;
	}
	else if (Weight > KINDA_SMALL_NUMBER)
	{
		CurrentPhase = EAdjustPhase::BlendIn;
	}
	else
	{
		CurrentPhase = EAdjustPhase::Inactive;
	}
}

bool UNamiCameraAdjustFeature::CheckAndHandleInterrupt(
	FNamiCameraPipelineContext& Context,
	float DeltaTime)
{
	// 只在 bAllowPlayerInput == false 时检查打断
	if (AdjustConfig.bAllowPlayerInput)
	{
		return false;
	}

	// 如果已经退出，不再检查
	if (IsExiting())
	{
		return Context.bIsInterrupted;
	}

	// 检查玩家输入
	if (Context.PlayerInput.bHasInput)
	{
		// 触发打断
		Context.bIsInterrupted = true;
		Context.InterruptTime = Context.DeltaTime;
		
		// 保存混出起始状态
		if (!bHasBlendOutStartState)
		{
			BlendOutStartState = BuildStateFromView(Context.EffectView, DeltaTime);
			bHasBlendOutStartState = true;
		}

		// 保存混出时的相机旋转
		if (!bHasBlendOutCameraRotation)
		{
			BlendOutCameraRotation = Context.EffectView.CameraRotation;
			bHasBlendOutCameraRotation = true;
		}

		return true;
	}

	return false;
}

void UNamiCameraAdjustFeature::UpdateParamControl(
	FNamiCameraPipelineContext& Context,
	float Weight)
{
	// 根据 bAllowPlayerInput 和打断状态设置参数更新控制
	if (AdjustConfig.bAllowPlayerInput)
	{
		// 玩家输入模式：旋转参数由玩家输入控制
		Context.ParamUpdateControl.SetRotationParamsMode(ENamiCameraParamUpdateMode::PlayerInput);
		Context.ParamUpdateControl.SetLocationParamsMode(ENamiCameraParamUpdateMode::Normal);
		Context.InputControlState = ENamiCameraInputControlState::PlayerInput;
	}
	else if (Context.bIsInterrupted)
	{
		// 打断模式：旋转参数由玩家输入控制，位置参数继续混出
		Context.ParamUpdateControl.SetRotationParamsMode(ENamiCameraParamUpdateMode::PlayerInput);
		Context.ParamUpdateControl.SetLocationParamsMode(ENamiCameraParamUpdateMode::Normal);
		Context.InputControlState = ENamiCameraInputControlState::Interrupted;
	}
	else
	{
		// 完全控制模式：所有参数由 CameraAdjust 控制
		Context.ParamUpdateControl.SetRotationParamsMode(ENamiCameraParamUpdateMode::Normal);
		Context.ParamUpdateControl.SetLocationParamsMode(ENamiCameraParamUpdateMode::Normal);
		Context.InputControlState = ENamiCameraInputControlState::FullControl;
	}
}

void UNamiCameraAdjustFeature::ProcessBlendIn(
	const FNamiCameraView& BaseView,
	float Weight,
	float DeltaTime,
	FNamiCameraPipelineContext& Context,
	FNamiCameraView& OutView)
{
	// 构建基础状态
	FNamiCameraState BaseState = BuildStateFromView(BaseView, DeltaTime);

	// 应用配置到状态
	FNamiCameraState TargetState = BaseState;
	ApplyConfigToState(TargetState, Weight, Context);

	// 混合状态
	FNamiCameraState FinalState;
	BlendStates(BaseState, TargetState, Weight, FinalState);

	// 计算视图
	ComputeViewFromState(FinalState, OutView);
}

void UNamiCameraAdjustFeature::ProcessActive(
	const FNamiCameraView& BaseView,
	float Weight,
	float DeltaTime,
	FNamiCameraPipelineContext& Context,
	FNamiCameraView& OutView)
{
	// 构建基础状态
	FNamiCameraState BaseState = BuildStateFromView(BaseView, DeltaTime);

	// 应用配置到状态（权重为1.0）
	ApplyConfigToState(BaseState, 1.0f, Context);

	// 计算视图
	ComputeViewFromState(BaseState, OutView);
}

void UNamiCameraAdjustFeature::ProcessBlendOut(
	const FNamiCameraView& BaseView,
	float Weight,
	float DeltaTime,
	FNamiCameraPipelineContext& Context,
	FNamiCameraView& OutView)
{
	// 混出起点：混出开始时的状态（应用配置后的状态）
	FNamiCameraState BlendOutStart = bHasBlendOutStartState 
		? BlendOutStartState 
		: BuildStateFromView(BaseView, DeltaTime);

	// 混出终点：基础状态（来自 Mode）
	FNamiCameraState BlendOutEnd = bHasBlendInBaseState 
		? BlendInBaseState 
		: BuildStateFromView(BaseView, DeltaTime);

	// 混合状态（Weight 从 1.0 降到 0.0，所以从 Start 混合到 End）
	FNamiCameraState FinalState;
	BlendStates(BlendOutStart, BlendOutEnd, Weight, FinalState);

	// 如果是打断混出，旋转参数应该由玩家输入控制，不应用混合结果
	if (CurrentPhase == EAdjustPhase::BlendOutInterrupted)
	{
		// 旋转参数使用玩家输入（通过参数更新控制处理）
		// 位置参数使用混合结果
		// 这里只计算位置相关的参数
		FinalState.PivotRotation = BlendOutStart.PivotRotation; // 保持混出起点，后续由参数更新控制覆盖
		FinalState.ArmRotation = BlendOutStart.ArmRotation;
		FinalState.CameraRotationOffset = BlendOutStart.CameraRotationOffset;
	}

	// 计算视图
	ComputeViewFromState(FinalState, OutView);

	// 如果是打断混出，保持混出时的相机旋转
	if (CurrentPhase == EAdjustPhase::BlendOutInterrupted && bHasBlendOutCameraRotation)
	{
		OutView.CameraRotation = BlendOutCameraRotation;
	}
}

FNamiCameraState UNamiCameraAdjustFeature::BuildStateFromView(
	const FNamiCameraView& View,
	float DeltaTime) const
{
	FNamiCameraState State;

	// 1. 直接获取的参数
	State.PivotLocation = View.PivotLocation;
	State.CameraRotation = View.CameraRotation;
	State.FieldOfView = View.FOV;

	// 2. 计算 PivotRotation（从 ControlRotation，应用速度限制）
	FRotator NormalizedRawPivotRotation = FNamiCameraMath::NormalizeRotatorTo360(View.ControlRotation);
	if (bHasLastPivotRotation)
	{
		FRotator NormalizedLast = FNamiCameraMath::NormalizeRotatorTo360(LastPivotRotation);
		float PitchDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedLast.Pitch, NormalizedRawPivotRotation.Pitch);
		float YawDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedLast.Yaw, NormalizedRawPivotRotation.Yaw);
		float RollDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedLast.Roll, NormalizedRawPivotRotation.Roll);

		// 应用角度变化速度限制
		if (MaxPivotRotationSpeed > 0.0f && DeltaTime > 0.0f)
		{
			float MaxDeltaPerFrame = MaxPivotRotationSpeed * DeltaTime;
			PitchDelta = FMath::Clamp(PitchDelta, -MaxDeltaPerFrame, MaxDeltaPerFrame);
			YawDelta = FMath::Clamp(YawDelta, -MaxDeltaPerFrame, MaxDeltaPerFrame);
			RollDelta = FMath::Clamp(RollDelta, -MaxDeltaPerFrame, MaxDeltaPerFrame);
		}

		State.PivotRotation.Pitch = NormalizedLast.Pitch + PitchDelta;
		State.PivotRotation.Yaw = NormalizedLast.Yaw + YawDelta;
		State.PivotRotation.Roll = NormalizedLast.Roll + RollDelta;
		State.PivotRotation = FNamiCameraMath::NormalizeRotatorTo360(State.PivotRotation);
	}
	else
	{
		State.PivotRotation = NormalizedRawPivotRotation;
	}

	// 3. 反推吊臂参数
	FVector CameraToPivot = View.PivotLocation - View.CameraLocation;
	float Distance = CameraToPivot.Size();

	if (Distance > KINDA_SMALL_NUMBER)
	{
		// 计算 ArmRotation
		FVector Direction = CameraToPivot.GetSafeNormal();
		FQuat BasePivotQuat = State.PivotRotation.Quaternion();
		FVector LocalDirection = BasePivotQuat.Inverse().RotateVector(Direction);
		State.ArmRotation = LocalDirection.Rotation();
		State.ArmRotation.Normalize();

		// 计算 ArmLength（简化：直接使用距离）
		State.ArmLength = Distance;
	}
	else
	{
		State.ArmLength = 0.0f;
		State.ArmRotation = FRotator::ZeroRotator;
	}

	// 4. 其他参数使用默认值（简化实现）
	State.ArmOffset = FVector::ZeroVector;
	State.CameraLocationOffset = FVector::ZeroVector;
	State.CameraRotationOffset = FRotator::ZeroRotator;

	// 5. 计算输出
	State.ComputeOutput();

	// 更新 LastPivotRotation
	LastPivotRotation = State.PivotRotation;
	bHasLastPivotRotation = true;

	return State;
}

void UNamiCameraAdjustFeature::ApplyConfigToState(
	FNamiCameraState& InOutState,
	float Weight,
	const FNamiCameraPipelineContext& Context) const
{
	// 应用 AdjustConfig 到状态（根据混合模式）
	AdjustConfig.ApplyToState(InOutState, Weight);

	// 重新计算输出
	InOutState.ComputeOutput();
}

void UNamiCameraAdjustFeature::ComputeViewFromState(
	const FNamiCameraState& State,
	FNamiCameraView& OutView) const
{
	OutView.PivotLocation = State.PivotLocation;
	OutView.CameraLocation = State.CameraLocation;
	OutView.CameraRotation = State.CameraRotation;
	OutView.FOV = State.FieldOfView;
	OutView.ControlLocation = State.PivotLocation;
	OutView.ControlRotation = State.PivotRotation;
}

void UNamiCameraAdjustFeature::BlendStates(
	const FNamiCameraState& From,
	const FNamiCameraState& To,
	float Weight,
	FNamiCameraState& OutState) const
{
	// 线性混合所有参数
	OutState.PivotLocation = FMath::Lerp(From.PivotLocation, To.PivotLocation, Weight);
	OutState.PivotRotation = FMath::Lerp(From.PivotRotation, To.PivotRotation, Weight);
	OutState.ArmLength = FMath::Lerp(From.ArmLength, To.ArmLength, Weight);
	OutState.ArmRotation = FMath::Lerp(From.ArmRotation, To.ArmRotation, Weight);
	OutState.ArmOffset = FMath::Lerp(From.ArmOffset, To.ArmOffset, Weight);
	OutState.CameraLocationOffset = FMath::Lerp(From.CameraLocationOffset, To.CameraLocationOffset, Weight);
	OutState.CameraRotationOffset = FMath::Lerp(From.CameraRotationOffset, To.CameraRotationOffset, Weight);
	OutState.FieldOfView = FMath::Lerp(From.FieldOfView, To.FieldOfView, Weight);

	// 重新计算输出
	OutState.ComputeOutput();
}

void UNamiCameraAdjustFeature::ApplyParamUpdateControlToView(
	FNamiCameraView& InOutView,
	const FNamiCameraPipelineContext& Context) const
{
	// 应用参数更新控制到视图
	// 根据 Context.ParamUpdateControl 的模式，修改视图参数

	// 旋转参数
	if (Context.ParamUpdateControl.PivotRotation == ENamiCameraParamUpdateMode::PlayerInput)
	{
		// 使用玩家输入
		if (Context.bHasPlayerInputState)
		{
			InOutView.ControlRotation = Context.PlayerInputState.PivotRotation;
		}
	}
	else if (Context.ParamUpdateControl.PivotRotation == ENamiCameraParamUpdateMode::Frozen)
	{
		// 冻结：使用保存的值
		if (Context.bHasPreservedValues)
		{
			InOutView.ControlRotation = Context.PreservedValues.PivotRotation;
		}
	}
	else if (Context.ParamUpdateControl.PivotRotation == ENamiCameraParamUpdateMode::BaseValue)
	{
		// 基础值：使用基础状态
		if (Context.bHasBaseState)
		{
			InOutView.ControlRotation = Context.BaseState.PivotRotation;
		}
	}

	// 类似地处理其他参数...
	// （简化实现，实际需要处理所有参数）
}

FVector UNamiCameraAdjustFeature::GetPivotLocation(const FNamiCameraView& View) const
{
	if (!View.PivotLocation.IsNearlyZero())
	{
		return View.PivotLocation;
	}

	// 回退：尝试从 Pawn 获取
	UNamiCameraModeBase* Mode = GetCameraMode();
	if (Mode)
	{
		UNamiCameraComponent* Component = Mode->GetCameraComponent();
		if (Component)
		{
			APawn* OwnerPawn = Component->GetOwnerPawn();
			if (IsValid(OwnerPawn))
			{
				return OwnerPawn->GetActorLocation();
			}
		}
	}

	return FVector::ZeroVector;
}

FRotator UNamiCameraAdjustFeature::GetCharacterRotation() const
{
	UNamiCameraModeBase* Mode = GetCameraMode();
	if (Mode)
	{
		UNamiCameraComponent* Component = Mode->GetCameraComponent();
		if (Component)
		{
			APawn* OwnerPawn = Component->GetOwnerPawn();
			if (IsValid(OwnerPawn))
			{
				return OwnerPawn->GetActorRotation();
			}
		}
	}

	return FRotator::ZeroRotator;
}
