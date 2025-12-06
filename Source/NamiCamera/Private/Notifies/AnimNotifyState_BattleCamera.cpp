// Copyright Epic Games, Inc. All Rights Reserved.

#include "Notifies/AnimNotifyState_BattleCamera.h"
#include "Features/NamiCameraAdjustFeature.h"
#include "Features/NamiCameraShakeFeature.h"
#include "Components/NamiCameraComponent.h"
#include "GameFramework/Actor.h"
#include "NamiCameraTags.h"
#include "LogNamiCamera.h"
#include "LogNamiCameraMacros.h"
#include "Logging/MessageLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyState_BattleCamera)

UAnimNotifyState_BattleCamera::UAnimNotifyState_BattleCamera()
{
	// 默认配置
}

void UAnimNotifyState_BattleCamera::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, 
												float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	// 生成唯一的效果名称（基于动画和实例）
	const FName StateEffectName = GenerateUniqueEffectName(Animation, TEXT("_State"));
	const FName ShakeEffectName = GenerateUniqueEffectName(Animation, TEXT("_Shake"));
	
	if (!MeshComp)
	{
		return;
	}

	// ========== 获取 NamiCameraComponent ==========
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		NAMI_LOG_WARNING(TEXT("[ANS_BattleCamera] NotifyBegin: 无法获取 Owner"));
		return;
	}

	UNamiCameraComponent* CameraComponent = Owner->FindComponentByClass<UNamiCameraComponent>();
	if (!CameraComponent)
	{
		NAMI_LOG_WARNING(TEXT("[ANS_BattleCamera] NotifyBegin: 无法获取 NamiCameraComponent"));
		return;
	}

	// 获取当前激活的 Mode（用于添加 Feature）
	UNamiCameraModeBase* ActiveMode = CameraComponent->GetActiveCameraMode();
	if (!ActiveMode)
	{
		NAMI_LOG_WARNING(TEXT("[ANS_BattleCamera] NotifyBegin: 无法获取激活的相机模式"));
		return;
	}

	// 互斥处理：按开关清理旧效果
	ClearExistingEffects(CameraComponent, ActiveMode, StateEffectName, ShakeEffectName);
	
	// ========== 创建 AdjustFeature（新架构）==========
	ActiveAdjustFeature = CreateAdjustFeature(CameraComponent, StateEffectName, TotalDuration);
	
	// ========== 创建震动 Feature（新架构）==========
	ActiveShakeFeature = CreateShakeFeature(ActiveMode, ShakeEffectName);
	
	// 总结日志：记录所有配置的参数
	TArray<FString> ConfigSummary;
	if (ArmLength.bEnabled)
	{
		ConfigSummary.Add(FString::Printf(TEXT("吊臂长度(模式=%d, 值=%.2f)"), (int32)ArmLength.BlendMode, ArmLength.Value));
	}
	if (ArmRotation.bEnabled)
	{
		ConfigSummary.Add(FString::Printf(TEXT("吊臂旋转(模式=%d, 值=%s)"), (int32)ArmRotation.BlendMode, *ArmRotation.Value.ToString()));
	}
	if (ArmOffset.bEnabled)
	{
		ConfigSummary.Add(FString::Printf(TEXT("吊臂偏移(模式=%d, 值=%s)"), (int32)ArmOffset.BlendMode, *ArmOffset.Value.ToString()));
	}
	if (ControlRotationOffset.bEnabled)
	{
		ConfigSummary.Add(FString::Printf(TEXT("控制旋转偏移(模式=%d, 值=%s)"), (int32)ControlMode, *ControlRotationOffset.Value.ToString()));
	}
	if (CameraRotationOffset.bEnabled)
	{
		ConfigSummary.Add(FString::Printf(TEXT("相机旋转偏移(模式=%d, 值=%s)"), (int32)CameraRotationOffset.BlendMode, *CameraRotationOffset.Value.ToString()));
	}
	if (FieldOfView.bEnabled)
	{
		ConfigSummary.Add(FString::Printf(TEXT("FOV(模式=%d, 值=%.2f)"), (int32)FieldOfView.BlendMode, FieldOfView.Value));
	}
	
	NAMI_LOG_ANS(Log, TEXT("[ANS_BattleCamera] NotifyBegin: 激活战斗相机效果，配置参数: [%s]"),
		ConfigSummary.Num() > 0 ? *FString::Join(ConfigSummary, TEXT(", ")) : TEXT("无"));
}

