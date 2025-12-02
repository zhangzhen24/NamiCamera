// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/NamiCameraComponent.h"
#include "LogNamiCamera.h"
#include "Camera/CameraModifier.h"
#include "Modes/NamiCameraModeBase.h"
#include "Modes/Templates/NamiFollowCameraMode.h"
#include "Components/NamiPlayerCameraManager.h"
#include "Structs/View/NamiCameraView.h"
#include "Settings/NamiCameraProjectSettings.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FNamiCameraModeHandle
/// 
namespace CameraModeHandle_Impl
{
	static int32 LastHandleId = 0;
	static int32 GetNextQueuedHandleIdForUse() { return ++LastHandleId; }
}

bool FNamiCameraModeHandle::IsValid() const
{
	return Owner.IsValid() && HandleId != 0;
}

void FNamiCameraModeHandle::Reset()
{
	Owner.Reset();
	HandleId = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
///	UNamiCameraComponent
UNamiCameraComponent::UNamiCameraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bWantsInitializeComponent = true;
	PrimaryComponentTick.bCanEverTick = true;
	DefaultCameraMode = UNamiCameraModeBase::StaticClass();

	CameraModeInstancePool.Empty();
	CameraModePriorityStack.Empty();
}

void UNamiCameraComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UNamiCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerPawn = Cast<APawn>(GetOwner());
	OwnerPlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	OwnerPlayerCameraManager = OwnerPlayerController ? Cast<ANamiPlayerCameraManager>(OwnerPlayerController->PlayerCameraManager) : nullptr;
	if (!OwnerPlayerCameraManager)
	{
		return;
	}
	OnPushCameraMode.AddDynamic(this, &ThisClass::NotifyCameraModeInitialize);
	if (IsValid(DefaultCameraMode))
	{
		PushCameraMode(DefaultCameraMode);
	}
}

void UNamiCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	FNamiCameraView CameraModeView;
	// 评估BlendingStack 的堆栈权重
	if (!BlendingStack.EvaluateStack(DeltaTime, CameraModeView))
	{
		Super::GetCameraView(DeltaTime, DesiredView);
		return;
	}

	// PlayerController
	if (APlayerController* PC = GetOwnerPlayerController())
	{
		FHitResult HitResult;
		PC->K2_SetActorLocation(CameraModeView.ControlLocation, false, HitResult, true);
		PC->SetControlRotation(CameraModeView.ControlRotation);
	}

	// 保持相机组件与最新CameraModeView数据同步
	SetWorldLocationAndRotation(CameraModeView.CameraLocation, CameraModeView.CameraRotation);
	FieldOfView = CameraModeView.FOV;

	// 填写所需的视图
	DesiredView.Location = CameraModeView.CameraLocation;
	DesiredView.Rotation = CameraModeView.CameraRotation;
	DesiredView.FOV = CameraModeView.FOV;
	DesiredView.OrthoWidth = OrthoWidth;
	DesiredView.OrthoNearClipPlane = OrthoNearClipPlane;
	DesiredView.OrthoFarClipPlane = OrthoFarClipPlane;
	DesiredView.AspectRatio = AspectRatio;
	DesiredView.bConstrainAspectRatio = bConstrainAspectRatio;
	DesiredView.bUseFieldOfViewForLOD = bUseFieldOfViewForLOD;
	DesiredView.ProjectionMode = ProjectionMode;

	// 查看 CameraActor 是否想要覆盖所使用的 PostProcess (后处理) 设置
	DesiredView.PostProcessBlendWeight = PostProcessBlendWeight;
	if (PostProcessBlendWeight > 0.0f)
	{
		DesiredView.PostProcessSettings = PostProcessSettings;
	}

	// Debug日志（如果启用）
	if (UNamiCameraProjectSettings::ShouldEnableStackDebugLog())
	{
		const UNamiCameraProjectSettings* Settings = UNamiCameraProjectSettings::Get();
		if (Settings)
		{
			BlendingStack.DumpCameraModeStack(true, false, Settings->DebugLogTextColor, Settings->DebugLogDuration);
		}
	}
}

APawn* UNamiCameraComponent::GetOwnerPawn() const
{
	return OwnerPawn.Get();
}

APlayerController* UNamiCameraComponent::GetOwnerPlayerController() const
{
	return OwnerPlayerController.Get();
}

ANamiPlayerCameraManager* UNamiCameraComponent::GetOwnerPlayerCameraManager() const
{
	return OwnerPlayerCameraManager.Get();
}

