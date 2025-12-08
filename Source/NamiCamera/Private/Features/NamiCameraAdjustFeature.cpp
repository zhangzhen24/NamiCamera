// Copyright Epic Games, Inc. All Rights Reserved.

#include "Features/NamiCameraAdjustFeature.h"
#include "Modes/NamiCameraModeBase.h"
#include "Components/NamiCameraComponent.h"
#include "Interfaces/NamiCameraInputProvider.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "LogNamiCamera.h"
#include "LogNamiCameraMacros.h"
#include "Libraries/NamiCameraMath.h"
#include "Components/NamiCameraComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraAdjustFeature)
void UNamiCameraAdjustFeature::DeactivateEffect(bool bForceImmediate)
{
	Super::DeactivateEffect(bForceImmediate);

	// 清理与打断/混出相关的缓存
	bHasExitStartRotationState = false;
	bHasExitStartCameraRotation = false;
	bHasExitStartState = false;
	bInputEnabledOnInterrupt = false;
	bHasLastOutputState = false;
	LastOutputState = FNamiCameraState();
}


UNamiCameraAdjustFeature::UNamiCameraAdjustFeature()
	: bHasCachedInitialState(false)
	, LastPivotRotation(FRotator::ZeroRotator)
	, bHasLastPivotRotation(false)
{
	FeatureName = TEXT("CameraAdjust");
	Priority = 100;  // 高优先级，在其他 Feature 之后应用
}

void UNamiCameraAdjustFeature::Initialize_Implementation(UNamiCameraModeBase* InCameraMode)
{
	Super::Initialize_Implementation(InCameraMode);
}

void UNamiCameraAdjustFeature::Activate_Implementation()
{
	Super::Activate_Implementation();
	
	// 重置状态
	bHasCachedInitialState = false;
	bHasLastPivotRotation = false;
	LastPivotRotation = FRotator::ZeroRotator;
	bHasLastCharacterRotation = false;
	LastCharacterRotation = FRotator::ZeroRotator;
	bInputVectorCacheValid = false;
	bInputMagnitudeCacheValid = false;
	InputAccumulationTime = 0.0f;
	bInputEnabledOnInterrupt = false;
}

void UNamiCameraAdjustFeature::Update_Implementation(float DeltaTime)
{
	Super::Update_Implementation(DeltaTime);
	
	// 重置输入缓存（每帧重新计算）
	bInputVectorCacheValid = false;
	bInputMagnitudeCacheValid = false;
	
	// 检查是否需要输入打断（仅在 bAllowPlayerInput==false 时检查）
	if (IsActive() && !IsExiting() && !AdjustConfig.bAllowPlayerInput)
	{
		// 检查条件：ArmRotation 必须启用，且 ControlMode 必须是 Suggestion
		if (AdjustConfig.ArmRotation.bEnabled && 
			AdjustConfig.ControlMode == ENamiCameraControlMode::Suggestion)
		{
			float InputMagnitude = GetPlayerCameraInputMagnitude();
			
			// 检查是否超过阈值
			if (InputMagnitude > AdjustConfig.CameraInputThreshold)
			{
				// 累积输入时间
				InputAccumulationTime += DeltaTime;
				
				// 立即触发打断（可以根据需要改为累积触发）
				// 开启玩家输入，允许玩家控制旋转
				AdjustConfig.bAllowPlayerInput = true;
				bInputEnabledOnInterrupt = true;
				
				// 触发打断，开始混出
				DeactivateEffect(false);  // false = 平滑混出，不是立即停止
				InputAccumulationTime = 0.0f;  // 重置累积时间
				
				NAMI_LOG_INPUT_INTERRUPT(Log, TEXT("[CameraAdjust] 检测到玩家输入，触发打断。输入大小=%.3f，阈值=%.3f"),
					InputMagnitude, AdjustConfig.CameraInputThreshold);
			}
			else
			{
				// 输入低于阈值，重置累积时间
				InputAccumulationTime = 0.0f;
			}
		}
		else
		{
			// 不需要检查打断，重置累积时间
			InputAccumulationTime = 0.0f;
		}
	}
	else
	{
		// 不在激活状态、正在退出或允许玩家输入，重置累积时间
		InputAccumulationTime = 0.0f;
	}
}

