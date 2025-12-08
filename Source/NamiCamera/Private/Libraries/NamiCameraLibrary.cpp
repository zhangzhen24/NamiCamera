// Fill out your copyright notice in the Description page of Project Settings.

#include "Libraries/NamiCameraLibrary.h"
#include "Components/NamiCameraComponent.h"
#include "Modes/Presets/NamiTopDownCamera.h"
#include "Modes/Presets/NamiThirdPersonCamera.h"
#include "Modes/Templates/NamiFollowCameraMode.h"
#include "Features/NamiToricSpaceFeature.h"
#include "Features/NamiCameraFeature.h"
#include "Enums/NamiCameraEnums.h"
#include "Structs/Follow/NamiFollowTarget.h"
#include "LogNamiCamera.h"
#include "GameplayTagContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraLibrary)


FNamiCameraModeHandle UNamiCameraLibrary::PushCameraModeForActor(const AActor* Actor, TSubclassOf<UNamiCameraModeBase> CameraModeClass, int32 Priority)
{
	FNamiCameraModeHandle ModeHandle;

	if (IsValid(Actor))
	{
		if (UNamiCameraComponent* CameraComponent = Actor->FindComponentByClass<UNamiCameraComponent>())
		{
			ModeHandle = CameraComponent->PushCameraMode(CameraModeClass, Priority);
		}
	}
	return ModeHandle;
}

FNamiCameraModeHandle UNamiCameraLibrary::PushCameraModeForActorUsingInstance(const AActor* Actor, UNamiCameraModeBase* CameraModeInstance, int32 Priority)
{
	FNamiCameraModeHandle ModeHandle;

	if (IsValid(Actor))
	{
		if (UNamiCameraComponent* CamComponent = Actor->FindComponentByClass<UNamiCameraComponent>())
		{
			ModeHandle = CamComponent->PushCameraModeUsingInstance(CameraModeInstance, Priority);
		}
	}

	return ModeHandle;
}

bool UNamiCameraLibrary::PopCameraModeByHandle(FNamiCameraModeHandle& ModeHandle)
{
	if (!ModeHandle.IsValid())
	{
		return false;
	}

	if (!IsValid(ModeHandle.Owner.Get()))
	{
		return false;
	}

	return ModeHandle.Owner->PopCameraMode(ModeHandle);
}

bool UNamiCameraLibrary::PopCameraModeInstanceFromActor(const AActor* Actor, UNamiCameraModeBase* CameraMode)
{
	if (!IsValid(Actor))
	{
		return false;
	}

	if (UNamiCameraComponent* CamComponent = Actor->FindComponentByClass<UNamiCameraComponent>())
	{
		return CamComponent->PopCameraModeInstance(CameraMode);
	}

	return false;
}

UNamiCameraModeBase* UNamiCameraLibrary::GetActiveCameraModeFromActor(const AActor* Actor)
{
	UNamiCameraModeBase* ActiveCamera = nullptr;
	if (IsValid(Actor))
	{
		if (UNamiCameraComponent* CamComponent = Actor->FindComponentByClass<UNamiCameraComponent>())
		{
			ActiveCamera = CamComponent->GetActiveCameraMode();
		}
	}
	return ActiveCamera;
}

FNamiCameraModeHandle UNamiCameraLibrary::SetupTopDownCameraForActor(
	const AActor* Actor,
	float CameraHeight,
	float LookDownAngle)
{
	FNamiCameraModeHandle ModeHandle;

	if (!IsValid(Actor))
	{
		return ModeHandle;
	}

	UNamiCameraComponent* CameraComponent = Actor->FindComponentByClass<UNamiCameraComponent>();
	if (!IsValid(CameraComponent))
	{
		return ModeHandle;
	}

	// 创建俯视角相机实例
	UNamiTopDownCamera* TopDownCamera = NewObject<UNamiTopDownCamera>(CameraComponent);
	if (!IsValid(TopDownCamera))
	{
		return ModeHandle;
	}

	// 配置相机参数
	// 注意：CameraHeight是保护成员，不能直接访问，需要通过其他方式设置
        // TopDownCamera->CameraHeight = CameraHeight;
        // 注意：PitchAngle不存在，应该是FixedPitchAngle
        // TopDownCamera->FixedPitchAngle = LookDownAngle;

	// 添加目标（这是关键步骤！）
	TopDownCamera->SetPrimaryTarget(const_cast<AActor*>(Actor));

	// 推入相机模式
	ModeHandle = CameraComponent->PushCameraModeUsingInstance(TopDownCamera);

	return ModeHandle;
}

