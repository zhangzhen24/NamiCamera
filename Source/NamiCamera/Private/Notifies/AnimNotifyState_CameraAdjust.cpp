// Copyright Epic Games, Inc. All Rights Reserved.

#include "Notifies/AnimNotifyState_CameraAdjust.h"
#include "Features/NamiCameraAdjustFeature.h"
#include "Features/NamiCameraShakeFeature.h"
#include "Modes/NamiCameraModeBase.h"
#include "Features/NamiCameraFeature.h"
#include "Components/NamiCameraComponent.h"
#include "GameFramework/Actor.h"
#include "NamiCameraTags.h"
#include "LogNamiCamera.h"
#include "LogNamiCameraMacros.h"
#include "Logging/MessageLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyState_CameraAdjust)

UAnimNotifyState_CameraAdjust::UAnimNotifyState_CameraAdjust()
{
	// 默认配置
}

void UAnimNotifyState_CameraAdjust::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, 
												float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	if (!MeshComp)
	{
		return;
	}

	// ========== 获取 NamiCameraComponent ==========
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		NAMI_LOG_WARNING(TEXT("[ANS_CameraAdjust] NotifyBegin: 无法获取 Owner"));
		return;
	}

	UNamiCameraComponent* CameraComponent = Owner->FindComponentByClass<UNamiCameraComponent>();
	if (!CameraComponent)
	{
		NAMI_LOG_WARNING(TEXT("[ANS_CameraAdjust] NotifyBegin: 无法获取 NamiCameraComponent"));
		return;
	}

	// 获取当前激活的 Mode（用于添加 Feature）
	UNamiCameraModeBase* ActiveMode = CameraComponent->GetActiveCameraMode();
	if (!ActiveMode)
	{
		NAMI_LOG_WARNING(TEXT("[ANS_CameraAdjust] NotifyBegin: 无法获取激活的相机模式"));
		return;
	}

	// 生成唯一的效果名称（基于动画和实例）
	const FName StateEffectName = GenerateUniqueEffectName(Animation, TEXT("_State"));
	const FName ShakeEffectName = GenerateUniqueEffectName(Animation, TEXT("_Shake"));

	// 构建修改配置
	FNamiCameraStateModify StateModifyConfig = BuildStateModify();

	// 检查是否有任何修改或震动
	if (!StateModifyConfig.HasAnyModification() && !CameraShake)
	{
		NAMI_LOG_WARNING(TEXT("[ANS_CameraAdjust] NotifyBegin: 未配置任何相机修改或震动，跳过创建"));
		return;
	}

	// 互斥处理：按开关清理旧效果
	ClearExistingEffects(CameraComponent, ActiveMode, StateEffectName, ShakeEffectName);

	// ========== 创建 AdjustFeature ==========
	ActiveAdjustFeature = CreateAdjustFeature(CameraComponent, StateEffectName, TotalDuration);

	// ========== 创建 ShakeFeature ==========
	ActiveShakeFeature = CreateShakeFeature(ActiveMode, ShakeEffectName);

	NAMI_LOG_ANS(Log, TEXT("[ANS_CameraAdjust] NotifyBegin: 激活相机调整效果，混入时间: %.2f, 总时长: %.2f"),
		BlendInTime, TotalDuration);
}

void UAnimNotifyState_CameraAdjust::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
											  const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	// 保持效果，不停止 Feature
	if (EndBehavior == ENamiCameraEndBehavior::Stay)
	{
		NAMI_LOG_ANS(Log, TEXT("[ANS_CameraAdjust] NotifyEnd: 保持效果，需手动停止"));
		return;
	}

	// 停止 AdjustFeature 和 ShakeFeature
	// 直接调用 Feature 的 Deactivate()，让 Deactivate_Implementation 中的逻辑来处理 EndBehavior
	if (ActiveAdjustFeature.IsValid())
	{
		StopAdjustFeature();
	}
	if (ActiveShakeFeature.IsValid())
	{
		StopShakeFeature();
	}

	NAMI_LOG_ANS(Log, TEXT("[ANS_CameraAdjust] NotifyEnd: 结束相机调整效果，混出时间=%.2f"), BlendOutTime);
}

