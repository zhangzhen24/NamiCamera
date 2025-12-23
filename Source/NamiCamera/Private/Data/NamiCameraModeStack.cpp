// Copyright Qiu, Inc. All Rights Reserved.

#include "Data/NamiCameraModeStack.h"
#include "Data/NamiCameraEnums.h"
#include "Logging/LogNamiCamera.h"
#include "Logging/LogNamiCameraMacros.h"
#include "Engine/Engine.h"
#include "CameraModes/NamiCameraModeBase.h"
#include "Data/NamiCameraView.h"

void FNamiCameraModeStack::PushCameraMode(UNamiCameraModeBase* CameraModeInstance)
{
	if (!IsValid(CameraModeInstance))
	{
		return;
	}

	// 检测是否到栈顶了
	int32 StackSize = CameraModeStack.Num();
	if (StackSize > 0 && CameraModeStack[0] == CameraModeInstance)
	{
		return;
	}

	// 堆栈Index && 堆栈贡献值
	int32 ExistingStackIndex = INDEX_NONE;
	float ExistingStackContribution = 1.0f;

	// 遍历堆栈，计算现有堆栈索引和贡献
	for (int32 StackIndex = 0; StackIndex < StackSize; ++StackIndex)
	{
		if (CameraModeStack[StackIndex] == CameraModeInstance)
		{
			ExistingStackIndex = StackIndex;
			ExistingStackContribution *= CameraModeInstance->GetBlendWeight();
			break;
		}
		else
		{
			ExistingStackContribution *= (1.0f - CameraModeStack[StackIndex]->GetBlendWeight());
		}
	}

	// 移除当前
	if (ExistingStackIndex != INDEX_NONE)
	{
		CameraModeStack.RemoveAt(ExistingStackIndex);
		StackSize--;
	}
	else
	{
		ExistingStackContribution = 0.0f;
	}

	// 计算起始权重
	const bool bShouldBlend = ((CameraModeInstance->GetBlendAlpha().BlendTime >= 0.0f) && (StackSize > 0));
	const float StartBlendWeight = (bShouldBlend ? ExistingStackContribution : 1.0f);

	NAMI_LOG_MODE_BLEND(Log,
		TEXT("[PushCameraMode] Mode=%s, bShouldBlend=%d, ExistingStackContribution=%.3f, StartBlendWeight=%.3f, StackSize=%d, BlendTime=%.3f"),
		*CameraModeInstance->GetName(),
		bShouldBlend ? 1 : 0,
		ExistingStackContribution,
		StartBlendWeight,
		StackSize,
		CameraModeInstance->GetBlendAlpha().BlendTime);

	CameraModeInstance->SetBlendWeight(StartBlendWeight);

	// 将新条目添加到堆栈
	CameraModeStack.Insert(CameraModeInstance, 0);

	// 让栈底（旧模式）准备淡出
	if (CameraModeStack.Num() > 1)
	{
		UNamiCameraModeBase* OldMode = CameraModeStack.Last();
		// 从当前权重平滑过渡到 0（淡出）
		float CurrentWeight = OldMode->GetBlendAlphaRef().GetBlendedValue();

		NAMI_LOG_MODE_BLEND(Log,
			TEXT("[PushCameraMode] OldMode=%s, CurrentWeight=%.3f -> 0.0 (fadeout)"),
			*OldMode->GetName(),
			CurrentWeight);

		// 1. 设置混合范围：从当前权重到 0
		OldMode->GetBlendAlphaRef().SetValueRange(CurrentWeight, 0.0f);
		// 2. 重置混合进度到起始位置，BlendedValue = Lerp(CurrentWeight, 0, 0) = CurrentWeight
		OldMode->GetBlendAlphaRef().SetAlpha(0.0f);
	}

	if (ExistingStackIndex == INDEX_NONE)
	{
		CameraModeInstance->Activate();
	}
}

bool FNamiCameraModeStack::EvaluateStack(float DeltaTime, FNamiCameraView& OutCameraModeView)
{
	if (!UpdateStack(DeltaTime))
	{
		return false;
	}

	BlendStack(OutCameraModeView, DeltaTime);

	return true;
}

void FNamiCameraModeStack::DumpCameraModeStack(const bool bPrintToScreen, const bool bPrintToLog, 
	const FLinearColor TextColor, const float Duration) const
{
	if (CameraModeStack.Num() == 0)
	{
		const FString Message = TEXT("[Camera Mode Stack]: Empty");
		if (bPrintToScreen && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, Duration, TextColor.ToFColor(true), Message);
		}
		if (bPrintToLog)
		{
			NAMI_LOG_STACK(Log, TEXT("%s"), *Message);
		}
		return;
	}

	// 打印所有相机模式和混合权重
	for (int32 StackIndex = 0; StackIndex < CameraModeStack.Num(); ++StackIndex)
	{
		UNamiCameraModeBase* CameraMode = CameraModeStack[StackIndex];
		check(CameraMode);

		const FString CameraModeName = CameraMode->GetName();
		const float BlendWeight = CameraMode->GetBlendWeight();
		const FString StatusName = StaticEnum<ENamiCameraModeState>()->GetNameStringByValue(
			static_cast<int64>(CameraMode->GetState()));
		
		const FString Message = FString::Printf(
			TEXT("[Stack %d] %s | Weight: %.3f | %s"),
			StackIndex,
			*CameraModeName,
			BlendWeight,
			*StatusName
		);

		if (bPrintToScreen && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1 - StackIndex,
				Duration,
				TextColor.ToFColor(true),
				Message
			);
		}
		if (bPrintToLog)
		{
			NAMI_LOG_STACK(Log, TEXT("%s"), *Message);
		}
	}
}

