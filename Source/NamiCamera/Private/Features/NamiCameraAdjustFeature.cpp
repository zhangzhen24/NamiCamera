// Copyright Epic Games, Inc. All Rights Reserved.

#include "Features/NamiCameraAdjustFeature.h"
#include "Modes/NamiCameraModeBase.h"
#include "Components/NamiCameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "LogNamiCamera.h"
#include "LogNamiCameraMacros.h"
#include "Libraries/NamiCameraMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraAdjustFeature)

UNamiCameraAdjustFeature::UNamiCameraAdjustFeature()
	: bAllowInputInterruptWholeEffect(false)
	, InterruptInputThreshold(0.1f)
	, InterruptCooldown(0.15f)
	, bAllowViewControlInterrupt(true)
	, ViewControlInputThreshold(0.1f)
	, bHasCachedInitialState(false)
	, InputInterruptCooldownTimer(0.0f)
	, bViewControlInterrupted(false)
	, LastCameraInputMagnitude(0.0f)
{
	FeatureName = TEXT("CameraAdjust");
	Priority = 100;  // 高优先级，在其他 Feature 之后应用
}

void UNamiCameraAdjustFeature::Initialize_Implementation(UNamiCameraModeBase* InCameraMode)
{
	Super::Initialize_Implementation(InCameraMode);
	
	// 初始化缓存的引用
	UpdateCachedReferences();
	
	// 尝试自动设置输入提供者
	if (!InputProvider.GetObject())
	{
		UNamiCameraComponent* CameraComponent = InCameraMode ? InCameraMode->GetCameraComponent() : nullptr;
		if (CameraComponent)
		{
			APlayerController* PC = CameraComponent->GetOwnerPlayerController();
			if (PC && PC->GetClass()->ImplementsInterface(UNamiCameraInputProvider::StaticClass()))
			{
				InputProvider.SetObject(PC);
				InputProvider.SetInterface(Cast<INamiCameraInputProvider>(PC));
			}
		}
	}
}

void UNamiCameraAdjustFeature::Activate_Implementation()
{
	Super::Activate_Implementation();
	
	// 重置状态
	bHasCachedInitialState = false;
	bViewControlInterrupted = false;
	InputInterruptCooldownTimer = 0.0f;
	LastCameraInputMagnitude = 0.0f;
	
	// 更新缓存的引用
	UpdateCachedReferences();
}

void UNamiCameraAdjustFeature::Update_Implementation(float DeltaTime)
{
	Super::Update_Implementation(DeltaTime);
	
	// 更新缓存的引用
	UpdateCachedReferences();
	
	// 更新输入打断冷却计时器
	if (InputInterruptCooldownTimer > 0.0f)
	{
		InputInterruptCooldownTimer = FMath::Max(0.0f, InputInterruptCooldownTimer - DeltaTime);
	}
}

void UNamiCameraAdjustFeature::ApplyEffect_Implementation(FNamiCameraView& InOutView, float Weight, float DeltaTime)
{
	// 检查是否有任何修改
	if (!AdjustConfig.HasAnyModification())
	{
		return;
	}

	// 检查视角控制输入打断
	if (bAllowViewControlInterrupt && !bViewControlInterrupted && 
		AdjustConfig.ControlRotationOffset.bEnabled && 
		AdjustConfig.ControlMode != ENamiCameraControlMode::Forced)
	{
		CheckViewControlInterrupt();
	}

	// 检查整段效果输入打断
	if (bAllowInputInterruptWholeEffect && !bIsExiting)
	{
		CheckWholeEffectInterrupt(DeltaTime);
	}

	// 如果还没有构建初始状态，先构建
	if (!bHasCachedInitialState)
	{
		FVector PivotLocation = GetPivotLocation(InOutView);
		CachedInitialState = BuildInitialStateFromView(InOutView);
		CachedInitialState.PivotLocation = PivotLocation;
		bHasCachedInitialState = true;
	}

	// 使用初始状态作为基础
	FNamiCameraState CurrentState = CachedInitialState;
	
	// 更新枢轴位置（角色可能移动了）
	CurrentState.PivotLocation = GetPivotLocation(InOutView);

	// 应用调整配置到 State
	ApplyAdjustConfigToState(CurrentState, Weight);

	// 计算输出
	CurrentState.ComputeOutput();

	// 从 State 计算视图
	FNamiCameraView TargetView;
	ComputeViewFromState(CurrentState, TargetView);

	// 从当前视图混合到目标视图
	// Weight 从 0.0 升到 1.0（表示效果的程度）
	// 当 Weight = 0.0 时，完全使用当前视图（效果未生效）
	// 当 Weight = 1.0 时，完全使用 TargetView（效果完全生效）
	InOutView.Blend(TargetView, Weight);
}