FString UAnimNotifyState_CameraAdjust::GetNotifyName_Implementation() const
{
	FNamiCameraStateModify StateModifyConfig = BuildStateModify();
	
	if (!StateModifyConfig.HasAnyModification() && !CameraShake)
	{
		return TEXT("相机调整");
	}

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

	// 输入打断模式
	if (ArmRotation.bEnabled)
	{
		switch (ControlMode)
		{
		case ENamiCameraControlMode::Suggestion:
			Effects.Add(TEXT("可打断"));
			break;
		case ENamiCameraControlMode::Forced:
			Effects.Add(TEXT("强制"));
			break;
		case ENamiCameraControlMode::Blended:
			Effects.Add(TEXT("混合"));
			break;
		}
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
		return FString::Printf(TEXT("相机调整: %s"), *FString::Join(Effects, TEXT(",")));
	}

	return TEXT("相机调整");
}

FNamiCameraStateModify UAnimNotifyState_CameraAdjust::BuildStateModify() const
{
	FNamiCameraStateModify StateModify;

	// 枢轴点参数
	StateModify.PivotLocation = PivotLocation;
	StateModify.PivotRotation = PivotRotation;

	// 吊臂参数
	StateModify.ArmLength = ArmLength;
	StateModify.ArmRotation = ArmRotation;
	StateModify.ArmOffset = ArmOffset;

	// 输入控制设置
	StateModify.bAllowPlayerInput = bAllowPlayerInput;
	
	// 输入打断设置（应用到 ArmRotation，仅在 bAllowPlayerInput = false 时生效）
	if (ArmRotation.bEnabled && !bAllowPlayerInput)
	{
		StateModify.ControlMode = ControlMode;
		StateModify.CameraInputThreshold = CameraInputThreshold;
	}

	// 相机参数
	StateModify.CameraLocationOffset = CameraLocationOffset;
	StateModify.CameraRotationOffset = CameraRotationOffset;
	StateModify.FieldOfView = FieldOfView;

	// 输出结果
	StateModify.CameraLocation = CameraLocation;
	StateModify.CameraRotation = CameraRotation;

	// 混出设置
	StateModify.bPreserveCameraRotationOnExit = bPreserveCameraRotationOnExit;

	return StateModify;
}

void UAnimNotifyState_CameraAdjust::ClearExistingEffects(UNamiCameraComponent* CameraComponent, UNamiCameraModeBase* ActiveMode, const FName& StateEffectName, const FName& ShakeEffectName) const
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

		NAMI_LOG_ANS(Log, TEXT("[ANS_CameraAdjust] 清理已有相机效果（bStopExistingEffects=true）"));
	}
}

UNamiCameraAdjustFeature* UAnimNotifyState_CameraAdjust::CreateAdjustFeature(UNamiCameraComponent* CameraComponent, const FName& StateEffectName, float TotalDuration)
{
	if (!CameraComponent)
	{
		return nullptr;
	}

	FNamiCameraStateModify StateModifyConfig = BuildStateModify();
	if (!StateModifyConfig.HasAnyModification())
	{
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
		NAMI_LOG_WARNING(TEXT("[ANS_CameraAdjust] CreateAdjustFeature: 无法获取激活的相机模式"));
		return nullptr;
	}

	// 创建新的 AdjustFeature（使用 CameraComponent 作为 Outer）
	UNamiCameraAdjustFeature* AdjustFeature = NewObject<UNamiCameraAdjustFeature>(CameraComponent);
	if (!AdjustFeature)
	{
		NAMI_LOG_WARNING(TEXT("[ANS_CameraAdjust] CreateAdjustFeature: 创建 AdjustFeature 失败"));
		return nullptr;
	}

	// 配置基本属性
	AdjustFeature->FeatureName = StateEffectName;
	AdjustFeature->Priority = 100;  // 高优先级，在其他 Feature 之后应用

	// 配置调整参数
	AdjustFeature->AdjustConfig = StateModifyConfig;

	// 配置生命周期
	// 如果结束行为是 Stay，Duration 应该设置为 0（无限），否则使用动画总时长
	if (EndBehavior == ENamiCameraEndBehavior::Stay)
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
			// 如果未配置来源 Tag，使用默认的 ANS CameraAdjust Tag
			AdjustFeature->Tags.AddTag(Tag_Camera_Feature_Source_ANS_CameraAdjust);
		}
	}
	else
	{
		AdjustFeature->Duration = (TotalDuration > KINDA_SMALL_NUMBER) ? TotalDuration : 0.0f;
	}
	AdjustFeature->BlendInTime = BlendInTime;
	AdjustFeature->BlendOutTime = BlendOutTime;
	AdjustFeature->EndBehavior = EndBehavior;
	
	// 配置平滑设置
	AdjustFeature->MaxPivotRotationSpeed = MaxPivotRotationSpeed;

	// 添加到 GlobalFeatures（而不是 Mode）
	CameraComponent->AddGlobalFeature(AdjustFeature);
	AdjustFeature->ActivateEffect(false);

	NAMI_LOG_ANS(Log, TEXT("[ANS_CameraAdjust] 创建 AdjustFeature 成功，效果名称: %s, Duration=%.2f"),
		*StateEffectName.ToString(), AdjustFeature->Duration);

	if ((BlendInTime + BlendOutTime) > (TotalDuration + KINDA_SMALL_NUMBER) && TotalDuration > 0.0f)
	{
		NAMI_LOG_ANS(Warning, TEXT("[ANS_CameraAdjust] BlendIn+BlendOut(%.2f) 大于动画时长(%.2f)，可能导致镜头提前/延后退出"),
			BlendInTime + BlendOutTime, TotalDuration);
	}

	return AdjustFeature;
}

