// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modes/Presets/NamiThirdPersonCamera.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/NamiCameraComponent.h"
#include "Libraries/NamiCameraMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiThirdPersonCamera)

UNamiThirdPersonCamera::UNamiThirdPersonCamera()
{
	// 标准第三人称相机默认配置
	DefaultFOV = 90.0f;

	// 使用目标旋转（跟随角色Yaw）
	bUseTargetRotation = true;
	bUseYawOnly = false; // 允许玩家控制Pitch

	// 清除父类的 CameraOffset，使用自己的弹簧臂系统
	CameraOffset = FVector::ZeroVector;

	// 设置父类的 PivotHeightOffset（观察点高度偏移）
	PivotHeightOffset = 80.0f;

	// 平滑强度配置（默认关闭平滑）
	PivotSmoothIntensity = 0.0f;
	CameraLocationSmoothIntensity = 0.0f;
	CameraRotationSmoothIntensity = 0.0f;

	// 初始化 SpringArm 配置（标准第三人称相机默认值）
	SpringArm.TargetArmLength = 350.0f;
	SpringArm.bDoCollisionTest = true;
	SpringArm.ProbeSize = 12.0f;
	SpringArm.ProbeChannel = ECC_Camera;

	// 初始化旋转
	LastCameraRotation = FRotator::ZeroRotator;
}

void UNamiThirdPersonCamera::Initialize_Implementation(UNamiCameraComponent *InCameraComponent)
{
	Super::Initialize_Implementation(InCameraComponent);

	// 初始化 SpringArm
	SpringArm.Initialize();
}

void UNamiThirdPersonCamera::Activate_Implementation()
{
	Super::Activate_Implementation();

	// 重新初始化 SpringArm（激活时重置状态）
	SpringArm.Initialize();
}

FNamiCameraView UNamiThirdPersonCamera::CalculateView_Implementation(float DeltaTime)
{
	// 1. 计算观察点（PivotLocation）
	FVector TargetPivot = CalculatePivotLocation();
	TargetPivot.Z += PivotHeightOffset;

	// 2. 获取控制旋转（相机旋转）
	FRotator ControlRotation = GetControlRotation();

	// 注意：鼠标灵敏度不应该应用在累积的控制旋转值上
	// 灵敏度应该在输入阶段应用，否则会导致旋转值不断放大
	// 移除这行代码，避免相机疯狂乱晃
	// ControlRotation.Pitch *= MouseSensitivity;
	// ControlRotation.Yaw *= MouseSensitivity;

	// 应用Pitch限制
	if (bLimitPitch)
	{
		// 使用FMath::ClampAngle而不是FMath::Clamp，因为ClampAngle会自动处理角度归一化
		// 这可以避免UE坐标系中pitch瞬间变值导致的问题
		ControlRotation.Pitch = FMath::ClampAngle(ControlRotation.Pitch, MinPitch, MaxPitch);
	}

	// 应用Yaw限制
	if (bLimitYaw)
	{
		ControlRotation.Yaw = FMath::ClampAngle(ControlRotation.Yaw, MinYaw, MaxYaw);
	}

	// 锁定Roll旋转
	if (bLockRoll)
	{
		ControlRotation.Roll = 0.0f;
	}

	// 注意：在第三人称相机中，我们直接使用玩家的输入旋转
	// 不需要使用目标旋转来覆盖玩家的输入
	// bUseTargetRotation和bUseYawOnly属性在CalculateCameraLocation_Implementation中使用
	// 但我们重写了CalculateView_Implementation，所以这些属性在这里不适用

	// 3. 准备 SpringArm 的输入
	// 应用相机距离缩放因子
	float ScaledArmLength = SpringArm.TargetArmLength * CameraDistanceScale;
	SpringArm.TargetArmLength = ScaledArmLength;

	FTransform InitialTransform(ControlRotation, TargetPivot);
	FVector OffsetLocation = FVector::ZeroVector;

	// 4. 准备忽略的Actor列表
	TArray<AActor *> IgnoreActors;
	AActor *PrimaryTarget = GetPrimaryTarget();
	if (IsValid(PrimaryTarget))
	{
		IgnoreActors.Add(PrimaryTarget);
		// 忽略附加的Actor
		PrimaryTarget->ForEachAttachedActors([&IgnoreActors](AActor *Actor)
											 { IgnoreActors.AddUnique(Actor); return true; });
	}

	// 5. 调用 SpringArm.Tick 计算相机位置
	UWorld *World = GetWorld();
	if (IsValid(World))
	{
		SpringArm.Tick(World, DeltaTime, IgnoreActors, InitialTransform, OffsetLocation);

		// 6. 从 SpringArm 获取最终相机Transform
		FTransform CameraTransform = SpringArm.GetCameraTransform();
		FRotator SpringArmRotation = CameraTransform.Rotator();

		// 7. 创建 View
		FNamiCameraView View;
		View.PivotLocation = TargetPivot;
		View.CameraLocation = CameraTransform.GetLocation();
		// 使用SpringArm返回的旋转，而不是直接使用ControlRotation
		// 这样可以确保相机旋转考虑了碰撞检测结果
		View.CameraRotation = SpringArmRotation;
		View.ControlLocation = CameraTransform.GetLocation();
		View.ControlRotation = SpringArmRotation;
		View.FOV = DefaultFOV;

		// 8. 应用平滑（使用父类的平滑逻辑）
		if (!bInitialized)
		{
			CurrentPivotLocation = View.PivotLocation;
			CurrentCameraLocation = View.CameraLocation;
			CurrentCameraRotation = View.CameraRotation;
			bInitialized = true;
		}
		else if (DeltaTime > 0.0f)
		{
			// 平滑枢轴点
			if (bEnableSmoothing && bEnablePivotSmoothing && PivotSmoothIntensity > 0.0f)
			{
				// 将平滑强度映射到实际平滑时间
				float SmoothTime = FNamiCameraMath::MapSmoothIntensity(PivotSmoothIntensity);
				CurrentPivotLocation = FNamiCameraMath::SmoothDamp(
					CurrentPivotLocation,
					View.PivotLocation,
					PivotVelocity,
					SmoothTime,
					DeltaTime);
			}
			else
			{
				CurrentPivotLocation = View.PivotLocation;
			}

			// 平滑相机位置
			if (bEnableSmoothing && bEnableCameraLocationSmoothing && CameraLocationSmoothIntensity > 0.0f)
			{
				// 将平滑强度映射到实际平滑时间
				float SmoothTime = FNamiCameraMath::MapSmoothIntensity(CameraLocationSmoothIntensity);
				CurrentCameraLocation = FNamiCameraMath::SmoothDamp(
					CurrentCameraLocation,
					View.CameraLocation,
					CameraVelocity,
					SmoothTime,
					DeltaTime);
			}
			else
			{
				CurrentCameraLocation = View.CameraLocation;
			}

			// 平滑旋转
			if (bEnableSmoothing && bEnableCameraRotationSmoothing && CameraRotationSmoothIntensity > 0.0f)
			{
				// 将平滑强度映射到实际平滑时间
				float SmoothTime = FNamiCameraMath::MapSmoothIntensity(CameraRotationSmoothIntensity);
				CurrentCameraRotation = FNamiCameraMath::SmoothDampRotator(
					CurrentCameraRotation,
					View.CameraRotation,
					YawVelocity,
					PitchVelocity,
					SmoothTime,
					SmoothTime,
					DeltaTime);
			}
			else
			{
				CurrentCameraRotation = View.CameraRotation;
			}
		}

		// 9. 使用平滑后的值
		View.PivotLocation = CurrentPivotLocation;
		View.CameraLocation = CurrentCameraLocation;
		View.CameraRotation = CurrentCameraRotation;
		View.ControlLocation = CurrentCameraLocation;
		View.ControlRotation = CurrentCameraRotation;

		LastCameraRotation = View.CameraRotation;

		return View;
	}
	else
	{
		// World 无效时的后备方案：直接使用 PivotLocation 作为相机位置
		FNamiCameraView View;
		View.PivotLocation = TargetPivot;
		View.CameraLocation = TargetPivot;
		View.CameraRotation = ControlRotation;
		View.ControlLocation = TargetPivot;
		View.ControlRotation = ControlRotation;
		View.FOV = DefaultFOV;
		return View;
	}
}