void UAnimNotifyState_BattleCamera::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
											  const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	// 保持效果，不停止任何 Feature
	if (EndBehavior == ENamiBattleCameraEndBehavior::Stay)
	{
		NAMI_LOG_ANS(Log, TEXT("[ANS_BattleCamera] NotifyEnd: 保持效果，需手动停止"));
		return;
	}
	
	// 停止AdjustFeature（新架构）
	// 直接调用 Feature 的 Deactivate()，让 Deactivate_Implementation 中的逻辑来处理 EndBehavior
	if (ActiveAdjustFeature.IsValid())
	{
		StopAdjustFeature();
	}
	
	// 停止震动 Feature（新架构）
	if (ActiveShakeFeature.IsValid())
	{
		StopShakeFeature();
	}
	
	NAMI_LOG_ANS(Log, TEXT("[ANS_BattleCamera] NotifyEnd: 结束战斗相机效果，混出时间=%.2f"), BlendOutTime);
}

FString UAnimNotifyState_BattleCamera::GetNotifyName_Implementation() const
{
	TArray<FString> Effects;
	
	// 吊臂参数
	if (ArmLength.bEnabled)
	{
		if (ArmLength.BlendMode == ENamiCameraBlendMode::Additive)
		{
			Effects.Add(FString::Printf(TEXT("距离%+.0f"), -ArmLength.Value));
		}
		else
		{
			Effects.Add(FString::Printf(TEXT("距离=%.0f"), ArmLength.Value));
		}
	}
	
	if (ArmRotation.bEnabled)
	{
		Effects.Add(TEXT("吊臂旋转"));
	}
	
	if (CameraRotationOffset.bEnabled)
	{
		Effects.Add(TEXT("视角偏移"));
	}
	
	if (FieldOfView.bEnabled)
	{
		if (FieldOfView.BlendMode == ENamiCameraBlendMode::Additive)
		{
			Effects.Add(FString::Printf(TEXT("FOV%+.0f"), FieldOfView.Value));
		}
		else
		{
			Effects.Add(FString::Printf(TEXT("FOV=%.0f"), FieldOfView.Value));
		}
	}
	
	// 控制旋转
	if (ControlRotationOffset.bEnabled)
	{
		switch (ControlMode)
		{
		case ENamiCameraControlMode::Suggestion:
			Effects.Add(TEXT("视角引导"));
			break;
		case ENamiCameraControlMode::Forced:
			Effects.Add(TEXT("强制视角"));
			break;
		case ENamiCameraControlMode::Blended:
			Effects.Add(TEXT("混合视角"));
			break;
		}
	}
	
	// 枢轴点参数
	if (PivotLocation.bEnabled)
	{
		Effects.Add(TEXT("枢轴位置"));
	}
	if (PivotRotation.bEnabled)
	{
		Effects.Add(TEXT("枢轴旋转"));
	}
	
	// 相机位置偏移
	if (CameraLocationOffset.bEnabled)
	{
		Effects.Add(TEXT("相机位置偏移"));
	}
	
	// 输出结果
	if (CameraLocation.bEnabled)
	{
		Effects.Add(TEXT("最终位置"));
	}
	if (CameraRotation.bEnabled)
	{
		Effects.Add(TEXT("最终旋转"));
	}
	
	// 效果
	if (CameraShake)
	{
		Effects.Add(TEXT("震动"));
	}
	
	if (Effects.Num() > 0)
	{
		return FString::Printf(TEXT("战斗相机: %s"), *FString::Join(Effects, TEXT(",")));
	}
	
	return TEXT("战斗相机");
}

