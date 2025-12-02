// Fill out your copyright notice in the Description page of Project Settings.

#include "Libraries/NamiCameraLibrary.h"
#include "Components/NamiCameraComponent.h"
#include "Modes/Presets/NamiTopDownCamera.h"
#include "Modes/Presets/NamiThirdPersonCamera.h"
#include "LogNamiCamera.h"

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

