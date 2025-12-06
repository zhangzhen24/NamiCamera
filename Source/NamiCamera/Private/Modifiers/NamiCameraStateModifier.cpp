// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modifiers/NamiCameraStateModifier.h"
#include "GameFramework/PlayerController.h"
#include "Components/NamiCameraComponent.h"
#include "Interfaces/NamiCameraInputProvider.h"
#include "LogNamiCamera.h"
#include "LogNamiCameraMacros.h"
#include "Settings/NamiCameraSettings.h"
#include "Libraries/NamiCameraDebugLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraStateModifier)

UNamiCameraStateModifier::UNamiCameraStateModifier()
{
}

bool UNamiCameraStateModifier::ApplyEffect(FMinimalViewInfo &InOutPOV, float Weight, float DeltaTime)
{
	// 更新缓存的引用（每帧更新，但使用缓存避免重复查找）
	UpdateCachedReferences();
	// 检查整段输入打断
	CheckWholeEffectInterrupt(DeltaTime);

	NAMI_LOG_STATE(Warning, TEXT("[StateModifier] Begin bExiting=%d Weight=%.3f POV Loc=%s Rot=%s FOV=%.2f"),
				   bIsExiting ? 1 : 0,
				   Weight,
				   *InOutPOV.Location.ToString(),
				   *InOutPOV.Rotation.ToString(),
				   InOutPOV.FOV);

	// 每帧重置退出起点缓存；仅在进入退出阶段时才会重新填充
	if (!bIsExiting)
	{
		bHasExitArmStart = false;
	}

	if (bIsExiting && StateModify.ArmRotation.bEnabled && StateModify.ArmRotation.BlendMode == ENamiCameraBlendMode::Override)
	{
		NAMI_LOG_STATE(Warning, TEXT("[ApplyEffect] ★★★ 退出帧: Weight=%.3f, ExitStartCached=%d"), Weight, bHasExitArmStart ? 1 : 0);
	}
	else if (bIsExiting)
	{
		// 如果退出但 ArmRotation 未启用或不是 Override，也输出一次，确认条件
		NAMI_LOG_STATE(Warning, TEXT("[ApplyEffect] ★★★ 退出帧(Arm未命中): Weight=%.3f, ArmEnabled=%d, Mode=%d"),
					   Weight,
					   StateModify.ArmRotation.bEnabled ? 1 : 0,
					   (int32)StateModify.ArmRotation.BlendMode);
	}

	// 调试：观测 BlendIn 初期的权重与配置，排查首帧跳变
	if (Weight < 1.0f)
	{
		NAMI_LOG_BLEND_PROBE(Verbose, TEXT("[StateBlendProbe] %s Weight=%.4f ArmRotEnabled=%d Mode=%d ArmRotValue=%s CachedState=%d"),
							 *EffectName.ToString(),
							 Weight,
							 StateModify.ArmRotation.bEnabled ? 1 : 0,
							 (int32)StateModify.ArmRotation.BlendMode,
							 *StateModify.ArmRotation.Value.ToString(),
							 bHasCachedInitialState ? 1 : 0);
	}

	// 多目标构图：在构建初始状态前应用（作为基础输入）
	if (bEnableFraming && bUseModifierFraming && FramingTargets.Num() > 0)
	{
		FNamiCameraFramingResult FramingResult;
		if (ComputeFramingResult(DeltaTime, InOutPOV, FramingResult))
		{
			// 如果已有缓存初始状态，直接应用到缓存以参与后续 StateModify
			if (bHasCachedInitialState)
			{
				CachedInitialState.ApplyFramingResult(FramingResult);
				InitialPivotLocation = CachedInitialState.PivotLocation;
			}
			else
			{
				// 还未缓存，则先构建，再应用
				FVector PivotLocation = GetPivotLocation(InOutPOV);
				CachedInitialState = BuildInitialStateFromPOV(InOutPOV, PivotLocation);
				CachedInitialState.ApplyFramingResult(FramingResult);
				InitialPivotLocation = CachedInitialState.PivotLocation;
				bHasCachedInitialState = true;
			}

			NAMI_LOG_FRAMING(Verbose, TEXT("[UNamiCameraStateModifier] Framing 应用：Center=%s, Arm=%.2f"),
							 *CachedInitialState.PivotLocation.ToString(), CachedInitialState.ArmLength);

			// 可视化调试
			if (bEnableFramingDebug && UNamiCameraSettings::ShouldEnableFramingDebug() && CameraOwner)
			{
				TArray<FVector> TargetPositions;
				for (const TWeakObjectPtr<AActor> &T : FramingTargets)
				{
					if (T.IsValid())
					{
						TargetPositions.Add(T->GetActorLocation());
					}
				}
				if (TargetPositions.Num() > 0)
				{
					const UNamiCameraSettings *Settings = UNamiCameraSettings::Get();
					UNamiCameraDebugLibrary::DrawFramingDebug(
						CameraOwner->GetWorld(),
						InOutPOV.Location,
						InOutPOV.Rotation,
						InOutPOV.FOV,
						CachedInitialState.PivotLocation,
						CachedInitialState.ArmLength,
						FramingConfig.ScreenSafeZone,
						TargetPositions,
						Settings ? Settings->FramingDebugDuration : 0.0f,
						Settings ? Settings->FramingDebugThickness : 1.5f);

					if (Settings && Settings->bEnableFramingLog)
					{
						NAMI_LOG_FRAMING(Log, TEXT("[FramingDebug] Ctr=%s Arm=%.1f FOV=%.1f Targets=%d"),
										 *CachedInitialState.PivotLocation.ToString(),
										 CachedInitialState.ArmLength,
										 InOutPOV.FOV,
										 TargetPositions.Num());
					}
				}
			}
		}
	}
	else if (bEnableFraming && !bUseModifierFraming && !bFramingWarningLogged)
	{
		// 提示：构图建议在模式层 Feature 实现，此处默认不执行
		NAMI_LOG_WARNING(TEXT("[UNamiCameraStateModifier] 构图已启用但 bUseModifierFraming=false，当前未在 Modifier 内执行构图。建议在模式层 FramingFeature 计算。"));
		bFramingWarningLogged = true;
	}

	// 如果还没有构建初始状态，在应用效果之前先构建
	// 这样可以确保初始状态是基于"干净"的相机视图（未应用当前 Modifier 的效果）
	if (StateModify.HasAnyModification() && !bHasCachedInitialState)
	{
		// 获取枢轴点位置（使用优化后的方法）
		FVector PivotLocation = GetPivotLocation(InOutPOV);

		// 使用当前的 InOutPOV 构建初始状态（此时还未应用当前 Modifier 的效果）
		CachedInitialState = BuildInitialStateFromPOV(InOutPOV, PivotLocation);
		InitialPivotLocation = PivotLocation; // 保存初始 PivotLocation
		bHasCachedInitialState = true;

		// 重置上一次应用的状态（每次激活效果时都从初始状态开始）
		bHasLastAppliedState = false;

		NAMI_LOG_EFFECT(Verbose, TEXT("[UNamiCameraStateModifier::ApplyEffect] %s: 在应用效果前构建并缓存初始State, InitialPivotLocation=%s"),
						*EffectName.ToString(), *InitialPivotLocation.ToString());
	}

	// 使用 State 系统计算相机视图
	if (StateModify.HasAnyModification())
	{
		// 计算目标视图，再由输出混合层从当前视图平滑到目标视图
		const FMinimalViewInfo CurrentPOV = InOutPOV; // 当前实际 POV（其他 Modifier 已作用完）
		FMinimalViewInfo TargetPOV = InOutPOV;

		if (bIsExiting)
		{
			TargetPOV = InOutPOV;
			ApplyStateModifyAndComputeView(TargetPOV, Weight, false);
			InOutPOV = TargetPOV;
			NAMI_LOG_STATE(Warning, TEXT("[StateModifier] ExitApplied Loc=%s Rot=%s FOV=%.2f"),
						   *InOutPOV.Location.ToString(),
						   *InOutPOV.Rotation.ToString(),
						   InOutPOV.FOV);
			return true;
		}
		else
		{
			// 进场/维持：目标为满权重效果
			ApplyStateModifyAndComputeView(TargetPOV, Weight, /*bForTargetMix=*/true);
		}

		if (bSnapOutput)
		{
			InOutPOV = TargetPOV;
		}
		else
		{
			InOutPOV.Location = FMath::Lerp(CurrentPOV.Location, TargetPOV.Location, Weight);
			const FQuat FromQuat = CurrentPOV.Rotation.Quaternion();
			const FQuat ToQuat = TargetPOV.Rotation.Quaternion();
			InOutPOV.Rotation = FQuat::Slerp(FromQuat, ToQuat, Weight).Rotator();
			InOutPOV.FOV = FMath::Lerp(CurrentPOV.FOV, TargetPOV.FOV, Weight);
		}
		NAMI_LOG_STATE(Warning, TEXT("[StateModifier] Applied Loc=%s Rot=%s FOV=%.2f (Target=%s/%s/%.2f)"),
					   *InOutPOV.Location.ToString(),
					   *InOutPOV.Rotation.ToString(),
					   InOutPOV.FOV,
					   *TargetPOV.Location.ToString(),
					   *TargetPOV.Rotation.ToString(),
					   TargetPOV.FOV);
		return true;
	}

	return false;
}

