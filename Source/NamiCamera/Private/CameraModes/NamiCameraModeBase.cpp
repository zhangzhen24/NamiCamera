// Copyright Qiu, Inc. All Rights Reserved.

#include "CameraModes/NamiCameraModeBase.h"
#include "Animation/BlendSpace.h"
#include "Components/NamiCameraComponent.h"
#include "Data/NamiCameraEnums.h"
#include "ModeComponents/NamiCameraModeComponent.h"
#include "GameFramework/Pawn.h"
#include "Logging/LogNamiCameraMacros.h"

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
	// 设置混合范围：从 0.0 到 1.0（默认淡入）
	CameraBlendAlpha.SetValueRange(0.0f, 1.0f);

	// 初始化所有组件
	for (UNamiCameraModeComponent* Component : ModeComponents)
	{
		if (IsValid(Component))
		{
			Component->Initialize(this);
		}
	}

	SortComponents();
}

void UNamiCameraModeBase::Activate_Implementation()
{
	State = ENamiCameraModeState::Active;
	bIsActivated = true;

	// 激活所有组件
	for (UNamiCameraModeComponent* Component : ModeComponents)
	{
		if (IsValid(Component) && Component->IsEnabled())
		{
			Component->Activate();
		}
	}
}

void UNamiCameraModeBase::Deactivate_Implementation()
{
	State = ENamiCameraModeState::Inactive;
	bIsActivated = false;

	// 停用所有组件
	for (UNamiCameraModeComponent* Component : ModeComponents)
	{
		if (IsValid(Component))
		{
			Component->Deactivate();
		}
	}
}

