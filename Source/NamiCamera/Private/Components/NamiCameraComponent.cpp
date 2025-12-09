// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/NamiCameraComponent.h"
#include "LogNamiCamera.h"
#include "LogNamiCameraMacros.h"
#include "Camera/CameraModifier.h"
#include "Modes/NamiCameraModeBase.h"
#include "Modes/Templates/NamiFollowCameraMode.h"
#include "Components/NamiPlayerCameraManager.h"
#include "Structs/View/NamiCameraView.h"
#include "Structs/Pipeline/NamiCameraPipelineContext.h"
#include "Settings/NamiCameraSettings.h"
#include "Features/NamiCameraFeature.h"
#include "GameplayTagContainer.h"
#include "NamiCameraTags.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Debug/NamiCameraDebugInfo.h"

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
UNamiCameraComponent::UNamiCameraComponent(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer)
{
	bWantsInitializeComponent = true;
	PrimaryComponentTick.bCanEverTick = true;
	DefaultCameraMode = nullptr; // 用户应该在 Blueprint 或 C++ 中显式设置

	CameraModeInstancePool.Empty();
	CameraModePriorityStack.Empty();

	// 初始化平滑混合层状态
	bHasInitializedCurrentView = false;
	LocationBlendSpeed = 0.0f; // 默认瞬切
	RotationBlendSpeed = 0.0f; // 默认瞬切
	FOVBlendSpeed = 0.0f;	   // 默认瞬切

	bHasInitializedControl = false;
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
	
	// 初始化缓存
	CachedPlayerController = OwnerPlayerController;
	CachedPlayerCameraManager = OwnerPlayerCameraManager;
	
	if (!OwnerPlayerCameraManager)
	{
		return;
	}
	OnPushCameraMode.AddDynamic(this, &ThisClass::NotifyCameraModeInitialize);

	// 重置平滑混合层状态（确保每次 BeginPlay 时重新初始化）
	bHasInitializedCurrentView = false;

	// 检查并推送默认相机模式
	if (IsValid(DefaultCameraMode))
	{
		PushCameraMode(DefaultCameraMode);
	}
	else
	{
		NAMI_LOG_WARNING(TEXT("[UNamiCameraComponent::BeginPlay] DefaultCameraMode is not set for %s. Please set a valid camera mode in Blueprint or C++."),
						 *GetOwner()->GetName());
	}
}

void UNamiCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo &DesiredView)
{
	// ========== 【阶段 0：预处理层】 ==========
	FNamiCameraPipelineContext Context;
	if (!PreProcessPipeline(DeltaTime, Context))
	{
		Super::GetCameraView(DeltaTime, DesiredView);
		return;
	}

	// ========== 【阶段 1：模式计算层】 ==========
	FNamiCameraView BaseView;
	if (!ProcessModeStack(DeltaTime, Context, BaseView))
	{
		Super::GetCameraView(DeltaTime, DesiredView);
		return;
	}

	// ========== 【阶段 2：全局效果层】 ==========
	FNamiCameraView EffectView = BaseView;
	ProcessGlobalFeatures(DeltaTime, Context, EffectView);
	
	// 保存 EffectView 用于 Debug（阶段5需要）
	Context.EffectView = EffectView;

	// ========== 【阶段 3：控制器同步层】 ==========
	ProcessControllerSync(DeltaTime, Context, EffectView);

	// ========== 【阶段 4：平滑混合层】 ==========
	FMinimalViewInfo SmoothedPOV;
	ProcessSmoothing(DeltaTime, EffectView, SmoothedPOV);

	// ========== 【阶段 5：后处理层】 ==========
	PostProcessPipeline(DeltaTime, Context, SmoothedPOV);

	// ========== 【最终输出】 ==========
	DesiredView = SmoothedPOV;
}

APawn *UNamiCameraComponent::GetOwnerPawn() const
{
	return OwnerPawn.Get();
}