#if WITH_EDITOR
void UAnimNotifyState_BattleCamera::ValidateAssociatedAssets()
{
	Super::ValidateAssociatedAssets();

	// 至少需有一项修改或震动
	const bool bHasAnyModify = BuildStateModify().HasAnyModification();
	if (!bHasAnyModify && !CameraShake)
	{
		FMessageLog("Animation").Warning()
			->AddToken(FTextToken::Create(NSLOCTEXT("NamiCamera", "BattleCamera_NoEffect", "BattleCamera: 未配置任何相机修改或震动")));
	}

	// Blend 时间校验
	if (BlendInTime < 0.f || BlendOutTime < 0.f)
	{
		FMessageLog("Animation").Warning()
			->AddToken(FTextToken::Create(NSLOCTEXT("NamiCamera", "BattleCamera_BlendNegative", "BattleCamera: Blend 时间不可为负")));
	}

	// 输入阈值校验
	if (CameraInputThreshold < 0.0f || CameraInputThreshold > 1.0f)
	{
		FMessageLog("Animation").Warning()
			->AddToken(FTextToken::Create(NSLOCTEXT("NamiCamera", "BattleCamera_InputThreshold", "BattleCamera: CameraInputThreshold 建议 0-1")));
	}
	if (bAllowInputInterrupt &&
		(InterruptInputThreshold < 0.0f || InterruptInputThreshold > 1.0f))
	{
		FMessageLog("Animation").Warning()
			->AddToken(FTextToken::Create(NSLOCTEXT("NamiCamera", "BattleCamera_InterruptThreshold", "BattleCamera: InterruptInputThreshold 建议 0-1")));
	}
}
#endif

FNamiCameraStateModify UAnimNotifyState_BattleCamera::BuildStateModify() const
{
	FNamiCameraStateModify StateModify;
	
	// 枢轴点参数
	StateModify.PivotLocation = PivotLocation;
	StateModify.PivotRotation = PivotRotation;
	
	// 吊臂参数
	StateModify.ArmLength = ArmLength;
	StateModify.ArmRotation = ArmRotation;
	StateModify.ArmOffset = ArmOffset;
	
	// 控制旋转参数
	StateModify.ControlRotationOffset = ControlRotationOffset;
	if (ControlRotationOffset.bEnabled)
	{
		StateModify.ControlMode = ControlMode;
		StateModify.CameraInputThreshold = CameraInputThreshold;
		StateModify.InputInterruptBlendTime = InputInterruptBlendTime;
	}
	
	// 相机参数
	StateModify.CameraLocationOffset = CameraLocationOffset;
	StateModify.CameraRotationOffset = CameraRotationOffset;
	StateModify.FieldOfView = FieldOfView;
	
	// 输出结果
	StateModify.CameraLocation = CameraLocation;
	StateModify.CameraRotation = CameraRotation;
	
	return StateModify;
}

ENamiCameraEndBehavior UAnimNotifyState_BattleCamera::ConvertEndBehavior(ENamiBattleCameraEndBehavior InBehavior) const
{
	switch (InBehavior)
	{
	case ENamiBattleCameraEndBehavior::BlendBack:
		return ENamiCameraEndBehavior::BlendBack;
	case ENamiBattleCameraEndBehavior::ForceEnd:
		return ENamiCameraEndBehavior::ForceEnd;
	case ENamiBattleCameraEndBehavior::Stay:
		return ENamiCameraEndBehavior::Stay;
	default:
		return ENamiCameraEndBehavior::BlendBack;
	}
}

FName UAnimNotifyState_BattleCamera::GenerateUniqueEffectName(const UAnimSequenceBase* Animation, const FString& Suffix) const
{
	if (!Animation)
	{
		// 如果没有动画，使用默认名称（向后兼容）
		return FName(*FString::Printf(TEXT("ANS_BattleCamera%s"), *Suffix));
	}
	
	// 生成唯一名称：ANS_动画名_后缀_实例ID
	FString AnimationName = Animation->GetName();
	FString UniqueName = FString::Printf(TEXT("ANS_%s%s_%08X"), 
		*AnimationName, 
		*Suffix,
		GetUniqueID());
	
	return FName(*UniqueName);
}