void UNamiCameraAdjustFeature::ApplyEffect_Implementation(FNamiCameraView& InOutView, float Weight, float DeltaTime)
{
	// 检查是否有任何修改
	if (!AdjustConfig.HasAnyModification())
	{
		return;
	}

	// ========== 分支判定 ==========
	const bool bIsExitingNow = IsExiting();
	const bool bInterruptBlendOut = bIsExitingNow && bInputEnabledOnInterrupt;
	const bool bIsFirstFrameOfExit = bIsExitingNow && !bHasExitStartState;

	// 基础视图（CameraMode输出）供混出目标使用
	FNamiCameraState BaseState = BuildCurrentStateFromView(InOutView, DeltaTime);
	BaseState.PivotLocation = GetPivotLocation(InOutView);

	// 在混入的第一帧，保存基础状态（不包含 AdjustConfig 效果）
	// 这个状态将用于混出时的目标状态
	if (!bIsExitingNow && Weight < 0.01f && !bHasCachedInitialState)
	{
		CachedInitialState = BaseState;
		bHasCachedInitialState = true;
	}

	FNamiCameraState CurrentViewState;
	FNamiCameraState TargetState;
	bool bFreezeRotations = false;

	// 根据是否混出决定起点和终点
	if (bIsExitingNow)
	{
		// 混出：根据是否有输入打断进行不同处理
		if (bInterruptBlendOut)
		{
			// 2.2 有输入打断：只混合位置参数，旋转交给玩家控制
			// 在混出的第一帧，保存当前状态作为混出起点（已经包含 AdjustConfig 效果）
			if (!bHasExitStartState)
			{
				// 优先使用上一帧的输出状态（已经包含 AdjustConfig 效果）
				if (bHasLastOutputState)
				{
					ExitStartState = LastOutputState;
				}
				else
				{
					// 从当前视图反推（当前视图已经包含 AdjustConfig 效果）
					ExitStartState = BaseState;
				}
				bHasExitStartState = true;
				
				// 保存进入混出状态时的相机臂旋转（用于有输入打断时保持旋转）
				ExitStartRotationState.ArmRotation = ExitStartState.ArmRotation;
				ExitStartRotationState.PivotRotation = ExitStartState.PivotRotation;
				bHasExitStartRotationState = true;
			}
			
			// 混出时，Weight 从 1.0 降到 0.0
			// FMath::Lerp(CurrentState, TargetState, Weight) 的含义：
			//   Weight = 1.0 时，结果是 TargetState
			//   Weight = 0.0 时，结果是 CurrentState
			// 混出时我们希望：
			//   Weight = 1.0 时，使用"应用配置后的状态"（ExitStartState）
			//   Weight = 0.0 时，使用"基础状态"（CachedInitialState）
			// 所以：CurrentViewState = CachedInitialState，TargetState = ExitStartState
			CurrentViewState = bHasCachedInitialState ? CachedInitialState : BaseState; // 基础状态（混出终点）
			TargetState = ExitStartState; // 应用配置后的状态（混出起点）
			
			// 冻结旋转参数，让玩家输入控制旋转
			bFreezeRotations = true;
		}
		else
		{
			// 2.1 无输入打断：根据 CameraAdjust 的结束方式处理不同逻辑
			// 在混出的第一帧，保存当前状态作为混出起点（已经包含 AdjustConfig 效果）
			if (!bHasExitStartState)
			{
				// 优先使用上一帧的输出状态（已经包含 AdjustConfig 效果）
				if (bHasLastOutputState)
				{
					ExitStartState = LastOutputState;
				}
				else
				{
					// 从当前视图反推（当前视图已经包含 AdjustConfig 效果）
					ExitStartState = BaseState;
				}
				bHasExitStartState = true;
			}
			
			// 立即结束：直接设置到基础状态
			if (EndBehavior == ENamiCameraEndBehavior::ForceEnd)
			{
				// 直接使用基础状态，不进行混合
				CurrentViewState = bHasCachedInitialState ? CachedInitialState : BaseState;
				TargetState = CurrentViewState; // 目标等于当前，不混合
			}
			else
			{
				// 正常混出：从"应用配置后的状态"混出到"基础状态"
				// 混出时，Weight 从 1.0 降到 0.0
				// FMath::Lerp(CurrentState, TargetState, Weight) 的含义：
				//   Weight = 1.0 时，结果是 TargetState
				//   Weight = 0.0 时，结果是 CurrentState
				// 混出时我们希望：
				//   Weight = 1.0 时，使用"应用配置后的状态"（ExitStartState）
				//   Weight = 0.0 时，使用"基础状态"（CachedInitialState）
				// 所以：CurrentViewState = CachedInitialState，TargetState = ExitStartState
				CurrentViewState = bHasCachedInitialState ? CachedInitialState : BaseState; // 基础状态（混出终点）
				TargetState = ExitStartState; // 应用配置后的状态（混出起点）
			}
			
			// 无输入打断时，不冻结旋转，正常混合
			bFreezeRotations = false;
		}
	}
	else
	{
		// 混入：起点=基础视图，终点=基础视图+配置
		CurrentViewState = BaseState;
		TargetState = BaseState;
		ApplyAdjustConfigToState(TargetState, 1.0f);
		bFreezeRotations = false;
	}
	
	// 3. 重置标志（如果不在混出状态）
	if (!IsExiting())
	{
		// 不在混出状态，重置标志（但保留 CachedInitialState，因为可能还在混入）
		bHasExitStartRotationState = false;
		bHasExitStartCameraRotation = false;
		bHasExitStartState = false;
		bInputEnabledOnInterrupt = false;
	}
	
	// 4. 在 State 层面混合参数（保证焦点不变）
	BlendParametersInState(CurrentViewState, TargetState, Weight, bFreezeRotations);
	
	// 4.1 如果有输入打断，确保旋转参数使用进入混出状态时的旋转（不被 CameraAdjust 覆盖）
	if (bInterruptBlendOut && bHasExitStartRotationState)
	{
		// 保持进入混出状态时的相机臂旋转，让玩家输入控制旋转
		CurrentViewState.ArmRotation = ExitStartRotationState.ArmRotation;
		CurrentViewState.PivotRotation = ExitStartRotationState.PivotRotation;
	}
	
	if (bIsFirstFrameOfExit)
	{
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("[CameraAdjust] ========== 打断瞬间 - 混合后的 CurrentViewState =========="));
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  Weight: %.3f, bFreezeRotations: %d"), Weight, bFreezeRotations);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  PivotLocation: (%.2f, %.2f, %.2f)"), 
			CurrentViewState.PivotLocation.X, CurrentViewState.PivotLocation.Y, CurrentViewState.PivotLocation.Z);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  ArmLength: %.2f"), CurrentViewState.ArmLength);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  ArmRotation: (%.2f, %.2f, %.2f)"), 
			CurrentViewState.ArmRotation.Pitch, CurrentViewState.ArmRotation.Yaw, CurrentViewState.ArmRotation.Roll);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  ArmOffset: (%.2f, %.2f, %.2f)"), 
			CurrentViewState.ArmOffset.X, CurrentViewState.ArmOffset.Y, CurrentViewState.ArmOffset.Z);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  PivotRotation: (%.2f, %.2f, %.2f)"), 
			CurrentViewState.PivotRotation.Pitch, CurrentViewState.PivotRotation.Yaw, CurrentViewState.PivotRotation.Roll);
	}
	
	// 5. 保存混出开始时的相机旋转（如果正在混出且修改了相机旋转且配置允许）
	bool bShouldPreserveCameraRotation = false;
	// TODO: 重写 bAllowPlayerInput==false 时的逻辑，移除 bInterruptBlendOut 相关判断
	if (IsExiting() && AdjustConfig.bPreserveCameraRotationOnExit && 
		(AdjustConfig.CameraRotation.bEnabled || AdjustConfig.CameraRotationOffset.bEnabled))
	{
		// 如果是混出的第一帧，保存当前的相机旋转
		if (!bHasExitStartCameraRotation)
		{
			ExitStartCameraRotation = InOutView.CameraRotation;
			bHasExitStartCameraRotation = true;
		}
		bShouldPreserveCameraRotation = true;
	}
	
	
	// 7. 重新计算输出（基于混合后的参数）
	CurrentViewState.ComputeOutput();
	
	if (bIsFirstFrameOfExit)
	{
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("[CameraAdjust] ========== 打断瞬间 - ComputeOutput 后的 CurrentViewState =========="));
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  CameraLocation: (%.2f, %.2f, %.2f)"), 
			CurrentViewState.CameraLocation.X, CurrentViewState.CameraLocation.Y, CurrentViewState.CameraLocation.Z);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  CameraRotation: (%.2f, %.2f, %.2f)"), 
			CurrentViewState.CameraRotation.Pitch, CurrentViewState.CameraRotation.Yaw, CurrentViewState.CameraRotation.Roll);
	}
	
	// 8. 直接应用结果（不调用 Blend，保证焦点不变）
	FVector CameraLocationBefore = InOutView.CameraLocation;
	ComputeViewFromState(CurrentViewState, InOutView);
	
	// 记录本帧输出，供下一帧（或打断瞬间）作为混出起点
	LastOutputState = CurrentViewState;
	bHasLastOutputState = true;

	// 混出结束或立即结束，对齐 ControlRotation，避免移除后跳变
	const bool bBlendOutFinished = bIsExitingNow && Weight <= KINDA_SMALL_NUMBER;
	if (bBlendOutFinished)
	{
		InOutView.ControlRotation = InOutView.CameraRotation;
	}
	else if (bIsExitingNow && EndBehavior == ENamiCameraEndBehavior::ForceEnd)
	{
		// 立即结束：在退出路径上也对齐一次，确保无突变
		InOutView.ControlRotation = InOutView.CameraRotation;
	}

	if (bIsFirstFrameOfExit)
	{
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("[CameraAdjust] ========== 打断瞬间 - 最终输出的 View =========="));
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  CameraLocation Before: (%.2f, %.2f, %.2f)"), 
			CameraLocationBefore.X, CameraLocationBefore.Y, CameraLocationBefore.Z);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  CameraLocation After: (%.2f, %.2f, %.2f)"), 
			InOutView.CameraLocation.X, InOutView.CameraLocation.Y, InOutView.CameraLocation.Z);
		FVector LocationDelta = InOutView.CameraLocation - CameraLocationBefore;
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  CameraLocation Delta: (%.2f, %.2f, %.2f), Size: %.2f"), 
			LocationDelta.X, LocationDelta.Y, LocationDelta.Z, LocationDelta.Size());
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  PivotLocation: (%.2f, %.2f, %.2f)"), 
			InOutView.PivotLocation.X, InOutView.PivotLocation.Y, InOutView.PivotLocation.Z);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  CameraRotation: (%.2f, %.2f, %.2f)"), 
			InOutView.CameraRotation.Pitch, InOutView.CameraRotation.Yaw, InOutView.CameraRotation.Roll);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("=========================================="));
	}
	
	// 9. 混出时保持相机旋转的当前状态，不混合回默认值
	if (bShouldPreserveCameraRotation && bHasExitStartCameraRotation)
	{
		// 保持混出开始时的相机旋转，不应用State计算出的旋转
		InOutView.CameraRotation = ExitStartCameraRotation;
	}
	// TODO: 重写 bAllowPlayerInput==false 时的逻辑，移除相关代码
	
	// 更新上一帧的 PivotRotation（在应用配置之后，使用最终计算出的值）
	LastPivotRotation = CurrentViewState.PivotRotation;
	bHasLastPivotRotation = true;
}


