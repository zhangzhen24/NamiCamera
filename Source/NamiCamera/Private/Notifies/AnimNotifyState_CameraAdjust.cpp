// Copyright Epic Games, Inc. All Rights Reserved.

#include "Notifies/AnimNotifyState_CameraAdjust.h"
#include "Components/NamiCameraComponent.h"
#include "Adjusts/NamiAnimNotifyCameraAdjust.h"
#include "LogNamiCamera.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UAnimNotifyState_CameraAdjust::UAnimNotifyState_CameraAdjust()
{
	// 默认值
	bEnableFOV = false;
	FOVOffset = 0.f;

	bEnableArmLength = false;
	ArmLengthOffset = 0.f;

	bEnableArmRotation = false;
	ArmRotationOffset = FRotator::ZeroRotator;

	bEnableCameraOffset = false;
	CameraLocationOffset = FVector::ZeroVector;

	bEnableCameraRotation = false;
	CameraRotationOffset = FRotator::ZeroRotator;

	bEnablePivotOffset = false;
	PivotOffset = FVector::ZeroVector;

	BlendInTime = 0.15f;
	BlendOutTime = 0.2f;
	BlendType = ENamiCameraBlendType::EaseInOut;
	BlendMode = ENamiCameraAdjustBlendMode::Additive;
	Priority = 100;
}

void UAnimNotifyState_CameraAdjust::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	// 获取相机组件
	UNamiCameraComponent* CameraComp = GetCameraComponent(MeshComp);
	if (!CameraComp)
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[AnimNotifyState_CameraAdjust] Failed to find NamiCameraComponent for %s"),
			*GetNameSafe(MeshComp->GetOwner()));
		return;
	}

	// 缓存相机组件
	CachedCameraComponent = CameraComp;

	// 创建调整器实例
	UNamiAnimNotifyCameraAdjust* Adjust = NewObject<UNamiAnimNotifyCameraAdjust>(CameraComp);
	if (!Adjust)
	{
		UE_LOG(LogNamiCamera, Error, TEXT("[AnimNotifyState_CameraAdjust] Failed to create CameraAdjust instance"));
		return;
	}

	// 配置混合参数
	Adjust->BlendInTime = BlendInTime;
	Adjust->BlendOutTime = BlendOutTime;
	Adjust->BlendType = BlendType;
	Adjust->BlendMode = BlendMode;
	Adjust->Priority = Priority;

	// 对于 Override 模式，设置臂旋转目标值
	// ArmRotationOffset 在 Override 模式下被解释为相对于角色朝向的目标位置
	if (BlendMode == ENamiCameraAdjustBlendMode::Override && bEnableArmRotation)
	{
		Adjust->ArmRotationTarget = ArmRotationOffset;
	}

	// 构建并设置调整参数
	FNamiCameraAdjustParams Params = BuildAdjustParams();
	Adjust->SetAdjustParams(Params);

	// 推送调整器
	if (CameraComp->PushCameraAdjustInstance(Adjust))
	{
		ActiveAdjust = Adjust;
		UE_LOG(LogNamiCamera, Log, TEXT("[AnimNotifyState_CameraAdjust] Started camera adjust for animation: %s"),
			*GetNameSafe(Animation));
	}
	else
	{
		UE_LOG(LogNamiCamera, Warning, TEXT("[AnimNotifyState_CameraAdjust] Failed to push CameraAdjust instance"));
	}
}

void UAnimNotifyState_CameraAdjust::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	// 停止调整器（开始BlendOut）
	if (ActiveAdjust.IsValid())
	{
		UNamiCameraComponent* CameraComp = CachedCameraComponent.Get();
		if (CameraComp)
		{
			// 使用false让它正常BlendOut，而不是立即停止
			CameraComp->PopCameraAdjust(ActiveAdjust.Get(), false);
			UE_LOG(LogNamiCamera, Log, TEXT("[AnimNotifyState_CameraAdjust] Ended camera adjust for animation: %s"),
				*GetNameSafe(Animation));
		}
		ActiveAdjust.Reset();
	}

	CachedCameraComponent.Reset();
}

FString UAnimNotifyState_CameraAdjust::GetNotifyName_Implementation() const
{
	// 构建描述性名称
	TArray<FString> ActiveAdjustments;

	if (bEnableFOV)
	{
		ActiveAdjustments.Add(FString::Printf(TEXT("FOV%+.0f"), FOVOffset));
	}
	if (bEnableArmLength)
	{
		ActiveAdjustments.Add(FString::Printf(TEXT("Arm%+.0f"), ArmLengthOffset));
	}
	if (bEnableArmRotation)
	{
		ActiveAdjustments.Add(TEXT("ArmRot"));
	}
	if (bEnableCameraOffset)
	{
		ActiveAdjustments.Add(TEXT("CamOff"));
	}
	if (bEnableCameraRotation)
	{
		ActiveAdjustments.Add(TEXT("CamRot"));
	}
	if (bEnablePivotOffset)
	{
		ActiveAdjustments.Add(TEXT("Pivot"));
	}

	if (ActiveAdjustments.Num() == 0)
	{
		return TEXT("Camera Adjust (None)");
	}

	return FString::Printf(TEXT("Camera Adjust: %s"), *FString::Join(ActiveAdjustments, TEXT(", ")));
}

UNamiCameraComponent* UAnimNotifyState_CameraAdjust::GetCameraComponent(USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return nullptr;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// 尝试直接从Owner获取
	UNamiCameraComponent* CameraComp = Owner->FindComponentByClass<UNamiCameraComponent>();
	if (CameraComp)
	{
		return CameraComp;
	}

	// 如果Owner是Pawn，尝试从Controller获取
	APawn* Pawn = Cast<APawn>(Owner);
	if (Pawn)
	{
		APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
		if (PC)
		{
			// 从PlayerCameraManager获取ViewTarget的相机组件
			if (PC->PlayerCameraManager)
			{
				AActor* ViewTarget = PC->PlayerCameraManager->GetViewTarget();
				if (ViewTarget)
				{
					CameraComp = ViewTarget->FindComponentByClass<UNamiCameraComponent>();
				}
			}
		}
	}

	return CameraComp;
}

FNamiCameraAdjustParams UAnimNotifyState_CameraAdjust::BuildAdjustParams() const
{
	FNamiCameraAdjustParams Params;

	// FOV
	if (bEnableFOV)
	{
		Params.FOVOffset = FOVOffset;
		Params.MarkFOVModified();
	}

	// 臂长
	if (bEnableArmLength)
	{
		Params.TargetArmLengthOffset = ArmLengthOffset;
		Params.MarkTargetArmLengthModified();
	}

	// 臂旋转
	if (bEnableArmRotation)
	{
		Params.ArmRotationOffset = ArmRotationOffset;
		Params.MarkArmRotationModified();
	}

	// 相机位置偏移
	if (bEnableCameraOffset)
	{
		Params.CameraLocationOffset = CameraLocationOffset;
		Params.MarkCameraLocationOffsetModified();
	}

	// 相机旋转偏移
	if (bEnableCameraRotation)
	{
		Params.CameraRotationOffset = CameraRotationOffset;
		Params.MarkCameraRotationOffsetModified();
	}

	// Pivot偏移
	if (bEnablePivotOffset)
	{
		Params.PivotOffset = PivotOffset;
		Params.MarkPivotOffsetModified();
	}

	return Params;
}