void UNamiCameraComponent::DumpCameraModeStack(const bool bPrintToScreen, const bool bPrintToLog, 
	const FLinearColor TextColor, const float Duration) const
{
	BlendingStack.DumpCameraModeStack(bPrintToScreen, bPrintToLog, TextColor, Duration);
}

FNamiCameraModeHandle UNamiCameraComponent::PushCameraMode(TSubclassOf<UNamiCameraModeBase> CameraModeClass, int32 Priority)
{
	return PushCameraModeUsingInstance(FindOrAddCameraModeInstanceInPool(CameraModeClass), Priority);
}

FNamiCameraModeHandle UNamiCameraComponent::PushCameraModeUsingInstance(UNamiCameraModeBase* CameraModeInstance, int32 Priority)
{
	if (!IsValid(CameraModeInstance))
	{
		UE_LOG(LogNamiCamera, Error, TEXT("[UNamiCameraComponent::PushCameraModeUsingInstance] CameraModeInstance is null"));
		return FNamiCameraModeHandle();
	}

	OnPushCameraMode.Broadcast(CameraModeInstance);

	// 准备ModeHandle
	FNamiCameraModeHandle ModeHandle;
	ModeHandle.Owner = this;
	ModeHandle.HandleId = CameraModeHandle_Impl::GetNextQueuedHandleIdForUse();

	// 检查优先级，插入到合适的位置
	int32 StackIndex = 0;
	for (; StackIndex < CameraModePriorityStack.Num(); ++StackIndex)
	{
		if (CameraModePriorityStack[StackIndex].Priority > Priority)
		{
			break;
		}
	}

	// 插入到合适的位置
	FNamiCameraModeStackEntry CameraModeEntry;
	CameraModeEntry.HandleId = ModeHandle.HandleId;
	CameraModeEntry.Priority = Priority;
	CameraModeEntry.CameraMode = CameraModeInstance;
	CameraModePriorityStack.Insert(CameraModeEntry, StackIndex);

	// 更新混合栈
	UpdateBlendingStack();

	return ModeHandle;
}

bool UNamiCameraComponent::PopCameraMode(FNamiCameraModeHandle& ModeHandle)
{
	bool bResult = false;
	if (ModeHandle.IsValid() && ModeHandle.Owner == this)
	{
		// 找到Index
		const int32 HandleId = ModeHandle.HandleId;
		const int32 FoundIndex = CameraModePriorityStack.IndexOfByPredicate(
			[HandleId](const FNamiCameraModeStackEntry& CameraModeEntry)
			{
				return (CameraModeEntry.HandleId == HandleId);
			});

		// 通过Index移除
		bResult = PullCameraModeAtIndex(FoundIndex);
		ModeHandle.Reset();
	}
	return bResult;
}

bool UNamiCameraComponent::PopCameraModeInstance(UNamiCameraModeBase* CameraMode)
{
	if (!IsValid(CameraMode))
	{
		return false;
	}

	const int32 FoundIndex = CameraModePriorityStack.IndexOfByPredicate(
		[CameraMode](const FNamiCameraModeStackEntry& CameraModeEntry)
		{
			if (CameraModeEntry.CameraMode.IsValid())
			{
				return (CameraModeEntry.CameraMode.Get() == CameraMode);
			}
			return false;
		});
	return PullCameraModeAtIndex(FoundIndex);
}

UNamiCameraModeBase* UNamiCameraComponent::GetActiveCameraMode() const
{
	UNamiCameraModeBase* ActiveCamera = nullptr;
	if (ensureAlways(CameraModePriorityStack.Num() > 0))
	{
		ActiveCamera = CameraModePriorityStack.Top().CameraMode.Get();
	}
	return ActiveCamera;
}

UCameraModifier* UNamiCameraComponent::PushCameraModifier(const TSubclassOf<UCameraModifier> CameraModifierClass)
{
	if (!IsValid(CameraModifierClass))
	{
		UE_LOG(LogNamiCamera, Error, TEXT("[UNamiCameraComponent::PushCameraModifier] CameraModifierClass is null"));
		return nullptr;
	}

	UCameraModifier* CameraModifierInstance = NewObject<UCameraModifier>(this, CameraModifierClass);
	if (!IsValid(CameraModifierInstance))
	{
		UE_LOG(LogNamiCamera, Error, TEXT("[UNamiCameraComponent::PushCameraModifier] Create CameraModifier failed. The CameraModifierClass is %s."),
		       *CameraModifierClass->GetName());
		return nullptr;
	}

	if (!PushCameraModifierInstance(CameraModifierInstance))
	{
		UE_LOG(LogNamiCamera, Error, TEXT("[UNamiCameraComponent::PushCameraModifier] Failed to push CameraModifierInstance."));
		return nullptr;
	}

	CameraModifierInstance->EnableModifier();

	return CameraModifierInstance;
}