FNamiCameraState UNamiCameraAdjustFeature::BuildInitialStateFromView(const FNamiCameraView& View)
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
		FRotator NewPivotRotation = Direction.Rotation();
		FRotator NormalizedNewPivotRotation = FNamiCameraMath::NormalizeRotatorTo360(NewPivotRotation);
		
		// 如果有上一帧的 PivotRotation，使用 FindDeltaAngle360 确保最短路径，避免跳变
		// 如果没有上一帧数据（首次构建），直接使用计算出的值
		if (bHasLastPivotRotation)
		{
			FRotator NormalizedLast = FNamiCameraMath::NormalizeRotatorTo360(LastPivotRotation);
			float PitchDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedLast.Pitch, NormalizedNewPivotRotation.Pitch);
			float YawDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedLast.Yaw, NormalizedNewPivotRotation.Yaw);
			float RollDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedLast.Roll, NormalizedNewPivotRotation.Roll);
			
			// 优化：直接应用差值，最后归一化一次
			State.PivotRotation.Pitch = NormalizedLast.Pitch + PitchDelta;
			State.PivotRotation.Yaw = NormalizedLast.Yaw + YawDelta;
			State.PivotRotation.Roll = NormalizedLast.Roll + RollDelta;
			// 确保结果在0-360度范围（只归一化一次）
			State.PivotRotation = FNamiCameraMath::NormalizeRotatorTo360(State.PivotRotation);
		}
		else
		{
			State.PivotRotation = NormalizedNewPivotRotation;
		}
	}
	else
	{
		// 如果距离太近，使用相机旋转
		FRotator NormalizedNewPivotRotation = FNamiCameraMath::NormalizeRotatorTo360(View.CameraRotation);
		
		// 如果有上一帧的 PivotRotation，使用 FindDeltaAngle360 确保最短路径
		if (bHasLastPivotRotation)
		{
			FRotator NormalizedLast = FNamiCameraMath::NormalizeRotatorTo360(LastPivotRotation);
			float PitchDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedLast.Pitch, NormalizedNewPivotRotation.Pitch);
			float YawDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedLast.Yaw, NormalizedNewPivotRotation.Yaw);
			float RollDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedLast.Roll, NormalizedNewPivotRotation.Roll);
			
			// 优化：直接应用差值，最后归一化一次
			State.PivotRotation.Pitch = NormalizedLast.Pitch + PitchDelta;
			State.PivotRotation.Yaw = NormalizedLast.Yaw + YawDelta;
			State.PivotRotation.Roll = NormalizedLast.Roll + RollDelta;
			// 确保结果在0-360度范围（只归一化一次）
			State.PivotRotation = FNamiCameraMath::NormalizeRotatorTo360(State.PivotRotation);
		}
		else
		{
			State.PivotRotation = NormalizedNewPivotRotation;
		}
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
	// ControlRotationOffset 已移除，不再需要

	return State;
}