void UNamiCameraStateModifier::AddedToCamera(APlayerCameraManager *Camera)
{
	Super::AddedToCamera(Camera);

	// 尝试自动设置输入提供者（从 PlayerController 获取）
	if (Camera)
	{
		APlayerController *PC = Camera->GetOwningPlayerController();
		if (PC)
		{
			// 如果 PlayerController 实现了接口，自动设置
			if (INamiCameraInputProvider *InputProviderInterface = Cast<INamiCameraInputProvider>(PC))
			{
				SetInputProvider(PC);
				NAMI_LOG_EFFECT(Verbose, TEXT("[UNamiCameraStateModifier::AddedToCamera] %s: 自动设置输入提供者（从PlayerController）"),
								*EffectName.ToString());
			}
		}
	}
}

void UNamiCameraStateModifier::SetInputProvider(TScriptInterface<INamiCameraInputProvider> InInputProvider)
{
	InputProvider = InInputProvider;

	if (InputProvider.GetInterface())
	{
		NAMI_LOG_EFFECT(Verbose, TEXT("[UNamiCameraStateModifier::SetInputProvider] %s: 设置输入提供者成功"),
						*EffectName.ToString());
	}
	else
	{
		NAMI_LOG_WARNING(TEXT("[UNamiCameraStateModifier::SetInputProvider] %s: 输入提供者无效"),
						 *EffectName.ToString());
	}
}