bool UNamiCameraAdjustFeature::CheckInputInterrupt() const
{
	// 检查整段效果输入打断
	if (bAllowInputInterruptWholeEffect)
	{
		float InputMagnitude = GetPlayerCameraInputMagnitude();
		return InputMagnitude > InterruptInputThreshold;
	}
	
	return false;
}

FNamiCameraState UNamiCameraAdjustFeature::BuildInitialStateFromView(const FNamiCameraView& View) const
{
	FNamiCameraState State;

	// 从当前相机视图构建初始 State（简化版本）
	// 只反推必要的参数，其他参数使用默认值，让修改配置直接应用

	// 1. 枢轴点位置
	FVector PivotLocation = GetPivotLocation(View);
	State.PivotLocation = PivotLocation;

	// 2. 计算吊臂长度（从 PivotLocation 到 CameraLocation 的距离）
	FVector CameraToPivot = PivotLocation - View.CameraLocation;
	float Distance = CameraToPivot.Size();
	State.ArmLength = Distance;

	// 3. 枢轴旋转（从相机位置反推）
	if (Distance > KINDA_SMALL_NUMBER)
	{
		FVector Direction = CameraToPivot.GetSafeNormal();
		State.PivotRotation = Direction.Rotation();
	}
	else
	{
		// 如果距离太近，使用相机旋转
		State.PivotRotation = View.CameraRotation;
	}

	// 4. 吊臂旋转初始化为零（让修改配置直接应用，不进行复杂的反推）
	State.ArmRotation = FRotator::ZeroRotator;

	// 5. 相机旋转
	State.CameraRotation = View.CameraRotation;

	// 6. FOV
	State.FieldOfView = View.FOV;

	// 7. 其他参数保持默认值
	State.ArmOffset = FVector::ZeroVector;
	State.CameraLocationOffset = FVector::ZeroVector;
	State.CameraRotationOffset = FRotator::ZeroRotator;
	State.ControlRotationOffset = FRotator::ZeroRotator;

	return State;
}

