// Copyright Epic Games, Inc. All Rights Reserved.

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

	int32 RemoveCount = 0;
	int32 RemoveIndex = INDEX_NONE;
	bool bHasValidCameraMode = false;

	for (int32 StackIndex = 0; StackIndex < StackSize; ++StackIndex)
	{
		UNamiCameraModeBase* CameraMode = CameraModeStack[StackIndex];
		check(CameraMode);

		if (CameraMode->bIsActivated)
		{
			bHasValidCameraMode = true;
			CameraMode->Tick(DeltaTime);

			if (CameraMode->GetBlendWeight() >= 1.0f)
			{
				RemoveIndex = (StackIndex + 1);
				RemoveCount = (StackSize - RemoveIndex);
				break;
			}
		}
	}

	if (RemoveCount > 0)
	{
		for (int32 StackIndex = RemoveIndex; StackIndex < StackSize; ++StackIndex)
		{
			UNamiCameraModeBase* CameraMode = CameraModeStack[StackIndex];
			check(CameraMode);
			CameraMode->Deactivate();
		}
		CameraModeStack.RemoveAt(RemoveIndex, RemoveCount);
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

	// ========== 阶段1: PivotLocation计算和混合 ==========
	// 使用每个Mode的View中的PivotLocation（已经包含了所有偏移，如PivotHeightOffset）
	// 这样可以确保混合的PivotLocation与最终View中的PivotLocation一致
	
	// 收集所有Mode的PivotLocation和权重
	struct FPivotLocationData
	{
		FVector PivotLocation;
		float Weight;
	};
	
	TArray<FPivotLocationData> PivotLocations;
	PivotLocations.Reserve(StackSize);
	
	// 从栈底到栈顶遍历（权重从高到低）
	for (int32 StackIndex = StackSize - 1; StackIndex >= 0; --StackIndex)
	{
		const UNamiCameraModeBase* CameraMode = CameraModeStack[StackIndex];
    check(CameraMode);
		
		// 使用Mode的View中的PivotLocation（已经包含了所有偏移）
		// 这确保了PivotLocation与CalculateView中计算的值一致
		FVector ModePivotLocation = CameraMode->GetView().PivotLocation;
		
		// 计算该Mode的混合权重（考虑堆栈权重）
		float ModeWeight = CameraMode->GetBlendWeight();
		if (StackIndex < StackSize - 1)
		{
			// 对于非栈底Mode，需要考虑前面Mode的权重
			float AccumulatedWeight = 1.0f;
			for (int32 PrevIndex = StackSize - 1; PrevIndex > StackIndex; --PrevIndex)
			{
				AccumulatedWeight *= (1.0f - CameraModeStack[PrevIndex]->GetBlendWeight());
			}
			ModeWeight *= AccumulatedWeight;
		}
		
		PivotLocations.Add({ModePivotLocation, ModeWeight});
	}
	
	// 混合所有PivotLocation（从栈底到栈顶，权重从高到低）
	FVector BlendedPivotLocation = FVector::ZeroVector;
	float TotalWeight = 0.0f;
	
	for (int32 i = PivotLocations.Num() - 1; i >= 0; --i)
	{
		const FPivotLocationData& Data = PivotLocations[i];
		if (Data.Weight > 0.0f)
		{
			if (TotalWeight <= 0.0f)
			{
				// 第一个有效权重，直接设置
				BlendedPivotLocation = Data.PivotLocation;
				TotalWeight = Data.Weight;
			}
			else
			{
				// 线性插值混合
				float BlendAlpha = Data.Weight / (TotalWeight + Data.Weight);
				BlendedPivotLocation = FMath::Lerp(BlendedPivotLocation, Data.PivotLocation, BlendAlpha);
				TotalWeight += Data.Weight;
			}
		}
	}
	
	// 如果所有权重都为0，使用栈底Mode的PivotLocation
	if (TotalWeight <= 0.0f && StackSize > 0)
	{
		BlendedPivotLocation = PivotLocations[0].PivotLocation;
	}
	
	// ========== 阶段2-4: 计算和混合完整View ==========
	// 获取栈底Mode的View（作为基础）
	const UNamiCameraModeBase* BaseCameraMode = CameraModeStack[StackSize - 1];
	check(BaseCameraMode);
	OutCameraModeView = BaseCameraMode->GetView();
	
	// 应用混合后的PivotLocation
	OutCameraModeView.PivotLocation = BlendedPivotLocation;
	
	// 混合其他Mode的View（从栈底到栈顶）
    for (int32 StackIndex = (StackSize - 2); StackIndex >= 0; --StackIndex)
    {
		const UNamiCameraModeBase* CameraMode = CameraModeStack[StackIndex];
        check(CameraMode);
		
		// 计算该Mode的混合权重（考虑堆栈权重）
		float ModeWeight = CameraMode->GetBlendWeight();
		if (StackIndex < StackSize - 1)
		{
			// 对于非栈底Mode，需要考虑前面Mode的权重
			float AccumulatedWeight = 1.0f;
			for (int32 PrevIndex = StackSize - 1; PrevIndex > StackIndex; --PrevIndex)
			{
				AccumulatedWeight *= (1.0f - CameraModeStack[PrevIndex]->GetBlendWeight());
			}
			ModeWeight *= AccumulatedWeight;
		}
		
		// 混合View（但PivotLocation已经混合好了，所以这里主要是混合其他属性）
		FNamiCameraView OtherView = CameraMode->GetView();
		OtherView.PivotLocation = BlendedPivotLocation; // 使用混合后的PivotLocation
		OutCameraModeView.Blend(OtherView, ModeWeight);

		// 确保PivotLocation保持为混合后的值
		OutCameraModeView.PivotLocation = BlendedPivotLocation;
    }
}