FNamiCameraState UNamiCameraStateModifier::BuildInitialStateFromPOV(const FMinimalViewInfo &POV, const FVector &PivotLocation) const
{
	FNamiCameraState State;

	// 从当前相机视图构建初始 State
	// 需要估算一些参数，因为 FMinimalViewInfo 不包含完整信息

	// 1. 枢轴点位置（使用传入的 PivotLocation，通常是角色位置）
	State.PivotLocation = PivotLocation;

	// 2. 计算吊臂方向（从 PivotLocation 指向 CameraLocation）
	FVector CameraToPivot = PivotLocation - POV.Location;
	float Distance = CameraToPivot.Size();

	// 3. 枢轴旋转（从相机位置反推）
	if (Distance > KINDA_SMALL_NUMBER)
	{
		FVector Direction = CameraToPivot.GetSafeNormal();
		State.PivotRotation = Direction.Rotation();
	}
	else
	{
		// 如果距离太近，使用相机旋转
		State.PivotRotation = POV.Rotation;
	}

	// 4. 吊臂长度
	State.ArmLength = Distance;

	// 5. 吊臂旋转：从相机位置反推出实际的 ArmRotation
	// 计算逻辑：
	// - 实际相机偏移向量 = CameraLocation - PivotLocation
	// - 基础偏移向量（ArmRotation=0时）= -BasePivotQuat.GetForwardVector() * ArmLength
	// - 计算两个向量的差异，反推出 ArmRotation（相对于 PivotRotation）
	// - 然后转换为相对于角色朝向的偏移
	if (Distance > KINDA_SMALL_NUMBER)
	{
		// 实际相机偏移向量（从 PivotLocation 指向 CameraLocation）
		FVector ActualCameraOffset = POV.Location - State.PivotLocation;

		// 基础偏移向量（如果 ArmRotation = 0）
		const FQuat BasePivotQuat = State.PivotRotation.Quaternion();
		FVector BaseCameraOffset = -BasePivotQuat.GetForwardVector() * State.ArmLength;

		// 将两个向量转到 PivotRotation 的本地空间
		FVector ActualOffsetLocal = BasePivotQuat.Inverse().RotateVector(ActualCameraOffset);
		FVector BaseOffsetLocal = BasePivotQuat.Inverse().RotateVector(BaseCameraOffset);

		// 计算本地空间的差异（归一化后计算旋转）
		if (!ActualOffsetLocal.IsNearlyZero() && !BaseOffsetLocal.IsNearlyZero())
		{
			ActualOffsetLocal.Normalize();
			BaseOffsetLocal.Normalize();

			// 计算从 BaseOffsetLocal 到 ActualOffsetLocal 的旋转（相对于 PivotRotation）
			FQuat RotationDiff = FQuat::FindBetweenNormals(BaseOffsetLocal, ActualOffsetLocal);
			FRotator PivotSpaceArmRotation = RotationDiff.Rotator();

			// 转换为相对于角色朝向的偏移
			// 获取角色朝向
			FRotator CharacterRotation = FRotator::ZeroRotator;
			if (CachedPawn.IsValid())
			{
				CharacterRotation = CachedPawn->GetActorRotation();
			}
			else
			{
				// 如果无法获取角色朝向，使用 PivotRotation 作为回退（保持原行为）
				CharacterRotation = State.PivotRotation;
			}

			// 转换：从 PivotRotation 空间转换到角色朝向空间
			// 在世界空间中：WorldRotation = PivotQuat * PivotSpaceArmQuat = CharacterQuat * CharacterSpaceArmQuat
			// 因此：CharacterSpaceArmQuat = CharacterQuat.Inverse() * PivotQuat * PivotSpaceArmQuat
			const FQuat CharacterQuat = CharacterRotation.Quaternion();
			const FQuat PivotSpaceArmQuat = PivotSpaceArmRotation.Quaternion();
			const FQuat CharacterSpaceArmQuat = CharacterQuat.Inverse() * BasePivotQuat * PivotSpaceArmQuat;
			State.ArmRotation = CharacterSpaceArmQuat.Rotator();

			// 使用 Warning 级别，确保能看到反推结果（用于调试 Additive vs Override）
			NAMI_LOG_WARNING(TEXT("[BuildInitialStateFromPOV] ★ 反推ArmRotation: 实际偏移=%s, 基础偏移=%s, PivotSpace=%s, CharacterSpace=%s, 角色朝向=%s"),
							 *ActualCameraOffset.ToString(), *BaseCameraOffset.ToString(),
							 *PivotSpaceArmRotation.ToString(), *State.ArmRotation.ToString(), *CharacterRotation.ToString());
		}
		else
		{
			State.ArmRotation = FRotator::ZeroRotator;
			NAMI_LOG_STATE(VeryVerbose, TEXT("[BuildInitialStateFromPOV] 无法反推ArmRotation: 向量过小"));
		}
	}
	else
	{
		// 距离太近，无法计算
		State.ArmRotation = FRotator::ZeroRotator;
		NAMI_LOG_STATE(VeryVerbose, TEXT("[BuildInitialStateFromPOV] 无法反推ArmRotation: 距离过近 (%.2f)"), Distance);
	}

	// 6. 相机旋转（使用当前相机旋转）
	State.CameraRotation = POV.Rotation;

	// 7. FOV
	State.FieldOfView = POV.FOV;

	// 8. 其他参数保持默认值
	State.ArmOffset = FVector::ZeroVector;
	State.CameraLocationOffset = FVector::ZeroVector;
	State.CameraRotationOffset = FRotator::ZeroRotator;
	State.ControlRotationOffset = FRotator::ZeroRotator;

	NAMI_LOG_STATE(VeryVerbose, TEXT("[UNamiCameraStateModifier::BuildInitialStateFromPOV] %s: 构建初始State - PivotLocation=%s, ArmLength=%.2f, PivotRotation=%s"),
				   *EffectName.ToString(), *State.PivotLocation.ToString(), State.ArmLength, *State.PivotRotation.ToString());

	return State;
}