void UNamiCameraAdjustFeature::ApplyAdjustConfigToState(FNamiCameraState& InOutState, float Weight) const
{
	// 创建临时配置副本
	FNamiCameraStateModify TempAdjustConfig = AdjustConfig;
	const bool bIsExitingNow = IsExiting();

	// 特殊处理 ArmRotation：需要在角色朝向空间中计算，然后转换到 PivotSpace
	if (TempAdjustConfig.ArmRotation.bEnabled)
	{
		// 获取角色朝向
		FRotator CharacterRotation = GetCharacterRotation(InOutState);
		
		// 获取初始 ArmRotation（相对于初始角色朝向的偏移）
		const FRotator InitialArmRotation = CachedInitialState.ArmRotation;
		
		// 根据混合模式和退出状态计算目标值（在角色朝向的局部空间中）
		FRotator TargetArmRotation;
		
		if (TempAdjustConfig.ArmRotation.BlendMode == ENamiCameraBlendMode::Additive)
		{
			// Additive：在当前吊臂状态基础上增加偏移
			// 需要将当前 ArmRotation 从 PivotSpace 转换到 CharacterSpace，叠加后保持在 CharacterSpace
			// 后续的统一转换代码会将 CharacterSpace 的值转换回 PivotSpace
			
			// 1. 将当前 ArmRotation 从 PivotSpace 转换到 CharacterSpace
			// 转换原理：BasePivotQuat * PivotSpaceArmQuat = CharacterQuat * CharacterSpaceArmQuat
			// 因此：CharacterSpaceArmQuat = CharacterQuat.Inverse() * BasePivotQuat * PivotSpaceArmQuat
			const FRotator BasePivotRotation = InOutState.PivotRotation + InOutState.ControlRotationOffset;
			const FQuat BasePivotQuat = BasePivotRotation.Quaternion();
			const FQuat CurrentPivotSpaceArmQuat = InOutState.ArmRotation.Quaternion();
			const FQuat CharacterQuat = CharacterRotation.Quaternion();
			const FQuat CurrentCharacterSpaceArmQuat = CharacterQuat.Inverse() * BasePivotQuat * CurrentPivotSpaceArmQuat;
			FRotator CurrentCharacterSpaceArmRotation = CurrentCharacterSpaceArmQuat.Rotator();
			CurrentCharacterSpaceArmRotation.Normalize();
			
			// 2. 在 CharacterSpace 中叠加偏移
			// 直接相加，不做 Slerp，确保是真正的增量叠加
			// 但需要处理角度叠加，避免跨越 360 度边界导致跳变
			FRotator AdditiveValue = TempAdjustConfig.ArmRotation.Value * Weight;
			
			// 计算叠加后的目标值（直接相加）
			FRotator RawTarget;
			RawTarget.Pitch = CurrentCharacterSpaceArmRotation.Pitch + AdditiveValue.Pitch;
			RawTarget.Yaw = CurrentCharacterSpaceArmRotation.Yaw + AdditiveValue.Yaw;
			RawTarget.Roll = CurrentCharacterSpaceArmRotation.Roll + AdditiveValue.Roll;
			
			// 对 RawTarget 进行归一化，确保值在 -180 到 180 范围内
			RawTarget.Normalize();
			
			// 对每个分量使用 FindDeltaAngleDegrees 确保选择最短路径，避免跳变
			// 例如：如果 Current 是 178 度，加上 45 度后 RawTarget 是 223 度（归一化后变成 -137 度）
			// FindDeltaAngleDegrees(178, -137) 会返回 45 度的差值（最短路径）
			// 这样 Result = 178 + 45 = 223 度（归一化后是 -137 度），但计算过程是连续的，不会跳变
			TargetArmRotation.Pitch = CurrentCharacterSpaceArmRotation.Pitch + FMath::FindDeltaAngleDegrees(CurrentCharacterSpaceArmRotation.Pitch, RawTarget.Pitch);
			TargetArmRotation.Yaw = CurrentCharacterSpaceArmRotation.Yaw + FMath::FindDeltaAngleDegrees(CurrentCharacterSpaceArmRotation.Yaw, RawTarget.Yaw);
			TargetArmRotation.Roll = CurrentCharacterSpaceArmRotation.Roll + FMath::FindDeltaAngleDegrees(CurrentCharacterSpaceArmRotation.Roll, RawTarget.Roll);
			
			TargetArmRotation.Normalize(); // 归一化，确保值在有效范围内
			// 注意：TargetArmRotation 此时在 CharacterSpace 中，后续统一转换代码会转换到 PivotSpace
		}
		else // Override（正常情况和退出情况）
		{
			// Override：在相对角度空间中，从初始相对偏移过渡到目标相对偏移
			// 使用 FindDeltaAngleDegrees 确保选择最短路径，避免 270度到-90度 的跳变
			FRotator NormalizedInitial = InitialArmRotation;
			NormalizedInitial.Normalize();
			
			FRotator NormalizedTargetValue = TempAdjustConfig.ArmRotation.Value;
			NormalizedTargetValue.Normalize();
			
			// 计算最短路径的差值
			FRotator DeltaRotation;
			DeltaRotation.Pitch = FMath::FindDeltaAngleDegrees(NormalizedInitial.Pitch, NormalizedTargetValue.Pitch);
			DeltaRotation.Yaw = FMath::FindDeltaAngleDegrees(NormalizedInitial.Yaw, NormalizedTargetValue.Yaw);
			DeltaRotation.Roll = FMath::FindDeltaAngleDegrees(NormalizedInitial.Roll, NormalizedTargetValue.Roll);
			
			// 计算插值后的目标值（使用最短路径）
			FRotator LerpedTarget = NormalizedInitial + DeltaRotation * Weight;
			LerpedTarget.Normalize();
			
			// 使用球面插值（Slerp）确保在球面上平滑过渡，实现"绕圈混合"效果
			// 注意：由于我们已经使用了 FindDeltaAngleDegrees 确保最短路径，转换为 Quaternion 时不会跳变
			const FQuat InitialQuat = NormalizedInitial.Quaternion();
			const FQuat TargetQuat = LerpedTarget.Quaternion();
			
			if (bIsExitingNow)
			{
				// 退出时：从目标值（效果完全生效时的值）混合回初始值
				// Weight 从 1.0 降到 0.0，所以混合因子是 (1.0 - Weight)
				// 当 Weight = 1.0 时，应该是目标值（效果完全生效）
				// 当 Weight = 0.0 时，应该是初始值（效果完全消失）
				// 使用归一化后的目标值（完全生效时的值）作为退出起点
				const FQuat ExitTargetQuat = NormalizedTargetValue.Quaternion();
				
				const float BlendAlpha = 1.0f - Weight; // Weight 从 1.0 降到 0.0，BlendAlpha 从 0.0 升到 1.0
				const FQuat ResultQuat = FQuat::Slerp(ExitTargetQuat, InitialQuat, BlendAlpha);
				TargetArmRotation = ResultQuat.Rotator();
			}
			else
			{
				// 正常情况：从初始值过渡到目标值
				const FQuat ResultQuat = FQuat::Slerp(InitialQuat, TargetQuat, Weight);
				TargetArmRotation = ResultQuat.Rotator();
			}
		}
		
		// 转换到 PivotSpace
		// 转换原理：
		// - 在世界空间中，如果 ArmRotation 是相对于角色朝向的：WorldRotation = CharacterQuat * TargetArmQuat
		// - 在 ComputeOutput 中，ArmRotation 在 BasePivotQuat 空间中：WorldRotation = BasePivotQuat * PivotSpaceArmQuat
		// - 要让两者相等：BasePivotQuat * PivotSpaceArmQuat = CharacterQuat * TargetArmQuat
		// - 因此：PivotSpaceArmQuat = BasePivotQuat.Inverse() * CharacterQuat * TargetArmQuat
		const FQuat CharacterQuat = CharacterRotation.Quaternion();
		const FRotator BasePivotRotation = InOutState.PivotRotation + InOutState.ControlRotationOffset;
		const FQuat BasePivotQuat = BasePivotRotation.Quaternion();
		const FQuat TargetArmQuat = TargetArmRotation.Quaternion();
		const FQuat PivotSpaceArmQuat = BasePivotQuat.Inverse() * CharacterQuat * TargetArmQuat;
		
		// 应用到 State
		InOutState.ArmRotation = PivotSpaceArmQuat.Rotator();
		InOutState.ChangedFlags.bArmRotation = true;
		
		// 禁用配置中的 ArmRotation，避免在 ApplyToState 中重复应用
		TempAdjustConfig.ArmRotation.bEnabled = false;
	}

	// 退出时的处理：对于其他 Override 模式的修改项，将 Value 设置为初始值
	if (bIsExitingNow)
	{
		// 对于所有 Override 模式的修改项，将 Value 设置为初始值
		if (TempAdjustConfig.PivotLocation.bEnabled && TempAdjustConfig.PivotLocation.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.PivotLocation.Value = CachedInitialState.PivotLocation;
		}
		if (TempAdjustConfig.PivotRotation.bEnabled && TempAdjustConfig.PivotRotation.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.PivotRotation.Value = CachedInitialState.PivotRotation;
		}
		if (TempAdjustConfig.ArmLength.bEnabled && TempAdjustConfig.ArmLength.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.ArmLength.Value = CachedInitialState.ArmLength;
		}
		// ArmRotation 已经在上面特殊处理了，退出时 Weight 会从 1.0 降到 0.0，自动混合回初始值
		if (TempAdjustConfig.ArmOffset.bEnabled && TempAdjustConfig.ArmOffset.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.ArmOffset.Value = CachedInitialState.ArmOffset;
		}
		if (TempAdjustConfig.ControlRotationOffset.bEnabled && TempAdjustConfig.ControlRotationOffset.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.ControlRotationOffset.Value = CachedInitialState.ControlRotationOffset;
		}
		if (TempAdjustConfig.CameraLocationOffset.bEnabled && TempAdjustConfig.CameraLocationOffset.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.CameraLocationOffset.Value = CachedInitialState.CameraLocationOffset;
		}
		if (TempAdjustConfig.CameraRotationOffset.bEnabled && TempAdjustConfig.CameraRotationOffset.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.CameraRotationOffset.Value = CachedInitialState.CameraRotationOffset;
		}
		if (TempAdjustConfig.FieldOfView.bEnabled && TempAdjustConfig.FieldOfView.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.FieldOfView.Value = CachedInitialState.FieldOfView;
		}
		if (TempAdjustConfig.CameraLocation.bEnabled && TempAdjustConfig.CameraLocation.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.CameraLocation.Value = CachedInitialState.CameraLocation;
		}
		if (TempAdjustConfig.CameraRotation.bEnabled && TempAdjustConfig.CameraRotation.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.CameraRotation.Value = CachedInitialState.CameraRotation;
		}
	}

	// 检查视角控制是否应该被打断
	if (TempAdjustConfig.ControlRotationOffset.bEnabled)
	{
		// 如果被打断，不应用 ControlRotationOffset
		if (bViewControlInterrupted)
		{
			// 临时禁用 ControlRotationOffset
			const bool bWasEnabled = TempAdjustConfig.ControlRotationOffset.bEnabled;
			TempAdjustConfig.ControlRotationOffset.bEnabled = false;
			TempAdjustConfig.ApplyToState(InOutState, Weight);
			TempAdjustConfig.ControlRotationOffset.bEnabled = bWasEnabled; // 恢复原值
		}
		else
		{
			// 计算有效权重（混合模式下根据玩家输入衰减）
			float EffectiveWeight = Weight;
			if (TempAdjustConfig.ControlMode == ENamiCameraControlMode::Blended)
			{
				float InputMagnitude = GetPlayerCameraInputMagnitude();
				// 玩家输入越大，效果权重越低
				EffectiveWeight = Weight * (1.0f - FMath::Clamp(InputMagnitude, 0.0f, 1.0f));
			}

			// 应用所有修改（包括 ControlRotationOffset）
			TempAdjustConfig.ApplyToState(InOutState, EffectiveWeight);
		}
	}
	else
	{
		// 没有视角控制，直接应用所有修改
		TempAdjustConfig.ApplyToState(InOutState, Weight);
	}
}