bool UNamiCameraComponent::PushCameraModifierInstance(UCameraModifier* CameraModifierInstance)
{
	if (!IsValid(CameraModifierInstance))
	{
		return false;
	}

	if (!IsValid(OwnerPlayerCameraManager))
	{
		return false;
	}

	return OwnerPlayerCameraManager->AddCameraModifierToList(CameraModifierInstance);
}

bool UNamiCameraComponent::PopCameraModifierInstance(UCameraModifier* CameraModifierInstance)
{
	if (!IsValid(CameraModifierInstance))
	{
		UE_LOG(LogNamiCamera, Error, TEXT("[UNamiCameraComponent::PopCameraModifierInstance] The CameraModifierInstance is null."));
		return false;
	}
	CameraModifierInstance->DisableModifier(false);
	
	if (!IsValid(OwnerPlayerCameraManager))
	{
		return false;
	}

	return OwnerPlayerCameraManager->RemoveCameraModifier(CameraModifierInstance);
}

TArray<UCameraModifier*> UNamiCameraComponent::GetActivateCameraModifiers() const
{
	if (!IsValid(OwnerPlayerCameraManager))
	{
		return TArray<UCameraModifier*>();
	}
	return OwnerPlayerCameraManager->ModifierList;
}

void UNamiCameraComponent::NotifyCameraModeInitialize(UNamiCameraModeBase* CameraModeInstance)
{
	if (IsValid(CameraModeInstance))
	{
		CameraModeInstance->SetCameraComponent(this);
		CameraModeInstance->Initialize(this);
		
		// 自动设置 PrimaryTarget 为 Owner（如果相机模式支持）
		if (UNamiFollowCameraMode* FollowMode = Cast<UNamiFollowCameraMode>(CameraModeInstance))
		{
			AActor* Owner = GetOwner();
			if (IsValid(Owner) && !FollowMode->GetPrimaryTarget())
			{
				FollowMode->SetPrimaryTarget(Owner);
			}
		}
	}
}

void UNamiCameraComponent::UpdateBlendingStack()
{
	// 保证优先级堆栈中至少有一个相机模式
	if (!ensureAlways(CameraModePriorityStack.Num() > 0))
	{
		PushCameraMode(DefaultCameraMode);
	}

	// 将优先级最高的（栈顶）的相机模式实例 压入堆栈
	const FNamiCameraModeStackEntry& TopEntry = CameraModePriorityStack.Top();
	if (TopEntry.CameraMode.IsValid())
	{
		BlendingStack.PushCameraMode(TopEntry.CameraMode.Get());
	}
}

UNamiCameraModeBase* UNamiCameraComponent::FindOrAddCameraModeInstanceInPool(TSubclassOf<UNamiCameraModeBase> CameraModeClass)
{
	if (!IsValid(CameraModeClass))
	{
		UE_LOG(LogNamiCamera, Error, TEXT("[UNamiCameraComponent::FindOrAddCameraModeInstanceInPool] CameraModeClass is null"));
		return nullptr;
	}

	// 首先，查找CameraModeInstancePool中是否已经存在
	for (UNamiCameraModeBase* CameraMode : CameraModeInstancePool)
	{
		if ((CameraMode != nullptr) && (CameraMode->GetClass() == CameraModeClass))
		{
			return CameraMode;
		}
	}

	// 如果不存在，创建一个新的CameraMode实例
	UNamiCameraModeBase* NewCameraMode = NewObject<UNamiCameraModeBase>(this, CameraModeClass, NAME_None, RF_NoFlags);
	if (!IsValid(NewCameraMode))
	{
		UE_LOG(LogNamiCamera, Error, TEXT("[UNamiCameraComponent::FindOrAddCameraModeInstanceInPool] Failed to create CameraMode instance"));
		return nullptr;
	}

	CameraModeInstancePool.Add(NewCameraMode);

	return NewCameraMode;
}

bool UNamiCameraComponent::PullCameraModeAtIndex(int32 Index)
{
	if (CameraModePriorityStack.IsValidIndex(Index))
	{
		OnPopCameraMode.Broadcast();
		CameraModePriorityStack.RemoveAt(Index);
		UpdateBlendingStack();
		return true;
	}
	return false;
}