UNamiCameraShakeFeature* UAnimNotifyState_CameraAdjust::CreateShakeFeature(UNamiCameraModeBase* ActiveMode, const FName& ShakeEffectName)
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

	NAMI_LOG_ANS(Log, TEXT("[ANS_CameraAdjust] 创建震动Feature成功：%s，强度：%.2f"),
		*CameraShake->GetName(), ShakeScale);

	return ShakeFeature;
}

void UAnimNotifyState_CameraAdjust::StopAdjustFeature()
{
	if (!ActiveAdjustFeature.IsValid())
	{
		return;
	}

	// 直接调用 Feature 的 Deactivate()，让 Deactivate_Implementation 中的逻辑来处理 EndBehavior
	ActiveAdjustFeature->Deactivate();

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

FName UAnimNotifyState_CameraAdjust::GenerateUniqueEffectName(const UAnimSequenceBase* Animation, const FString& Suffix) const
{
	if (!Animation)
	{
		return FName(*FString::Printf(TEXT("ANS_CameraAdjust%s"), *Suffix));
	}

	// 生成唯一名称：ANS_动画名_后缀_实例ID
	FString AnimationName = Animation->GetName();
	FString UniqueName = FString::Printf(TEXT("ANS_%s%s_%08X"),
		*AnimationName,
		*Suffix,
		GetUniqueID());

	return FName(*UniqueName);
}

void UAnimNotifyState_CameraAdjust::StopShakeFeature()
{
	if (!ActiveShakeFeature.IsValid())
	{
		return;
	}

	// 注意：ShakeFeature 没有 EndBehavior 属性，所以使用 ANS 的 EndBehavior
	bool bForceImmediate = (EndBehavior == ENamiCameraEndBehavior::ForceEnd);

	ActiveShakeFeature->StopShake(bForceImmediate);
	NAMI_LOG_ANS(Log, TEXT("[ANS_CameraAdjust] NotifyEnd: 停止震动Feature，立即=%s"),
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

#if WITH_EDITOR
void UAnimNotifyState_CameraAdjust::ValidateAssociatedAssets()
{
	Super::ValidateAssociatedAssets();

	// 至少需有一项修改或震动
	FNamiCameraStateModify StateModifyConfig = BuildStateModify();
	const bool bHasAnyModify = StateModifyConfig.HasAnyModification();
	if (!bHasAnyModify && !CameraShake)
	{
		FMessageLog("Animation").Warning()
			->AddToken(FTextToken::Create(NSLOCTEXT("NamiCamera", "CameraAdjust_NoEffect", "CameraAdjust: 未配置任何相机修改或震动")));
	}

	// Blend 时间校验
	if (BlendInTime < 0.f || BlendOutTime < 0.f)
	{
		FMessageLog("Animation").Warning()
			->AddToken(FTextToken::Create(NSLOCTEXT("NamiCamera", "CameraAdjust_BlendNegative", "CameraAdjust: Blend 时间不可为负")));
	}

	// 输入阈值校验
}
#endif