APlayerController *UNamiCameraComponent::GetOwnerPlayerController() const
{
	// 如果缓存有效，直接返回
	if (CachedPlayerController.IsValid())
	{
		return CachedPlayerController.Get();
	}

	// 否则从 Owner 获取并缓存
	if (IsValid(OwnerPawn))
	{
		APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
		if (PC)
		{
			CachedPlayerController = PC;
			return PC;
		}
	}

	return nullptr;
}

ANamiPlayerCameraManager *UNamiCameraComponent::GetOwnerPlayerCameraManager() const
{
	// 如果缓存有效，直接返回
	if (CachedPlayerCameraManager.IsValid())
	{
		return CachedPlayerCameraManager.Get();
	}

	// 否则从 PlayerController 获取并缓存
	if (APlayerController* PC = GetOwnerPlayerController())
	{
		ANamiPlayerCameraManager* Manager = Cast<ANamiPlayerCameraManager>(PC->PlayerCameraManager);
		if (Manager)
		{
			CachedPlayerCameraManager = Manager;
			return Manager;
		}
	}

	return nullptr;
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

FNamiCameraModeHandle UNamiCameraComponent::PushCameraModeUsingInstance(UNamiCameraModeBase *CameraModeInstance, int32 Priority)
{
	if (!IsValid(CameraModeInstance))
	{
		NAMI_LOG_COMPONENT(Error, TEXT("[UNamiCameraComponent::PushCameraModeUsingInstance] CameraModeInstance is null"));
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

bool UNamiCameraComponent::PopCameraMode(FNamiCameraModeHandle &ModeHandle)
{
	bool bResult = false;
	if (ModeHandle.IsValid() && ModeHandle.Owner == this)
	{
		// 找到Index
		const int32 HandleId = ModeHandle.HandleId;
		const int32 FoundIndex = CameraModePriorityStack.IndexOfByPredicate(
			[HandleId](const FNamiCameraModeStackEntry &CameraModeEntry)
			{
				return (CameraModeEntry.HandleId == HandleId);
			});

		// 通过Index移除
		bResult = PullCameraModeAtIndex(FoundIndex);
		ModeHandle.Reset();
	}
	return bResult;
}

bool UNamiCameraComponent::PopCameraModeInstance(UNamiCameraModeBase *CameraMode)
{
	if (!IsValid(CameraMode))
	{
		return false;
	}

	const int32 FoundIndex = CameraModePriorityStack.IndexOfByPredicate(
		[CameraMode](const FNamiCameraModeStackEntry &CameraModeEntry)
		{
			if (CameraModeEntry.CameraMode.IsValid())
			{
				return (CameraModeEntry.CameraMode.Get() == CameraMode);
			}
			return false;
		});
	return PullCameraModeAtIndex(FoundIndex);
}

UNamiCameraModeBase *UNamiCameraComponent::GetActiveCameraMode() const
{
	UNamiCameraModeBase *ActiveCamera = nullptr;
	if (ensureAlways(CameraModePriorityStack.Num() > 0))
	{
		ActiveCamera = CameraModePriorityStack.Top().CameraMode.Get();
	}
	return ActiveCamera;
}

UCameraModifier *UNamiCameraComponent::PushCameraModifier(const TSubclassOf<UCameraModifier> CameraModifierClass)
{
	if (!IsValid(CameraModifierClass))
	{
		NAMI_LOG_COMPONENT(Error, TEXT("[UNamiCameraComponent::PushCameraModifier] CameraModifierClass is null"));
		return nullptr;
	}

	UCameraModifier *CameraModifierInstance = NewObject<UCameraModifier>(this, CameraModifierClass);
	if (!IsValid(CameraModifierInstance))
	{
		NAMI_LOG_COMPONENT(Error, TEXT("[UNamiCameraComponent::PushCameraModifier] Create CameraModifier failed. The CameraModifierClass is %s."),
						   *CameraModifierClass->GetName());
		return nullptr;
	}

	if (!PushCameraModifierInstance(CameraModifierInstance))
	{
		NAMI_LOG_COMPONENT(Error, TEXT("[UNamiCameraComponent::PushCameraModifier] Failed to push CameraModifierInstance."));
		return nullptr;
	}

	CameraModifierInstance->EnableModifier();

	return CameraModifierInstance;
}

bool UNamiCameraComponent::PushCameraModifierInstance(UCameraModifier *CameraModifierInstance)
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

bool UNamiCameraComponent::PopCameraModifierInstance(UCameraModifier *CameraModifierInstance)
{
	if (!IsValid(CameraModifierInstance))
	{
		NAMI_LOG_COMPONENT(Error, TEXT("[UNamiCameraComponent::PopCameraModifierInstance] The CameraModifierInstance is null."));
		return false;
	}
	CameraModifierInstance->DisableModifier(false);

	if (!IsValid(OwnerPlayerCameraManager))
	{
		return false;
	}

	return OwnerPlayerCameraManager->RemoveCameraModifier(CameraModifierInstance);
}

TArray<UCameraModifier *> UNamiCameraComponent::GetActivateCameraModifiers() const
{
	if (!IsValid(OwnerPlayerCameraManager))
	{
		return TArray<UCameraModifier *>();
	}
	return OwnerPlayerCameraManager->ModifierList;
}

void UNamiCameraComponent::NotifyCameraModeInitialize(UNamiCameraModeBase *CameraModeInstance)
{
	if (IsValid(CameraModeInstance))
	{
		CameraModeInstance->SetCameraComponent(this);
		CameraModeInstance->Initialize(this);

		// 自动设置 PrimaryTarget 为 Owner（如果相机模式支持）
		if (UNamiFollowCameraMode *FollowMode = Cast<UNamiFollowCameraMode>(CameraModeInstance))
		{
			AActor *Owner = GetOwner();
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
	const FNamiCameraModeStackEntry &TopEntry = CameraModePriorityStack.Top();
	if (TopEntry.CameraMode.IsValid())
	{
		BlendingStack.PushCameraMode(TopEntry.CameraMode.Get());
	}
}

UNamiCameraModeBase *UNamiCameraComponent::FindOrAddCameraModeInstanceInPool(TSubclassOf<UNamiCameraModeBase> CameraModeClass)
{
	if (!IsValid(CameraModeClass))
	{
		NAMI_LOG_COMPONENT(Error, TEXT("[UNamiCameraComponent::FindOrAddCameraModeInstanceInPool] CameraModeClass is null"));
		return nullptr;
	}

	// 首先，查找CameraModeInstancePool中是否已经存在
	for (UNamiCameraModeBase *CameraMode : CameraModeInstancePool)
	{
		if ((CameraMode != nullptr) && (CameraMode->GetClass() == CameraModeClass))
		{
			return CameraMode;
		}
	}

	// 如果不存在，创建一个新的CameraMode实例
	UNamiCameraModeBase *NewCameraMode = NewObject<UNamiCameraModeBase>(this, CameraModeClass, NAME_None, RF_NoFlags);
	if (!IsValid(NewCameraMode))
	{
		NAMI_LOG_COMPONENT(Error, TEXT("[UNamiCameraComponent::FindOrAddCameraModeInstanceInPool] Failed to create CameraMode instance"));
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

void UNamiCameraComponent::AddGlobalFeature(UNamiCameraFeature* Feature)
{
	if (!IsValid(Feature))
	{
		NAMI_LOG_COMPONENT(Error, TEXT("[UNamiCameraComponent::AddGlobalFeature] Feature is null"));
		return;
	}

	// 检查是否已存在
	if (GlobalFeatures.Contains(Feature))
	{
		return;
	}

	GlobalFeatures.Add(Feature);
	
	// 初始化 Feature（需要设置一个临时的 Mode，或者修改 Feature 系统支持无 Mode）
	// 这里我们让 Feature 可以没有 Mode（可选）
	UNamiCameraModeBase* ActiveMode = GetActiveCameraMode();
	if (ActiveMode)
	{
		Feature->Initialize(ActiveMode);
	}

}

bool UNamiCameraComponent::RemoveGlobalFeature(UNamiCameraFeature* Feature)
{
	if (!IsValid(Feature))
	{
		return false;
	}

	int32 Index = GlobalFeatures.Find(Feature);
	if (Index != INDEX_NONE)
	{
		GlobalFeatures.RemoveAt(Index);
		return true;
	}

	return false;
}

UNamiCameraFeature* UNamiCameraComponent::FindGlobalFeatureByName(FName FeatureName) const
{
	for (UNamiCameraFeature* Feature : GlobalFeatures)
	{
		if (IsValid(Feature) && Feature->FeatureName == FeatureName)
		{
			return Feature;
		}
	}
	return nullptr;
}

void UNamiCameraComponent::RemoveGlobalFeaturesByTag(const FGameplayTag& Tag, bool bDeactivateFirst)
{
	if (!Tag.IsValid())
	{
		return;
	}

	TArray<UNamiCameraFeature*> FeaturesToRemove;

	for (UNamiCameraFeature* Feature : GlobalFeatures)
	{
		if (IsValid(Feature) && Feature->HasTag(Tag))
		{
			FeaturesToRemove.Add(Feature);
		}
	}

	for (UNamiCameraFeature* Feature : FeaturesToRemove)
	{
		if (IsValid(Feature))
		{
			if (bDeactivateFirst)
			{
				Feature->Deactivate();
			}
			RemoveGlobalFeature(Feature);
		}
	}
}

void UNamiCameraComponent::RemoveGlobalFeaturesByTags(const FGameplayTagContainer& TagContainer, bool bDeactivateFirst)
{
	if (TagContainer.IsEmpty())
	{
		return;
	}

	TArray<UNamiCameraFeature*> FeaturesToRemove;

	for (UNamiCameraFeature* Feature : GlobalFeatures)
	{
		if (IsValid(Feature) && Feature->HasAnyTag(TagContainer))
		{
			FeaturesToRemove.Add(Feature);
		}
	}

	for (UNamiCameraFeature* Feature : FeaturesToRemove)
	{
		if (IsValid(Feature))
		{
			if (bDeactivateFirst)
			{
				Feature->Deactivate();
			}
			RemoveGlobalFeature(Feature);
		}
	}
}

void UNamiCameraComponent::RemoveStayGlobalFeatures()
{
	RemoveGlobalFeaturesByTag(Tag_Camera_Feature_ManualCleanup, true);
}

UNamiCameraFeature* UNamiCameraComponent::GetFeatureByName(FName FeatureName) const
{
	// 先查找全局Feature
	if (UNamiCameraFeature* Feature = FindGlobalFeatureByName(FeatureName))
	{
		return Feature;
	}
	
	// 再查找当前激活Mode的Feature
	if (UNamiCameraModeBase* ActiveMode = GetActiveCameraMode())
	{
		const TArray<UNamiCameraFeature*>& ModeFeatures = ActiveMode->GetFeatures();
		for (UNamiCameraFeature* Feature : ModeFeatures)
		{
			if (IsValid(Feature) && Feature->FeatureName == FeatureName)
			{
				return Feature;
			}
		}
	}
	
	return nullptr;
}

TArray<UNamiCameraFeature*> UNamiCameraComponent::GetFeaturesByTag(const FGameplayTag& Tag) const
{
	TArray<UNamiCameraFeature*> Result;
	
	if (!Tag.IsValid())
	{
		return Result;
	}
	
	// 查找全局Feature
	for (UNamiCameraFeature* Feature : GlobalFeatures)
	{
		if (IsValid(Feature) && Feature->HasTag(Tag))
		{
			Result.Add(Feature);
		}
	}
	
	// 查找当前激活Mode的Feature
	if (UNamiCameraModeBase* ActiveMode = GetActiveCameraMode())
	{
		const TArray<UNamiCameraFeature*>& ModeFeatures = ActiveMode->GetFeatures();
		for (UNamiCameraFeature* Feature : ModeFeatures)
		{
			if (IsValid(Feature) && Feature->HasTag(Tag))
			{
				Result.Add(Feature);
			}
		}
	}
	
	return Result;
}

TArray<UNamiCameraFeature*> UNamiCameraComponent::GetFeaturesByTags(const FGameplayTagContainer& TagContainer) const
{
	TArray<UNamiCameraFeature*> Result;
	
	if (TagContainer.IsEmpty())
	{
		return Result;
	}
	
	// 查找全局Feature
	for (UNamiCameraFeature* Feature : GlobalFeatures)
	{
		if (IsValid(Feature) && Feature->HasAnyTag(TagContainer))
		{
			Result.Add(Feature);
		}
	}
	
	// 查找当前激活Mode的Feature
	if (UNamiCameraModeBase* ActiveMode = GetActiveCameraMode())
	{
		const TArray<UNamiCameraFeature*>& ModeFeatures = ActiveMode->GetFeatures();
		for (UNamiCameraFeature* Feature : ModeFeatures)
		{
			if (IsValid(Feature) && Feature->HasAnyTag(TagContainer))
			{
				Result.Add(Feature);
			}
		}
	}
	
	return Result;
}

void UNamiCameraComponent::DrawDebugCameraInfo(const FNamiCameraView& View) const
{
	if (!UNamiCameraSettings::ShouldEnableDrawDebug())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float DrawDuration = UNamiCameraSettings::GetDrawDebugDuration();
	const float DrawTime = DrawDuration > 0.0f ? DrawDuration : -1.0f;
	const float Thickness = UNamiCameraSettings::GetDrawDebugThickness();
	const float SphereSize = 15.0f;
	const float ArrowSize = 50.0f;

	// 绘制 PivotLocation（枢轴点）
	if (UNamiCameraSettings::ShouldDrawPivotLocation())
	{
		// 绿色球体表示枢轴点
		DrawDebugSphere(World, View.PivotLocation, SphereSize, 12, FColor::Green, false, DrawTime, 0, Thickness);
	}

	// 绘制 CameraLocation（相机位置）
	if (UNamiCameraSettings::ShouldDrawCameraLocation())
	{
		// 蓝色球体表示相机位置
		DrawDebugSphere(World, View.CameraLocation, SphereSize, 12, FColor::Blue, false, DrawTime, 0, Thickness);
	}

	// 绘制从相机到枢轴点的连线（吊臂）
	if (UNamiCameraSettings::ShouldDrawArmInfo() && 
		UNamiCameraSettings::ShouldDrawPivotLocation() && 
		UNamiCameraSettings::ShouldDrawCameraLocation())
	{
		// 红色连线表示吊臂
		DrawDebugLine(World, View.CameraLocation, View.PivotLocation, FColor::Red, false, DrawTime, 0, Thickness);
	}

	// 绘制相机方向（Forward/Right/Up）
	if (UNamiCameraSettings::ShouldDrawCameraDirection() && UNamiCameraSettings::ShouldDrawCameraLocation())
	{
		const FVector Forward = View.CameraRotation.Vector();
		const FVector Right = FRotationMatrix(View.CameraRotation).GetScaledAxis(EAxis::Y);
		const FVector Up = FRotationMatrix(View.CameraRotation).GetScaledAxis(EAxis::Z);
		
		// 前方向（红色箭头）
		DrawDebugDirectionalArrow(World, View.CameraLocation, 
			View.CameraLocation + Forward * ArrowSize, 
			ArrowSize * 0.3f, FColor::Red, false, DrawTime, 0, Thickness);
		
		// 右方向（绿色箭头）
		DrawDebugDirectionalArrow(World, View.CameraLocation, 
			View.CameraLocation + Right * ArrowSize * 0.5f, 
			ArrowSize * 0.15f, FColor::Green, false, DrawTime, 0, Thickness);
		
		// 上方向（蓝色箭头）
		DrawDebugDirectionalArrow(World, View.CameraLocation, 
			View.CameraLocation + Up * ArrowSize * 0.5f, 
			ArrowSize * 0.15f, FColor::Blue, false, DrawTime, 0, Thickness);
	}

	// 输出相机信息日志（如果启用）
	if (UNamiCameraSettings::ShouldLogCameraInfo())
	{
		FNamiCameraDebugInfo DebugInfo;
		DebugInfo.FromView(View);
		NAMI_LOG_CAMERA_INFO(Log, TEXT("%s"), *DebugInfo.ToString());
	}
}

// ========== 相机管线处理（重构后的阶段方法） ==========

bool UNamiCameraComponent::PreProcessPipeline(float DeltaTime, FNamiCameraPipelineContext& OutContext)
{
	// 重置上下文
	OutContext.Reset();
	OutContext.DeltaTime = DeltaTime;

	// 1. 检查基础组件
	OutContext.OwnerPawn = GetOwnerPawn();
	OutContext.OwnerPC = GetOwnerPlayerController();
	OutContext.CameraManager = GetOwnerPlayerCameraManager();

	// 2. 验证有效性
	if (!OutContext.OwnerPawn || !OutContext.OwnerPC || !OutContext.CameraManager)
	{
		OutContext.bIsValid = false;
		return false;
	}

	// 3. 检查模式堆栈
	if (CameraModePriorityStack.Num() == 0)
	{
		OutContext.bIsValid = false;
		return false;
	}

	OutContext.bIsValid = true;
	return true;
}

bool UNamiCameraComponent::ProcessModeStack(float DeltaTime, const FNamiCameraPipelineContext& Context, FNamiCameraView& OutBaseView)
{
	// 评估 BlendingStack 的堆栈权重
	// EvaluateStack 内部会：
	// 1. UpdateStack() - Tick 所有激活模式，更新混合权重
	// 2. BlendStack() - 混合所有模式的视图
	return BlendingStack.EvaluateStack(DeltaTime, OutBaseView);
}

void UNamiCameraComponent::ProcessGlobalFeatures(float DeltaTime, FNamiCameraPipelineContext& Context, FNamiCameraView& InOutView)
{
	// 1. 收集基础状态（来自 Mode Stack）
	FNamiCameraState BaseState;
	BaseState.PivotLocation = InOutView.PivotLocation;
	BaseState.PivotRotation = InOutView.ControlRotation;
	BaseState.CameraLocation = InOutView.CameraLocation;
	BaseState.CameraRotation = InOutView.CameraRotation;
	BaseState.FieldOfView = InOutView.FOV;
	// 其他参数使用默认值或从视图反推（简化实现）
	BaseState.ComputeOutput();
	Context.BaseState = BaseState;
	Context.bHasBaseState = true;

	// 2. 收集玩家输入状态（如果有 Input Feature）
	// TODO: 从 Input Feature 获取玩家输入状态
	// 暂时使用 ControlRotation 作为玩家输入
	FNamiCameraState PlayerInputState;
	PlayerInputState.PivotRotation = InOutView.ControlRotation;
	Context.PlayerInputState = PlayerInputState;
	Context.bHasPlayerInputState = true;

	// 3. 更新玩家输入信息
	// TODO: 从 Input Feature 获取实际输入向量
	// 暂时使用 ControlRotation 的变化来检测输入
	Context.PlayerInput.Update(FVector2D::ZeroVector); // 简化实现

	// 4. 保存 EffectView（用于 Feature 访问）
	Context.EffectView = InOutView;

	// 5. 应用全局 Feature（使用新接口）
	for (UNamiCameraFeature* Feature : GlobalFeatures)
	{
		if (!Feature || !Feature->IsEnabled())
		{
			continue;
		}

		Feature->Update(DeltaTime);
		
		// 调用新接口（带 Context）
		Feature->ApplyToViewWithContext(InOutView, DeltaTime, Context);
	}

	// 6. 应用参数更新控制到最终视图
	ApplyParamUpdateControl(Context, InOutView);

	// 7. 更新 EffectView
	Context.EffectView = InOutView;
}

void UNamiCameraComponent::ApplyParamUpdateControl(const FNamiCameraPipelineContext& Context, FNamiCameraView& InOutView)
{
	// 应用参数更新控制到视图
	// 根据 Context.ParamUpdateControl 的模式，修改视图参数

	// 旋转参数
	if (Context.ParamUpdateControl.PivotRotation == ENamiCameraParamUpdateMode::PlayerInput)
	{
		// 使用玩家输入
		if (Context.bHasPlayerInputState)
		{
			InOutView.ControlRotation = Context.PlayerInputState.PivotRotation;
		}
	}
	else if (Context.ParamUpdateControl.PivotRotation == ENamiCameraParamUpdateMode::Frozen)
	{
		// 冻结：使用保存的值
		if (Context.bHasPreservedValues)
		{
			InOutView.ControlRotation = Context.PreservedValues.PivotRotation;
		}
	}
	else if (Context.ParamUpdateControl.PivotRotation == ENamiCameraParamUpdateMode::BaseValue)
	{
		// 基础值：使用基础状态
		if (Context.bHasBaseState)
		{
			InOutView.ControlRotation = Context.BaseState.PivotRotation;
		}
	}

	// 相机旋转
	if (Context.ParamUpdateControl.CameraRotation == ENamiCameraParamUpdateMode::PlayerInput)
	{
		// 使用玩家输入
		if (Context.bHasPlayerInputState)
		{
			InOutView.CameraRotation = Context.PlayerInputState.CameraRotation;
		}
	}
	else if (Context.ParamUpdateControl.CameraRotation == ENamiCameraParamUpdateMode::Frozen)
	{
		// 冻结：使用保存的值
		if (Context.bHasPreservedValues)
		{
			InOutView.CameraRotation = Context.PreservedValues.CameraRotation;
		}
	}
	else if (Context.ParamUpdateControl.CameraRotation == ENamiCameraParamUpdateMode::BaseValue)
	{
		// 基础值：使用基础状态
		if (Context.bHasBaseState)
		{
			InOutView.CameraRotation = Context.BaseState.CameraRotation;
		}
	}

	// 位置参数
	if (Context.ParamUpdateControl.PivotLocation == ENamiCameraParamUpdateMode::BaseValue)
	{
		if (Context.bHasBaseState)
		{
			InOutView.PivotLocation = Context.BaseState.PivotLocation;
		}
	}

	if (Context.ParamUpdateControl.CameraLocation == ENamiCameraParamUpdateMode::BaseValue)
	{
		if (Context.bHasBaseState)
		{
			InOutView.CameraLocation = Context.BaseState.CameraLocation;
		}
	}

	// FOV
	if (Context.ParamUpdateControl.FieldOfView == ENamiCameraParamUpdateMode::BaseValue)
	{
		if (Context.bHasBaseState)
		{
			InOutView.FOV = Context.BaseState.FieldOfView;
		}
	}
}

void UNamiCameraComponent::ProcessControllerSync(float DeltaTime, const FNamiCameraPipelineContext& Context, const FNamiCameraView& InView)
{
	APlayerController* PC = Context.OwnerPC;
	if (!PC)
	{
		return;
	}

	const FVector DesiredCtrlLoc = InView.ControlLocation;
	const FRotator DesiredCtrlRot = InView.ControlRotation;

	// 初始化检查
	if (!bHasInitializedControl)
	{
		CurrentControlLocation = DesiredCtrlLoc;
		CurrentControlRotation = DesiredCtrlRot;
		bHasInitializedControl = true;
	}
	else
	{
		// 位置平滑混合
		if (ControlLocationBlendSpeed > 0.0f)
		{
			CurrentControlLocation = FMath::VInterpTo(
				CurrentControlLocation,
				DesiredCtrlLoc,
				DeltaTime,
				ControlLocationBlendSpeed);
		}
		else
		{
			CurrentControlLocation = DesiredCtrlLoc;
		}

		// 旋转平滑混合
		if (ControlRotationBlendSpeed > 0.0f)
		{
			CurrentControlRotation = FMath::RInterpTo(
				CurrentControlRotation,
				DesiredCtrlRot,
				DeltaTime,
				ControlRotationBlendSpeed);
		}
		else
		{
			CurrentControlRotation = DesiredCtrlRot;
		}
	}

	// 更新 PlayerController
	FHitResult HitResult;
	PC->K2_SetActorLocation(CurrentControlLocation, false, HitResult, true);
	PC->SetControlRotation(CurrentControlRotation);
}

void UNamiCameraComponent::ProcessSmoothing(float DeltaTime, const FNamiCameraView& InView, FMinimalViewInfo& OutPOV)
{
	// 构建目标 POV（Mode 层计算的结果）
	FMinimalViewInfo TargetPOV;
	TargetPOV.Location = InView.CameraLocation;
	TargetPOV.Rotation = InView.CameraRotation;
	TargetPOV.FOV = InView.FOV;
	TargetPOV.OrthoWidth = OrthoWidth;
	TargetPOV.OrthoNearClipPlane = OrthoNearClipPlane;
	TargetPOV.OrthoFarClipPlane = OrthoFarClipPlane;
	TargetPOV.AspectRatio = AspectRatio;
	TargetPOV.bConstrainAspectRatio = bConstrainAspectRatio;
	TargetPOV.bUseFieldOfViewForLOD = bUseFieldOfViewForLOD;
	TargetPOV.ProjectionMode = ProjectionMode;
	TargetPOV.PostProcessBlendWeight = PostProcessBlendWeight;
	if (PostProcessBlendWeight > 0.0f)
	{
		TargetPOV.PostProcessSettings = PostProcessSettings;
	}

	// 初始化检查（首次调用时）
	if (!bHasInitializedCurrentView)
	{
		CurrentActualView = TargetPOV;
		bHasInitializedCurrentView = true;
	}

	// 位置平滑混合
	if (LocationBlendSpeed > 0.0f)
	{
		CurrentActualView.Location = FMath::VInterpTo(
			CurrentActualView.Location,
			TargetPOV.Location,
			DeltaTime,
			LocationBlendSpeed);
	}
	else
	{
		CurrentActualView.Location = TargetPOV.Location; // 瞬切
	}

	// 旋转平滑混合（球面插值）
	if (RotationBlendSpeed > 0.0f)
	{
		CurrentActualView.Rotation = FMath::RInterpTo(
			CurrentActualView.Rotation,
			TargetPOV.Rotation,
			DeltaTime,
			RotationBlendSpeed);
	}
	else
	{
		CurrentActualView.Rotation = TargetPOV.Rotation; // 瞬切
	}

	// FOV 平滑混合
	if (FOVBlendSpeed > 0.0f)
	{
		CurrentActualView.FOV = FMath::FInterpTo(
			CurrentActualView.FOV,
			TargetPOV.FOV,
			DeltaTime,
			FOVBlendSpeed);
	}
	else
	{
		CurrentActualView.FOV = TargetPOV.FOV; // 瞬切
	}

	// 同步其他属性（不需要平滑的属性）
	CurrentActualView.OrthoWidth = TargetPOV.OrthoWidth;
	CurrentActualView.OrthoNearClipPlane = TargetPOV.OrthoNearClipPlane;
	CurrentActualView.OrthoFarClipPlane = TargetPOV.OrthoFarClipPlane;
	CurrentActualView.AspectRatio = TargetPOV.AspectRatio;
	CurrentActualView.bConstrainAspectRatio = TargetPOV.bConstrainAspectRatio;
	CurrentActualView.bUseFieldOfViewForLOD = TargetPOV.bUseFieldOfViewForLOD;
	CurrentActualView.ProjectionMode = TargetPOV.ProjectionMode;
	CurrentActualView.PostProcessBlendWeight = TargetPOV.PostProcessBlendWeight;
	CurrentActualView.PostProcessSettings = TargetPOV.PostProcessSettings;

	// 输出
	OutPOV = CurrentActualView;
}

void UNamiCameraComponent::PostProcessPipeline(float DeltaTime, const FNamiCameraPipelineContext& Context, FMinimalViewInfo& InOutPOV)
{
	// 5.1 Debug 绘制（使用阶段2的 EffectView）
	DrawDebugCameraInfo(Context.EffectView);

	// 5.2 Debug 日志（如果启用）
	if (UNamiCameraSettings::ShouldEnableStackDebugLog())
	{
		const UNamiCameraSettings* Settings = UNamiCameraSettings::Get();
		if (Settings)
		{
			BlendingStack.DumpCameraModeStack(true, false, Settings->DebugLogTextColor, Settings->DebugLogDuration);
		}
	}

	// 5.3 同步到组件
	SetWorldLocationAndRotation(InOutPOV.Location, InOutPOV.Rotation);
	FieldOfView = InOutPOV.FOV;
}
