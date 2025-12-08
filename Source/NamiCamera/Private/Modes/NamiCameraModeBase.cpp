// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modes/NamiCameraModeBase.h"
#include "Features/NamiCameraFeature.h"
#include "Components/NamiCameraComponent.h"
#include "Animation/BlendSpace.h"
#include "Enums/NamiCameraEnums.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraModeBase)

UNamiCameraModeBase::UNamiCameraModeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DefaultFOV(90.0f)
	, Priority(0)
	, BlendWeight(0.0f)
	, State(ENamiCameraModeState::None)
	, bIsActivated(false)
{
	CurrentView.FOV = DefaultFOV;
	
	// 初始化混合配置默认值
	BlendConfig.BlendTime = 0.5f;
	BlendConfig.BlendType = ENamiCameraBlendType::EaseInOut;
	
	// 同步到 BlendStack（与 EnhancedCameraSystem 兼容）
	BlendStack.BlendTime = BlendConfig.BlendTime;
	BlendStack.BlendOption = EAlphaBlendOption::Linear;
}

void UNamiCameraModeBase::Initialize_Implementation(UNamiCameraComponent* InCameraComponent)
{
	CameraComponent = InCameraComponent;
	State = ENamiCameraModeState::Initialized;

	// 同步 BlendConfig 到 BlendStack（保持与 EnhancedCameraSystem 兼容）
	BlendStack.BlendTime = BlendConfig.BlendTime;
	
	// 转换 ENamiCameraBlendType 到 EAlphaBlendOption
	EAlphaBlendOption BlendOption = EAlphaBlendOption::Linear;
	switch (BlendConfig.BlendType)
	{
	case ENamiCameraBlendType::Linear:
		BlendOption = EAlphaBlendOption::Linear;
		break;
	case ENamiCameraBlendType::EaseIn:
	case ENamiCameraBlendType::EaseOut:
	case ENamiCameraBlendType::EaseInOut:
		// 暂时都使用 Linear，后续可以根据实际引擎版本的枚举值调整
		BlendOption = EAlphaBlendOption::Linear;
		break;
	case ENamiCameraBlendType::CustomCurve:
		BlendOption = EAlphaBlendOption::Custom;
		break;
	}
	BlendStack.BlendOption = BlendOption;
	BlendStack.CustomCurve = BlendConfig.BlendCurve;

	// 初始化 FAlphaBlend（使用 BlendStack，与 EnhancedCameraSystem 保持一致）
	CameraBlendAlpha.Reset();
	CameraBlendAlpha.SetBlendOption(BlendStack.BlendOption);
	CameraBlendAlpha.SetBlendTime(BlendStack.BlendTime);
	if (IsValid(BlendStack.CustomCurve))
	{
		CameraBlendAlpha.SetCustomCurve(BlendStack.CustomCurve);
	}

	// 初始化所有Feature
	for (UNamiCameraFeature* Feature : Features)
	{
		if (IsValid(Feature))
		{
			Feature->Initialize(this);
		}
	}

	SortFeatures();
}

void UNamiCameraModeBase::Activate_Implementation()
{
	State = ENamiCameraModeState::Active;
	bIsActivated = true;

	// 激活所有Feature
	for (UNamiCameraFeature* Feature : Features)
	{
		if (IsValid(Feature) && Feature->IsEnabled())
		{
			Feature->Activate();
		}
	}
}

void UNamiCameraModeBase::Deactivate_Implementation()
{
	State = ENamiCameraModeState::Inactive;
	bIsActivated = false;

	// 停用所有Feature
	for (UNamiCameraFeature* Feature : Features)
	{
		if (IsValid(Feature))
		{
			Feature->Deactivate();
		}
	}
}

void UNamiCameraModeBase::Tick_Implementation(float DeltaTime)
{
	// 更新混合权重（使用 FAlphaBlend）
	UpdateBlending(DeltaTime);

	// 更新所有Feature
	UpdateFeatures(DeltaTime);

	// 计算视图
	CurrentView = CalculateView(DeltaTime);

	// 应用Feature到视图
	ApplyFeaturesToView(CurrentView, DeltaTime);
}

