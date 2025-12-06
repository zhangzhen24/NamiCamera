// Copyright Epic Games, Inc. All Rights Reserved.

#include "Notifies/AnimNotifyState_CameraAdjust.h"
#include "Features/NamiCameraAdjustFeature.h"
#include "Modes/NamiCameraModeBase.h"
#include "Features/NamiCameraFeature.h"
#include "Components/NamiCameraComponent.h"
#include "GameFramework/Actor.h"
#include "NamiCameraTags.h"
#include "LogNamiCamera.h"
#include "LogNamiCameraMacros.h"

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

	// 检查是否有任何修改
	if (!AdjustConfig.HasAnyModification())
	{
		NAMI_LOG_WARNING(TEXT("[ANS_CameraAdjust] NotifyBegin: 未配置任何相机修改，跳过创建"));
		return;
	}

	// ========== 创建 AdjustFeature ==========
	const FName EffectName = GenerateUniqueEffectName(Animation);
	ActiveAdjustFeature = CreateAdjustFeature(CameraComponent, EffectName, TotalDuration);

	if (ActiveAdjustFeature.IsValid())
	{
		NAMI_LOG_ANS(Log, TEXT("[ANS_CameraAdjust] NotifyBegin: 激活相机调整效果，效果名称: %s, 混入时间: %.2f, 总时长: %.2f"),
			*EffectName.ToString(), BlendInTime, TotalDuration);
	}
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

	// 停止 AdjustFeature
	// 直接调用 Feature 的 Deactivate()，让 Deactivate_Implementation 中的逻辑来处理 EndBehavior
	if (ActiveAdjustFeature.IsValid())
	{
		StopAdjustFeature();
		NAMI_LOG_ANS(Log, TEXT("[ANS_CameraAdjust] NotifyEnd: 结束相机调整效果，混出时间=%.2f"), BlendOutTime);
	}
}

FString UAnimNotifyState_CameraAdjust::GetNotifyName_Implementation() const
{
	if (!AdjustConfig.HasAnyModification())
	{
		return TEXT("相机调整");
	}

	TArray<FString> Effects;

	// 检查启用的修改项
	if (AdjustConfig.ArmLength.bEnabled)
	{
		if (AdjustConfig.ArmLength.BlendMode == ENamiCameraBlendMode::Additive)
		{
			Effects.Add(FString::Printf(TEXT("距离%+.0f"), AdjustConfig.ArmLength.Value));
		}
		else
		{
			Effects.Add(FString::Printf(TEXT("距离=%.0f"), AdjustConfig.ArmLength.Value));
		}
	}

	if (AdjustConfig.ArmRotation.bEnabled)
	{
		Effects.Add(TEXT("吊臂旋转"));
	}

	if (AdjustConfig.CameraRotationOffset.bEnabled)
	{
		Effects.Add(TEXT("视角偏移"));
	}

	if (AdjustConfig.FieldOfView.bEnabled)
	{
		if (AdjustConfig.FieldOfView.BlendMode == ENamiCameraBlendMode::Additive)
		{
			Effects.Add(FString::Printf(TEXT("FOV%+.0f"), AdjustConfig.FieldOfView.Value));
		}
		else
		{
			Effects.Add(FString::Printf(TEXT("FOV=%.0f"), AdjustConfig.FieldOfView.Value));
		}
	}

	if (Effects.Num() > 0)
	{
		return FString::Printf(TEXT("相机调整: %s"), *FString::Join(Effects, TEXT(",")));
	}

	return TEXT("相机调整");
}

UNamiCameraAdjustFeature* UAnimNotifyState_CameraAdjust::CreateAdjustFeature(UNamiCameraComponent* CameraComponent, const FName& EffectName, float TotalDuration)
{
	if (!CameraComponent)
	{
		return nullptr;
	}

	// 检查是否已存在同名 GlobalFeature
	UNamiCameraFeature* ExistingFeature = CameraComponent->FindGlobalFeatureByName(EffectName);
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
	AdjustFeature->FeatureName = EffectName;
	AdjustFeature->Priority = 100;  // 高优先级，在其他 Feature 之后应用

	// 配置调整参数
	AdjustFeature->AdjustConfig = AdjustConfig;

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

	// 添加到 GlobalFeatures（而不是 Mode）
	CameraComponent->AddGlobalFeature(AdjustFeature);
	AdjustFeature->ActivateEffect(false);

	return AdjustFeature;
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

FName UAnimNotifyState_CameraAdjust::GenerateUniqueEffectName(const UAnimSequenceBase* Animation) const
{
	if (!Animation)
	{
		return FName(*FString::Printf(TEXT("ANS_CameraAdjust_%08X"), GetUniqueID()));
	}

	// 生成唯一名称：ANS_动画名_实例ID
	FString AnimationName = Animation->GetName();
	FString UniqueName = FString::Printf(TEXT("ANS_%s_CameraAdjust_%08X"), 
		*AnimationName, 
		GetUniqueID());

	return FName(*UniqueName);
}