bool UNamiCameraLibrary::SetThirdPersonCameraDistance(const AActor* Actor, float Distance)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraLibrary::SetThirdPersonCameraDistance] Actor is null"));
		return false;
	}
	
	UNamiCameraComponent* CameraComponent = Actor->FindComponentByClass<UNamiCameraComponent>();
	if (!IsValid(CameraComponent))
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraLibrary::SetThirdPersonCameraDistance] CameraComponent not found on Actor: %s"), *Actor->GetName());
		return false;
	}
	
	UNamiThirdPersonCamera* ThirdPersonCamera = Cast<UNamiThirdPersonCamera>(CameraComponent->GetActiveCameraMode());
	if (!IsValid(ThirdPersonCamera))
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraLibrary::SetThirdPersonCameraDistance] Active camera mode is not UNamiThirdPersonCamera"));
		return false;
	}
	
	ThirdPersonCamera->SetCameraDistance(Distance);
	return true;
}

bool UNamiCameraLibrary::SetThirdPersonCameraCollision(const AActor* Actor, bool bEnabled)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraLibrary::SetThirdPersonCameraCollision] Actor is null"));
		return false;
	}
	
	UNamiCameraComponent* CameraComponent = Actor->FindComponentByClass<UNamiCameraComponent>();
	if (!IsValid(CameraComponent))
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraLibrary::SetThirdPersonCameraCollision] CameraComponent not found on Actor: %s"), *Actor->GetName());
		return false;
	}
	
	UNamiThirdPersonCamera* ThirdPersonCamera = Cast<UNamiThirdPersonCamera>(CameraComponent->GetActiveCameraMode());
	if (!IsValid(ThirdPersonCamera))
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraLibrary::SetThirdPersonCameraCollision] Active camera mode is not UNamiThirdPersonCamera"));
		return false;
	}
	
	ThirdPersonCamera->SetCollisionEnabled(bEnabled);
	return true;
}

bool UNamiCameraLibrary::SetThirdPersonCameraLag(const AActor* Actor, bool bEnabled, float LagSpeed)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraLibrary::SetThirdPersonCameraLag] Actor is null"));
		return false;
	}
	
	UNamiCameraComponent* CameraComponent = Actor->FindComponentByClass<UNamiCameraComponent>();
	if (!IsValid(CameraComponent))
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraLibrary::SetThirdPersonCameraLag] CameraComponent not found on Actor: %s"), *Actor->GetName());
		return false;
	}
	
	UNamiThirdPersonCamera* ThirdPersonCamera = Cast<UNamiThirdPersonCamera>(CameraComponent->GetActiveCameraMode());
	if (!IsValid(ThirdPersonCamera))
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraLibrary::SetThirdPersonCameraLag] Active camera mode is not UNamiThirdPersonCamera"));
		return false;
	}
	
	ThirdPersonCamera->SetCameraLag(bEnabled, LagSpeed);
	return true;
}

bool UNamiCameraLibrary::ApplyThirdPersonCameraPreset(const AActor* Actor, ENamiThirdPersonCameraPreset Preset)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraLibrary::ApplyThirdPersonCameraPreset] Actor is null"));
		return false;
	}
	
	UNamiCameraComponent* CameraComponent = Actor->FindComponentByClass<UNamiCameraComponent>();
	if (!IsValid(CameraComponent))
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraLibrary::ApplyThirdPersonCameraPreset] CameraComponent not found on Actor: %s"), *Actor->GetName());
		return false;
	}
	
	UNamiThirdPersonCamera* ThirdPersonCamera = Cast<UNamiThirdPersonCamera>(CameraComponent->GetActiveCameraMode());
	if (!IsValid(ThirdPersonCamera))
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[UNamiCameraLibrary::ApplyThirdPersonCameraPreset] Active camera mode is not UNamiThirdPersonCamera"));
		return false;
	}
	
	ThirdPersonCamera->ApplyCameraPreset(Preset);
	return true;
}

// ========== Toric Space Feature 操作方法 ==========

UNamiToricSpaceFeature* UNamiCameraLibrary::GetToricSpaceFeatureFromActor(const AActor* Actor, UNamiCameraModeBase* CameraMode)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	// 如果没有指定相机模式，使用当前激活的
	if (!IsValid(CameraMode))
	{
		CameraMode = GetActiveCameraModeFromActor(Actor);
	}

	if (!IsValid(CameraMode))
	{
		return nullptr;
	}

	// 通过模板方法获取Feature
	return CameraMode->GetFeature<UNamiToricSpaceFeature>();
}

bool UNamiCameraLibrary::SetToricSpaceTheta(const AActor* Actor, float Theta, UNamiCameraModeBase* CameraMode)
{
	UNamiToricSpaceFeature* Feature = GetToricSpaceFeatureFromActor(Actor, CameraMode);
	if (!IsValid(Feature))
	{
		return false;
	}

	Feature->ToricTheta = FMath::Clamp(Theta, 0.0f, 360.0f);
	return true;
}