void UAnimNotifyState_BattleCamera::ClearExistingEffects(UNamiCameraComponent* CameraComponent, UNamiCameraModeBase* ActiveMode, const FName& StateEffectName, const FName& ShakeEffectName) const
{
	if (!CameraComponent || !ActiveMode)
	{
		return;
	}

	// 从 GlobalFeatures 移除 AdjustFeature
	auto RemoveAdjustFeatureByName = [CameraComponent](const FName& FeatureName)
	{
		if (UNamiCameraFeature* Existing = CameraComponent->FindGlobalFeatureByName(FeatureName))
		{
			if (UNamiCameraAdjustFeature* Adjust = Cast<UNamiCameraAdjustFeature>(Existing))
			{
				Adjust->DeactivateEffect(true);
				CameraComponent->RemoveGlobalFeature(Adjust);
				return;
			}
		}
	};

	// 从 Mode 移除 ShakeFeature
	auto RemoveShakeFeatureByName = [ActiveMode](const FName& FeatureName)
	{
		if (UNamiCameraFeature* Existing = ActiveMode->GetFeatureByName(FeatureName))
		{
			if (UNamiCameraShakeFeature* Shake = Cast<UNamiCameraShakeFeature>(Existing))
			{
				Shake->StopShake(true);
				ActiveMode->RemoveFeature(Shake);
				return;
			}
		}
	};

	// 同名互斥
	RemoveAdjustFeatureByName(StateEffectName);
	RemoveShakeFeatureByName(ShakeEffectName);

	// 可选整清
	if (bStopExistingEffects)
	{
		// 从 GlobalFeatures 移除所有 AdjustFeature
		TArray<UNamiCameraFeature*> GlobalFeaturesToRemove;
		for (UNamiCameraFeature* Feature : CameraComponent->GetGlobalFeatures())
		{
			if (!IsValid(Feature))
			{
				continue;
			}

			if (Feature->IsA<UNamiCameraAdjustFeature>())
			{
				GlobalFeaturesToRemove.Add(Feature);
			}
		}

		for (UNamiCameraFeature* Feature : GlobalFeaturesToRemove)
		{
			if (UNamiCameraAdjustFeature* Adjust = Cast<UNamiCameraAdjustFeature>(Feature))
			{
				Adjust->DeactivateEffect(true);
				CameraComponent->RemoveGlobalFeature(Adjust);
			}
		}

		// 从 Mode 移除所有 ShakeFeature
		TArray<UNamiCameraFeature*> ModeFeaturesToRemove;
		for (UNamiCameraFeature* Feature : ActiveMode->GetFeatures())
		{
			if (!IsValid(Feature))
			{
				continue;
			}

			if (Feature->IsA<UNamiCameraShakeFeature>())
			{
				ModeFeaturesToRemove.Add(Feature);
			}
		}

		for (UNamiCameraFeature* Feature : ModeFeaturesToRemove)
		{
			if (UNamiCameraShakeFeature* Shake = Cast<UNamiCameraShakeFeature>(Feature))
			{
				Shake->StopShake(true);
				ActiveMode->RemoveFeature(Shake);
			}
		}

		NAMI_LOG_ANS(Log, TEXT("[ANS_BattleCamera] 清理已有相机效果（bStopExistingEffects=true）"));
	}
}

