// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modifiers/NamiCameraEffectModifierBase.h"
#include "LogNamiCamera.h"
#include "LogNamiCameraMacros.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraEffectModifierBase)

UNamiCameraEffectModifierBase::UNamiCameraEffectModifierBase()
{
	// 默认禁用，需要手动激活
	bDisabled = true;
}

bool UNamiCameraEffectModifierBase::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);
	
	if (!bIsActive || bIsPaused)
	{
		return false;
	}
	
	// 保存原始视图
	OriginalPOV = InOutPOV;
	
	// 更新激活时间
	ActiveTime += DeltaTime;
	
	// 检查是否超时
	if (Duration > 0.0f && ActiveTime >= Duration && !bIsExiting)
	{
		DeactivateEffect(false);
	}
	
	// 计算混合权重
	float Weight = CalculateBlendWeight(DeltaTime);
	CurrentBlendWeight = Weight;

	// 调试：首段时间内记录混合权重，排查首帧跳变
	if (ActiveTime <= 0.2f)
	{
		NAMI_LOG_BLEND_PROBE(Verbose, TEXT("[EffectBlendProbe] %s ActiveTime=%.4f Delta=%.4f BlendIn=%.3f BlendOut=%.3f Weight=%.4f Exiting=%d Active=%d"),
			*EffectName.ToString(),
			ActiveTime,
			DeltaTime,
			BlendInTime,
			BlendOutTime,
			Weight,
			bIsExiting ? 1 : 0,
			bIsActive ? 1 : 0);
	}
	
	// 检查是否应该继续运行（子类可以重写，如震动需要保持激活）
	if (ShouldKeepActive(Weight))
	{
		if (Weight <= 0.0f)
		{
			return true;  // 保持激活但权重为0
		}
	}
	
	// 如果权重为 0 且不需要保持激活，不应用任何效果
	// 注意：使用小的阈值避免浮点数精度问题，确保混出完成后再移除
	if (Weight <= KINDA_SMALL_NUMBER && !ShouldKeepActive(Weight))
	{
		if (bIsExiting)
		{
			// 完全退出，移除修改器
			// 确保混出时间已经完成（ActiveTime >= BlendOutTime）
			// 或者 BlendOutTime 为 0（立即退出）
			if (BlendOutTime <= 0.0f || ActiveTime >= BlendOutTime)
			{
				DisableModifier(true);
				bIsActive = false;
				bIsExiting = false;
				return false;
			}
			// 如果混出还没完成，继续执行 ApplyEffect 让退出逻辑落地（即使权重≈0）
		}
		else
		{
			return false;
		}
	}
	
	// 调用子类的应用方法
	return ApplyEffect(InOutPOV, Weight, DeltaTime);
}

void UNamiCameraEffectModifierBase::AddedToCamera(APlayerCameraManager* Camera)
{
	Super::AddedToCamera(Camera);
	
	NAMI_LOG_EFFECT(Log, TEXT("[UNamiCameraEffectModifierBase] 添加到相机：%s"), *EffectName.ToString());
}

void UNamiCameraEffectModifierBase::ActivateEffect(bool bResetTimer)
{
	if (bResetTimer || !bIsActive)
	{
		ActiveTime = 0.0f;
	}
	
	bIsActive = true;
	bIsExiting = false;
	bIsPaused = false;
	bDisabled = false;
	BlendOutStartWeight = 1.0f;  // 重置混出起始权重
	
	NAMI_LOG_EFFECT(Log, TEXT("[UNamiCameraEffectModifierBase] 激活效果：%s，持续时间：%.2f"), 
		*EffectName.ToString(), Duration);
}

void UNamiCameraEffectModifierBase::DeactivateEffect(bool bForceImmediate)
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
		DisableModifier(true);
	}
	else
	{
		// 开始退出混合
		// 记录当前混合权重作为混出起始权重（如果在混入过程中提前混出，从当前权重开始）
		BlendOutStartWeight = CurrentBlendWeight;
		bIsExiting = true;
		ActiveTime = 0.0f;  // 重置时间用于 BlendOut
		
		NAMI_LOG_EFFECT(Warning, TEXT("[UNamiCameraEffectModifierBase] 开始混出：%s，起始权重=%.3f"), 
			*EffectName.ToString(), BlendOutStartWeight);
	}
	
	NAMI_LOG_EFFECT(Log, TEXT("[UNamiCameraEffectModifierBase] 停止效果：%s，立即：%s"), 
		*EffectName.ToString(), bForceImmediate ? TEXT("是") : TEXT("否"));
}

void UNamiCameraEffectModifierBase::PauseEffect()
{
	bIsPaused = true;
	NAMI_LOG_EFFECT(Verbose, TEXT("[UNamiCameraEffectModifierBase] 暂停效果：%s"), *EffectName.ToString());
}

void UNamiCameraEffectModifierBase::ResumeEffect()
{
	bIsPaused = false;
	NAMI_LOG_EFFECT(Verbose, TEXT("[UNamiCameraEffectModifierBase] 恢复效果：%s"), *EffectName.ToString());
}

void UNamiCameraEffectModifierBase::InterruptEffect()
{
	if (!bIsActive)
	{
		return;
	}
	
	// 使用打断混合时间
	float OriginalBlendOutTime = BlendOutTime;
	BlendOutTime = InterruptBlendTime;
	
	DeactivateEffect(false);
	
	// 恢复原始混合时间（以防重复使用）
	BlendOutTime = OriginalBlendOutTime;
	
	NAMI_LOG_EFFECT(Log, TEXT("[UNamiCameraEffectModifierBase] 打断效果：%s，混合时间：%.2f"), 
		*EffectName.ToString(), InterruptBlendTime);
}

float UNamiCameraEffectModifierBase::CalculateBlendWeight(float DeltaTime)
{
	float Weight = 1.0f;
	
	if (bIsExiting)
	{
		// 退出混合：权重从 BlendOutStartWeight 降到 0.0
		// 如果在混入过程中提前混出，从当前权重开始混出，而不是从 1.0
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

