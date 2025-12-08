// Copyright Epic Games, Inc. All Rights Reserved.

#include "Features/NamiSpringArmFeature.h"
#include "Modes/Templates/NamiFollowCameraMode.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiSpringArmFeature)

UNamiSpringArmFeature::UNamiSpringArmFeature()
{
	FeatureName = TEXT("SpringArm");
	Priority = 50;  // 中等优先级，在速度预测之后执行
}

void UNamiSpringArmFeature::Initialize_Implementation(UNamiCameraModeBase* InCameraMode)
{
	Super::Initialize_Implementation(InCameraMode);
	
	// 初始化 SpringArm
	SpringArm.Initialize();
}

void UNamiSpringArmFeature::Activate_Implementation()
{
	Super::Activate_Implementation();
	
	// 重新初始化 SpringArm（激活时重置状态）
	SpringArm.Initialize();
}

void UNamiSpringArmFeature::Update_Implementation(float DeltaTime)
{
	Super::Update_Implementation(DeltaTime);
	
	// SpringArm 的更新在 ApplyToView 中进行
}

void UNamiSpringArmFeature::ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime)
{
	Super::ApplyToView_Implementation(InOutView, DeltaTime);

	if (!IsEnabled())
	{
		return;
	}

	// 准备 SpringArm 的输入
	// 使用 PivotLocation 作为 SpringArm 的起点
	FVector SpringArmOrigin = InOutView.PivotLocation;
	
	// 使用当前的相机旋转
	FRotator SpringArmRotation = InOutView.CameraRotation;
	
	// 创建初始Transform
	FTransform InitialTransform(SpringArmRotation, SpringArmOrigin);
	FVector OffsetLocation = FVector::ZeroVector;  // 可以用于额外的偏移
	
	// 获取忽略的Actor列表
	TArray<AActor*> IgnoreActorList = GetIgnoreActors();
	
	// 调用 SpringArm.Tick 计算相机位置
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		SpringArm.Tick(World, DeltaTime, IgnoreActorList, InitialTransform, OffsetLocation);
	}
	
	// 从 SpringArm 获取最终相机Transform
	FTransform CameraTransform = SpringArm.GetCameraTransform();
	
	// 更新视图的相机位置和旋转
	InOutView.CameraLocation = CameraTransform.GetLocation();
	InOutView.CameraRotation = CameraTransform.Rotator();
	
	// 应用 CameraOffset（如果相机模式是 NamiFollowCameraMode）
	// 这样可以确保 CameraOffset 的效果不会被 SpringArm 覆盖
	UNamiCameraModeBase* OwnerMode = GetCameraMode();
	if (IsValid(OwnerMode))
	{
		UNamiFollowCameraMode* FollowMode = Cast<UNamiFollowCameraMode>(OwnerMode);
		if (IsValid(FollowMode))
		{
			FVector CameraOffset = FollowMode->CameraOffset;
			
			// 如果 CameraOffset 不为零，应用它
			if (!CameraOffset.IsNearlyZero())
			{
				// 应用目标旋转（如果启用）
				if (FollowMode->bUseTargetRotation)
				{
					// 获取主目标的旋转
					AActor* PrimaryTarget = FollowMode->GetPrimaryTarget();
					if (IsValid(PrimaryTarget))
					{
						FRotator TargetRotation = PrimaryTarget->GetActorRotation();
						if (FollowMode->bUseYawOnly)
						{
							TargetRotation.Pitch = 0.0f;
							TargetRotation.Roll = 0.0f;
						}
						CameraOffset = TargetRotation.RotateVector(CameraOffset);
					}
				}
				
				// 将 CameraOffset 应用到相机位置
				InOutView.CameraLocation += CameraOffset;
			}
		}
	}
}

TArray<AActor*> UNamiSpringArmFeature::GetIgnoreActors() const
{
	TArray<AActor*> Result;
	
	if (bAutoGetTarget)
	{
		// 从 FollowCameraMode 获取主目标
		UNamiCameraModeBase* OwnerMode = GetCameraMode();
		if (IsValid(OwnerMode))
		{
			UNamiFollowCameraMode* FollowMode = Cast<UNamiFollowCameraMode>(OwnerMode);
			if (IsValid(FollowMode))
			{
				AActor* PrimaryTarget = FollowMode->GetPrimaryTarget();
				if (IsValid(PrimaryTarget))
				{
					Result.Add(PrimaryTarget);
					
					// 忽略附加的Actor
					PrimaryTarget->ForEachAttachedActors([&Result](AActor* Actor)
					{
						Result.AddUnique(Actor);
						return true;
					});
				}
			}
		}
	}
	else
	{
		// 使用手动指定的忽略Actor列表
		for (AActor* Actor : IgnoreActors)
		{
			if (IsValid(Actor))
			{
				Result.Add(Actor);
			}
		}
	}
	
	return Result;
}