void UNamiCameraAdjustFeature::ComputeViewFromState(const FNamiCameraState& State, FNamiCameraView& OutView) const
{
	// 从 State 计算视图
	OutView.PivotLocation = State.PivotLocation;
	OutView.CameraLocation = State.CameraLocation;
	OutView.CameraRotation = State.CameraRotation;
	OutView.FOV = State.FieldOfView;
	
	// ControlLocation 和 ControlRotation 从 State 计算
	// ControlLocation 通常是 PivotLocation
	OutView.ControlLocation = State.PivotLocation;
	
	// ControlRotation = PivotRotation + ControlRotationOffset
	OutView.ControlRotation = State.PivotRotation + State.ControlRotationOffset;
}

FVector UNamiCameraAdjustFeature::GetPivotLocation(const FNamiCameraView& View) const
{
	// 优先使用 View 的 PivotLocation
	if (!View.PivotLocation.IsNearlyZero())
	{
		return View.PivotLocation;
	}
	
	// 回退：尝试从 Pawn 获取
	if (CachedPawn.IsValid())
	{
		return CachedPawn->GetActorLocation();
	}
	
	// 最后回退：使用相机位置
	return View.CameraLocation;
}

FRotator UNamiCameraAdjustFeature::GetCharacterRotation(const FNamiCameraState& InState) const
{
	// 优先从 Pawn 获取角色朝向（Mesh 朝向）
	if (CachedPawn.IsValid())
	{
		return CachedPawn->GetActorRotation();
	}
	
	// 回退：使用 PivotRotation（相机朝向）
	return InState.PivotRotation;
}

