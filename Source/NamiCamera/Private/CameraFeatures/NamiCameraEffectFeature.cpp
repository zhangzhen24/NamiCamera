// Copyright Epic Games, Inc. All Rights Reserved.

#include "CameraFeatures/NamiCameraEffectFeature.h"
#include "Logging/LogNamiCamera.h"
#include "Logging/LogNamiCameraMacros.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraEffectFeature)

UNamiCameraEffectFeature::UNamiCameraEffectFeature()
	: CurrentBlendWeight(0.0f)
	, ActiveTime(0.0f)
	, bIsActive(false)
	, bIsExiting(false)
	, BlendOutStartWeight(1.0f)
	, bIsPaused(false)
	, bHasSavedOriginalView(false)
{
}

void UNamiCameraEffectFeature::Activate_Implementation()
{
	Super::Activate_Implementation();
	
	// 激活效果
	ActivateEffect(true);
}

void UNamiCameraEffectFeature::Deactivate_Implementation()
{
	Super::Deactivate_Implementation();
	
	// 根据结束行为决定如何停用
	switch (EndBehavior)
	{
	case ENamiCameraEndBehavior::BlendBack:
		DeactivateEffect(false);  // 平滑混合回去
		break;
	case ENamiCameraEndBehavior::ForceEnd:
		DeactivateEffect(true);   // 立即结束
		break;
	case ENamiCameraEndBehavior::Stay:
		// 保持效果，不自动停用
		break;
	}
}

void UNamiCameraEffectFeature::Update_Implementation(float DeltaTime)
{
	Super::Update_Implementation(DeltaTime);

	if (!bIsActive || bIsPaused)
	{
		return;
	}

	// 更新激活时间
	ActiveTime += DeltaTime;

	// 检查是否超时
	if (Duration > 0.0f && ActiveTime >= Duration && !bIsExiting)
	{
		DeactivateEffect(false);
	}

	// 计算混合权重
	CurrentBlendWeight = CalculateBlendWeight(DeltaTime);
}

void UNamiCameraEffectFeature::ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime)
{
	Super::ApplyToView_Implementation(InOutView, DeltaTime);

	// 如果权重为 0 且不需要保持激活，不应用任何效果
	if (CurrentBlendWeight <= KINDA_SMALL_NUMBER && !ShouldKeepActive(CurrentBlendWeight))
	{
		return;
	}

	// 首次应用时保存原始视图（用于退出时混合回去）
	if (!bHasSavedOriginalView && bIsActive && !bIsExiting)
	{
		OriginalView = InOutView;
		bHasSavedOriginalView = true;
	}

	// 调用子类的 ApplyEffect，传入混合权重
	ApplyEffect(InOutView, CurrentBlendWeight, DeltaTime);
}

void UNamiCameraEffectFeature::ActivateEffect(bool bResetTimer)
{
	if (bResetTimer || !bIsActive)
	{
		ActiveTime = 0.0f;
	}

	bIsActive = true;
	bIsExiting = false;
	bIsPaused = false;
	bEnabled = true;
	BlendOutStartWeight = 1.0f;
	bHasSavedOriginalView = false;
	CurrentBlendWeight = 0.0f;  // 从 0 开始混入

	NAMI_LOG_EFFECT(Log, TEXT("[UNamiCameraEffectFeature] 激活效果：%s，持续时间：%.2f"), 
		*EffectName.ToString(), Duration);
}

void UNamiCameraEffectFeature::DeactivateEffect(bool bForceImmediate)
{
	if (!bIsActive)
	{
		return;
	}

	if (bForceImmediate)
	{
		// 立即停止
		bIsActive = false;
		bIsExiting = false;
		CurrentBlendWeight = 0.0f;
		BlendOutStartWeight = 1.0f;
		bEnabled = false;
	}
	else
	{
		// 开始退出混合
		BlendOutStartWeight = CurrentBlendWeight;
		bIsExiting = true;
		ActiveTime = 0.0f;  // 重置时间用于 BlendOut
		
		NAMI_LOG_EFFECT(Warning, TEXT("[UNamiCameraEffectFeature] 开始混出：%s，起始权重=%.3f"), 
			*EffectName.ToString(), BlendOutStartWeight);
	}

	NAMI_LOG_EFFECT(Log, TEXT("[UNamiCameraEffectFeature] 停止效果：%s，立即：%s"), 
		*EffectName.ToString(), bForceImmediate ? TEXT("是") : TEXT("否"));
}

void UNamiCameraEffectFeature::PauseEffect()
{
	bIsPaused = true;
	NAMI_LOG_EFFECT(Verbose, TEXT("[UNamiCameraEffectFeature] 暂停效果：%s"), *EffectName.ToString());
}

void UNamiCameraEffectFeature::ResumeEffect()
{
	bIsPaused = false;
	NAMI_LOG_EFFECT(Verbose, TEXT("[UNamiCameraEffectFeature] 恢复效果：%s"), *EffectName.ToString());
}


float UNamiCameraEffectFeature::CalculateBlendWeight(float DeltaTime)
{
	float Weight = 1.0f;

	if (bIsExiting)
	{
		// 退出混合：权重从 BlendOutStartWeight 降到 0.0
		if (BlendOutTime > 0.0f)
		{
			// BlendAlpha: 0.0 -> 1.0 (随时间增加)
			float BlendAlpha = FMath::Clamp(ActiveTime / BlendOutTime, 0.0f, 1.0f);
			
			// 应用缓动函数到 BlendAlpha
			if (BlendCurve)
			{
				BlendAlpha = BlendCurve->GetFloatValue(BlendAlpha);
			}
			else
			{
				BlendAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, BlendAlpha, 2.0f);
			}
			
			// 权重从 BlendOutStartWeight 降到 0.0
			Weight = FMath::Lerp(BlendOutStartWeight, 0.0f, BlendAlpha);
		}
		else
		{
			Weight = 0.0f;
		}
	}
	else
	{
		// 进入混合：权重从 0.0 升到 1.0
		if (BlendInTime > 0.0f && ActiveTime < BlendInTime)
		{
			// BlendAlpha: 0.0 -> 1.0 (随时间增加)
			float BlendAlpha = FMath::Clamp(ActiveTime / BlendInTime, 0.0f, 1.0f);
			
			// 应用缓动函数
			if (BlendCurve)
			{
				Weight = BlendCurve->GetFloatValue(BlendAlpha);
			}
			else
			{
				Weight = FMath::InterpEaseInOut(0.0f, 1.0f, BlendAlpha, 2.0f);
			}
		}
	}

	return Weight;
}

void UNamiCameraEffectFeature::ApplyEffect_Implementation(FNamiCameraView& InOutView, float Weight, float DeltaTime)
{
	// 基类实现为空，子类应该重写
}