FVector UNamiThirdPersonCamera::CalculateCameraLocation_Implementation(const FVector &InPivotLocation) const
{
	// 这个方法不会被调用，因为我们重写了 CalculateView_Implementation
	// 但为了完整性，我们提供一个实现
	return InPivotLocation;
}

FRotator UNamiThirdPersonCamera::CalculateCameraRotation_Implementation(
	const FVector &InCameraLocation,
	const FVector &InPivotLocation) const
{
	// 获取控制旋转
	FRotator ControlRotation = GetControlRotation();

	// 注意：鼠标灵敏度不应该应用在累积的控制旋转值上
	// 灵敏度应该在输入阶段应用，否则会导致旋转值不断放大
	// 移除这行代码，避免相机疯狂乱晃
	// ControlRotation.Pitch *= MouseSensitivity;
	// ControlRotation.Yaw *= MouseSensitivity;

	// 应用Pitch限制
	if (bLimitPitch)
	{
		ControlRotation.Pitch = FMath::ClampAngle(ControlRotation.Pitch, MinPitch, MaxPitch);
	}

	// 应用Yaw限制
	if (bLimitYaw)
	{
		ControlRotation.Yaw = FMath::ClampAngle(ControlRotation.Yaw, MinYaw, MaxYaw);
	}

	// 锁定Roll旋转
	if (bLockRoll)
	{
		ControlRotation.Roll = 0.0f;
	}

	return ControlRotation;
}

FRotator UNamiThirdPersonCamera::GetControlRotation() const
{
	// 从相机组件的 Owner 获取控制旋转
	UNamiCameraComponent *CameraComp = GetCameraComponent();
	if (CameraComp)
	{
		// 优先从 Owner Pawn 获取
		APawn *OwnerPawn = CameraComp->GetOwnerPawn();
		if (IsValid(OwnerPawn))
		{
			return OwnerPawn->GetControlRotation();
		}

		// 尝试从 PlayerController 获取
		APlayerController *PC = CameraComp->GetOwnerPlayerController();
		if (IsValid(PC))
		{
			return PC->GetControlRotation();
		}
	}

	// 后备：返回默认旋转，避免使用可能导致循环依赖的变量
	return FRotator::ZeroRotator;
}