FNamiCameraState UNamiCameraAdjustFeature::BuildCurrentStateFromView(const FNamiCameraView& View, float DeltaTime) const
{
	FNamiCameraState State;
	
	// 1. 直接获取的参数
	State.PivotLocation = View.PivotLocation;
	
	// 在打断的第一帧，从实际相机位置反推 PivotRotation（而不是使用 View.ControlRotation）
	// 因为打断瞬间 View.ControlRotation 可能已经反映玩家输入，但 View.CameraLocation 仍是打断前状态
	const bool bIsFirstFrameOfExit = IsExiting() && !bHasExitStartRotationState;
	const FVector CameraToPivot = View.PivotLocation - View.CameraLocation;
	const float Distance = CameraToPivot.Size();
	
	if (bIsFirstFrameOfExit && Distance > KINDA_SMALL_NUMBER)
	{
		// 打断瞬间：从实际相机位置反推 PivotRotation（与 BuildInitialStateFromView 逻辑一致）
		FVector Direction = CameraToPivot.GetSafeNormal();
		FRotator NewPivotRotation = Direction.Rotation();
		FRotator NormalizedNewPivotRotation = FNamiCameraMath::NormalizeRotatorTo360(NewPivotRotation);
		
		if (bHasLastPivotRotation)
		{
			FRotator NormalizedLast = FNamiCameraMath::NormalizeRotatorTo360(LastPivotRotation);
			float PitchDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedLast.Pitch, NormalizedNewPivotRotation.Pitch);
			float YawDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedLast.Yaw, NormalizedNewPivotRotation.Yaw);
			float RollDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedLast.Roll, NormalizedNewPivotRotation.Roll);
			
			// 优化：直接应用差值，最后归一化一次
			State.PivotRotation.Pitch = NormalizedLast.Pitch + PitchDelta;
			State.PivotRotation.Yaw = NormalizedLast.Yaw + YawDelta;
			State.PivotRotation.Roll = NormalizedLast.Roll + RollDelta;
			// 确保结果在0-360度范围（只归一化一次）
			State.PivotRotation = FNamiCameraMath::NormalizeRotatorTo360(State.PivotRotation);
		}
		else
		{
			State.PivotRotation = NormalizedNewPivotRotation;
		}
		
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  [打断瞬间] 从实际相机位置反推 PivotRotation: (%.2f, %.2f, %.2f)"), 
			State.PivotRotation.Pitch, State.PivotRotation.Yaw, State.PivotRotation.Roll);
	}
	else
	{
		// 正常情况：使用 View.ControlRotation 计算 PivotRotation（应用速度限制）
		// 使用0-360度归一化确保 PivotRotation 不会发生 360 度跳变
	// 这在混合过程中非常重要，因为 View.ControlRotation 可能发生跳变
		FRotator NormalizedRawPivotRotation = FNamiCameraMath::NormalizeRotatorTo360(View.ControlRotation);
	if (bHasLastPivotRotation)
	{
			FRotator NormalizedLast = FNamiCameraMath::NormalizeRotatorTo360(LastPivotRotation);
			float PitchDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedLast.Pitch, NormalizedRawPivotRotation.Pitch);
			float YawDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedLast.Yaw, NormalizedRawPivotRotation.Yaw);
			float RollDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedLast.Roll, NormalizedRawPivotRotation.Roll);
			
			// 应用角度变化速度限制，防止鼠标快速输入时导致 PivotLocation 跳变
			if (MaxPivotRotationSpeed > 0.0f && DeltaTime > 0.0f)
			{
				float MaxDeltaPerFrame = MaxPivotRotationSpeed * DeltaTime;
				
				// 限制每个分量的变化速度
				PitchDelta = FMath::Clamp(PitchDelta, -MaxDeltaPerFrame, MaxDeltaPerFrame);
				YawDelta = FMath::Clamp(YawDelta, -MaxDeltaPerFrame, MaxDeltaPerFrame);
				RollDelta = FMath::Clamp(RollDelta, -MaxDeltaPerFrame, MaxDeltaPerFrame);
			}
			
			// 优化：直接应用差值，最后归一化一次
			State.PivotRotation.Pitch = NormalizedLast.Pitch + PitchDelta;
			State.PivotRotation.Yaw = NormalizedLast.Yaw + YawDelta;
			State.PivotRotation.Roll = NormalizedLast.Roll + RollDelta;
			// 确保结果在0-360度范围（只归一化一次）
			State.PivotRotation = FNamiCameraMath::NormalizeRotatorTo360(State.PivotRotation);
	}
	else
	{
			State.PivotRotation = NormalizedRawPivotRotation;
		}
	}
	
	State.CameraRotation = View.CameraRotation;
	State.FieldOfView = View.FOV;
	
	// 2. 反推吊臂参数
	// 注意：如果 bIsFirstFrameOfExit 为 true，CameraToPivot 和 Distance 已在上面计算
	// 如果为 false，需要重新计算
	FVector CameraToPivotForArm = bIsFirstFrameOfExit ? CameraToPivot : (View.PivotLocation - View.CameraLocation);
	float DistanceForArm = bIsFirstFrameOfExit ? Distance : CameraToPivotForArm.Size();
	
	// 在打断的第一帧添加详细日志
	if (bIsFirstFrameOfExit)
	{
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("[CameraAdjust] BuildCurrentStateFromView - 反推 ArmLength:"));
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  CameraLocation: (%.2f, %.2f, %.2f)"), 
			View.CameraLocation.X, View.CameraLocation.Y, View.CameraLocation.Z);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  PivotLocation: (%.2f, %.2f, %.2f)"), 
			View.PivotLocation.X, View.PivotLocation.Y, View.PivotLocation.Z);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  CameraToPivot: (%.2f, %.2f, %.2f), Distance: %.2f"), 
			CameraToPivotForArm.X, CameraToPivotForArm.Y, CameraToPivotForArm.Z, DistanceForArm);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  View.ControlRotation: (%.2f, %.2f, %.2f)"), 
			View.ControlRotation.Pitch, View.ControlRotation.Yaw, View.ControlRotation.Roll);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  LastPivotRotation: (%.2f, %.2f, %.2f), bHasLastPivotRotation: %d"), 
			LastPivotRotation.Pitch, LastPivotRotation.Yaw, LastPivotRotation.Roll, bHasLastPivotRotation);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  KINDA_SMALL_NUMBER: %.6f"), KINDA_SMALL_NUMBER);
	}
	
	if (DistanceForArm > KINDA_SMALL_NUMBER)
	{
		// 计算 ArmRotation
		// 相机到 Pivot 的方向（世界空间）
		const FVector Direction = CameraToPivotForArm.GetSafeNormal();
		
		// 基础方向是 PivotRotation 的 Forward 方向
		const FQuat BasePivotQuat = State.PivotRotation.Quaternion();
		
		// 在 BasePivot 的局部空间中，从 (1,0,0) 到 LocalDirection 的旋转
		// LocalDirection = BasePivotQuat.Inverse() * Direction
		const FVector LocalDirection = BasePivotQuat.Inverse().RotateVector(Direction);
		
		// 从 (1, 0, 0) 到 LocalDirection 的旋转就是 ArmRotation
		State.ArmRotation = LocalDirection.Rotation();
		State.ArmRotation.Normalize();
		
		// 3. 反推 ArmLength（考虑偏移的影响）
		// 先计算基于 ArmRotation 的基础位置
		const FQuat ArmQuat = State.ArmRotation.Quaternion();
		const FQuat LocalRotationQuat = BasePivotQuat * ArmQuat * BasePivotQuat.Inverse();
		const FVector BaseArmDirection = BasePivotQuat.GetForwardVector();
		const FVector BaseCameraOffset = -BaseArmDirection;
		
		// 旋转后的方向
		const FVector RotatedDirection = LocalRotationQuat.RotateVector(BaseCameraOffset);
		
		// 计算实际方向与理论方向的差异
		const FVector ActualDirection = CameraToPivotForArm.GetSafeNormal();
		const float DirectionDot = FVector::DotProduct(RotatedDirection, ActualDirection);
		
		if (bIsFirstFrameOfExit)
		{
			NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  State.PivotRotation: (%.2f, %.2f, %.2f)"), 
				State.PivotRotation.Pitch, State.PivotRotation.Yaw, State.PivotRotation.Roll);
			NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  State.ArmRotation: (%.2f, %.2f, %.2f)"), 
				State.ArmRotation.Pitch, State.ArmRotation.Yaw, State.ArmRotation.Roll);
			NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  ActualDirection (CameraToPivot normalized): (%.6f, %.6f, %.6f)"), 
				ActualDirection.X, ActualDirection.Y, ActualDirection.Z);
			NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  RotatedDirection (理论方向): (%.6f, %.6f, %.6f)"), 
				RotatedDirection.X, RotatedDirection.Y, RotatedDirection.Z);
			NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  DirectionDot: %.6f"), DirectionDot);
		}
		
		// 如果方向基本一致（差异很小），说明主要是 ArmLength 的影响
		// 否则可能是 ArmOffset 或 CameraLocationOffset 的影响
		if (DirectionDot > 0.99f) // 方向基本一致
		{
			// 直接使用距离作为 ArmLength
			State.ArmLength = DistanceForArm;
			if (bIsFirstFrameOfExit)
			{
				NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  ArmLength: 方向一致，使用距离 = %.2f"), State.ArmLength);
			}
		}
		else
		{
			// 方向不一致，可能是偏移的影响
			// 使用投影距离作为 ArmLength 的近似值
			const float ProjectedDistance = FVector::DotProduct(CameraToPivotForArm, RotatedDirection);
			
			// 如果方向完全相反（DirectionDot < 0.0）或投影距离为负，说明反推失败
			// 在这种情况下，使用实际距离作为回退值，避免 ArmLength 被错误计算为 0.0
			if (DirectionDot < 0.0f || ProjectedDistance < 0.0f)
			{
				// 反推失败，使用实际距离作为回退值
				State.ArmLength = DistanceForArm;
				if (bIsFirstFrameOfExit)
				{
					NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  ArmLength: 反推失败（DirectionDot=%.6f，投影距离=%.2f），使用距离作为回退值 = %.2f"), 
						DirectionDot, ProjectedDistance, State.ArmLength);
				}
			}
			else
			{
				// 方向不一致但投影距离为正，使用投影距离
				State.ArmLength = ProjectedDistance;
				if (bIsFirstFrameOfExit)
				{
					NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  ArmLength: 方向不一致，DirectionDot=%.6f，投影距离=%.2f，最终=%.2f"), 
						DirectionDot, ProjectedDistance, State.ArmLength);
				}
			}
		}
	}
	else
	{
		// 距离太近，使用默认值
		State.ArmLength = 0.0f;
		State.ArmRotation = FRotator::ZeroRotator;
		if (bIsFirstFrameOfExit)
		{
			NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  ArmLength: 距离太近（%.2f <= %.6f），使用默认值 0.0"), 
				DistanceForArm, KINDA_SMALL_NUMBER);
		}
	}
	
	// 4. 反推偏移参数（基于位置差异）
	// 计算基于当前参数应该得到的相机位置
	FNamiCameraState TempState = State;
	TempState.ArmOffset = FVector::ZeroVector;
	TempState.CameraLocationOffset = FVector::ZeroVector;
	TempState.CameraRotationOffset = FRotator::ZeroRotator;
	TempState.ComputeOutput();
	
	// 计算实际位置与理论位置的差异
	const FVector PositionDiff = View.CameraLocation - TempState.CameraLocation;
	const float PositionDiffSize = PositionDiff.Size();
	
	if (PositionDiffSize > KINDA_SMALL_NUMBER)
	{
		// 尝试将差异分配到 ArmOffset 和 CameraLocationOffset
		// 策略：优先分配到 ArmOffset（吊臂本地空间），剩余分配到 CameraLocationOffset（相机本地空间）
		
		const FQuat BasePivotQuat = State.PivotRotation.Quaternion();
		const FQuat ArmQuat = State.ArmRotation.Quaternion();
		const FQuat FinalArmQuat = BasePivotQuat * ArmQuat;
		
		// 将位置差异转换到吊臂本地空间
		const FVector ArmOffsetWorld = PositionDiff;
		const FVector ArmOffsetLocal = FinalArmQuat.Inverse().RotateVector(ArmOffsetWorld);
		
		// 如果差异主要在吊臂方向上，分配到 ArmOffset
		// 否则分配到 CameraLocationOffset
		const FVector ArmForward = FinalArmQuat.GetForwardVector();
		const float ArmDirectionDot = FMath::Abs(FVector::DotProduct(ArmOffsetWorld.GetSafeNormal(), ArmForward));
		
		if (ArmDirectionDot < 0.5f) // 差异主要在吊臂的垂直方向
		{
			// 分配到 ArmOffset（吊臂本地空间）
			State.ArmOffset = ArmOffsetLocal;
			
			// 重新计算，看是否还有剩余差异
			TempState.ArmOffset = State.ArmOffset;
			TempState.ComputeOutput();
			const FVector RemainingDiff = View.CameraLocation - TempState.CameraLocation;
			
			// 剩余差异分配到 CameraLocationOffset（相机本地空间）
			if (RemainingDiff.Size() > KINDA_SMALL_NUMBER)
			{
				const FQuat CameraQuat = View.CameraRotation.Quaternion();
				State.CameraLocationOffset = CameraQuat.Inverse().RotateVector(RemainingDiff);
			}
			else
			{
				State.CameraLocationOffset = FVector::ZeroVector;
			}
		}
		else
		{
			// 差异主要在吊臂方向上，分配到 CameraLocationOffset
			const FQuat CameraQuat = View.CameraRotation.Quaternion();
			State.CameraLocationOffset = CameraQuat.Inverse().RotateVector(PositionDiff);
			State.ArmOffset = FVector::ZeroVector;
		}
	}
	else
	{
		// 位置差异很小，使用零值
		State.ArmOffset = FVector::ZeroVector;
		State.CameraLocationOffset = FVector::ZeroVector;
	}
	
	// 5. 反推 CameraRotationOffset
	// 计算基于 ArmRotation 应该得到的相机旋转
	const FQuat BasePivotQuat = State.PivotRotation.Quaternion();
	const FQuat ArmQuat = State.ArmRotation.Quaternion();
	const FQuat FinalArmQuat = BasePivotQuat * ArmQuat;
	const FRotator ExpectedCameraRotation = FinalArmQuat.Rotator();
	
	// 计算实际旋转与理论旋转的差异
	FRotator RotationDiff = View.CameraRotation - ExpectedCameraRotation;
	RotationDiff.Normalize();
	
	// 如果差异很小，使用零值；否则使用差异值
	if (FMath::Abs(RotationDiff.Pitch) < 1.0f && 
		FMath::Abs(RotationDiff.Yaw) < 1.0f && 
		FMath::Abs(RotationDiff.Roll) < 1.0f)
	{
		State.CameraRotationOffset = FRotator::ZeroRotator;
	}
	else
	{
		State.CameraRotationOffset = RotationDiff;
		State.CameraRotationOffset.Normalize();
	}
	
	return State;
}

