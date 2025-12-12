// Copyright Epic Games, Inc. All Rights Reserved.

#include "Notifies/AnimNotifyState_CameraAdjust.h"
#include "Components/NamiCameraComponent.h"
#include "Adjusts/NamiAnimNotifyCameraAdjust.h"
#include "LogNamiCamera.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UAnimNotifyState_CameraAdjust::UAnimNotifyState_CameraAdjust()
{
	// 默认值 - 包装结构体默认都是禁用状态
	BlendInTime = 0.15f;
	BlendOutTime = 0.2f;
	BlendType = ENamiCameraBlendType::EaseInOut;
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
	Adjust->Priority = Priority;

	// 对于 ArmRotation Override 模式，设置臂旋转目标值
	if (ArmRotation.bEnabled && ArmRotation.BlendMode == ENamiCameraAdjustBlendMode::Override)
	{
		Adjust->ArmRotationTarget = ArmRotation.Value;
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

	if (FOV.bEnabled)
	{
		ActiveAdjustments.Add(FString::Printf(TEXT("FOV%+.0f"), FOV.Value));
	}
	if (ArmLength.bEnabled)
	{
		ActiveAdjustments.Add(FString::Printf(TEXT("Arm%+.0f"), ArmLength.Value));
	}
	if (ArmRotation.bEnabled)
	{
		if (ArmRotation.BlendMode == ENamiCameraAdjustBlendMode::Override)
		{
			ActiveAdjustments.Add(TEXT("ArmRot[O]"));
		}
		else
		{
			ActiveAdjustments.Add(TEXT("ArmRot"));
		}
	}
	if (CameraOffset.bEnabled)
	{
		ActiveAdjustments.Add(TEXT("CamOff"));
	}
	if (CameraRotation.bEnabled)
	{
		ActiveAdjustments.Add(TEXT("CamRot"));
	}
	if (PivotOffset.bEnabled)
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
	if (FOV.bEnabled)
	{
		Params.FOVOffset = FOV.Value;
		Params.FOVBlendMode = FOV.BlendMode;
		Params.MarkFOVModified();
	}

	// 臂长
	if (ArmLength.bEnabled)
	{
		Params.TargetArmLengthOffset = ArmLength.Value;
		Params.ArmLengthBlendMode = ArmLength.BlendMode;
		Params.MarkTargetArmLengthModified();
	}

	// 臂旋转
	if (ArmRotation.bEnabled)
	{
		// Additive 模式：Value 作为偏移量
		// Override 模式：Value 作为目标（已在 NotifyBegin 中设置到 Adjust->ArmRotationTarget）
		if (ArmRotation.BlendMode == ENamiCameraAdjustBlendMode::Additive)
		{
			Params.ArmRotationOffset = ArmRotation.Value;
		}
		else
		{
			// Override 模式下，偏移值在 CalculateCombinedAdjustParams 中从 Target 计算
			Params.ArmRotationOffset = FRotator::ZeroRotator;
		}
		Params.ArmRotationBlendMode = ArmRotation.BlendMode;
		Params.MarkArmRotationModified();
	}

	// 相机位置偏移
	if (CameraOffset.bEnabled)
	{
		Params.CameraLocationOffset = CameraOffset.Value;
		Params.CameraOffsetBlendMode = CameraOffset.BlendMode;
		Params.MarkCameraLocationOffsetModified();
	}

	// 相机旋转偏移
	if (CameraRotation.bEnabled)
	{
		Params.CameraRotationOffset = CameraRotation.Value;
		Params.CameraRotationBlendMode = CameraRotation.BlendMode;
		Params.MarkCameraRotationOffsetModified();
	}

	// Pivot偏移
	if (PivotOffset.bEnabled)
	{
		Params.PivotOffset = PivotOffset.Value;
		Params.PivotOffsetBlendMode = PivotOffset.BlendMode;
		Params.MarkPivotOffsetModified();
	}

	return Params;
}