void UNamiCameraStateModifier::ApplyStateModifyAndComputeView(FMinimalViewInfo &InOutPOV, float Weight, bool bForTargetMix /*=false*/)
{
	// 对于目标计算，强制使用满权重，并跳过退出混出逻辑
	Weight = bForTargetMix ? 1.0f : Weight;
	const bool bApplyExitLogic = bIsExiting && !bForTargetMix;

	// 1. 获取枢轴点位置（使用优化后的方法）
	FVector PivotLocation = GetPivotLocation(InOutPOV);

	// 2. 使用缓存的初始 State（应该在 ApplyEffect 中已经构建）
	// 如果还没有构建（理论上不应该发生），则现在构建
	if (!bHasCachedInitialState)
	{
		CachedInitialState = BuildInitialStateFromPOV(InOutPOV, PivotLocation);
		bHasCachedInitialState = true;

		NAMI_LOG_WARNING(TEXT("[UNamiCameraStateModifier::ApplyStateModifyAndComputeView] %s: 警告：在 ApplyStateModifyAndComputeView 中构建初始State（应该在 ApplyEffect 中构建）"),
						 *EffectName.ToString());
	}

	// 3. 使用上一次应用的状态（如果有），否则使用初始状态
	// 这样 Additive 和 Override 模式都能从上一次的结果继续，而不是每次都从初始状态开始
	FNamiCameraState CurrentState;
	if (bHasLastAppliedState)
	{
		// 从上一次的结果继续（确保 Additive 和 Override 模式正确工作）
		CurrentState = LastAppliedState;
		NAMI_LOG_STATE(VeryVerbose, TEXT("[ApplyStateModifyAndComputeView] 使用上一次应用的状态，ArmRotation=%s"),
					   *CurrentState.ArmRotation.ToString());
	}
	else
	{
		// 第一次应用，使用初始状态
		CurrentState = CachedInitialState;
		NAMI_LOG_STATE(VeryVerbose, TEXT("[ApplyStateModifyAndComputeView] 使用初始状态，ArmRotation=%s"),
					   *CurrentState.ArmRotation.ToString());
	}
	CurrentState.PivotLocation = PivotLocation; // 更新枢轴位置（角色可能移动了）

	// 3.5. 特殊处理 ArmRotation：Additive 和 Override 模式都应该基于初始状态，而不是当前状态
	// 问题：
	// - Additive 模式：如果基于当前状态（已修改），每帧都会累加，导致一直旋转
	// - Override 模式：如果基于当前状态（已修改），效果像 Additive，而不是基于角色朝向的覆盖
	// 解决方案：对于 ArmRotation，始终基于初始状态进行计算
	//
	// ArmRotation 应该是相对于角色朝向（Mesh 朝向）的偏移，而不是相对于 PivotRotation（相机朝向）
	// - Additive：在初始相对偏移基础上增加偏移
	// - Override：从初始相对偏移过渡到目标相对偏移（在相对角度空间中，基于角色朝向）
	FNamiCameraStateModify TempStateModify = StateModify;

	// 退出时的 ArmRotation 处理在 3.6 中单独处理，这里跳过
	if (TempStateModify.ArmRotation.bEnabled && !(bApplyExitLogic && TempStateModify.ArmRotation.BlendMode == ENamiCameraBlendMode::Override))
	{
		// 获取角色朝向（Mesh 朝向）
		FRotator CharacterRotation = FRotator::ZeroRotator;
		if (CachedPawn.IsValid())
		{
			CharacterRotation = CachedPawn->GetActorRotation();
		}
		else
		{
			// 如果无法获取角色朝向，回退到使用 PivotRotation（相机朝向）
			CharacterRotation = CurrentState.PivotRotation;
			NAMI_LOG_STATE(Warning, TEXT("[ApplyStateModifyAndComputeView] 无法获取角色朝向，使用 PivotRotation 作为回退"));
		}

		// 保存初始状态的 ArmRotation（相对于初始角色朝向的偏移）
		const FRotator InitialArmRotation = CachedInitialState.ArmRotation;

		// 根据混合模式计算目标值（在角色朝向的局部空间中）
		FRotator TargetArmRotation;
		if (TempStateModify.ArmRotation.BlendMode == ENamiCameraBlendMode::Additive)
		{
			// Additive：基于初始相对偏移进行 Additive
			TargetArmRotation = InitialArmRotation + TempStateModify.ArmRotation.Value * Weight;
		}
		else // Override
		{
			// Override：在相对角度空间中，从初始相对偏移过渡到目标相对偏移
			// 使用球面插值（Slerp）确保在球面上平滑过渡，实现"绕圈混合"效果
			// 这样相机会沿着以角色为中心的球面弧线移动，而不是直线移动
			const FQuat InitialQuat = InitialArmRotation.Quaternion();
			const FQuat TargetQuat = TempStateModify.ArmRotation.Value.Quaternion();
			const FQuat ResultQuat = FQuat::Slerp(InitialQuat, TargetQuat, Weight);
			TargetArmRotation = ResultQuat.Rotator();
		}

		// 注意：TargetArmRotation 现在是相对于角色朝向的偏移
		// 但 ComputeOutput 中 ArmRotation 是在 BasePivotRotation 的局部空间中应用的
		// 所以需要将 TargetArmRotation 从角色朝向空间转换到 PivotRotation 空间
		//
		// 转换原理：
		// - 在世界空间中，如果 ArmRotation 是相对于角色朝向的：WorldRotation = CharacterQuat * TargetArmQuat
		// - 在 ComputeOutput 中，ArmRotation 在 BasePivotQuat 空间中：WorldRotation = BasePivotQuat * PivotSpaceArmQuat
		// - 要让两者相等：BasePivotQuat * PivotSpaceArmQuat = CharacterQuat * TargetArmQuat
		// - 因此：PivotSpaceArmQuat = BasePivotQuat.Inverse() * CharacterQuat * TargetArmQuat
		const FQuat CharacterQuat = CharacterRotation.Quaternion();
		const FRotator BasePivotRotation = CurrentState.PivotRotation + CurrentState.ControlRotationOffset;
		const FQuat BasePivotQuat = BasePivotRotation.Quaternion();
		const FQuat TargetArmQuat = TargetArmRotation.Quaternion();
		const FQuat PivotSpaceArmQuat = BasePivotQuat.Inverse() * CharacterQuat * TargetArmQuat;
		TargetArmRotation = PivotSpaceArmQuat.Rotator();

		// 直接设置到 CurrentState，避免在 ApplyInputToState 中再次应用
		CurrentState.ArmRotation = TargetArmRotation;
		CurrentState.ChangedFlags.bArmRotation = true;

		// 临时禁用 ArmRotation 的修改，避免在 ApplyInputToState 中重复应用
		TempStateModify.ArmRotation.bEnabled = false;

		NAMI_LOG_STATE(Verbose, TEXT("[ApplyStateModifyAndComputeView] ArmRotation特殊处理: 角色朝向=%s, 初始相对偏移=%s, 目标相对偏移=%s, 转换后=%s, 模式=%d, 权重=%.3f"),
					   *CharacterRotation.ToString(),
					   *InitialArmRotation.ToString(),
					   *TempStateModify.ArmRotation.Value.ToString(),
					   *TargetArmRotation.ToString(),
					   (int32)TempStateModify.ArmRotation.BlendMode,
					   Weight);
	}

	// 3.6. 如果正在退出（效果停止），对于 Override 模式，需要将 Value 设置为初始值，以便从当前值混合回初始值
	if (bApplyExitLogic)
	{
		// 对于 ArmRotation，如果之前被特殊处理过（被禁用了），退出时需要在角色朝向空间中混合回初始值
		NAMI_LOG_STATE(Warning, TEXT("[ApplyStateModifyAndComputeView] ★★★ 退出检查: bIsExiting=%d, ArmRotation.bEnabled=%d, BlendMode=%d, Weight=%.3f"),
					   bApplyExitLogic ? 1 : 0,
					   StateModify.ArmRotation.bEnabled ? 1 : 0,
					   (int32)StateModify.ArmRotation.BlendMode,
					   Weight);

		if (StateModify.ArmRotation.bEnabled && StateModify.ArmRotation.BlendMode == ENamiCameraBlendMode::Override)
		{
			// 首次进入退出时缓存起点，避免每帧随基准旋转变化导致抖动/跳变
			if (!bHasExitArmStart)
			{
				FRotator CharacterRotation = FRotator::ZeroRotator;
				if (CachedPawn.IsValid())
				{
					CharacterRotation = CachedPawn->GetActorRotation();
				}
				else
				{
					CharacterRotation = CurrentState.PivotRotation;
					NAMI_LOG_STATE(Warning, TEXT("[ApplyStateModifyAndComputeView] 退出时无法获取角色朝向，使用 PivotRotation 作为回退"));
				}

				const FQuat CharacterQuat = CharacterRotation.Quaternion();
				const FRotator BasePivotRotation = CurrentState.PivotRotation + CurrentState.ControlRotationOffset;
				const FQuat BasePivotQuat = BasePivotRotation.Quaternion();

				const FQuat InitialQuat = CachedInitialState.ArmRotation.Quaternion();
				const FQuat TargetQuat = StateModify.ArmRotation.Value.Quaternion();
				const float StartAlpha = FMath::Clamp(Weight, 0.0f, 1.0f);
				const FQuat CurrentEffectCharacterSpaceArmQuat = FQuat::Slerp(InitialQuat, TargetQuat, StartAlpha);

				const FQuat CurrentEffectPivotSpaceArmQuat = BasePivotQuat.Inverse() * CharacterQuat * CurrentEffectCharacterSpaceArmQuat;
				const FQuat StartCharacterSpaceArmQuat = CharacterQuat.Inverse() * BasePivotQuat * CurrentEffectPivotSpaceArmQuat;

				ExitStartCharacterSpaceArmRotation = StartCharacterSpaceArmQuat.Rotator();
				ExitStartBasePivotQuat = BasePivotQuat;
				ExitStartCharacterQuat = CharacterQuat;
				bHasExitArmStart = true;

				NAMI_LOG_STATE(Warning, TEXT("[ApplyStateModifyAndComputeView] ★★★ 退出起点缓存: StartChar=%s, StartPivot=%s"),
							   *ExitStartCharacterSpaceArmRotation.ToString(),
							   *CurrentEffectPivotSpaceArmQuat.Rotator().ToString());
			}

			// 在角色朝向空间中，从起始值混合回初始值（使用球面插值）
			// Weight: 1.0 -> 0.0 (退出时)
			// 当 Weight = 1.0 时，应该是起始值（效果完全生效时的值）
			// 当 Weight = 0.0 时，应该是初始值（效果完全消失时的值）
			// 所以混合因子应该是 (1.0 - Weight)，从起始值到初始值
			const FQuat InitialCharacterSpaceArmQuat = CachedInitialState.ArmRotation.Quaternion();
			const FQuat StartCharacterSpaceArmQuat = ExitStartCharacterSpaceArmRotation.Quaternion();
			const float BlendAlpha = 1.0f - Weight; // Weight 从 1.0 降到 0.0，BlendAlpha 从 0.0 升到 1.0
			const FQuat ResultCharacterSpaceArmQuat = FQuat::Slerp(StartCharacterSpaceArmQuat, InitialCharacterSpaceArmQuat, BlendAlpha);
			const FRotator ResultCharacterSpaceArmRotation = ResultCharacterSpaceArmQuat.Rotator();

			// 将结果转换回 PivotRotation 空间
			// PivotSpaceArmQuat = BasePivotQuat.Inverse() * CharacterQuat * ResultCharacterSpaceArmQuat
			const FQuat ResultPivotSpaceArmQuat = ExitStartBasePivotQuat.Inverse() * ExitStartCharacterQuat * ResultCharacterSpaceArmQuat;
			CurrentState.ArmRotation = ResultPivotSpaceArmQuat.Rotator();

			NAMI_LOG_STATE(Verbose, TEXT("[ApplyStateModifyAndComputeView] ArmRotation退出: 起始CharacterSpace=%s, 初始CharacterSpace=%s, 结果CharacterSpace=%s, 结果PivotSpace=%s, Weight=%.3f, BlendAlpha=%.3f"),
						   *ExitStartCharacterSpaceArmRotation.ToString(),
						   *CachedInitialState.ArmRotation.ToString(),
						   *ResultCharacterSpaceArmRotation.ToString(),
						   *CurrentState.ArmRotation.ToString(),
						   Weight,
						   BlendAlpha);
		}

		// 对于其他 Override 模式的修改项，将 Value 设置为初始值
		if (TempStateModify.ArmLength.bEnabled && TempStateModify.ArmLength.BlendMode == ENamiCameraBlendMode::Override)
		{
			if (!bHasExitArmLengthStart)
			{
				ExitStartArmLength = CurrentState.ArmLength;
				bHasExitArmLengthStart = true;
			}
			const float BlendAlphaArmLen = 1.0f - Weight;
			CurrentState.ArmLength = FMath::Lerp(ExitStartArmLength, CachedInitialState.ArmLength, BlendAlphaArmLen);
			CurrentState.ChangedFlags.bArmLength = true;
			TempStateModify.ArmLength.bEnabled = false;
		}
		if (TempStateModify.PivotLocation.bEnabled && TempStateModify.PivotLocation.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempStateModify.PivotLocation.Value = CachedInitialState.PivotLocation;
		}
		if (TempStateModify.PivotRotation.bEnabled && TempStateModify.PivotRotation.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempStateModify.PivotRotation.Value = CachedInitialState.PivotRotation;
		}
		if (TempStateModify.ArmOffset.bEnabled && TempStateModify.ArmOffset.BlendMode == ENamiCameraBlendMode::Override)
		{
			if (!bHasExitArmOffsetStart)
			{
				ExitStartArmOffset = CurrentState.ArmOffset;
				bHasExitArmOffsetStart = true;
			}
			const float BlendAlphaArmOffset = 1.0f - Weight;
			CurrentState.ArmOffset = FMath::Lerp(ExitStartArmOffset, CachedInitialState.ArmOffset, BlendAlphaArmOffset);
			CurrentState.ChangedFlags.bArmOffset = true;
			TempStateModify.ArmOffset.bEnabled = false;
		}
		if (TempStateModify.ControlRotationOffset.bEnabled && TempStateModify.ControlRotationOffset.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempStateModify.ControlRotationOffset.Value = CachedInitialState.ControlRotationOffset;
		}
		if (TempStateModify.CameraLocationOffset.bEnabled && TempStateModify.CameraLocationOffset.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempStateModify.CameraLocationOffset.Value = CachedInitialState.CameraLocationOffset;
		}
		if (TempStateModify.CameraRotationOffset.bEnabled && TempStateModify.CameraRotationOffset.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempStateModify.CameraRotationOffset.Value = CachedInitialState.CameraRotationOffset;
		}
		if (TempStateModify.FieldOfView.bEnabled && TempStateModify.FieldOfView.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempStateModify.FieldOfView.Value = CachedInitialState.FieldOfView;
		}
		if (TempStateModify.CameraLocation.bEnabled && TempStateModify.CameraLocation.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempStateModify.CameraLocation.Value = CachedInitialState.CameraLocation;
		}
		if (TempStateModify.CameraRotation.bEnabled && TempStateModify.CameraRotation.BlendMode == ENamiCameraBlendMode::Override)
		{
			TempStateModify.CameraRotation.Value = CachedInitialState.CameraRotation;
		}
	}

	// 4. 检查视角控制是否应该被打断（使用 StateModify 中的配置）
	if (TempStateModify.ControlRotationOffset.bEnabled)
	{
		CheckViewControlInterrupt();

		// 如果被打断，不应用 ControlRotationOffset
		if (bViewControlInterrupted)
		{
			// 临时禁用 ControlRotationOffset（避免创建临时对象，直接修改后恢复）
			const bool bWasEnabled = TempStateModify.ControlRotationOffset.bEnabled;
			TempStateModify.ControlRotationOffset.bEnabled = false;
			TempStateModify.ApplyToState(CurrentState, Weight);
			TempStateModify.ControlRotationOffset.bEnabled = bWasEnabled; // 恢复原值
		}
		else
		{
			// 计算有效权重（混合模式下根据玩家输入衰减）
			float EffectiveWeight = Weight;
			if (TempStateModify.ControlMode == ENamiCameraControlMode::Blended)
			{
				// 使用缓存的输入大小（如果有效）
				if (!bInputMagnitudeCacheValid)
				{
					CachedInputMagnitude = GetPlayerCameraInputMagnitude();
					bInputMagnitudeCacheValid = true;
				}
				// 玩家输入越大，效果权重越低
				EffectiveWeight = Weight * (1.0f - FMath::Clamp(CachedInputMagnitude, 0.0f, 1.0f));
			}

			// 应用所有修改（包括 ControlRotationOffset）
			TempStateModify.ApplyToState(CurrentState, EffectiveWeight);
		}
	}
	else
	{
		// 没有视角控制，直接应用所有修改
		TempStateModify.ApplyToState(CurrentState, Weight);
	}

	// 5. 如果权重为 0 或极小，且不在退出状态，不应用任何修改，避免位置跳变
	// 注意：退出时即使权重为 0，也要执行混出逻辑，确保最后一帧正确应用
	if (Weight <= KINDA_SMALL_NUMBER && !bApplyExitLogic)
	{
		return;
	}

	// 6. 计算输出结果
	FVector PovBefore = InOutPOV.Location;
	CurrentState.ComputeOutput();

	// 保存应用后的状态，用于下一次应用（确保 Additive 和 Override 模式从上一次结果继续）
	LastAppliedState = CurrentState;
	bHasLastAppliedState = true;

	// 7. 应用计算结果到 POV（始终使用 Lerp，确保平滑过渡）
	// 保存原始 POV（用于平滑混合）
	FVector OriginalLocation = InOutPOV.Location;
	FRotator OriginalRotation = InOutPOV.Rotation;
	float OriginalFOV = InOutPOV.FOV;

	// 检测 PivotLocation 是否发生了显著变化（角色移动了）
	FVector PivotLocationDelta = PivotLocation - InitialPivotLocation;
	float PivotLocationDeltaSize = PivotLocationDelta.Size();

	// 如果 PivotLocation 变化很大（> 10cm），需要额外的平滑处理
	if (PivotLocationDeltaSize > 10.0f && Weight < 0.5f)
	{
		// 使用双重平滑：先按权重混合，再考虑 PivotLocation 变化
		float SmoothWeight = FMath::Clamp(Weight * 2.0f, 0.0f, 1.0f); // 在权重 < 0.5 时进行平滑
		InOutPOV.Location = FMath::Lerp(OriginalLocation, CurrentState.CameraLocation, SmoothWeight);
		InOutPOV.Rotation = FMath::Lerp(OriginalRotation, CurrentState.CameraRotation, SmoothWeight);
		InOutPOV.FOV = FMath::Lerp(OriginalFOV, CurrentState.FieldOfView, SmoothWeight);

		NAMI_LOG_STATE(VeryVerbose, TEXT("[UNamiCameraStateModifier::ApplyStateModifyAndComputeView] %s: PivotLocation变化=%.2f, 使用平滑过渡, 权重=%.3f->%.3f"),
					   *EffectName.ToString(), PivotLocationDeltaSize, Weight, SmoothWeight);
	}
	else
	{
		// 正常情况：使用权重进行 Lerp（从原始位置到计算位置）
		InOutPOV.Location = FMath::Lerp(OriginalLocation, CurrentState.CameraLocation, Weight);
		InOutPOV.Rotation = FMath::Lerp(OriginalRotation, CurrentState.CameraRotation, Weight);
		InOutPOV.FOV = FMath::Lerp(OriginalFOV, CurrentState.FieldOfView, Weight);
	}

	// 调试：BlendIn 初期记录位置写入，排查首帧跳变
	if (Weight < 1.0f)
	{
		NAMI_LOG_BLEND_PROBE(Verbose, TEXT("[StateBlendProbe] %s Weight=%.4f POVBefore=%s POVAfter=%s CurrentStateCam=%s InitialPivot=%s"),
							 *EffectName.ToString(),
							 Weight,
							 *PovBefore.ToString(),
							 *InOutPOV.Location.ToString(),
							 *CurrentState.CameraLocation.ToString(),
							 *InitialPivotLocation.ToString());
	}

	// 清除输入缓存（下一帧重新计算）
	bInputMagnitudeCacheValid = false;

	NAMI_LOG_STATE(VeryVerbose, TEXT("[UNamiCameraStateModifier::ApplyStateModifyAndComputeView] %s: 权重=%.3f, 最终Location=%s, Rotation=%s, FOV=%.2f"),
				   *EffectName.ToString(), Weight, *InOutPOV.Location.ToString(), *InOutPOV.Rotation.ToString(), InOutPOV.FOV);
}