UNamiCameraAdjustFeature* UAnimNotifyState_BattleCamera::CreateAdjustFeature(UNamiCameraComponent* CameraComponent, const FName& StateEffectName, float TotalDuration)
{
	if (!CameraComponent)
	{
		return nullptr;
	}

	FNamiCameraStateModify StateModifyConfig = BuildStateModify();
	if (!StateModifyConfig.HasAnyModification())
	{
		NAMI_LOG_WARNING(TEXT("[ANS_BattleCamera] StateModify 没有启用任何修改，跳过 AdjustFeature 创建"));
		return nullptr;
	}

	// 检查是否已存在同名 GlobalFeature
	UNamiCameraFeature* ExistingFeature = CameraComponent->FindGlobalFeatureByName(StateEffectName);
	if (ExistingFeature)
	{
		if (UNamiCameraAdjustFeature* ExistingAdjust = Cast<UNamiCameraAdjustFeature>(ExistingFeature))
		{
			ExistingAdjust->DeactivateEffect(true);
			CameraComponent->RemoveGlobalFeature(ExistingAdjust);
		}
	}

	// 获取当前激活的 Mode（用于初始化 Feature）
	UNamiCameraModeBase* ActiveMode = CameraComponent->GetActiveCameraMode();
	if (!ActiveMode)
	{
		NAMI_LOG_WARNING(TEXT("[ANS_BattleCamera] CreateAdjustFeature: 无法获取激活的相机模式"));
		return nullptr;
	}

	// 创建新的 AdjustFeature（使用 CameraComponent 作为 Outer）
	UNamiCameraAdjustFeature* AdjustFeature = NewObject<UNamiCameraAdjustFeature>(CameraComponent);
	if (!AdjustFeature)
	{
		return nullptr;
	}

	// 配置基本属性
	AdjustFeature->FeatureName = StateEffectName;
	AdjustFeature->Priority = 100;  // 高优先级，在其他 Feature 之后应用

	// 配置调整参数
	AdjustFeature->AdjustConfig = StateModifyConfig;

	// 配置生命周期
	// 如果结束行为是 Stay，Duration 应该设置为 0（无限），否则使用动画总时长
	ENamiCameraEndBehavior ConvertedEndBehavior = ConvertEndBehavior(EndBehavior);
	if (ConvertedEndBehavior == ENamiCameraEndBehavior::Stay)
	{
		AdjustFeature->Duration = 0.0f;  // 无限，需要手动停止
		
		// 添加手动清理 Tag，用于后续手动清理
		FGameplayTag TagToAdd = StayTag;
		if (!TagToAdd.IsValid())
		{
			// 如果未配置，使用默认值
			TagToAdd = Tag_Camera_Feature_ManualCleanup;
		}
		if (TagToAdd.IsValid())
		{
			AdjustFeature->Tags.AddTag(TagToAdd);
		}
		
		// 可选：添加来源标识 Tag
		if (SourceTag.IsValid())
		{
			AdjustFeature->Tags.AddTag(SourceTag);
		}
		else
		{
			// 如果未配置来源 Tag，使用默认的 ANS BattleCamera Tag
			AdjustFeature->Tags.AddTag(Tag_Camera_Feature_Source_ANS_BattleCamera);
		}
	}
	else
	{
		AdjustFeature->Duration = (TotalDuration > KINDA_SMALL_NUMBER) ? TotalDuration : 0.0f;
	}
	AdjustFeature->BlendInTime = BlendInTime;
	AdjustFeature->BlendOutTime = BlendOutTime;
	AdjustFeature->EndBehavior = ConvertedEndBehavior;

	// 配置输入打断
	AdjustFeature->bAllowInputInterruptWholeEffect = bAllowInputInterrupt;
	AdjustFeature->InterruptInputThreshold = FMath::Clamp(InterruptInputThreshold, 0.0f, 1.0f);
	AdjustFeature->InterruptCooldown = FMath::Max(0.0f, InterruptCooldown);
	AdjustFeature->InterruptBlendTime = FMath::Max(0.0f, InputInterruptBlendTime);
	const bool bEnableViewControl = ControlRotationOffset.bEnabled && ControlMode != ENamiCameraControlMode::Forced;
	AdjustFeature->bAllowViewControlInterrupt = bEnableViewControl;
	AdjustFeature->ViewControlInputThreshold = FMath::Clamp(CameraInputThreshold, 0.0f, 1.0f);

	// 添加到 GlobalFeatures（而不是 Mode）
	CameraComponent->AddGlobalFeature(AdjustFeature);
	AdjustFeature->ActivateEffect(false);

	NAMI_LOG_ANS(Log, TEXT("[ANS_BattleCamera] 创建 AdjustFeature 成功，效果名称: %s, Duration=%.2f"),
		*StateEffectName.ToString(), AdjustFeature->Duration);

	if ((BlendInTime + BlendOutTime) > (TotalDuration + KINDA_SMALL_NUMBER) && TotalDuration > 0.0f)
	{
		NAMI_LOG_ANS(Warning, TEXT("[ANS_BattleCamera] BlendIn+BlendOut(%.2f) 大于动画时长(%.2f)，可能导致镜头提前/延后退出"),
			BlendInTime + BlendOutTime, TotalDuration);
	}

	return AdjustFeature;
}

