// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modes/Templates/NamiSpringArmCameraMode.h"
#include "Components/NamiCameraComponent.h"
#include "LogNamiCamera.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiSpringArmCameraMode)

UNamiSpringArmCameraMode::UNamiSpringArmCameraMode()
{
	// 配置内置 SpringArm 默认值
	SpringArm.SpringArmLength = 350.0f;
	SpringArm.bDoCollisionTest = true;
	SpringArm.bEnableCameraLag = false;
	SpringArm.CameraLagSpeed = 10.0f;
	SpringArm.ProbeSize = 12.0f;
	SpringArm.ProbeChannel = ECC_Camera;

	// 清除父类的 CameraOffset，使用 SpringArm 控制距离
	CameraOffset = FVector::ZeroVector;
}

void UNamiSpringArmCameraMode::Initialize_Implementation(UNamiCameraComponent* InCameraComponent)
{
	Super::Initialize_Implementation(InCameraComponent);

	// 初始化 SpringArm
	SpringArm.Initialize();
	bSpringArmInitialized = true;

	UE_LOG(LogNamiCamera, Log, TEXT("[UNamiSpringArmCameraMode::Initialize] SpringArm 已初始化。"));
}

void UNamiSpringArmCameraMode::Activate_Implementation()
{
	Super::Activate_Implementation();
}

FNamiCameraView UNamiSpringArmCameraMode::CalculateView_Implementation(float DeltaTime)
{
	// 1. 计算观察点（PivotLocation）并应用偏移
	FVector BasePivot = CalculatePivotLocation(DeltaTime);
	FVector TargetPivot = ApplyPivotLocationOffset(BasePivot);

	// 2. 获取控制旋转
	FRotator ControlRotation = GetControlRotation();

	// 3. 创建基础视图
	FNamiCameraView View;
	View.PivotLocation = TargetPivot;
	View.CameraLocation = TargetPivot;  // 初始位置，将由 SpringArm 修改
	View.CameraRotation = ControlRotation;
	View.ControlLocation = TargetPivot;
	View.ControlRotation = ControlRotation;
	View.FOV = DefaultFOV;

	// 4. 应用内置 SpringArm
	if (bSpringArmInitialized)
	{
		// 构建初始 Transform
		FTransform InitialTransform;
		InitialTransform.SetLocation(View.PivotLocation);
		InitialTransform.SetRotation(View.CameraRotation.Quaternion());

		// 收集忽略 Actor
		TArray<AActor*> IgnoreActors;
		for (const FNamiFollowTarget& Target : Targets)
		{
			if (Target.Target.IsValid())
			{
				IgnoreActors.AddUnique(Target.Target.Get());
			}
		}

		// 执行 SpringArm 计算
		SpringArm.Tick(this, DeltaTime, IgnoreActors, InitialTransform, FVector::ZeroVector);

		// 获取结果并更新 View
		const FTransform& CameraTransform = SpringArm.GetCameraTransform();
		View.CameraLocation = CameraTransform.GetLocation();
		View.CameraRotation = CameraTransform.Rotator();
	}

	return View;
}
