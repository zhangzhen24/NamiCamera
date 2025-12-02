// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modifiers/NamiCameraEffectModifier.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraShakeBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "LogNamiCamera.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraEffectModifier)

UNamiCameraEffectModifier::UNamiCameraEffectModifier()
{
	// 默认禁用，需要手动激活
	bDisabled = true;
}

bool UNamiCameraEffectModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);
	
	if (!bIsActive || bIsPaused)
	{
		return false;
	}
	
	// 保存原始视图
	OriginalPOV = InOutPOV;
	
	// 更新激活时间
	ActiveTime += DeltaTime;
	
	// 检查是否超时
	if (Duration > 0.0f && ActiveTime >= Duration && !bIsExiting)
	{
		DeactivateEffect(false);
	}
	
	// 计算混合权重
	float Weight = CalculateBlendWeight(DeltaTime);
	CurrentBlendWeight = Weight;
	
	// 如果权重为 0，不应用任何效果
	if (Weight <= 0.0f)
	{
		if (bIsExiting)
		{
			// 完全退出，移除修改器
			DisableModifier(true);
			bIsActive = false;
			bIsExiting = false;
		}
		return false;
	}
	
	// 应用各种效果
	ApplyLocationOffset(InOutPOV, Weight);
	ApplyRotationOffset(InOutPOV, Weight);
	ApplyFOVOffset(InOutPOV, Weight);
	ApplyLookAt(InOutPOV, Weight);
	ApplyPostProcess(InOutPOV, Weight);
	
	return true;
}

void UNamiCameraEffectModifier::AddedToCamera(APlayerCameraManager* Camera)
{
	Super::AddedToCamera(Camera);
	
	UE_LOG(LogNamiCamera, Log, TEXT("[UNamiCameraEffectModifier] 添加到相机：%s"), *EffectName.ToString());
}

void UNamiCameraEffectModifier::ActivateEffect(bool bResetTimer)
{
	if (bResetTimer || !bIsActive)
	{
		ActiveTime = 0.0f;
	}
	
	bIsActive = true;
	bIsExiting = false;
	bIsPaused = false;
	bDisabled = false;
	
	// 播放相机震动
	if (bPlayShakeOnStart && CameraShake && CameraOwner)
	{
		ActiveShakeInstance = CameraOwner->StartCameraShake(CameraShake, ShakeScale);
	}
	
	UE_LOG(LogNamiCamera, Log, TEXT("[UNamiCameraEffectModifier] 激活效果：%s，持续时间：%.2f"), 
		*EffectName.ToString(), Duration);
}

void UNamiCameraEffectModifier::DeactivateEffect(bool bForceImmediate)
{
	if (!bIsActive)
	{
		return;
	}
	
	// 停止相机震动
	if (ActiveShakeInstance.IsValid() && CameraOwner)
	{
		CameraOwner->StopCameraShake(ActiveShakeInstance.Get(), bForceImmediate);
		ActiveShakeInstance = nullptr;
	}
	
	if (bForceImmediate)
	{
		// 立即停止
		bIsActive = false;
		bIsExiting = false;
		CurrentBlendWeight = 0.0f;
		DisableModifier(true);
	}
	else
	{
		// 开始退出混合
		bIsExiting = true;
		ActiveTime = 0.0f;  // 重置时间用于 BlendOut
	}
	
	UE_LOG(LogNamiCamera, Log, TEXT("[UNamiCameraEffectModifier] 停止效果：%s，立即：%s"), 
		*EffectName.ToString(), bForceImmediate ? TEXT("是") : TEXT("否"));
}

void UNamiCameraEffectModifier::PauseEffect()
{
	bIsPaused = true;
	UE_LOG(LogNamiCamera, Verbose, TEXT("[UNamiCameraEffectModifier] 暂停效果：%s"), *EffectName.ToString());
}

void UNamiCameraEffectModifier::ResumeEffect()
{
	bIsPaused = false;
	UE_LOG(LogNamiCamera, Verbose, TEXT("[UNamiCameraEffectModifier] 恢复效果：%s"), *EffectName.ToString());
}

void UNamiCameraEffectModifier::SetLookAtTarget(AActor* Target, FVector Offset)
{
	bOverrideLookAtTarget = true;
	bUseLookAtActor = true;
	LookAtTarget = Target;
	LookAtOffset = Offset;
}

void UNamiCameraEffectModifier::SetLookAtLocation(FVector Location)
{
	bOverrideLookAtTarget = true;
	bUseLookAtActor = false;
	LookAtLocation = Location;
}

