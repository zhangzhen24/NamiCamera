// Copyright Qiu, Inc. All Rights Reserved.

#include "Components/NamiCameraComponent.h"
#include "Logging/LogNamiCamera.h"
#include "Logging/LogNamiCameraMacros.h"
#include "Camera/CameraModifier.h"
#include "CameraModes/NamiCameraModeBase.h"
#include "CameraModes/NamiFollowCameraMode.h"
#include "Components/NamiPlayerCameraManager.h"
#include "Data/NamiCameraView.h"
#include "Data/NamiCameraPipelineContext.h"
#include "DevelopSetting/NamiCameraSettings.h"
#include "CameraFeatures/NamiCameraFeature.h"
#include "CameraAdjust/NamiCameraAdjust.h"
#include "Math/NamiCameraMath.h"
#include "Stat/NamiCameraStats.h"
#include "GameplayTagContainer.h"
#include "Data/NamiCameraTags.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Data/NamiCameraDebugInfo.h"

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
	LocationBlendSpeed = 1000.0f;  // 默认平滑（1000 cm/s = 10 m/s）
	RotationBlendSpeed = 360.0f;   // 默认平滑（360 度/s）
	FOVBlendSpeed = 90.0f;         // 默认平滑（90 度/s）

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
	SCOPE_CYCLE_COUNTER(STAT_NamiCamera_GetCameraView);

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

	// ========== 【阶段 2.5：相机调整层】 ==========
	ProcessCameraAdjusts(DeltaTime, Context, EffectView);

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
	// 优先返回缓存的 OwnerPlayerController
	if (IsValid(OwnerPlayerController))
	{
		return OwnerPlayerController.Get();
	}

	// 如果缓存失效，从 OwnerPawn 获取
	if (IsValid(OwnerPawn))
	{
		return Cast<APlayerController>(OwnerPawn->GetController());
	}

	return nullptr;
}