void UNamiCameraAdjustFeature::ApplyAdjustConfigToState(FNamiCameraState& InOutState, float Weight) const
{
	// 创建临时配置副本
	FNamiCameraStateModify TempAdjustConfig = AdjustConfig;
	const bool bIsExitingNow = IsExiting();

	// 如果允许玩家输入，禁用旋转参数，只应用位置参数（让玩家输入控制旋转）
	// 这包括：1. 正常情况下的 bAllowPlayerInput=true
	//         2. 打断时开启玩家输入后的情况（混出期间和混出完成后）
	if (TempAdjustConfig.bAllowPlayerInput)
	{
		// 禁用旋转参数（由玩家输入控制）
		TempAdjustConfig.ArmRotation.bEnabled = false;
		TempAdjustConfig.CameraRotationOffset.bEnabled = false;
		TempAdjustConfig.PivotRotation.bEnabled = false;
		
		NAMI_LOG_INPUT_INTERRUPT(VeryVerbose, TEXT("[CameraAdjust] 允许玩家输入，禁用旋转参数。bAllowPlayerInput=%d, bIsExitingNow=%d, bInputEnabledOnInterrupt=%d"),
			TempAdjustConfig.bAllowPlayerInput, bIsExitingNow, bInputEnabledOnInterrupt);
	}
	// TODO: 重写 bAllowPlayerInput==false 时的混出逻辑
	// 当前逻辑已移除，需要重新实现：
	// - 如果没有输入打断，完全由 CameraAdjust 管理，正常的混入混出流程
	// - 如果触发了输入打断，相机臂旋转完全由玩家控制，CameraAdjust 只处理位置参数（臂长、FOV等）

	// 特殊处理 ArmRotation：需要在角色朝向空间中计算，然后转换到 PivotSpace
	if (TempAdjustConfig.ArmRotation.bEnabled)
	{
		// 获取角色朝向
		FRotator RawCharacterRotation = GetCharacterRotation(InOutState);
		
		// 使用最短路径确保 CharacterRotation 不会发生 360 度跳变
		// 这在混合过程中玩家输入改变角色朝向时非常重要
		FRotator CharacterRotation;
		if (bHasLastCharacterRotation)
		{
			CharacterRotation.Pitch = LastCharacterRotation.Pitch + FMath::FindDeltaAngleDegrees(LastCharacterRotation.Pitch, RawCharacterRotation.Pitch);
			CharacterRotation.Yaw = LastCharacterRotation.Yaw + FMath::FindDeltaAngleDegrees(LastCharacterRotation.Yaw, RawCharacterRotation.Yaw);
			CharacterRotation.Roll = LastCharacterRotation.Roll + FMath::FindDeltaAngleDegrees(LastCharacterRotation.Roll, RawCharacterRotation.Roll);
			CharacterRotation.Normalize();
		}
		else
		{
			CharacterRotation = RawCharacterRotation;
		}
		
		// 更新缓存的 CharacterRotation
		LastCharacterRotation = CharacterRotation;
		bHasLastCharacterRotation = true;
		
		// 获取当前 ArmRotation（View转State方案：使用当前 State 的值作为"初始值"）
		// 对于 Override 模式，从当前值过渡到目标值
		// 对于 Additive 模式，在当前值基础上叠加
		const FRotator CurrentArmRotation = InOutState.ArmRotation;
		
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
			const FRotator BasePivotRotation = InOutState.PivotRotation;
			const FQuat BasePivotQuat = BasePivotRotation.Quaternion();
			const FQuat CurrentPivotSpaceArmQuat = InOutState.ArmRotation.Quaternion();
			const FQuat CharacterQuat = CharacterRotation.Quaternion();
			const FQuat CurrentCharacterSpaceArmQuat = CharacterQuat.Inverse() * BasePivotQuat * CurrentPivotSpaceArmQuat;
			FRotator CurrentCharacterSpaceArmRotation = CurrentCharacterSpaceArmQuat.Rotator();
			CurrentCharacterSpaceArmRotation.Normalize();
			
			// 2. 在 CharacterSpace 中叠加偏移
			FRotator AdditiveValue = TempAdjustConfig.ArmRotation.Value * Weight;
			
			// 计算叠加后的目标值（直接相加）
			FRotator RawTarget;
			RawTarget.Pitch = CurrentCharacterSpaceArmRotation.Pitch + AdditiveValue.Pitch;
			RawTarget.Yaw = CurrentCharacterSpaceArmRotation.Yaw + AdditiveValue.Yaw;
			RawTarget.Roll = CurrentCharacterSpaceArmRotation.Roll + AdditiveValue.Roll;
			
			// 对 RawTarget 进行归一化，确保值在 -180 到 180 范围内
			RawTarget.Normalize();
			
			// 使用 FindDeltaAngleDegrees 确保选择最短路径，避免跳变
			TargetArmRotation.Pitch = CurrentCharacterSpaceArmRotation.Pitch + FMath::FindDeltaAngleDegrees(CurrentCharacterSpaceArmRotation.Pitch, RawTarget.Pitch);
			TargetArmRotation.Yaw = CurrentCharacterSpaceArmRotation.Yaw + FMath::FindDeltaAngleDegrees(CurrentCharacterSpaceArmRotation.Yaw, RawTarget.Yaw);
			TargetArmRotation.Roll = CurrentCharacterSpaceArmRotation.Roll + FMath::FindDeltaAngleDegrees(CurrentCharacterSpaceArmRotation.Roll, RawTarget.Roll);
			TargetArmRotation.Normalize();
		}
		else // Override
		{
			// Override：从当前值过渡到目标值，使用 FindDeltaAngleDegrees 确保最短路径
			
			// 将当前 ArmRotation 从 PivotSpace 转换到 CharacterSpace
			const FRotator BasePivotRotation = InOutState.PivotRotation;
			const FQuat BasePivotQuat = BasePivotRotation.Quaternion();
			const FQuat CurrentPivotSpaceArmQuat = CurrentArmRotation.Quaternion();
			const FQuat CharacterQuat = CharacterRotation.Quaternion();
			const FQuat CurrentCharacterSpaceArmQuat = CharacterQuat.Inverse() * BasePivotQuat * CurrentPivotSpaceArmQuat;
			FRotator CurrentCharacterSpaceArmRotation = CurrentCharacterSpaceArmQuat.Rotator();
			CurrentCharacterSpaceArmRotation.Normalize();
			
			FRotator NormalizedInitial = CurrentCharacterSpaceArmRotation;
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
			
			// 使用球面插值（Slerp）确保在球面上平滑过渡
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
				FRotator SlerpedRotation = ResultQuat.Rotator();
				SlerpedRotation.Normalize();
				
				// 使用 FindDeltaAngleDegrees 确保最短路径，避免跳变
				TargetArmRotation.Pitch = NormalizedTargetValue.Pitch + FMath::FindDeltaAngleDegrees(NormalizedTargetValue.Pitch, SlerpedRotation.Pitch);
				TargetArmRotation.Yaw = NormalizedTargetValue.Yaw + FMath::FindDeltaAngleDegrees(NormalizedTargetValue.Yaw, SlerpedRotation.Yaw);
				TargetArmRotation.Roll = NormalizedTargetValue.Roll + FMath::FindDeltaAngleDegrees(NormalizedTargetValue.Roll, SlerpedRotation.Roll);
				TargetArmRotation.Normalize();
			}
			else
			{
				// 正常情况：从初始值过渡到目标值
				const FQuat ResultQuat = FQuat::Slerp(InitialQuat, TargetQuat, Weight);
				FRotator SlerpedRotation = ResultQuat.Rotator();
				SlerpedRotation.Normalize();
				
				// 使用 FindDeltaAngleDegrees 确保最短路径，避免跳变
				TargetArmRotation.Pitch = NormalizedInitial.Pitch + FMath::FindDeltaAngleDegrees(NormalizedInitial.Pitch, SlerpedRotation.Pitch);
				TargetArmRotation.Yaw = NormalizedInitial.Yaw + FMath::FindDeltaAngleDegrees(NormalizedInitial.Yaw, SlerpedRotation.Yaw);
				TargetArmRotation.Roll = NormalizedInitial.Roll + FMath::FindDeltaAngleDegrees(NormalizedInitial.Roll, SlerpedRotation.Roll);
				TargetArmRotation.Normalize();
			}
		}
		
		// 转换到 PivotSpace
		// 转换原理：
		// - 在世界空间中，如果 ArmRotation 是相对于角色朝向的：WorldRotation = CharacterQuat * TargetArmQuat
		// - 在 ComputeOutput 中，ArmRotation 在 BasePivotQuat 空间中：WorldRotation = BasePivotQuat * PivotSpaceArmQuat
		// - 要让两者相等：BasePivotQuat * PivotSpaceArmQuat = CharacterQuat * TargetArmQuat
		// - 因此：PivotSpaceArmQuat = BasePivotQuat.Inverse() * CharacterQuat * TargetArmQuat
		const FQuat CharacterQuat = CharacterRotation.Quaternion();
		const FRotator BasePivotRotation = InOutState.PivotRotation;
		const FQuat BasePivotQuat = BasePivotRotation.Quaternion();
		const FQuat TargetArmQuat = TargetArmRotation.Quaternion();
		const FQuat PivotSpaceArmQuat = BasePivotQuat.Inverse() * CharacterQuat * TargetArmQuat;
		
		// 应用到 State
		FRotator NewArmRotation = PivotSpaceArmQuat.Rotator();
		NewArmRotation.Normalize();
		
		// 使用 FindDeltaAngleDegrees 确保从当前值到新值的最短路径，避免跳变
		FRotator CurrentArmRotationForBlend = InOutState.ArmRotation;
		CurrentArmRotationForBlend.Normalize();
		
		const float CurrentPitch = CurrentArmRotationForBlend.Pitch;
		const float CurrentYaw = CurrentArmRotationForBlend.Yaw;
		const float CurrentRoll = CurrentArmRotationForBlend.Roll;
		const float NewPitch = NewArmRotation.Pitch;
		const float NewYaw = NewArmRotation.Yaw;
		const float NewRoll = NewArmRotation.Roll;
		
		InOutState.ArmRotation.Pitch = CurrentPitch + FMath::FindDeltaAngleDegrees(CurrentPitch, NewPitch);
		InOutState.ArmRotation.Yaw = CurrentYaw + FMath::FindDeltaAngleDegrees(CurrentYaw, NewYaw);
		InOutState.ArmRotation.Roll = CurrentRoll + FMath::FindDeltaAngleDegrees(CurrentRoll, NewRoll);
		InOutState.ArmRotation.Normalize();
		
		InOutState.ChangedFlags.bArmRotation = true;
		
		// 禁用配置中的 ArmRotation，避免在 ApplyToState 中重复应用
		TempAdjustConfig.ArmRotation.bEnabled = false;
	}

	// 退出时的处理：对于 Override 模式的修改项，将 Value 设置为当前 State 的值
	// View转State方案：退出时从目标值混合回当前值（即首次激活时的值）
	// 注意：这里使用 InOutState 的当前值，而不是缓存的初始值
	// 特殊处理：相机旋转在混出时保持当前状态，不混合回默认值
	if (bIsExitingNow)
	{
		// 对于所有 Override 模式的修改项，将 Value 设置为当前 State 的值
		// 这样退出时会从目标值混合回当前值
		if (TempAdjustConfig.PivotLocation.bEnabled && TempAdjustConfig.PivotLocation.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.PivotLocation.Value = InOutState.PivotLocation;
		}
		if (TempAdjustConfig.PivotRotation.bEnabled && TempAdjustConfig.PivotRotation.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.PivotRotation.Value = InOutState.PivotRotation;
		}
		if (TempAdjustConfig.ArmLength.bEnabled && TempAdjustConfig.ArmLength.BlendMode == ENamiCameraBlendMode::Override)
		{
			const float ArmLengthBefore = TempAdjustConfig.ArmLength.Value;
			TempAdjustConfig.ArmLength.Value = InOutState.ArmLength;
			NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("[CameraAdjust] ApplyAdjustConfigToState - 退出时设置 ArmLength.Value: %.2f -> %.2f (InOutState.ArmLength)"), 
				ArmLengthBefore, TempAdjustConfig.ArmLength.Value);
		}
		// ArmRotation 已经在上面特殊处理了，退出时 Weight 会从 1.0 降到 0.0，自动混合回当前值
		if (TempAdjustConfig.ArmOffset.bEnabled && TempAdjustConfig.ArmOffset.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.ArmOffset.Value = InOutState.ArmOffset;
		}
		// ControlRotationOffset 已移除，不再需要处理
		if (TempAdjustConfig.CameraLocationOffset.bEnabled && TempAdjustConfig.CameraLocationOffset.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.CameraLocationOffset.Value = InOutState.CameraLocationOffset;
		}
		// 混出时保持相机旋转不变（如果配置允许）
		if (TempAdjustConfig.bPreserveCameraRotationOnExit)
		{
			if (TempAdjustConfig.CameraRotationOffset.bEnabled && TempAdjustConfig.CameraRotationOffset.BlendMode == ENamiCameraBlendMode::Override)
			{
				TempAdjustConfig.CameraRotationOffset.bEnabled = false; // 禁用，保持当前状态
			}
		}
		if (TempAdjustConfig.FieldOfView.bEnabled && TempAdjustConfig.FieldOfView.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.FieldOfView.Value = InOutState.FieldOfView;
		}
		if (TempAdjustConfig.CameraLocation.bEnabled && TempAdjustConfig.CameraLocation.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempAdjustConfig.CameraLocation.Value = InOutState.CameraLocation;
		}
		if (TempAdjustConfig.bPreserveCameraRotationOnExit)
		{
			if (TempAdjustConfig.CameraRotation.bEnabled && TempAdjustConfig.CameraRotation.BlendMode == ENamiCameraBlendMode::Override)
			{
				TempAdjustConfig.CameraRotation.bEnabled = false; // 禁用，保持当前状态
			}
		}
	}

	// 应用 ArmRotation（如果启用）
	if (TempAdjustConfig.ArmRotation.bEnabled)
	{
		// 计算有效权重（混合模式下根据玩家输入衰减）
		float EffectiveWeight = Weight;
		if (TempAdjustConfig.ControlMode == ENamiCameraControlMode::Blended)
		{
			// 获取输入大小（使用缓存的输入）
			float InputMagnitude = GetPlayerCameraInputMagnitude();
			
			// 输入衰减公式：输入越大，权重越低
			// 输入 0.0 时权重 = Weight，输入 1.0 时权重 = Weight * 0.3
			// 可以根据手感调整衰减曲线
			float InputAttenuation = FMath::Lerp(1.0f, 0.3f, InputMagnitude);
			EffectiveWeight = Weight * InputAttenuation;
			
			NAMI_LOG_INPUT_INTERRUPT(VeryVerbose, TEXT("[CameraAdjust] Blended模式：输入=%.3f，衰减=%.3f，有效权重=%.3f"),
				InputMagnitude, InputAttenuation, EffectiveWeight);
		}

		// 应用所有修改（包括 ArmRotation）
		TempAdjustConfig.ApplyToState(InOutState, EffectiveWeight);
	}
	else
	{
		// 没有视角控制，直接应用所有修改
		TempAdjustConfig.ApplyToState(InOutState, Weight);
	}
}