FNamiCameraView UNamiCameraModeBase::CalculateView_Implementation(float DeltaTime)
{
	// 基类返回默认视图，子类应该重写此方法
	FNamiCameraView View;
	View.FOV = DefaultFOV;
	return View;
}

FVector UNamiCameraModeBase::CalculatePivotLocation_Implementation(float DeltaTime)
{
	// 基类默认实现：尝试从Pawn获取位置
	if (CameraComponent.IsValid())
	{
		APawn* OwnerPawn = CameraComponent->GetOwnerPawn();
		if (OwnerPawn)
		{
			return OwnerPawn->GetActorLocation();
		}
	}
	
	// 回退：返回零向量
	return FVector::ZeroVector;
}

void UNamiCameraModeBase::AddFeature(UNamiCameraFeature* Feature)
{
	if (!IsValid(Feature))
	{
		return;
	}

	// 检查是否已存在
	if (Features.Contains(Feature))
	{
		return;
	}

	Features.Add(Feature);
	Feature->Initialize(this);

	// 如果模式已激活，同时激活Feature
	if (State == ENamiCameraModeState::Active && Feature->IsEnabled())
	{
		Feature->Activate();
	}

	SortFeatures();
}

void UNamiCameraModeBase::RemoveFeature(UNamiCameraFeature* Feature)
{
	if (!IsValid(Feature))
	{
		return;
	}

	int32 Index = Features.Find(Feature);
	if (Index != INDEX_NONE)
	{
		// 如果模式已激活，先停用Feature
		if (State == ENamiCameraModeState::Active)
		{
			Feature->Deactivate();
		}

		Features.RemoveAt(Index);
	}
}

UNamiCameraFeature* UNamiCameraModeBase::GetFeatureByName(FName FeatureName) const
{
	for (UNamiCameraFeature* Feature : Features)
	{
		if (IsValid(Feature) && Feature->FeatureName == FeatureName)
		{
			return Feature;
		}
	}
	return nullptr;
}

UWorld* UNamiCameraModeBase::GetWorld() const
{
	if (CameraComponent.IsValid())
	{
		return CameraComponent->GetWorld();
	}
	return nullptr;
}

void UNamiCameraModeBase::SetCameraComponent(UNamiCameraComponent* NewCameraComponent)
{
	CameraComponent = NewCameraComponent;
}

void UNamiCameraModeBase::SetBlendWeight(float InWeight)
{
	BlendWeight = FMath::Clamp(InWeight, 0.0f, 1.0f);
	CameraBlendAlpha.SetAlpha(BlendWeight);
}

void UNamiCameraModeBase::ApplyFeaturesToView(FNamiCameraView& InOutView, float DeltaTime)
{
	for (UNamiCameraFeature* Feature : Features)
	{
		if (IsValid(Feature) && Feature->IsEnabled())
		{
			Feature->ApplyToView(InOutView, DeltaTime);
		}
	}
}

void UNamiCameraModeBase::UpdateFeatures(float DeltaTime)
{
	for (UNamiCameraFeature* Feature : Features)
	{
		if (IsValid(Feature) && Feature->IsEnabled())
		{
			Feature->Update(DeltaTime);
		}
	}
}

void UNamiCameraModeBase::SortFeatures()
{
	Features.Sort([](const UNamiCameraFeature& A, const UNamiCameraFeature& B)
	{
		return A.Priority > B.Priority;
	});
}

void UNamiCameraModeBase::UpdateBlending(float DeltaTime)
{
	// 更新 FAlphaBlend（自动从当前 Alpha 过渡到 DesiredValue，默认 1.0）
	CameraBlendAlpha.Update(DeltaTime);
	
	// 获取混合后的值（线性混合值）
	float BlendedValue = CameraBlendAlpha.GetBlendedValue();
	
	// 应用混合曲线（使用 BlendStack，与 EnhancedCameraSystem 保持一致）
	BlendWeight = FAlphaBlend::AlphaToBlendOption(
		BlendedValue, 
		BlendStack.BlendOption, 
		BlendStack.CustomCurve
	);
	
	// 确保权重在有效范围内
	BlendWeight = FMath::Clamp(BlendWeight, 0.0f, 1.0f);
}