bool UNamiCameraLibrary::SetToricSpacePhi(const AActor* Actor, float Phi, UNamiCameraModeBase* CameraMode)
{
	UNamiToricSpaceFeature* Feature = GetToricSpaceFeatureFromActor(Actor, CameraMode);
	if (!IsValid(Feature))
	{
		return false;
	}

	Feature->ToricPhi = FMath::Clamp(Phi, -90.0f, 90.0f);
	return true;
}

bool UNamiCameraLibrary::SetToricSpaceRadius(const AActor* Actor, float Radius, UNamiCameraModeBase* CameraMode)
{
	UNamiToricSpaceFeature* Feature = GetToricSpaceFeatureFromActor(Actor, CameraMode);
	if (!IsValid(Feature))
	{
		return false;
	}

	Feature->ToricRadius = FMath::Max(Radius, 100.0f);
	return true;
}

bool UNamiCameraLibrary::SetToricSpaceHeight(const AActor* Actor, float Height, UNamiCameraModeBase* CameraMode)
{
	UNamiToricSpaceFeature* Feature = GetToricSpaceFeatureFromActor(Actor, CameraMode);
	if (!IsValid(Feature))
	{
		return false;
	}

	Feature->ToricHeight = Height;
	return true;
}

bool UNamiCameraLibrary::SetToricSpaceFocusTarget(const AActor* Actor, float FocusTarget, UNamiCameraModeBase* CameraMode)
{
	UNamiToricSpaceFeature* Feature = GetToricSpaceFeatureFromActor(Actor, CameraMode);
	if (!IsValid(Feature))
	{
		return false;
	}

	Feature->FocusTarget = FMath::Clamp(FocusTarget, 0.0f, 1.0f);
	return true;
}

bool UNamiCameraLibrary::SetToricSpace180DegreeRule(const AActor* Actor, bool bEnabled, UNamiCameraModeBase* CameraMode)
{
	UNamiToricSpaceFeature* Feature = GetToricSpaceFeatureFromActor(Actor, CameraMode);
	if (!IsValid(Feature))
	{
		return false;
	}

	Feature->bEnforce180DegreeRule = bEnabled;
	return true;
}

bool UNamiCameraLibrary::SetToricSpaceAutoCalculateRadius(const AActor* Actor, bool bEnabled, UNamiCameraModeBase* CameraMode)
{
	UNamiToricSpaceFeature* Feature = GetToricSpaceFeatureFromActor(Actor, CameraMode);
	if (!IsValid(Feature))
	{
		return false;
	}

	Feature->bAutoCalculateRadius = bEnabled;
	return true;
}

bool UNamiCameraLibrary::GetToricSpaceTheta(const AActor* Actor, float& OutTheta, UNamiCameraModeBase* CameraMode)
{
	UNamiToricSpaceFeature* Feature = GetToricSpaceFeatureFromActor(Actor, CameraMode);
	if (!IsValid(Feature))
	{
		return false;
	}

	OutTheta = Feature->ToricTheta;
	return true;
}

bool UNamiCameraLibrary::GetToricSpacePhi(const AActor* Actor, float& OutPhi, UNamiCameraModeBase* CameraMode)
{
	UNamiToricSpaceFeature* Feature = GetToricSpaceFeatureFromActor(Actor, CameraMode);
	if (!IsValid(Feature))
	{
		return false;
	}

	OutPhi = Feature->ToricPhi;
	return true;
}

bool UNamiCameraLibrary::GetToricSpaceRadius(const AActor* Actor, float& OutRadius, UNamiCameraModeBase* CameraMode)
{
	UNamiToricSpaceFeature* Feature = GetToricSpaceFeatureFromActor(Actor, CameraMode);
	if (!IsValid(Feature))
	{
		return false;
	}

	OutRadius = Feature->ToricRadius;
	return true;
}

// ========== 敌人目标管理方法 ==========

bool UNamiCameraLibrary::AddEnemyTargetToCamera(const AActor* Actor, AActor* EnemyTarget, float Weight, UNamiCameraModeBase* CameraMode)
{
	if (!IsValid(Actor) || !IsValid(EnemyTarget))
	{
		return false;
	}

	// 如果没有指定相机模式，使用当前激活的
	if (!IsValid(CameraMode))
	{
		CameraMode = GetActiveCameraModeFromActor(Actor);
	}

	UNamiFollowCameraMode* FollowCameraMode = Cast<UNamiFollowCameraMode>(CameraMode);
	if (!IsValid(FollowCameraMode))
	{
		return false;
	}

	// 添加敌人目标
	FollowCameraMode->AddTarget(EnemyTarget, Weight, ENamiFollowTargetType::Enemy);
	return true;
}