void UNamiCameraAdjustFeature::BlendParametersInState(FNamiCameraState& CurrentState, const FNamiCameraState& TargetState, float Weight, bool bFreezeRotations) const
{
	// 检查是否是打断的第一帧（通过检查 IsExiting() 和 Weight 来判断）
	const bool bIsFirstFrameOfExit = IsExiting() && Weight < 1.0f && Weight > 0.99f;  // Weight 接近 1.0 但小于 1.0 表示刚开始混出
	
	if (bIsFirstFrameOfExit)
	{
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("[CameraAdjust] ========== BlendParametersInState - 打断瞬间 =========="));
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  Weight: %.6f, bFreezeRotations: %d"), Weight, bFreezeRotations);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  CurrentState.ArmLength: %.2f -> TargetState.ArmLength: %.2f"), 
			CurrentState.ArmLength, TargetState.ArmLength);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  CurrentState.ArmRotation: (%.2f, %.2f, %.2f)"), 
			CurrentState.ArmRotation.Pitch, CurrentState.ArmRotation.Yaw, CurrentState.ArmRotation.Roll);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  TargetState.ArmRotation: (%.2f, %.2f, %.2f)"), 
			TargetState.ArmRotation.Pitch, TargetState.ArmRotation.Yaw, TargetState.ArmRotation.Roll);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  CurrentState.ArmOffset: (%.2f, %.2f, %.2f)"), 
			CurrentState.ArmOffset.X, CurrentState.ArmOffset.Y, CurrentState.ArmOffset.Z);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  TargetState.ArmOffset: (%.2f, %.2f, %.2f)"), 
			TargetState.ArmOffset.X, TargetState.ArmOffset.Y, TargetState.ArmOffset.Z);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  AdjustConfig.ArmLength.bEnabled: %d"), AdjustConfig.ArmLength.bEnabled);
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  AdjustConfig.ArmRotation.bEnabled: %d"), AdjustConfig.ArmRotation.bEnabled);
	}
	
	// 1. 混合位置相关参数（保证焦点不变）
	if (AdjustConfig.ArmLength.bEnabled)
	{
		const float ArmLengthBefore = CurrentState.ArmLength;
		CurrentState.ArmLength = FMath::Lerp(
			CurrentState.ArmLength,
			TargetState.ArmLength,
			Weight
		);
		
		if (bIsFirstFrameOfExit)
		{
			NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  ArmLength: %.2f -> %.2f (Delta: %.2f)"), 
				ArmLengthBefore, CurrentState.ArmLength, CurrentState.ArmLength - ArmLengthBefore);
		}
	}
	
	if (AdjustConfig.ArmRotation.bEnabled && !bFreezeRotations)
	{
		const FRotator ArmRotationBefore = CurrentState.ArmRotation;
		
		// 使用球面插值（Slerp）确保在球面上平滑过渡
		const FQuat CurrentQuat = CurrentState.ArmRotation.Quaternion();
		const FQuat TargetQuat = TargetState.ArmRotation.Quaternion();
		const FQuat BlendedQuat = FQuat::Slerp(CurrentQuat, TargetQuat, Weight);
		FRotator BlendedRotation = BlendedQuat.Rotator();
		BlendedRotation.Normalize();
		
		// 使用 FindDeltaAngleDegrees 确保最短路径，避免跳变
		CurrentState.ArmRotation.Pitch = CurrentState.ArmRotation.Pitch + 
			FMath::FindDeltaAngleDegrees(CurrentState.ArmRotation.Pitch, BlendedRotation.Pitch);
		CurrentState.ArmRotation.Yaw = CurrentState.ArmRotation.Yaw + 
			FMath::FindDeltaAngleDegrees(CurrentState.ArmRotation.Yaw, BlendedRotation.Yaw);
		CurrentState.ArmRotation.Roll = CurrentState.ArmRotation.Roll + 
			FMath::FindDeltaAngleDegrees(CurrentState.ArmRotation.Roll, BlendedRotation.Roll);
		CurrentState.ArmRotation.Normalize();
		
		if (bIsFirstFrameOfExit)
		{
			FRotator ArmRotationDelta = CurrentState.ArmRotation - ArmRotationBefore;
			ArmRotationDelta.Normalize();
			NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  ArmRotation: (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f) (Delta: %.2f, %.2f, %.2f)"), 
				ArmRotationBefore.Pitch, ArmRotationBefore.Yaw, ArmRotationBefore.Roll,
				CurrentState.ArmRotation.Pitch, CurrentState.ArmRotation.Yaw, CurrentState.ArmRotation.Roll,
				ArmRotationDelta.Pitch, ArmRotationDelta.Yaw, ArmRotationDelta.Roll);
		}
	}
	else if (bIsFirstFrameOfExit && AdjustConfig.ArmRotation.bEnabled)
	{
		NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  ArmRotation: 冻结（bFreezeRotations=true）"));
	}
	
	if (AdjustConfig.ArmOffset.bEnabled)
	{
		const FVector ArmOffsetBefore = CurrentState.ArmOffset;
		CurrentState.ArmOffset = FMath::Lerp(
			CurrentState.ArmOffset,
			TargetState.ArmOffset,
			Weight
		);
		
		if (bIsFirstFrameOfExit)
		{
			FVector ArmOffsetDelta = CurrentState.ArmOffset - ArmOffsetBefore;
			NAMI_LOG_INPUT_INTERRUPT(Warning, TEXT("  ArmOffset: (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f) (Delta: %.2f, %.2f, %.2f)"), 
				ArmOffsetBefore.X, ArmOffsetBefore.Y, ArmOffsetBefore.Z,
				CurrentState.ArmOffset.X, CurrentState.ArmOffset.Y, CurrentState.ArmOffset.Z,
				ArmOffsetDelta.X, ArmOffsetDelta.Y, ArmOffsetDelta.Z);
		}
	}
	
	// 2. 混合枢轴点参数（根据配置决定是否混合）
	if (AdjustConfig.PivotLocation.bEnabled)
	{
		CurrentState.PivotLocation = FMath::Lerp(
			CurrentState.PivotLocation,
			TargetState.PivotLocation,
			Weight
		);
	}
	
	if (AdjustConfig.PivotRotation.bEnabled && !bFreezeRotations)
	{
		// 使用0-360度归一化和最短路径混合
		// 优化：只在混合前归一化一次，混合后归一化一次
		FRotator NormalizedCurrent = FNamiCameraMath::NormalizeRotatorTo360(CurrentState.PivotRotation);
		FRotator NormalizedTarget = FNamiCameraMath::NormalizeRotatorTo360(TargetState.PivotRotation);
		
		float PitchDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Pitch, NormalizedTarget.Pitch);
		float YawDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Yaw, NormalizedTarget.Yaw);
		float RollDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Roll, NormalizedTarget.Roll);
		
		// 应用权重插值（结果会在最后归一化）
		CurrentState.PivotRotation.Pitch = NormalizedCurrent.Pitch + PitchDelta * Weight;
		CurrentState.PivotRotation.Yaw = NormalizedCurrent.Yaw + YawDelta * Weight;
		CurrentState.PivotRotation.Roll = NormalizedCurrent.Roll + RollDelta * Weight;
		// 确保结果在0-360度范围（只归一化一次）
		CurrentState.PivotRotation = FNamiCameraMath::NormalizeRotatorTo360(CurrentState.PivotRotation);
	}
	
	// 3. 混合其他参数
	if (AdjustConfig.FieldOfView.bEnabled)
	{
		CurrentState.FieldOfView = FMath::Lerp(
			CurrentState.FieldOfView,
			TargetState.FieldOfView,
			Weight
		);
	}
	
	// 相机旋转在混出时保持当前状态，不混合回默认值（如果配置允许）
	if (AdjustConfig.CameraRotationOffset.bEnabled && 
		!bFreezeRotations &&
		(!IsExiting() || !AdjustConfig.bPreserveCameraRotationOnExit))
	{
		// 使用0-360度归一化和最短路径混合
		// 优化：只在混合前归一化一次，混合后归一化一次
		FRotator NormalizedCurrent = FNamiCameraMath::NormalizeRotatorTo360(CurrentState.CameraRotationOffset);
		FRotator NormalizedTarget = FNamiCameraMath::NormalizeRotatorTo360(TargetState.CameraRotationOffset);
		
		float PitchDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Pitch, NormalizedTarget.Pitch);
		float YawDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Yaw, NormalizedTarget.Yaw);
		float RollDelta = FNamiCameraMath::FindDeltaAngle360(NormalizedCurrent.Roll, NormalizedTarget.Roll);
		
		// 应用权重插值（结果会在最后归一化）
		CurrentState.CameraRotationOffset.Pitch = NormalizedCurrent.Pitch + PitchDelta * Weight;
		CurrentState.CameraRotationOffset.Yaw = NormalizedCurrent.Yaw + YawDelta * Weight;
		CurrentState.CameraRotationOffset.Roll = NormalizedCurrent.Roll + RollDelta * Weight;
		// 确保结果在0-360度范围（只归一化一次）
		CurrentState.CameraRotationOffset = FNamiCameraMath::NormalizeRotatorTo360(CurrentState.CameraRotationOffset);
	}
	
	if (AdjustConfig.CameraLocationOffset.bEnabled)
	{
		CurrentState.CameraLocationOffset = FMath::Lerp(
			CurrentState.CameraLocationOffset,
			TargetState.CameraLocationOffset,
			Weight
		);
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
	
	// ControlRotation = PivotRotation（合并后，ArmRotation 不再影响 ControlRotation）
	OutView.ControlRotation = State.PivotRotation;
}