UNamiCameraShakeFeature* UAnimNotifyState_BattleCamera::CreateShakeFeature(UNamiCameraModeBase* ActiveMode, const FName& ShakeEffectName)
{
	if (!ActiveMode || !CameraShake)
	{
		return nullptr;
	}

	UNamiCameraShakeFeature* ShakeFeature = NewObject<UNamiCameraShakeFeature>(ActiveMode);
	if (!ShakeFeature)
	{
		return nullptr;
	}

	// 配置基本属性
	ShakeFeature->FeatureName = ShakeEffectName;
	ShakeFeature->Priority = 10;  // 低优先级，在其他 Feature 之前应用

	// 配置震动参数
	ShakeFeature->CameraShake = CameraShake;
	ShakeFeature->ShakeScale = ShakeScale;

	// 添加到 Mode
	ActiveMode->AddFeature(ShakeFeature);
	ShakeFeature->Activate();

	NAMI_LOG_ANS(Log, TEXT("[ANS_BattleCamera] 创建震动Feature成功：%s，强度：%.2f"),
		*CameraShake->GetName(), ShakeScale);

	return ShakeFeature;
}

void UAnimNotifyState_BattleCamera::StopAdjustFeature()
{
	if (!ActiveAdjustFeature.IsValid())
	{
		return;
	}

	// 直接调用 Feature 的 Deactivate()，让 Deactivate_Implementation 中的逻辑来处理 EndBehavior
	ActiveAdjustFeature->Deactivate();
	NAMI_LOG_ANS(Log, TEXT("[ANS_BattleCamera] NotifyEnd: 停止AdjustFeature"));

	// 根据 Feature 的 EndBehavior 来决定是否立即移除
	// ForceEnd 时，Deactivate_Implementation 会立即停用，此时可以立即移除
	// BlendBack 时，Feature 会在混出完成后自动停用，不需要立即移除
	if (ActiveAdjustFeature->EndBehavior == ENamiCameraEndBehavior::ForceEnd)
	{
		// 从 GlobalFeatures 移除（Feature 的 Outer 是 CameraComponent）
		UNamiCameraComponent* CameraComponent = Cast<UNamiCameraComponent>(ActiveAdjustFeature->GetOuter());
		if (CameraComponent)
		{
			CameraComponent->RemoveGlobalFeature(ActiveAdjustFeature.Get());
		}
	}

	ActiveAdjustFeature = nullptr;
}

void UAnimNotifyState_BattleCamera::StopShakeFeature()
{
	if (!ActiveShakeFeature.IsValid())
	{
		return;
	}

	// 根据 EndBehavior 决定是否立即停止
	// 注意：ShakeFeature 没有 EndBehavior 属性，所以使用 ANS 的 EndBehavior
	bool bForceImmediate = (EndBehavior == ENamiBattleCameraEndBehavior::ForceEnd);
	
	ActiveShakeFeature->StopShake(bForceImmediate);
	NAMI_LOG_ANS(Log, TEXT("[ANS_BattleCamera] NotifyEnd: 停止震动Feature，立即=%s"),
		bForceImmediate ? TEXT("是") : TEXT("否"));

	if (bForceImmediate)
	{
		if (UNamiCameraModeBase* Mode = ActiveShakeFeature->GetCameraMode())
		{
			Mode->RemoveFeature(ActiveShakeFeature.Get());
		}
	}

	ActiveShakeFeature = nullptr;
}