void UNamiCameraModeBase::Tick_Implementation(float DeltaTime)
{
	// 更新混合权重（使用 FAlphaBlend）
	UpdateBlending(DeltaTime);

	// 更新所有组件
	UpdateComponents(DeltaTime);

	// 计算视图
	CurrentView = CalculateView(DeltaTime);

	// 应用组件到视图
	ApplyComponentsToView(CurrentView, DeltaTime);
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

void UNamiCameraModeBase::AddComponent(UNamiCameraModeComponent* Component)
{
	if (!IsValid(Component))
	{
		return;
	}

	// 检查是否已存在
	if (ModeComponents.Contains(Component))
	{
		return;
	}

	ModeComponents.Add(Component);
	Component->Initialize(this);

	// 如果模式已激活，同时激活组件
	if (State == ENamiCameraModeState::Active && Component->IsEnabled())
	{
		Component->Activate();
	}

	SortComponents();

	// 标记 ComponentMap 需要重建
	bComponentMapDirty = true;
}

void UNamiCameraModeBase::RemoveComponent(UNamiCameraModeComponent* Component)
{
	if (!IsValid(Component))
	{
		return;
	}

	int32 Index = ModeComponents.Find(Component);
	if (Index != INDEX_NONE)
	{
		// 如果模式已激活，先停用组件
		if (State == ENamiCameraModeState::Active)
		{
			Component->Deactivate();
		}

		ModeComponents.RemoveAt(Index);

		// 标记 ComponentMap 需要重建
		bComponentMapDirty = true;
	}
}

UNamiCameraModeComponent* UNamiCameraModeBase::GetComponentByName(FName ComponentName) const
{
	// 如果 ComponentMap 需要重建，先重建
	if (bComponentMapDirty)
	{
		RebuildComponentMap();
	}

	// 从 Map 中查找（O(1) 时间复杂度）
	if (UNamiCameraModeComponent** FoundComponent = ComponentMap.Find(ComponentName))
	{
		return *FoundComponent;
	}

	return nullptr;
}

void UNamiCameraModeBase::RebuildComponentMap() const
{
	ComponentMap.Empty();

	for (UNamiCameraModeComponent* Component : ModeComponents)
	{
		if (IsValid(Component) && Component->ComponentName != NAME_None)
		{
			ComponentMap.Add(Component->ComponentName, Component);
		}
	}

	bComponentMapDirty = false;
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

	// 调试：记录设置前的状态
	NAMI_LOG_MODE_BLEND(Log,
		TEXT("[%s] SetBlendWeight: InWeight=%.3f, BeforeBlendedValue=%.3f, BeforeBegin=%.3f, BeforeDesired=%.3f, BlendTime=%.3f"),
		*GetName(),
		InWeight,
		CameraBlendAlpha.GetBlendedValue(),
		CameraBlendAlpha.GetBeginValue(),
		CameraBlendAlpha.GetDesiredValue(),
		CameraBlendAlpha.GetBlendTime());

	// 1. 设置混合范围：从起始权重（InWeight）到目标值（1.0）
	CameraBlendAlpha.SetValueRange(BlendWeight, 1.0f);

	// 2. 重要：SetValueRange 不会重置 BlendedValue，需要手动重置混合进度
	//    SetAlpha(0) 将混合进度重置到起始位置
	//    此时 BlendedValue = Lerp(InWeight, 1.0, 0) = InWeight
	CameraBlendAlpha.SetAlpha(0.0f);

	// 调试：记录设置后的状态
	NAMI_LOG_MODE_BLEND(Log,
		TEXT("[%s] SetBlendWeight After: BlendedValue=%.3f, Begin=%.3f, Desired=%.3f"),
		*GetName(),
		CameraBlendAlpha.GetBlendedValue(),
		CameraBlendAlpha.GetBeginValue(),
		CameraBlendAlpha.GetDesiredValue());
}

void UNamiCameraModeBase::ApplyComponentsToView(FNamiCameraView& InOutView, float DeltaTime)
{
	// 优化：只遍历有效的组件，减少无效检查
	for (UNamiCameraModeComponent* Component : ModeComponents)
	{
		if (!Component)
		{
			continue;
		}

		// 快速路径：如果组件未启用，跳过 ApplyToView
		if (!Component->IsEnabled())
		{
			continue;
		}

		Component->ApplyToView(InOutView, DeltaTime);
	}
}

void UNamiCameraModeBase::UpdateComponents(float DeltaTime)
{
	// 优化：只遍历有效的组件，减少无效检查
	for (UNamiCameraModeComponent* Component : ModeComponents)
	{
		if (!Component)
		{
			continue;
		}

		// 快速路径：如果组件未启用，跳过 Update
		if (!Component->IsEnabled())
		{
			continue;
		}

		Component->Update(DeltaTime);
	}
}

void UNamiCameraModeBase::SortComponents()
{
	ModeComponents.Sort([](const UNamiCameraModeComponent& A, const UNamiCameraModeComponent& B)
	{
		return A.Priority > B.Priority;
	});

	// 排序后需要重建 ComponentMap
	bComponentMapDirty = true;
}

void UNamiCameraModeBase::UpdateBlending(float DeltaTime)
{
	// 调试：记录更新前的状态
	float BeforeAlpha = CameraBlendAlpha.GetAlpha();
	float BeforeBlendedValue = CameraBlendAlpha.GetBlendedValue();
	float BeforeBeginValue = CameraBlendAlpha.GetBeginValue();
	float BeforeDesiredValue = CameraBlendAlpha.GetDesiredValue();
	float BlendTimeRemaining = CameraBlendAlpha.GetBlendTimeRemaining();

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

	// 调试：打印混合状态（只在权重不是0或1时打印，避免日志刷屏）
	if (BlendWeight > 0.01f && BlendWeight < 0.99f)
	{
		NAMI_LOG_MODE_BLEND(Log,
			TEXT("[%s] Blend: Alpha=%.3f->%.3f, BlendedValue=%.3f->%.3f, Begin=%.3f, Desired=%.3f, TimeRemaining=%.3f, Weight=%.3f"),
			*GetName(),
			BeforeAlpha, CameraBlendAlpha.GetAlpha(),
			BeforeBlendedValue, BlendedValue,
			BeforeBeginValue, BeforeDesiredValue,
			BlendTimeRemaining,
			BlendWeight);
	}
}

AActor* UNamiCameraModeBase::GetOwnerActor() const
{
	if (CameraComponent.IsValid())
	{
		return CameraComponent->GetOwnerPawn();
	}
	return nullptr;
}