FVector UNamiCameraAdjustFeature::GetPivotLocation(const FNamiCameraView& View) const
{
	// 优先使用 View 的 PivotLocation
	if (!View.PivotLocation.IsNearlyZero())
	{
		return View.PivotLocation;
	}
	
	// 回退：尝试从 Pawn 获取
	UNamiCameraModeBase* Mode = GetCameraMode();
	if (Mode)
	{
		UNamiCameraComponent* CameraComponent = Mode->GetCameraComponent();
		if (CameraComponent)
		{
			APawn* OwnerPawn = CameraComponent->GetOwnerPawn();
			if (OwnerPawn)
			{
				return OwnerPawn->GetActorLocation();
			}
		}
	}
	
	// 最后回退：使用相机位置
	return View.CameraLocation;
}

FRotator UNamiCameraAdjustFeature::GetCharacterRotation(const FNamiCameraState& InState) const
{
	// 优先从 Pawn 获取角色朝向（Mesh 朝向）
	UNamiCameraModeBase* Mode = GetCameraMode();
	if (Mode)
	{
		UNamiCameraComponent* CameraComponent = Mode->GetCameraComponent();
		if (CameraComponent)
		{
			APawn* OwnerPawn = CameraComponent->GetOwnerPawn();
			if (OwnerPawn)
			{
				return OwnerPawn->GetActorRotation();
			}
		}
	}
	
	// 回退：使用 PivotRotation（相机朝向）
	return InState.PivotRotation;
}