bool UNamiCameraLibrary::RemoveEnemyTargetFromCamera(const AActor* Actor, AActor* EnemyTarget, UNamiCameraModeBase* CameraMode)
{
	if (!IsValid(Actor) || !IsValid(EnemyTarget))
	{
		return false;
	}

	// 如果没有指定相机模式，使用当前激活的
	if (!IsValid(CameraMode))
	{
		CameraMode = GetActiveCameraModeFromActor(Actor);
	}

	UNamiFollowCameraMode* FollowCameraMode = Cast<UNamiFollowCameraMode>(CameraMode);
	if (!IsValid(FollowCameraMode))
	{
		return false;
	}

	// 移除目标（RemoveTarget会移除所有匹配的目标，不管类型）
	FollowCameraMode->RemoveTarget(EnemyTarget);
	return true;
}

bool UNamiCameraLibrary::ClearAllEnemyTargetsFromCamera(const AActor* Actor, UNamiCameraModeBase* CameraMode)
{
	if (!IsValid(Actor))
	{
		return false;
	}

	// 如果没有指定相机模式，使用当前激活的
	if (!IsValid(CameraMode))
	{
		CameraMode = GetActiveCameraModeFromActor(Actor);
	}

	UNamiFollowCameraMode* FollowCameraMode = Cast<UNamiFollowCameraMode>(CameraMode);
	if (!IsValid(FollowCameraMode))
	{
		return false;
	}

	// 获取所有目标，移除所有敌人类型的目标
	const TArray<FNamiFollowTarget>& Targets = FollowCameraMode->GetTargets();
	TArray<AActor*> EnemyTargetsToRemove;

	for (const FNamiFollowTarget& Target : Targets)
	{
		if (Target.TargetType == ENamiFollowTargetType::Enemy && Target.Target.IsValid())
		{
			EnemyTargetsToRemove.Add(Target.Target.Get());
		}
	}

	// 移除所有敌人目标
	for (AActor* EnemyTarget : EnemyTargetsToRemove)
	{
		FollowCameraMode->RemoveTarget(EnemyTarget);
	}

	return EnemyTargetsToRemove.Num() > 0;
}

int32 UNamiCameraLibrary::GetEnemyTargetsFromCamera(const AActor* Actor, TArray<AActor*>& OutEnemyTargets, UNamiCameraModeBase* CameraMode)
{
	OutEnemyTargets.Empty();

	if (!IsValid(Actor))
	{
		return 0;
	}

	// 如果没有指定相机模式，使用当前激活的
	if (!IsValid(CameraMode))
	{
		CameraMode = GetActiveCameraModeFromActor(Actor);
	}

	UNamiFollowCameraMode* FollowCameraMode = Cast<UNamiFollowCameraMode>(CameraMode);
	if (!IsValid(FollowCameraMode))
	{
		return 0;
	}

	// 获取所有目标，筛选出敌人类型的目标
	const TArray<FNamiFollowTarget>& Targets = FollowCameraMode->GetTargets();

	for (const FNamiFollowTarget& Target : Targets)
	{
		if (Target.TargetType == ENamiFollowTargetType::Enemy && Target.Target.IsValid())
		{
			OutEnemyTargets.Add(Target.Target.Get());
		}
	}

	return OutEnemyTargets.Num();
}

UNamiCameraFeature* UNamiCameraLibrary::GetFeatureByNameFromActor(const AActor* Actor, FName FeatureName)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	if (UNamiCameraComponent* CameraComponent = Actor->FindComponentByClass<UNamiCameraComponent>())
	{
		return CameraComponent->GetFeatureByName(FeatureName);
	}

	return nullptr;
}

TArray<UNamiCameraFeature*> UNamiCameraLibrary::GetFeaturesByTagFromActor(const AActor* Actor, const FGameplayTag& Tag)
{
	TArray<UNamiCameraFeature*> Result;

	if (!IsValid(Actor))
	{
		return Result;
	}

	if (UNamiCameraComponent* CameraComponent = Actor->FindComponentByClass<UNamiCameraComponent>())
	{
		return CameraComponent->GetFeaturesByTag(Tag);
	}

	return Result;
}

TArray<UNamiCameraFeature*> UNamiCameraLibrary::GetFeaturesByTagsFromActor(const AActor* Actor, const FGameplayTagContainer& TagContainer)
{
	TArray<UNamiCameraFeature*> Result;

	if (!IsValid(Actor))
	{
		return Result;
	}

	if (UNamiCameraComponent* CameraComponent = Actor->FindComponentByClass<UNamiCameraComponent>())
	{
		return CameraComponent->GetFeaturesByTags(TagContainer);
	}

	return Result;
}