float UNamiCameraEffectModifier::CalculateBlendWeight(float DeltaTime)
{
	float Weight = 1.0f;
	
	if (bIsExiting)
	{
		// 退出混合
		if (BlendOutTime > 0.0f)
		{
			Weight = 1.0f - FMath::Clamp(ActiveTime / BlendOutTime, 0.0f, 1.0f);
			
			// 应用混合函数
			if (BlendCurve)
			{
				Weight = BlendCurve->GetFloatValue(1.0f - Weight);
			}
			else
			{
				Weight = FMath::InterpEaseInOut(0.0f, 1.0f, 1.0f - Weight, 2.0f);
			}
		}
		else
		{
			Weight = 0.0f;
		}
	}
	else
	{
		// 进入混合
		if (BlendInTime > 0.0f && ActiveTime < BlendInTime)
		{
			Weight = FMath::Clamp(ActiveTime / BlendInTime, 0.0f, 1.0f);
			
			// 应用混合函数
			if (BlendCurve)
			{
				Weight = BlendCurve->GetFloatValue(Weight);
			}
			else
			{
				Weight = FMath::InterpEaseInOut(0.0f, 1.0f, Weight, 2.0f);
			}
		}
	}
	
	return Weight;
}

void UNamiCameraEffectModifier::ApplyLocationOffset(FMinimalViewInfo& InOutPOV, float Weight)
{
	if (!bEnableLocationOffset || LocationOffset.IsNearlyZero())
	{
		return;
	}
	
	FVector Offset = LocationOffset * Weight;
	
	// 根据空间类型应用偏移
	switch (LocationOffsetSpace)
	{
	case RTS_World:
		InOutPOV.Location += Offset;
		break;
		
	case RTS_Actor:
	case RTS_Component:
		// 局部空间：根据相机旋转转换偏移
		InOutPOV.Location += InOutPOV.Rotation.RotateVector(Offset);
		break;
		
	default:
		InOutPOV.Location += Offset;
		break;
	}
}

void UNamiCameraEffectModifier::ApplyRotationOffset(FMinimalViewInfo& InOutPOV, float Weight)
{
	if (!bEnableRotationOffset || RotationOffset.IsNearlyZero())
	{
		return;
	}
	
	FRotator Offset = RotationOffset * Weight;
	InOutPOV.Rotation += Offset;
	InOutPOV.Rotation.Normalize();
}

void UNamiCameraEffectModifier::ApplyFOVOffset(FMinimalViewInfo& InOutPOV, float Weight)
{
	if (!bEnableFOVOffset)
	{
		return;
	}
	
	if (bOverrideFOV)
	{
		// 覆盖 FOV
		InOutPOV.FOV = FMath::Lerp(InOutPOV.FOV, TargetFOV, Weight);
	}
	else
	{
		// 偏移 FOV
		InOutPOV.FOV += FOVOffset * Weight;
	}
	
	// 限制 FOV 范围
	InOutPOV.FOV = FMath::Clamp(InOutPOV.FOV, 5.0f, 170.0f);
}

void UNamiCameraEffectModifier::ApplyLookAt(FMinimalViewInfo& InOutPOV, float Weight)
{
	if (!bOverrideLookAtTarget)
	{
		return;
	}
	
	FVector TargetLocation = FVector::ZeroVector;
	bool bHasTarget = false;
	
	if (bUseLookAtActor && LookAtTarget.IsValid())
	{
		TargetLocation = LookAtTarget->GetActorLocation() + LookAtOffset;
		bHasTarget = true;
	}
	else if (!bUseLookAtActor)
	{
		TargetLocation = LookAtLocation;
		bHasTarget = true;
	}
	
	if (bHasTarget)
	{
		// 计算 LookAt 旋转
		FRotator LookAtRotation = (TargetLocation - InOutPOV.Location).Rotation();
		
		// 混合旋转
		FRotator BlendedRotation = FMath::Lerp(InOutPOV.Rotation, LookAtRotation, LookAtWeight * Weight);
		InOutPOV.Rotation = BlendedRotation;
	}
}

void UNamiCameraEffectModifier::ApplyPostProcess(FMinimalViewInfo& InOutPOV, float Weight)
{
	if (!bEnablePostProcess || PostProcessWeight <= 0.0f)
	{
		return;
	}
	
	float FinalWeight = PostProcessWeight * Weight;
	
	// 混合后处理设置
	InOutPOV.PostProcessBlendWeight = FinalWeight;
	InOutPOV.PostProcessSettings = PostProcessSettings;
}