void UNamiCameraAdjustFeature::CheckWholeEffectInterrupt(float DeltaTime)
{
	if (bIsExiting || InputInterruptCooldownTimer > 0.0f)
	{
		return;
	}

	float InputMagnitude = GetPlayerCameraInputMagnitude();
	if (InputMagnitude > InterruptInputThreshold)
	{
		// 触发打断
		InterruptEffect();
		InputInterruptCooldownTimer = InterruptCooldown;
	}
}

void UNamiCameraAdjustFeature::CheckViewControlInterrupt()
{
	if (bViewControlInterrupted || 
		AdjustConfig.ControlMode == ENamiCameraControlMode::Forced ||
		!AdjustConfig.ControlRotationOffset.bEnabled)
	{
		return;
	}

	float InputMagnitude = GetPlayerCameraInputMagnitude();
	if (InputMagnitude > ViewControlInputThreshold)
	{
		// 视角控制被输入打断
		bViewControlInterrupted = true;
		// 禁用 ControlRotationOffset
		AdjustConfig.ControlRotationOffset.bEnabled = false;
	}
}

float UNamiCameraAdjustFeature::GetPlayerCameraInputMagnitude() const
{
	FVector2D InputVector = FVector2D::ZeroVector;

	// 优先使用输入提供者接口（解耦设计）
	if (InputProvider.GetInterface())
	{
		// 调用接口方法（如果类实现了 _Implementation，会调用实现；否则返回零向量）
		InputVector = InputProvider->Execute_GetCameraInputVector(InputProvider.GetObject());
	}
	else if (CachedPlayerController.IsValid())
	{
		// 尝试从 PlayerController 获取接口
		if (INamiCameraInputProvider* InputProviderInterface = Cast<INamiCameraInputProvider>(CachedPlayerController.Get()))
		{
			InputVector = InputProviderInterface->GetCameraInputVector();
		}
		else
		{
			// 没有实现接口，输出警告
			static bool bHasWarned = false;
			if (!bHasWarned)
			{
				NAMI_LOG_WARNING(TEXT("[UNamiCameraAdjustFeature] PlayerController 未实现 INamiCameraInputProvider 接口，无法获取相机输入。请让 PlayerController 实现此接口。"));
				bHasWarned = true;
			}
		}
	}
	else
	{
		// 没有 PlayerController，输出警告
		static bool bHasWarned = false;
		if (!bHasWarned)
		{
			NAMI_LOG_WARNING(TEXT("[UNamiCameraAdjustFeature] 无法获取 PlayerController，无法获取相机输入。"));
			bHasWarned = true;
		}
	}

	// 返回输入向量的大小
	return InputVector.Size();
}

void UNamiCameraAdjustFeature::UpdateCachedReferences()
{
	UNamiCameraModeBase* Mode = GetCameraMode();
	if (!Mode)
	{
		return;
	}

	UNamiCameraComponent* CameraComponent = Mode->GetCameraComponent();
	if (!CameraComponent)
	{
		return;
	}

	CachedPlayerController = CameraComponent->GetOwnerPlayerController();
	CachedPawn = CameraComponent->GetOwnerPawn();
}

void UNamiCameraAdjustFeature::SetInputProvider(TScriptInterface<INamiCameraInputProvider> InInputProvider)
{
	InputProvider = InInputProvider;
}

