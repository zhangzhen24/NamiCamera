// Copyright Epic Games, Inc. All Rights Reserved.

#include "Features/NamiCameraInputFeature.h"
#include "Modes/NamiCameraModeBase.h"
#include "Components/NamiCameraComponent.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraInputFeature)

UNamiCameraInputFeature::UNamiCameraInputFeature()
	: InputMode(ENamiCameraInputMode::WithInput)
	, InputWeight(1.0f)
	, bEnableInputSmoothing(true)
	, InputSmoothTime(0.1f)
	, InputVelocity(FVector2D::ZeroVector)
{
}

void UNamiCameraInputFeature::Initialize_Implementation(UNamiCameraModeBase* InCameraMode)
{
	Super::Initialize_Implementation(InCameraMode);
	
	// 重置输入速度
	InputVelocity = FVector2D::ZeroVector;
}

void UNamiCameraInputFeature::ProcessPlayerInput_Implementation(UPARAM(ref) FNamiCameraState& InOutState, float DeltaTime)
{
	// 基类默认实现：不处理输入
	// 子类应该重写此方法来提供具体的输入处理逻辑
}