bool FNamiCameraModeStack::UpdateStack(float DeltaTime)
{
	const int32 StackSize = CameraModeStack.Num();
	if (StackSize <= 0)
	{
		return false;
	}

	bool bHasValidCameraMode = false;

	// 从后往前遍历，这样删除元素时不会影响索引
	for (int32 StackIndex = StackSize - 1; StackIndex >= 0; --StackIndex)
	{
		UNamiCameraModeBase* CameraMode = CameraModeStack[StackIndex];
		check(CameraMode);

		if (CameraMode->bIsActivated)
		{
			bHasValidCameraMode = true;
			CameraMode->Tick(DeltaTime);

			// 只在非栈顶模式权重为 0 时移除（已完全淡出）
			// 栈顶模式 (Index 0) 永远不移除，它是当前活跃模式
			// 使用小阈值避免浮点精度问题
			if (StackIndex > 0 && CameraMode->GetBlendWeight() <= KINDA_SMALL_NUMBER)
			{
				NAMI_LOG_MODE_BLEND(Log,
					TEXT("[UpdateStack] Removing mode %s at index %d (BlendWeight=%.6f, fully faded out)"),
					*CameraMode->GetName(),
					StackIndex,
					CameraMode->GetBlendWeight());

				CameraMode->Deactivate();
				CameraModeStack.RemoveAt(StackIndex);
			}
		}
	}

	return bHasValidCameraMode;
}

void FNamiCameraModeStack::BlendStack(FNamiCameraView& OutCameraModeView, float DeltaTime) const
{
	const int32 StackSize = CameraModeStack.Num();
	if (StackSize <= 0)
	{
		return;
	}

	// ========== 阶段0: 预计算所有 Mode 的混合权重（避免重复计算） ==========
	// 权重数组：索引对应 CameraModeStack 的索引
	TArray<float> PrecomputedWeights;
	PrecomputedWeights.SetNumUninitialized(StackSize);

	// 从栈底到栈顶计算权重
	// 栈底 (StackSize-1) 的权重就是它自己的 BlendWeight
	// 其他 Mode 的权重 = 自己的 BlendWeight * (1 - 后面所有 Mode 的 BlendWeight 的乘积)
	float AccumulatedWeight = 1.0f;
	for (int32 StackIndex = StackSize - 1; StackIndex >= 0; --StackIndex)
	{
		const float ModeBlendWeight = CameraModeStack[StackIndex]->GetBlendWeight();
		PrecomputedWeights[StackIndex] = ModeBlendWeight * AccumulatedWeight;
		AccumulatedWeight *= (1.0f - ModeBlendWeight);
	}

	// ========== 阶段1: PivotLocation 混合 ==========
	// 使用每个 Mode 的 View 中的 PivotLocation（已经包含了所有偏移）
	FVector BlendedPivotLocation = FVector::ZeroVector;
	float TotalWeight = 0.0f;

	// 从栈底到栈顶遍历
	for (int32 StackIndex = StackSize - 1; StackIndex >= 0; --StackIndex)
	{
		const float ModeWeight = PrecomputedWeights[StackIndex];
		if (ModeWeight > 0.0f)
		{
			const FVector ModePivotLocation = CameraModeStack[StackIndex]->GetView().PivotLocation;
			if (TotalWeight <= 0.0f)
			{
				// 第一个有效权重，直接设置
				BlendedPivotLocation = ModePivotLocation;
				TotalWeight = ModeWeight;
			}
			else
			{
				// 线性插值混合
				const float BlendAlpha = ModeWeight / (TotalWeight + ModeWeight);
				BlendedPivotLocation = FMath::Lerp(BlendedPivotLocation, ModePivotLocation, BlendAlpha);
				TotalWeight += ModeWeight;
			}
		}
	}

	// 如果所有权重都为 0，使用栈底 Mode 的 PivotLocation
	if (TotalWeight <= 0.0f)
	{
		BlendedPivotLocation = CameraModeStack[StackSize - 1]->GetView().PivotLocation;
	}

	// ========== 阶段2: 混合完整 View ==========
	// 获取栈底 Mode 的 View（作为基础）
	const UNamiCameraModeBase* BaseCameraMode = CameraModeStack[StackSize - 1];
	check(BaseCameraMode);
	OutCameraModeView = BaseCameraMode->GetView();
	OutCameraModeView.PivotLocation = BlendedPivotLocation;

	// 混合其他 Mode 的 View（从栈底到栈顶，跳过栈底）
	for (int32 StackIndex = StackSize - 2; StackIndex >= 0; --StackIndex)
	{
		const float ModeWeight = PrecomputedWeights[StackIndex];
		if (ModeWeight > 0.0f)
		{
			const UNamiCameraModeBase* CameraMode = CameraModeStack[StackIndex];
			check(CameraMode);

			// 混合 View（但 PivotLocation 已经混合好了）
			FNamiCameraView OtherView = CameraMode->GetView();
			OtherView.PivotLocation = BlendedPivotLocation;
			OutCameraModeView.Blend(OtherView, ModeWeight);

			// 确保 PivotLocation 保持为混合后的值
			OutCameraModeView.PivotLocation = BlendedPivotLocation;
		}
	}
}
