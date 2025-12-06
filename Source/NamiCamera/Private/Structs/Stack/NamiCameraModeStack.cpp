// Copyright Epic Games, Inc. All Rights Reserved.

#include "Structs/Stack/NamiCameraModeStack.h"
#include "Modes/NamiCameraModeBase.h"
#include "Structs/View/NamiCameraView.h"
#include "Engine/Engine.h"
#include "Enums/NamiCameraEnums.h"
#include "LogNamiCamera.h"

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
	CameraModeInstance->SetBlendWeight(StartBlendWeight);

	// 将新条目添加到堆栈
	CameraModeStack.Insert(CameraModeInstance, 0);

	// 确保堆栈底部的权重始终为 100%
	CameraModeStack.Last()->SetBlendWeight(1.0f);

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

	BlendStack(OutCameraModeView);

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

void FNamiCameraModeStack::BlendStack(FNamiCameraView& OutCameraModeView) const
{
	const int32 StackSize = CameraModeStack.Num();
	if (StackSize <= 0)
	{
		return;
	}

    // Get TopCameraMode's view
    const UNamiCameraModeBase* CameraMode = CameraModeStack[StackSize - 1];
    check(CameraMode);
    OutCameraModeView = CameraMode->GetView();

	// Blend other camera mode
    for (int32 StackIndex = (StackSize - 2); StackIndex >= 0; --StackIndex)
    {
        CameraMode = CameraModeStack[StackIndex];
        check(CameraMode);
        OutCameraModeView.Blend(CameraMode->GetView(), CameraMode->GetBlendWeight());
    }
}