FVector UNamiCameraStateModifier::GetPivotLocation(const FMinimalViewInfo &InOutPOV) const
{
	// 优先使用缓存的 Pawn
	if (CachedPawn.IsValid())
	{
		return CachedPawn->GetActorLocation();
	}

	// 如果无法获取，使用相机位置作为后备
	return InOutPOV.Location;
}

void UNamiCameraStateModifier::UpdateCachedReferences()
{
	// 更新缓存的 PlayerController 和 Pawn
	if (CameraOwner)
	{
		APlayerController *PC = CameraOwner->GetOwningPlayerController();
		if (PC != CachedPlayerController.Get())
		{
			CachedPlayerController = PC;
			CachedPawn = PC ? PC->GetPawn() : nullptr;
		}
		else if (PC && CachedPawn.Get() != PC->GetPawn())
		{
			// PlayerController 相同但 Pawn 可能变化了
			CachedPawn = PC->GetPawn();
		}
	}
	else
	{
		CachedPlayerController = nullptr;
		CachedPawn = nullptr;
	}
}

float UNamiCameraStateModifier::GetPlayerCameraInputMagnitude() const
{
	FVector2D InputVector = FVector2D::ZeroVector;

	// 优先使用输入提供者接口（解耦设计）
	if (InputProvider.GetInterface())
	{
		// 调用接口方法（如果类实现了 _Implementation，会调用实现；否则返回零向量）
		InputVector = InputProvider->Execute_GetCameraInputVector(InputProvider.GetObject());
	}
	else if (CameraOwner)
	{
		// 尝试从 PlayerController 获取接口
		APlayerController *PC = CameraOwner->GetOwningPlayerController();
		if (PC)
		{
			// 检查 PlayerController 是否实现了接口
			if (INamiCameraInputProvider *InputProviderInterface = Cast<INamiCameraInputProvider>(PC))
			{
				InputVector = InputProviderInterface->GetCameraInputVector();
			}
			else
			{
				// 没有实现接口，输出警告
				static bool bHasWarned = false;
				if (!bHasWarned)
				{
					NAMI_LOG_EFFECT(Warning, TEXT("[UNamiCameraStateModifier] PlayerController 未实现 INamiCameraInputProvider 接口，无法获取相机输入。请让 PlayerController 实现此接口。"));
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
				NAMI_LOG_EFFECT(Warning, TEXT("[UNamiCameraStateModifier] 无法获取 PlayerController，无法获取相机输入。"));
				bHasWarned = true;
			}
		}
	}
	else
	{
		// 没有 CameraOwner，输出警告
		static bool bHasWarned = false;
		if (!bHasWarned)
		{
			NAMI_LOG_EFFECT(Warning, TEXT("[UNamiCameraStateModifier] CameraOwner 为空，无法获取相机输入。"));
			bHasWarned = true;
		}
	}

	// 返回输入向量的大小
	return InputVector.Size();
}

void UNamiCameraStateModifier::CheckViewControlInterrupt()
{
	// 使用 StateModify 中的配置
	bool bHasViewControl = StateModify.ControlRotationOffset.bEnabled;
	ENamiCameraControlMode ControlMode = StateModify.ControlMode;
	float InputThreshold = StateModify.CameraInputThreshold;

	if (!bHasViewControl || bViewControlInterrupted)
	{
		return;
	}

	// 强制模式不检测打断
	if (ControlMode == ENamiCameraControlMode::Forced)
	{
		return;
	}

	// 建议模式：检测玩家输入并打断
	if (ControlMode == ENamiCameraControlMode::Suggestion)
	{
		// 使用缓存的输入大小（如果有效），否则重新计算
		float InputMagnitude;
		if (bInputMagnitudeCacheValid)
		{
			InputMagnitude = CachedInputMagnitude;
		}
		else
		{
			InputMagnitude = GetPlayerCameraInputMagnitude();
			CachedInputMagnitude = InputMagnitude;
			bInputMagnitudeCacheValid = true;
		}

		if (InputMagnitude > InputThreshold)
		{
			// 玩家有输入，打断视角控制
			bViewControlInterrupted = true;

			NAMI_LOG_EFFECT(Log, TEXT("[UNamiCameraStateModifier] 视角控制被玩家输入打断：%s，输入大小：%.3f，阈值：%.3f"),
							*EffectName.ToString(), InputMagnitude, InputThreshold);

			// 触发效果打断（使用打断混合时间退出视角控制部分）
			// 注意：只打断视角控制，其他效果（如震动、FOV）继续生效
		}

		LastCameraInputMagnitude = InputMagnitude;
	}
}

void UNamiCameraStateModifier::CheckWholeEffectInterrupt(float DeltaTime)
{
	// 冷却计时
	if (InterruptCooldownRemaining > 0.0f)
	{
		InterruptCooldownRemaining = FMath::Max(0.0f, InterruptCooldownRemaining - DeltaTime);
	}

	// 仅当允许、未处于退出阶段时检测
	if (!bAllowInputInterrupt || bIsExiting)
	{
		return;
	}

	// 获取输入大小
	float InputMagnitude = GetPlayerCameraInputMagnitude();
	LastCameraInputMagnitude = InputMagnitude;

	// 在冷却中不触发
	if (InterruptCooldownRemaining > 0.0f)
	{
		return;
	}

	if (InputMagnitude > InputInterruptThreshold)
	{
		// 触发整段技能镜头打断（使用基类的 InterruptEffect -> InterruptBlendTime）
		InterruptEffect();
		InterruptCooldownRemaining = InputInterruptCooldown;

		NAMI_LOG_EFFECT(Log, TEXT("[UNamiCameraStateModifier] 整段技能镜头被输入打断：%s，输入=%.3f 阈值=%.3f 冷却=%.3f"),
						*EffectName.ToString(), InputMagnitude, InputInterruptThreshold, InputInterruptCooldown);
	}
}

void UNamiCameraStateModifier::SetFramingTargets(const TArray<AActor *> &InTargets)
{
	FramingTargets.Reset();
	for (AActor *Target : InTargets)
	{
		if (IsValid(Target))
		{
			FramingTargets.Add(Target);
		}
	}
}

void UNamiCameraStateModifier::ClearFramingTargets()
{
	FramingTargets.Reset();
}

bool UNamiCameraStateModifier::ComputeFramingResult(float DeltaTime, const FMinimalViewInfo &InOutPOV, FNamiCameraFramingResult &OutResult)
{
	TArray<FVector> Positions;
	for (const TWeakObjectPtr<AActor> &Target : FramingTargets)
	{
		if (Target.IsValid())
		{
			Positions.Add(Target->GetActorLocation());
		}
	}
	if (Positions.Num() == 0)
	{
		return false;
	}

	// 计算中心
	FVector Center = FVector::ZeroVector;
	for (const FVector &Pos : Positions)
	{
		Center += Pos;
	}
	Center /= Positions.Num();

	// 计算包围半径
	float MaxRadius = 0.0f;
	for (const FVector &Pos : Positions)
	{
		MaxRadius = FMath::Max(MaxRadius, FVector::Dist(Pos, Center));
	}
	MaxRadius += FramingConfig.bEnableFraming ? 50.0f : 0.0f; // 轻微填充

	// 根据 FOV 和期望屏占比估算距离
	const float DesiredPercent = FMath::Clamp(FramingConfig.DesiredScreenPercentage, 0.05f, 0.8f);
	const float HalfFovRadians = FMath::DegreesToRadians(FMath::Max(1.0f, InOutPOV.FOV * 0.5f));
	float NeededDistance = MaxRadius / FMath::Tan(HalfFovRadians) / DesiredPercent;

	// 限制距离
	NeededDistance = FMath::Clamp(NeededDistance, FramingConfig.MinArmLength, FramingConfig.MaxArmLength);

	// 平滑
	const float LerpAlpha = (FramingConfig.LerpSpeed > 0.0f) ? FMath::Clamp(DeltaTime * FramingConfig.LerpSpeed, 0.0f, 1.0f) : 1.0f;
	FVector SmoothedCenter = FMath::Lerp(LastFramingCenter, Center + FVector(0, 0, FramingConfig.VerticalOffset), LerpAlpha);
	float SmoothedDistance = FMath::Lerp(LastFramingDistance, NeededDistance, LerpAlpha);

	// 生成结果
	OutResult.bHasResult = true;
	OutResult.TargetCenter = SmoothedCenter;
	OutResult.RecommendedArmLength = SmoothedDistance;
	OutResult.RecommendedArmRotation = FRotator::ZeroRotator;

	// 侧移偏好：沿 POV 右向量偏移
	if (!FMath::IsNearlyZero(FramingConfig.SideBias))
	{
		const FVector Right = FRotationMatrix(InOutPOV.Rotation).GetScaledAxis(EAxis::Y);
		OutResult.RecommendedCameraOffset = Right * (SmoothedDistance * 0.1f * FramingConfig.SideBias);
	}

	// 更新缓存
	LastFramingCenter = SmoothedCenter;
	LastFramingDistance = SmoothedDistance;

	return true;
}