ANamiPlayerCameraManager *UNamiCameraComponent::GetOwnerPlayerCameraManager() const
{
	// 优先返回缓存的 OwnerPlayerCameraManager
	if (IsValid(OwnerPlayerCameraManager))
	{
		return OwnerPlayerCameraManager.Get();
	}

	// 如果缓存失效，从 PlayerController 获取
	if (APlayerController* PC = GetOwnerPlayerController())
	{
		return Cast<ANamiPlayerCameraManager>(PC->PlayerCameraManager);
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

	// Mode 切换时，混出所有活跃的 CameraAdjust，避免叠加在新 Mode 上导致位置错误
	for (UNamiCameraAdjust* Adjust : CameraAdjustStack)
	{
		if (IsValid(Adjust) && !Adjust->IsBlendingOut() && !Adjust->IsFullyInactive())
		{
			Adjust->RequestDeactivate(false);  // false = 平滑混出
			NAMI_LOG_COMPONENT(Log, TEXT("[UNamiCameraComponent::PushCameraModeUsingInstance] Blending out %s due to mode switch"),
				*Adjust->GetClass()->GetName());
		}
	}

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

#if WITH_EDITOR
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
#endif // WITH_EDITOR

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

	// 2. 保存 EffectView（用于 Feature 访问）
	Context.EffectView = InOutView;

	// 3. 应用全局 Feature
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

	// 4. 更新 EffectView
	Context.EffectView = InOutView;
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
		// 使用四元数插值（QInterpTo）代替 RInterpTo，自动处理最短路径，避免 ±180° 跳变
		if (ControlRotationBlendSpeed > 0.0f)
		{
			FQuat CurrentQuat = CurrentControlRotation.Quaternion();
			FQuat DesiredQuat = DesiredCtrlRot.Quaternion();
			FQuat ResultQuat = FMath::QInterpTo(CurrentQuat, DesiredQuat, DeltaTime, ControlRotationBlendSpeed);
			// 归一化到 0-360° 范围，确保与系统其他部分一致
			CurrentControlRotation = FNamiCameraMath::NormalizeRotatorTo360(ResultQuat.Rotator());
		}
		else
		{
			CurrentControlRotation = FNamiCameraMath::NormalizeRotatorTo360(DesiredCtrlRot);
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
	// 使用四元数插值（QInterpTo）代替 RInterpTo，自动处理最短路径，避免 ±180° 跳变
	if (RotationBlendSpeed > 0.0f)
	{
		FQuat CurrentQuat = CurrentActualView.Rotation.Quaternion();
		FQuat TargetQuat = TargetPOV.Rotation.Quaternion();
		FQuat ResultQuat = FMath::QInterpTo(CurrentQuat, TargetQuat, DeltaTime, RotationBlendSpeed);
		// 归一化到 0-360° 范围，确保与系统其他部分一致
		CurrentActualView.Rotation = FNamiCameraMath::NormalizeRotatorTo360(ResultQuat.Rotator());
	}
	else
	{
		CurrentActualView.Rotation = FNamiCameraMath::NormalizeRotatorTo360(TargetPOV.Rotation); // 瞬切
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

// ========== Camera Adjust 实现 ==========

UNamiCameraAdjust* UNamiCameraComponent::PushCameraAdjust(TSubclassOf<UNamiCameraAdjust> AdjustClass,
	ENamiCameraAdjustDuplicatePolicy DuplicatePolicy)
{
	if (!IsValid(AdjustClass))
	{
		NAMI_LOG_COMPONENT(Error, TEXT("[UNamiCameraComponent::PushCameraAdjust] AdjustClass is null"));
		return nullptr;
	}

	// 检查同类 Adjust 是否已存在
	UNamiCameraAdjust* ExistingAdjust = FindCameraAdjustByClass(AdjustClass);
	if (ExistingAdjust)
	{
		switch (DuplicatePolicy)
		{
		case ENamiCameraAdjustDuplicatePolicy::KeepExisting:
			// 保持现有，返回已存在的实例
			NAMI_LOG_COMPONENT(Log, TEXT("[UNamiCameraComponent::PushCameraAdjust] %s already exists, keeping existing (KeepExisting policy)"),
				*AdjustClass->GetName());
			return ExistingAdjust;

		case ENamiCameraAdjustDuplicatePolicy::Replace:
			// 平滑替换：混出现有的
			NAMI_LOG_COMPONENT(Log, TEXT("[UNamiCameraComponent::PushCameraAdjust] %s already exists, replacing with blend out (Replace policy)"),
				*AdjustClass->GetName());
			PopCameraAdjust(ExistingAdjust, false);
			break;

		case ENamiCameraAdjustDuplicatePolicy::ForceReplace:
			// 强制替换：立即移除现有的
			NAMI_LOG_COMPONENT(Log, TEXT("[UNamiCameraComponent::PushCameraAdjust] %s already exists, force replacing (ForceReplace policy)"),
				*AdjustClass->GetName());
			PopCameraAdjust(ExistingAdjust, true);
			break;

		case ENamiCameraAdjustDuplicatePolicy::AllowDuplicate:
			// 允许重复：继续创建新实例
			NAMI_LOG_COMPONENT(Log, TEXT("[UNamiCameraComponent::PushCameraAdjust] %s already exists, allowing duplicate (AllowDuplicate policy)"),
				*AdjustClass->GetName());
			break;
		}
	}

	// 创建实例
	UNamiCameraAdjust* AdjustInstance = NewObject<UNamiCameraAdjust>(this, AdjustClass);
	if (!IsValid(AdjustInstance))
	{
		NAMI_LOG_COMPONENT(Error, TEXT("[UNamiCameraComponent::PushCameraAdjust] Failed to create Adjust instance"));
		return nullptr;
	}

	// 推送实例（使用 AllowDuplicate 策略，因为我们已经在上面处理了重复检查）
	if (!PushCameraAdjustInstance(AdjustInstance, ENamiCameraAdjustDuplicatePolicy::AllowDuplicate))
	{
		return nullptr;
	}

	return AdjustInstance;
}

bool UNamiCameraComponent::PushCameraAdjustInstance(UNamiCameraAdjust* AdjustInstance,
	ENamiCameraAdjustDuplicatePolicy DuplicatePolicy)
{
	if (!IsValid(AdjustInstance))
	{
		NAMI_LOG_COMPONENT(Error, TEXT("[UNamiCameraComponent::PushCameraAdjustInstance] AdjustInstance is null"));
		return false;
	}

	// 检查同一实例是否已存在
	if (CameraAdjustStack.Contains(AdjustInstance))
	{
		NAMI_LOG_COMPONENT(Warning, TEXT("[UNamiCameraComponent::PushCameraAdjustInstance] AdjustInstance already exists in stack"));
		return false;
	}

	// 检查同类 Adjust 是否已存在
	UClass* AdjustClass = AdjustInstance->GetClass();
	UNamiCameraAdjust* ExistingAdjust = FindCameraAdjustByClass(AdjustClass);
	if (ExistingAdjust)
	{
		switch (DuplicatePolicy)
		{
		case ENamiCameraAdjustDuplicatePolicy::KeepExisting:
			// 保持现有，拒绝新实例
			NAMI_LOG_COMPONENT(Log, TEXT("[UNamiCameraComponent::PushCameraAdjustInstance] %s already exists, rejecting new instance (KeepExisting policy)"),
				*AdjustClass->GetName());
			return false;

		case ENamiCameraAdjustDuplicatePolicy::Replace:
			// 平滑替换：混出现有的
			NAMI_LOG_COMPONENT(Log, TEXT("[UNamiCameraComponent::PushCameraAdjustInstance] %s already exists, replacing with blend out (Replace policy)"),
				*AdjustClass->GetName());
			PopCameraAdjust(ExistingAdjust, false);
			break;

		case ENamiCameraAdjustDuplicatePolicy::ForceReplace:
			// 强制替换：立即移除现有的
			NAMI_LOG_COMPONENT(Log, TEXT("[UNamiCameraComponent::PushCameraAdjustInstance] %s already exists, force replacing (ForceReplace policy)"),
				*AdjustClass->GetName());
			PopCameraAdjust(ExistingAdjust, true);
			break;

		case ENamiCameraAdjustDuplicatePolicy::AllowDuplicate:
			// 允许重复：继续推送新实例
			break;
		}
	}

	// 初始化
	AdjustInstance->Initialize(this);

	// 按优先级插入到正确的位置（优先级低的在前面，优先级高的在后面）
	int32 InsertIndex = 0;
	for (; InsertIndex < CameraAdjustStack.Num(); ++InsertIndex)
	{
		if (CameraAdjustStack[InsertIndex]->Priority > AdjustInstance->Priority)
		{
			break;
		}
	}

	CameraAdjustStack.Insert(AdjustInstance, InsertIndex);

	NAMI_LOG_COMPONENT(Log, TEXT("[UNamiCameraComponent::PushCameraAdjustInstance] Pushed %s (Priority: %d) at index %d"),
		*AdjustClass->GetName(), AdjustInstance->Priority, InsertIndex);

	return true;
}

bool UNamiCameraComponent::PopCameraAdjust(UNamiCameraAdjust* AdjustInstance, bool bForceImmediate)
{
	if (!IsValid(AdjustInstance))
	{
		return false;
	}

	int32 Index = CameraAdjustStack.Find(AdjustInstance);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	// 请求停用
	AdjustInstance->RequestDeactivate(bForceImmediate);

	// 如果是立即停用，直接从堆栈中移除
	if (bForceImmediate)
	{
		CameraAdjustStack.RemoveAt(Index);
	}
	
	// 否则会在 CleanupInactiveCameraAdjusts 中移除
	return true;
}

bool UNamiCameraComponent::PopCameraAdjustByClass(TSubclassOf<UNamiCameraAdjust> AdjustClass, bool bForceImmediate)
{
	if (!IsValid(AdjustClass))
	{
		return false;
	}

	// 找到所有匹配的实例
	TArray<UNamiCameraAdjust*> ToRemove;
	for (UNamiCameraAdjust* Adjust : CameraAdjustStack)
	{
		if (IsValid(Adjust) && Adjust->GetClass() == AdjustClass)
		{
			ToRemove.Add(Adjust);
		}
	}

	bool bResult = ToRemove.Num() > 0;
	for (UNamiCameraAdjust* Adjust : ToRemove)
	{
		PopCameraAdjust(Adjust, bForceImmediate);
	}

	return bResult;
}

UNamiCameraAdjust* UNamiCameraComponent::FindCameraAdjustByClass(TSubclassOf<UNamiCameraAdjust> AdjustClass) const
{
	if (!IsValid(AdjustClass))
	{
		return nullptr;
	}

	for (UNamiCameraAdjust* Adjust : CameraAdjustStack)
	{
		if (IsValid(Adjust) && Adjust->GetClass() == AdjustClass)
		{
			return Adjust;
		}
	}

	return nullptr;
}

const TArray<UNamiCameraAdjust*>& UNamiCameraComponent::GetCameraAdjusts() const
{
	// 将 TObjectPtr 数组转换为原始指针数组
	static TArray<UNamiCameraAdjust*> Result;
	Result.Reset();
	for (const TObjectPtr<UNamiCameraAdjust>& Adjust : CameraAdjustStack)
	{
		if (Adjust.Get())
		{
			Result.Add(Adjust.Get());
		}
	}
	return Result;
}

bool UNamiCameraComponent::HasCameraAdjust(TSubclassOf<UNamiCameraAdjust> AdjustClass) const
{
	return FindCameraAdjustByClass(AdjustClass) != nullptr;
}

void UNamiCameraComponent::ProcessCameraAdjusts(float DeltaTime, FNamiCameraPipelineContext& Context, FNamiCameraView& InOutView)
{
	if (CameraAdjustStack.Num() == 0)
	{
		return;
	}

	// 计算当前臂旋转（用于 Override 模式）
	FVector CurrentArmDir = InOutView.CameraLocation - InOutView.PivotLocation;
	FRotator CurrentArmRotation = CurrentArmDir.Rotation();

	// 调试：追踪打断后的帧（在开头记录 CameraAdjust 应用前的状态）
	if (InputInterruptDebugFrameCounter > 0)
	{
		UE_LOG(LogNamiCamera, Log, TEXT("[InputInterrupt] === 第 %d 帧 (CameraAdjust应用前) ==="), InputInterruptDebugFrameCounter);
		UE_LOG(LogNamiCamera, Log, TEXT("  CameraLocation: X=%.2f Y=%.2f Z=%.2f"),
			InOutView.CameraLocation.X, InOutView.CameraLocation.Y, InOutView.CameraLocation.Z);
		UE_LOG(LogNamiCamera, Log, TEXT("  ArmRotation(Mode输出): P=%.2f Y=%.2f R=%.2f"),
			CurrentArmRotation.Pitch, CurrentArmRotation.Yaw, CurrentArmRotation.Roll);
		UE_LOG(LogNamiCamera, Log, TEXT("  ControlRotation(View): P=%.2f Y=%.2f R=%.2f"),
			InOutView.ControlRotation.Pitch, InOutView.ControlRotation.Yaw, InOutView.ControlRotation.Roll);

		InputInterruptDebugFrameCounter++;
		if (InputInterruptDebugFrameCounter > 3)
		{
			InputInterruptDebugFrameCounter = 0; // 只追踪3帧
		}
	}

	// 计算合并后的调整参数
	FNamiCameraAdjustParams CombinedParams = CalculateCombinedAdjustParams(DeltaTime, CurrentArmRotation, InOutView);

	// 应用调整参数到视图
	ApplyAdjustParamsToView(CombinedParams, InOutView);

	// 应用待同步的 ControlRotation（如果有）
	// 这确保 ProcessControllerSync 使用正确的值，而不是 Mode 的输出
	if (bPendingControlRotationSync)
	{
		InOutView.ControlRotation = PendingControlRotation;
		bPendingControlRotationSync = false;
		UE_LOG(LogNamiCamera, Log, TEXT("[InputInterrupt] 应用待同步 ControlRotation 到 InOutView: P=%.2f Y=%.2f R=%.2f"),
			PendingControlRotation.Pitch, PendingControlRotation.Yaw, PendingControlRotation.Roll);
	}

	// 调试：记录 CameraAdjust 应用后的最终视图
	if (InputInterruptDebugFrameCounter == 1)
	{
		// 打断帧：保存 CameraAdjust 应用后的实际相机位置
		InputInterruptSavedView = InOutView;
		UE_LOG(LogNamiCamera, Log, TEXT("[InputInterrupt] 打断帧 - CameraAdjust 应用后 View:"));
		UE_LOG(LogNamiCamera, Log, TEXT("  CameraLocation: X=%.2f Y=%.2f Z=%.2f"),
			InOutView.CameraLocation.X, InOutView.CameraLocation.Y, InOutView.CameraLocation.Z);
		UE_LOG(LogNamiCamera, Log, TEXT("  CameraRotation: P=%.2f Y=%.2f R=%.2f"),
			InOutView.CameraRotation.Pitch, InOutView.CameraRotation.Yaw, InOutView.CameraRotation.Roll);

		// 计算应用后的臂旋转
		FVector FinalArmDir = InOutView.CameraLocation - InOutView.PivotLocation;
		FRotator FinalArmRotation = FinalArmDir.Rotation();
		UE_LOG(LogNamiCamera, Log, TEXT("  FinalArmRotation(计算): P=%.2f Y=%.2f R=%.2f"),
			FinalArmRotation.Pitch, FinalArmRotation.Yaw, FinalArmRotation.Roll);
	}
	else if (InputInterruptDebugFrameCounter > 1)
	{
		// 后续帧：记录 CameraAdjust 应用后的视图并对比
		UE_LOG(LogNamiCamera, Log, TEXT("[InputInterrupt] 第 %d 帧 - CameraAdjust 应用后 View:"), InputInterruptDebugFrameCounter);
		UE_LOG(LogNamiCamera, Log, TEXT("  CameraLocation: X=%.2f Y=%.2f Z=%.2f"),
			InOutView.CameraLocation.X, InOutView.CameraLocation.Y, InOutView.CameraLocation.Z);
		UE_LOG(LogNamiCamera, Log, TEXT("  CameraRotation: P=%.2f Y=%.2f R=%.2f"),
			InOutView.CameraRotation.Pitch, InOutView.CameraRotation.Yaw, InOutView.CameraRotation.Roll);

		// 计算应用后的臂旋转
		FVector FinalArmDir = InOutView.CameraLocation - InOutView.PivotLocation;
		FRotator FinalArmRotation = FinalArmDir.Rotation();
		UE_LOG(LogNamiCamera, Log, TEXT("  FinalArmRotation(计算): P=%.2f Y=%.2f R=%.2f"),
			FinalArmRotation.Pitch, FinalArmRotation.Yaw, FinalArmRotation.Roll);

		// 与打断帧对比
		FVector LocDelta = InOutView.CameraLocation - InputInterruptSavedView.CameraLocation;
		FRotator RotDelta = InOutView.CameraRotation - InputInterruptSavedView.CameraRotation;
		UE_LOG(LogNamiCamera, Log, TEXT("[InputInterrupt] 与打断帧对比:"));
		UE_LOG(LogNamiCamera, Log, TEXT("  CameraLocation 差异: X=%.2f Y=%.2f Z=%.2f (距离=%.2f)"),
			LocDelta.X, LocDelta.Y, LocDelta.Z, LocDelta.Size());
		UE_LOG(LogNamiCamera, Log, TEXT("  CameraRotation 差异: P=%.2f Y=%.2f R=%.2f"),
			RotDelta.Pitch, RotDelta.Yaw, RotDelta.Roll);
	}

	// 清理已完全停用的调整器
	CleanupInactiveCameraAdjusts();
}

FNamiCameraAdjustParams UNamiCameraComponent::CalculateCombinedAdjustParams(float DeltaTime, const FRotator& CurrentArmRotation, const FNamiCameraView& CurrentView)
{
	FNamiCameraAdjustParams CombinedParams;

	// 使用四元数累积臂旋转偏移（避免欧拉角插值问题）
	FQuat CombinedArmRotationQuat = FQuat::Identity;
	FQuat CurrentArmQuat = CurrentArmRotation.Quaternion();
	bool bHasArmRotation = false;

	for (UNamiCameraAdjust* Adjust : CameraAdjustStack)
	{
		if (!IsValid(Adjust))
		{
			continue;
		}

		// 获取经过权重缩放的参数（这会触发状态更新）
		FNamiCameraAdjustParams AdjustParams = Adjust->GetWeightedAdjustParams(DeltaTime);

		// 跳过权重为0的调整器
		float Weight = Adjust->GetCurrentBlendWeight();
		if (Weight <= 0.f)
		{
			continue;
		}

		// 按每个参数的 BlendMode 分别处理

		// FOV
		if (AdjustParams.HasFlag(ENamiCameraAdjustModifiedFlags::FOV))
		{
			// 目前 FOV 只支持 Additive 模式，直接累加
			CombinedParams.FOVOffset += AdjustParams.FOVOffset;
			CombinedParams.FOVBlendMode = AdjustParams.FOVBlendMode;
			CombinedParams.MarkFOVModified();
		}

		// ArmLength
		if (AdjustParams.HasFlag(ENamiCameraAdjustModifiedFlags::TargetArmLength))
		{
			CombinedParams.TargetArmLengthOffset += AdjustParams.TargetArmLengthOffset;
			CombinedParams.ArmLengthBlendMode = AdjustParams.ArmLengthBlendMode;
			CombinedParams.MarkTargetArmLengthModified();
		}

		// ArmRotation - 检查玩家输入控制后使用四元数进行混合计算
		if (AdjustParams.HasFlag(ENamiCameraAdjustModifiedFlags::ArmRotation))
		{
			bool bSkipArmRotation = false;

			// 检查是否允许玩家输入
			if (Adjust->bAllowPlayerInput)
			{
				// 允许玩家输入，跳过 ArmRotation 混合
				bSkipArmRotation = true;
			}
			// 检查混出状态 - 混出期间允许玩家输入
			else if (Adjust->IsBlendingOut())
			{
				// 混出开始的第一帧：同步 ControlRotation 到当前位置（防止瞬切）
				if (!Adjust->IsBlendOutSynced())
				{
					// 计算混出前相机臂的实际位置
					// 混出前是 Active 状态（权重=1.0），相机就在目标位置
					FRotator AdjustedArmRotation = CurrentArmRotation;
					if (AdjustParams.ArmRotationBlendMode == ENamiCameraAdjustBlendMode::Override)
					{
						// Override 模式：混出前相机就在缓存的目标位置
						// 不需要 Slerp 计算，直接使用目标旋转
						AdjustedArmRotation = Adjust->GetCachedWorldArmRotationTarget();
					}
					else
					{
						// Additive 模式：使用满权重的偏移
						// AdjustParams 中的偏移已经被当前权重缩放过了，需要还原到满权重
						float CurrentWeight = Adjust->GetCurrentBlendWeight();
						FRotator FullWeightOffset = (CurrentWeight > KINDA_SMALL_NUMBER)
							? AdjustParams.ArmRotationOffset * (1.0f / CurrentWeight)
							: AdjustParams.ArmRotationOffset;
						AdjustedArmRotation = CurrentArmRotation + FullWeightOffset;
					}

					// 将 ArmRotation 转换为 ControlRotation
					// 注意：不使用 Normalize()，避免 ±180° 边界跳变
					FRotator AdjustedControlRotation = AdjustedArmRotation;
					AdjustedControlRotation.Yaw += 180.0f;
					AdjustedControlRotation.Pitch = -AdjustedControlRotation.Pitch;

					// 获取同步前的状态
					APlayerController* PC = GetOwnerPlayerController();
					FRotator OldControlRotation = PC ? PC->GetControlRotation() : FRotator::ZeroRotator;

					UE_LOG(LogNamiCamera, Log, TEXT("[BlendOut] ========== 混出同步 =========="));
					UE_LOG(LogNamiCamera, Log, TEXT("[BlendOut] 混出前 View:"));
					UE_LOG(LogNamiCamera, Log, TEXT("  CameraLocation: X=%.2f Y=%.2f Z=%.2f"),
						CurrentView.CameraLocation.X, CurrentView.CameraLocation.Y, CurrentView.CameraLocation.Z);
					UE_LOG(LogNamiCamera, Log, TEXT("  CameraRotation: P=%.2f Y=%.2f R=%.2f"),
						CurrentView.CameraRotation.Pitch, CurrentView.CameraRotation.Yaw, CurrentView.CameraRotation.Roll);
					UE_LOG(LogNamiCamera, Log, TEXT("  PivotLocation: X=%.2f Y=%.2f Z=%.2f"),
						CurrentView.PivotLocation.X, CurrentView.PivotLocation.Y, CurrentView.PivotLocation.Z);
					UE_LOG(LogNamiCamera, Log, TEXT("  ControlRotation(View): P=%.2f Y=%.2f"),
						CurrentView.ControlRotation.Pitch, CurrentView.ControlRotation.Yaw);
					UE_LOG(LogNamiCamera, Log, TEXT("  FOV: %.2f"), CurrentView.FOV);
					UE_LOG(LogNamiCamera, Log, TEXT("[BlendOut] 相机臂信息:"));
					UE_LOG(LogNamiCamera, Log, TEXT("  CurrentArmRotation(Mode输出): P=%.2f Y=%.2f"),
						CurrentArmRotation.Pitch, CurrentArmRotation.Yaw);
					UE_LOG(LogNamiCamera, Log, TEXT("  TargetArmRotation(缓存目标): P=%.2f Y=%.2f"),
						Adjust->GetCachedWorldArmRotationTarget().Pitch, Adjust->GetCachedWorldArmRotationTarget().Yaw);
					UE_LOG(LogNamiCamera, Log, TEXT("  AdjustedArmRotation(同步值): P=%.2f Y=%.2f"),
						AdjustedArmRotation.Pitch, AdjustedArmRotation.Yaw);
					UE_LOG(LogNamiCamera, Log, TEXT("  BlendWeight: %.3f"), Weight);
					UE_LOG(LogNamiCamera, Log, TEXT("[BlendOut] ControlRotation:"));
					UE_LOG(LogNamiCamera, Log, TEXT("  PC->ControlRotation(同步前): P=%.2f Y=%.2f"),
						OldControlRotation.Pitch, OldControlRotation.Yaw);
					UE_LOG(LogNamiCamera, Log, TEXT("  AdjustedControlRotation(同步值): P=%.2f Y=%.2f"),
						AdjustedControlRotation.Pitch, AdjustedControlRotation.Yaw);

					// 同步 ControlRotation
					SyncArmRotationToControlRotation(AdjustedControlRotation);
					bPendingControlRotationSync = true;
					PendingControlRotation = AdjustedControlRotation;

					// 记录同步后的状态
					FRotator NewControlRotation = PC ? PC->GetControlRotation() : FRotator::ZeroRotator;
					UE_LOG(LogNamiCamera, Log, TEXT("[BlendOut] 同步后:"));
					UE_LOG(LogNamiCamera, Log, TEXT("  PC->ControlRotation(同步后): P=%.2f Y=%.2f"),
						NewControlRotation.Pitch, NewControlRotation.Yaw);

					// 标记已同步
					Adjust->MarkBlendOutSynced();

					// 【关键】这一帧继续应用 CameraAdjust 偏移
					// 因为 Mode 是用旧的 ControlRotation 计算的
					// 下一帧 Mode 才会用新同步的 ControlRotation
				}
				else
				{
					// 第二帧及以后：Mode 已经用新 ControlRotation，跳过偏移
					bSkipArmRotation = true;
				}
			}
			// 检查输入打断（仅在未被打断时检测）
			else if (!Adjust->IsInputInterrupted())
			{
				bool bHasInput = DetectPlayerCameraInput(Adjust->InputInterruptThreshold);
				// 每秒打印一次输入检测状态（避免日志刷屏）
				static float LastLogTime = 0.f;
				float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
				if (CurrentTime - LastLogTime > 1.0f)
				{
					UE_LOG(LogNamiCamera, Log, TEXT("[InputInterrupt] 输入检测: bHasInput=%s, Threshold=%.2f"),
						bHasInput ? TEXT("true") : TEXT("false"), Adjust->InputInterruptThreshold);
					LastLogTime = CurrentTime;
				}
				if (bHasInput)
				{
					// 计算应用 CameraAdjust 偏移后的臂旋转
					// 这才是相机实际所在位置对应的臂旋转
					FRotator AdjustedArmRotation = CurrentArmRotation;
					if (AdjustParams.ArmRotationBlendMode == ENamiCameraAdjustBlendMode::Override)
					{
						// Override 模式：计算 Slerp 混合后的旋转
						FQuat TargetQuat = Adjust->GetCachedWorldArmRotationTarget().Quaternion();
						FQuat InterpolatedQuat = FQuat::Slerp(CurrentArmQuat, TargetQuat, Weight);
						// 归一化到 0-360° 范围，避免 ±180° 边界跳变
						AdjustedArmRotation = FNamiCameraMath::NormalizeRotatorTo360(InterpolatedQuat.Rotator());
					}
					else
					{
						// Additive 模式：加上偏移，然后归一化
						AdjustedArmRotation = FNamiCameraMath::NormalizeRotatorTo360(CurrentArmRotation + AdjustParams.ArmRotationOffset);
					}

					// 将 ArmRotation 转换为 ControlRotation
					// ArmRotation 是从 Pivot 到相机的方向
					// ControlRotation 是相机看的方向（朝向角色）= ArmRotation + 180°
					// 注意：不使用 Normalize()，避免 ±180° 边界跳变
					FRotator AdjustedControlRotation = AdjustedArmRotation;
					AdjustedControlRotation.Yaw += 180.0f;
					AdjustedControlRotation.Pitch = -AdjustedControlRotation.Pitch;

					// 记录打断前的状态
					APlayerController* PC = GetOwnerPlayerController();
					FRotator OldControlRotation = PC ? PC->GetControlRotation() : FRotator::ZeroRotator;

					UE_LOG(LogNamiCamera, Log, TEXT("[InputInterrupt] ========== 输入打断触发 =========="));
					UE_LOG(LogNamiCamera, Log, TEXT("[InputInterrupt] 打断前 View (CameraAdjust应用前):"));
					UE_LOG(LogNamiCamera, Log, TEXT("  CameraLocation: X=%.2f Y=%.2f Z=%.2f"),
						CurrentView.CameraLocation.X, CurrentView.CameraLocation.Y, CurrentView.CameraLocation.Z);
					UE_LOG(LogNamiCamera, Log, TEXT("  CameraRotation: P=%.2f Y=%.2f R=%.2f"),
						CurrentView.CameraRotation.Pitch, CurrentView.CameraRotation.Yaw, CurrentView.CameraRotation.Roll);
					UE_LOG(LogNamiCamera, Log, TEXT("  PivotLocation: X=%.2f Y=%.2f Z=%.2f"),
						CurrentView.PivotLocation.X, CurrentView.PivotLocation.Y, CurrentView.PivotLocation.Z);
					UE_LOG(LogNamiCamera, Log, TEXT("  ControlRotation(View): P=%.2f Y=%.2f R=%.2f"),
						CurrentView.ControlRotation.Pitch, CurrentView.ControlRotation.Yaw, CurrentView.ControlRotation.Roll);
					UE_LOG(LogNamiCamera, Log, TEXT("  FOV: %.2f"), CurrentView.FOV);

					UE_LOG(LogNamiCamera, Log, TEXT("[InputInterrupt] 相机臂信息:"));
					UE_LOG(LogNamiCamera, Log, TEXT("  CurrentArmRotation(Mode输出): P=%.2f Y=%.2f R=%.2f"),
						CurrentArmRotation.Pitch, CurrentArmRotation.Yaw, CurrentArmRotation.Roll);
					UE_LOG(LogNamiCamera, Log, TEXT("  AdjustedArmRotation(Slerp后): P=%.2f Y=%.2f R=%.2f"),
						AdjustedArmRotation.Pitch, AdjustedArmRotation.Yaw, AdjustedArmRotation.Roll);
					UE_LOG(LogNamiCamera, Log, TEXT("  AdjustedControlRotation(Arm+180): P=%.2f Y=%.2f R=%.2f"),
						AdjustedControlRotation.Pitch, AdjustedControlRotation.Yaw, AdjustedControlRotation.Roll);
					UE_LOG(LogNamiCamera, Log, TEXT("  TargetArmRotation: P=%.2f Y=%.2f R=%.2f"),
						Adjust->GetCachedWorldArmRotationTarget().Pitch, Adjust->GetCachedWorldArmRotationTarget().Yaw, Adjust->GetCachedWorldArmRotationTarget().Roll);
					UE_LOG(LogNamiCamera, Log, TEXT("  BlendWeight: %.3f"), Weight);

					UE_LOG(LogNamiCamera, Log, TEXT("[InputInterrupt] ControlRotation:"));
					UE_LOG(LogNamiCamera, Log, TEXT("  PC->ControlRotation(打断前): P=%.2f Y=%.2f R=%.2f"),
						OldControlRotation.Pitch, OldControlRotation.Yaw, OldControlRotation.Roll);
					UE_LOG(LogNamiCamera, Log, TEXT("  CurrentControlRotation(缓存): P=%.2f Y=%.2f R=%.2f"),
						CurrentControlRotation.Pitch, CurrentControlRotation.Yaw, CurrentControlRotation.Roll);

					// 保存打断前的视图（用于后续帧对比）
					// 注意：这里保存的是 CameraAdjust 应用前的视图
					// 实际相机位置需要在 ApplyAdjustParamsToView 后才知道
					InputInterruptSavedView = CurrentView;
					InputInterruptDebugFrameCounter = 1;

					// 同步 ControlRotation（不是 ArmRotation！）
					// ControlRotation = 相机朝向 = ArmRotation + 180°
					// 这样下一帧 Mode 用新 ControlRotation 计算出的 ArmRotation = 当前相机臂位置
					SyncArmRotationToControlRotation(AdjustedControlRotation);

					// 设置待同步标记，让 ProcessCameraAdjusts 修改 InOutView.ControlRotation
					// 这样 ProcessControllerSync 就会使用正确的值，而不是 Mode 的输出
					bPendingControlRotationSync = true;
					PendingControlRotation = AdjustedControlRotation;

					// 记录打断后的状态
					FRotator NewControlRotation = PC ? PC->GetControlRotation() : FRotator::ZeroRotator;
					UE_LOG(LogNamiCamera, Log, TEXT("[InputInterrupt] 同步后:"));
					UE_LOG(LogNamiCamera, Log, TEXT("  PC->ControlRotation(打断后): P=%.2f Y=%.2f R=%.2f"),
						NewControlRotation.Pitch, NewControlRotation.Yaw, NewControlRotation.Roll);
					UE_LOG(LogNamiCamera, Log, TEXT("  CurrentControlRotation(缓存): P=%.2f Y=%.2f R=%.2f"),
						CurrentControlRotation.Pitch, CurrentControlRotation.Yaw, CurrentControlRotation.Roll);

					// 触发输入打断
					Adjust->TriggerInputInterrupt();

					// 【关键】这一帧继续应用 ArmRotation 偏移，不设置 bSkipArmRotation
					// 下一帧 IsInputInterrupted() 为 true，才会跳过
				}
			}
			else
			{
				// 已被打断，跳过 ArmRotation
				bSkipArmRotation = true;
			}

			if (!bSkipArmRotation)
			{
				bHasArmRotation = true;

				if (AdjustParams.ArmRotationBlendMode == ENamiCameraAdjustBlendMode::Override)
				{
					// Override 模式：使用四元数 Slerp 从当前位置混合到目标
					FQuat TargetQuat = Adjust->GetCachedWorldArmRotationTarget().Quaternion();
					// Slerp 计算从当前到目标的插值旋转
					FQuat InterpolatedQuat = FQuat::Slerp(CurrentArmQuat, TargetQuat, Weight);
					// 计算相对于当前的偏移四元数
					FQuat OffsetQuat = InterpolatedQuat * CurrentArmQuat.Inverse();
					// 组合到累积的旋转四元数
					CombinedArmRotationQuat = CombinedArmRotationQuat * OffsetQuat;
				}
				else
				{
					// Additive 模式：将偏移转换为四元数后组合（偏移已被权重缩放）
					FQuat AdditiveQuat = AdjustParams.ArmRotationOffset.Quaternion();
					CombinedArmRotationQuat = CombinedArmRotationQuat * AdditiveQuat;
				}
				CombinedParams.ArmRotationBlendMode = AdjustParams.ArmRotationBlendMode;
			}
		}

		// CameraOffset
		if (AdjustParams.HasFlag(ENamiCameraAdjustModifiedFlags::CameraLocationOffset))
		{
			CombinedParams.CameraLocationOffset += AdjustParams.CameraLocationOffset;
			CombinedParams.CameraOffsetBlendMode = AdjustParams.CameraOffsetBlendMode;
			CombinedParams.MarkCameraLocationOffsetModified();
		}

		// CameraRotation
		if (AdjustParams.HasFlag(ENamiCameraAdjustModifiedFlags::CameraRotationOffset))
		{
			CombinedParams.CameraRotationOffset += AdjustParams.CameraRotationOffset;
			CombinedParams.CameraRotationBlendMode = AdjustParams.CameraRotationBlendMode;
			CombinedParams.MarkCameraRotationOffsetModified();
		}

		// PivotOffset
		if (AdjustParams.HasFlag(ENamiCameraAdjustModifiedFlags::PivotOffset))
		{
			CombinedParams.PivotOffset += AdjustParams.PivotOffset;
			CombinedParams.PivotOffsetBlendMode = AdjustParams.PivotOffsetBlendMode;
			CombinedParams.MarkPivotOffsetModified();
		}
	}

	// 将四元数转换回欧拉角偏移
	// 归一化到 0-360° 范围，确保与系统其他部分一致，避免 ±180° 边界跳变
	if (bHasArmRotation)
	{
		CombinedParams.ArmRotationOffset = FNamiCameraMath::NormalizeRotatorTo360(CombinedArmRotationQuat.Rotator());
		CombinedParams.MarkArmRotationModified();
	}

	return CombinedParams;
}

void UNamiCameraComponent::ApplyAdjustParamsToView(const FNamiCameraAdjustParams& Params, FNamiCameraView& InOutView)
{
	// 应用FOV调整
	if (Params.HasFlag(ENamiCameraAdjustModifiedFlags::FOV))
	{
		InOutView.FOV += Params.FOVOffset;
		InOutView.FOV *= Params.FOVMultiplier;
		InOutView.FOV = FMath::Clamp(InOutView.FOV, 5.f, 170.f);
	}

	// 应用相机位置偏移
	if (Params.HasFlag(ENamiCameraAdjustModifiedFlags::CameraLocationOffset))
	{
		// 转换为相机本地空间的偏移
		FRotator CamRot = InOutView.CameraRotation;
		FVector WorldOffset = CamRot.RotateVector(Params.CameraLocationOffset);
		InOutView.CameraLocation += WorldOffset;
	}

	// 应用相机旋转偏移
	// 归一化到 0-360° 范围，避免 ±180° 边界跳变
	if (Params.HasFlag(ENamiCameraAdjustModifiedFlags::CameraRotationOffset))
	{
		InOutView.CameraRotation = FNamiCameraMath::NormalizeRotatorTo360(
			(FQuat(InOutView.CameraRotation) * FQuat(Params.CameraRotationOffset)).Rotator());
	}

	// 应用Pivot偏移
	if (Params.HasFlag(ENamiCameraAdjustModifiedFlags::PivotOffset))
	{
		InOutView.PivotLocation += Params.PivotOffset;
	}

	// 应用臂旋转偏移（使用四元数进行球面旋转）
	if (Params.HasFlag(ENamiCameraAdjustModifiedFlags::ArmRotation))
	{
		// 重新计算相机位置（基于臂旋转变化，绕Pivot点旋转）
		FVector ArmDir = InOutView.CameraLocation - InOutView.PivotLocation;
		float ArmLength = ArmDir.Size();
		if (ArmLength > KINDA_SMALL_NUMBER)
		{
			// 直接使用 ArmRotationOffset 的四元数表示
			// 这与 CalculateCombinedAdjustParams 中的四元数混合计算兼容
			FQuat CombinedRotation = Params.ArmRotationOffset.Quaternion();

			// 应用旋转到臂方向
			FVector NewArmDir = CombinedRotation.RotateVector(ArmDir);
			InOutView.CameraLocation = InOutView.PivotLocation + NewArmDir;

			// 重新计算相机朝向，使其始终看向 PivotLocation
			FVector LookDirection = InOutView.PivotLocation - InOutView.CameraLocation;
			if (!LookDirection.IsNearlyZero())
			{
				InOutView.CameraRotation = LookDirection.Rotation();
			}
		}
	}

	// 应用臂长偏移
	if (Params.HasFlag(ENamiCameraAdjustModifiedFlags::ArmLength) ||
		Params.HasFlag(ENamiCameraAdjustModifiedFlags::TargetArmLength))
	{
		FVector ArmDir = InOutView.CameraLocation - InOutView.PivotLocation;
		float CurrentArmLength = ArmDir.Size();
		if (CurrentArmLength > KINDA_SMALL_NUMBER)
		{
			ArmDir.Normalize();
			float NewArmLength = (CurrentArmLength + Params.TargetArmLengthOffset) * Params.TargetArmLengthMultiplier;
			NewArmLength = FMath::Max(NewArmLength, 0.f);
			InOutView.CameraLocation = InOutView.PivotLocation + ArmDir * NewArmLength;
		}
	}
}

void UNamiCameraComponent::CleanupInactiveCameraAdjusts()
{
	// 从后往前遍历，移除已完全停用的调整器
	for (int32 i = CameraAdjustStack.Num() - 1; i >= 0; --i)
	{
		UNamiCameraAdjust* Adjust = CameraAdjustStack[i];
		if (!IsValid(Adjust) || Adjust->IsFullyInactive())
		{
			CameraAdjustStack.RemoveAt(i);
		}
	}
}

bool UNamiCameraComponent::DetectPlayerCameraInput(float Threshold) const
{
	APlayerController* PC = GetOwnerPlayerController();
	if (!PC)
	{
		return false;
	}

	float TurnInput = 0.f;
	float LookInput = 0.f;
	PC->GetInputMouseDelta(TurnInput, LookInput);

	return FMath::Abs(TurnInput) > Threshold || FMath::Abs(LookInput) > Threshold;
}

void UNamiCameraComponent::SyncArmRotationToControlRotation(const FRotator& ArmRotation)
{
	// 同步缓存变量，跳过 ProcessControllerSync 的平滑混合
	CurrentControlRotation = ArmRotation;

	// 重置相机视图初始化标记，让 ProcessSmoothing 下一帧重新初始化
	// 这样相机位置会直接使用新的计算结果，避免从旧缓存位置平滑过渡导致瞬切
	bHasInitializedCurrentView = false;

	// 同步 PlayerController
	APlayerController* PC = GetOwnerPlayerController();
	if (PC)
	{
		PC->SetControlRotation(ArmRotation);
	}
}