FVector2D UNamiCameraAdjustFeature::GetPlayerCameraInputVector() const
{
	if (bInputVectorCacheValid)
	{
		return CachedInputVector;
	}
	
	FVector2D InputVector = FVector2D::ZeroVector;
	
	// 从 CameraMode 获取输入提供者
	UNamiCameraModeBase* Mode = GetCameraMode();
	if (IsValid(Mode))
	{
		UNamiCameraComponent* CameraComp = Mode->GetCameraComponent();
		if (IsValid(CameraComp))
		{
			APlayerController* PC = CameraComp->GetOwnerPlayerController();
			if (IsValid(PC) && PC->GetClass()->ImplementsInterface(UNamiCameraInputProvider::StaticClass()))
			{
				// 使用 Execute_ 方法调用，支持蓝图实现
				InputVector = INamiCameraInputProvider::Execute_GetCameraInputVector(PC);
			}
		}
	}
	
	CachedInputVector = InputVector;
	bInputVectorCacheValid = true;
	
	return InputVector;
}

float UNamiCameraAdjustFeature::GetPlayerCameraInputMagnitude() const
{
	if (bInputMagnitudeCacheValid)
	{
		return CachedInputMagnitude;
	}
	
	FVector2D InputVector = GetPlayerCameraInputVector();
	CachedInputMagnitude = InputVector.Size();
	bInputMagnitudeCacheValid = true;
	
	return CachedInputMagnitude;
}


