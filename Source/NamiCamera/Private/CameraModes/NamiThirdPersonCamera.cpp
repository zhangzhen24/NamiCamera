// Copyright Epic Games, Inc. All Rights Reserved.

#include "CameraModes/NamiThirdPersonCamera.h"

#include "Components/NamiCameraComponent.h"
#include "Logging/LogNamiCamera.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiThirdPersonCamera)

// ========== UNamiThirdPersonCamera Implementation ==========

UNamiThirdPersonCamera::UNamiThirdPersonCamera()
{
	// 标准第三人称相机默认配置
	DefaultFOV = 90.0f;

	// 使用目标旋转（跟随角色Yaw）
	bUseTargetRotation = true;
	bUseYawOnly = false; // 允许玩家控制Pitch
	
	// 初始化旋转
	LastCameraRotation = FRotator::ZeroRotator;
	LastValidControlRotation = FRotator::ZeroRotator;

	// SpringArm 配置已在父类 SpringArmCameraMode 中完成
}

void UNamiThirdPersonCamera::Initialize_Implementation(UNamiCameraComponent* InCameraComponent)
{
	Super::Initialize_Implementation(InCameraComponent);
}

void UNamiThirdPersonCamera::Activate_Implementation()
{
	Super::Activate_Implementation();
}

FVector UNamiThirdPersonCamera::CalculatePivotLocation_Implementation(float DeltaTime)
{
	// 获取主要目标
	AActor* PrimaryTarget = GetPrimaryTarget();
	if (!IsValid(PrimaryTarget))
	{
		// 如果没有主要目标，使用父类的默认逻辑
		return Super::CalculatePivotLocation_Implementation(DeltaTime);
	}

	// 尝试从 Pawn 获取眼睛位置
	APawn* PawnTarget = Cast<APawn>(PrimaryTarget);
	if (PawnTarget)
	{
		// 使用 GetPawnViewLocation() 获取眼睛位置
		// 这会自动考虑 BaseEyeHeight、蹲下、跳跃等状态
		return PawnTarget->GetPawnViewLocation();
	}

	// 如果不是Pawn，尝试获取Actor的中心位置
	// 使用Bounds的中心而不是根位置，这样更合理
	FVector Origin, BoxExtent;
	PrimaryTarget->GetActorBounds(false, Origin, BoxExtent);
	return Origin;
}

FNamiCameraView UNamiThirdPersonCamera::CalculateView_Implementation(float DeltaTime)
{
	// 1-4. 调用父类（SpringArmCameraMode）计算基础视图并应用 SpringArm
	FNamiCameraView View = Super::CalculateView_Implementation(DeltaTime);

	// 5. 应用旋转限制（ThirdPersonCamera 的特定逻辑）
	FRotator FinalRotation = ApplyRotationConstraints(
		View.CameraRotation,
		bLimitPitch, MinPitch, MaxPitch,
		bLimitYaw, MinYaw, MaxYaw,
		bLockRoll);
	View.CameraRotation = FinalRotation;
	View.ControlRotation = FinalRotation;

	LastCameraRotation = View.CameraRotation;

	return View;
}
