// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modes/Presets/NamiThirdPersonCamera.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/NamiCameraComponent.h"
#include "Libraries/NamiCameraMath.h"
#include "Features/NamiSpringArmFeature.h"
#include "LogNamiCamera.h"

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
	CameraLocationSmoothIntensity = 0.0f;
	CameraRotationSmoothIntensity = 0.0f;

	// 初始化旋转
	LastCameraRotation = FRotator::ZeroRotator;
	LastValidControlRotation = FRotator::ZeroRotator;
}

void UNamiThirdPersonCamera::Initialize_Implementation(UNamiCameraComponent *InCameraComponent)
{
	Super::Initialize_Implementation(InCameraComponent);

	// 确保有 SpringArmFeature
	UNamiSpringArmFeature* Feature = GetFeature<UNamiSpringArmFeature>();
	if (!Feature)
	{
		// 自动创建 SpringArmFeature
		Feature = CreateAndAddFeature<UNamiSpringArmFeature>();
		if (Feature)
		{
			UE_LOG(LogNamiCamera, Log, TEXT("[UNamiThirdPersonCamera::Initialize] 自动创建 SpringArmFeature。"));
		}
		else
		{
			UE_LOG(LogNamiCamera, Error, TEXT("[UNamiThirdPersonCamera::Initialize] 创建 SpringArmFeature 失败！"));
			return;
		}
	}
	
	// 同步快捷配置到 SpringArmFeature
	SyncQuickConfigToFeature();
}

void UNamiThirdPersonCamera::SyncQuickConfigToFeature()
{
	UNamiSpringArmFeature* Feature = GetSpringArmFeature();
	if (!Feature)
	{
		return;
	}
	
	// 同步相机距离
	Feature->SpringArm.TargetArmLength = CameraDistance;
	
	// 同步碰撞检测
	Feature->SpringArm.bDoCollisionTest = bEnableCollision;
	
	// 同步相机滞后
	Feature->SpringArm.bEnableCameraLag = bEnableCameraLag;
	Feature->SpringArm.CameraLagSpeed = CameraLagSpeed;
	
	// 设置其他默认值（如果还没设置）
	if (Feature->SpringArm.ProbeSize == 0.0f)
	{
		Feature->SpringArm.ProbeSize = 12.0f;
	}
	if (Feature->SpringArm.ProbeChannel == ECC_WorldStatic)
	{
		Feature->SpringArm.ProbeChannel = ECC_Camera;
	}
}

void UNamiThirdPersonCamera::Activate_Implementation()
{
	Super::Activate_Implementation();
	
	// SpringArm 的激活由 Feature 系统自动处理
}

FNamiCameraView UNamiThirdPersonCamera::CalculateView_Implementation(float DeltaTime)
{
	// 1. 计算观察点（PivotLocation）
	FVector TargetPivot = CalculatePivotLocation();
	TargetPivot.Z += PivotHeightOffset;

	// 2. 获取控制旋转（相机旋转）
	FRotator ControlRotation = GetControlRotation();

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

	// 使用0-360度归一化，避免180/-180跳变问题
	ControlRotation = FNamiCameraMath::NormalizeRotatorTo360(ControlRotation);

	// 3. 创建基础视图
	FNamiCameraView View;
	View.PivotLocation = TargetPivot;
	View.CameraLocation = TargetPivot;  // 初始位置，将由 SpringArmFeature 修改
	View.CameraRotation = ControlRotation;
	View.ControlLocation = TargetPivot;
	View.ControlRotation = ControlRotation;
	View.FOV = DefaultFOV;

	// 4. 应用相机距离缩放因子到 SpringArmFeature
	UNamiSpringArmFeature* SpringArmFeature = GetSpringArmFeature();
	if (SpringArmFeature)
	{
		float OriginalArmLength = SpringArmFeature->SpringArm.TargetArmLength;
		SpringArmFeature->SpringArm.TargetArmLength = OriginalArmLength * CameraDistanceScale;
	}

	// 5. 调用父类实现，让 Feature 系统处理（包括 SpringArmFeature）
	// Feature 系统会自动应用 SpringArm、VelocityPrediction 等功能
	View = Super::CalculateView_Implementation(DeltaTime);

	// 6. 恢复原始 ArmLength（避免影响下一帧）
	if (SpringArmFeature)
	{
		SpringArmFeature->SpringArm.TargetArmLength = SpringArmFeature->SpringArm.TargetArmLength / CameraDistanceScale;
	}

	// 7. 重新应用旋转限制（因为 Feature 可能修改了旋转）
	FRotator FinalRotation = View.CameraRotation;
	if (bLimitPitch)
	{
		FinalRotation.Pitch = FMath::ClampAngle(FinalRotation.Pitch, MinPitch, MaxPitch);
	}
	if (bLimitYaw)
	{
		FinalRotation.Yaw = FMath::ClampAngle(FinalRotation.Yaw, MinYaw, MaxYaw);
	}
	if (bLockRoll)
	{
		FinalRotation.Roll = 0.0f;
	}
	// 使用0-360度归一化，避免180/-180跳变问题
	FinalRotation = FNamiCameraMath::NormalizeRotatorTo360(FinalRotation);
	View.CameraRotation = FinalRotation;
	View.ControlRotation = FinalRotation;

	LastCameraRotation = View.CameraRotation;

	return View;
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

	// 使用0-360度归一化，避免180/-180跳变问题
	return FNamiCameraMath::NormalizeRotatorTo360(ControlRotation);
}

UNamiSpringArmFeature* UNamiThirdPersonCamera::GetSpringArmFeature() const
{
	return GetFeature<UNamiSpringArmFeature>();
}

void UNamiThirdPersonCamera::SetCameraDistance(float NewDistance)
{
	CameraDistance = FMath::Clamp(NewDistance, 50.0f, 2000.0f);
	
	// 同步到 SpringArmFeature
	UNamiSpringArmFeature* Feature = GetSpringArmFeature();
	if (Feature)
	{
		Feature->SpringArm.TargetArmLength = CameraDistance;
	}
}

void UNamiThirdPersonCamera::SetCollisionEnabled(bool bEnabled)
{
	bEnableCollision = bEnabled;
	
	// 同步到 SpringArmFeature
	UNamiSpringArmFeature* Feature = GetSpringArmFeature();
	if (Feature)
	{
		Feature->SpringArm.bDoCollisionTest = bEnabled;
	}
}

void UNamiThirdPersonCamera::SetCameraLag(bool bEnabled, float LagSpeed)
{
	bEnableCameraLag = bEnabled;
	CameraLagSpeed = FMath::Clamp(LagSpeed, 0.0f, 100.0f);
	
	// 同步到 SpringArmFeature
	UNamiSpringArmFeature* Feature = GetSpringArmFeature();
	if (Feature)
	{
		Feature->SpringArm.bEnableCameraLag = bEnabled;
		Feature->SpringArm.CameraLagSpeed = CameraLagSpeed;
	}
}

void UNamiThirdPersonCamera::ApplyCameraPreset(ENamiThirdPersonCameraPreset Preset)
{
	CameraPreset = Preset;
	
	switch (Preset)
	{
	case ENamiThirdPersonCameraPreset::CloseCombat:
		// 近距离战斗：快速响应，紧凑视角
		SetCameraDistance(200.0f);
		SetCollisionEnabled(true);
		SetCameraLag(false, 15.0f);
		MinPitch = -70.0f;
		MaxPitch = 40.0f;
		MouseSensitivity = 1.2f;
		UE_LOG(LogNamiCamera, Log, TEXT("[UNamiThirdPersonCamera] 应用预设：近距离战斗"));
		break;
		
	case ENamiThirdPersonCameraPreset::Standard:
		// 标准第三人称：平衡的设置
		SetCameraDistance(350.0f);
		SetCollisionEnabled(true);
		SetCameraLag(false, 10.0f);
		MinPitch = -60.0f;
		MaxPitch = 30.0f;
		MouseSensitivity = 1.0f;
		UE_LOG(LogNamiCamera, Log, TEXT("[UNamiThirdPersonCamera] 应用预设：标准第三人称"));
		break;
		
	case ENamiThirdPersonCameraPreset::Exploration:
		// 远距离探索：广阔视野，平滑移动
		SetCameraDistance(650.0f);
		SetCollisionEnabled(true);
		SetCameraLag(true, 8.0f);
		MinPitch = -50.0f;
		MaxPitch = 20.0f;
		MouseSensitivity = 0.9f;
		UE_LOG(LogNamiCamera, Log, TEXT("[UNamiThirdPersonCamera] 应用预设：远距离探索"));
		break;
		
	case ENamiThirdPersonCameraPreset::Cinematic:
		// 电影感：平滑滞后，电影般的跟随
		SetCameraDistance(450.0f);
		SetCollisionEnabled(true);
		SetCameraLag(true, 5.0f);
		MinPitch = -55.0f;
		MaxPitch = 25.0f;
		MouseSensitivity = 0.8f;
		// 启用父类的平滑
		bEnableSmoothing = true;
		CameraLocationSmoothIntensity = 0.4f;
		CameraRotationSmoothIntensity = 0.2f;
		UE_LOG(LogNamiCamera, Log, TEXT("[UNamiThirdPersonCamera] 应用预设：电影感"));
		break;
		
	case ENamiThirdPersonCameraPreset::Competitive:
		// 竞技：精确控制，快速响应
		SetCameraDistance(250.0f);
		SetCollisionEnabled(true);
		SetCameraLag(false, 20.0f);
		MinPitch = -65.0f;
		MaxPitch = 35.0f;
		MouseSensitivity = 1.3f;
		// 禁用平滑以获得最快响应
		bEnableSmoothing = false;
		UE_LOG(LogNamiCamera, Log, TEXT("[UNamiThirdPersonCamera] 应用预设：竞技"));
		break;
		
	case ENamiThirdPersonCameraPreset::Custom:
	default:
		// 自定义：不修改任何参数
		UE_LOG(LogNamiCamera, Log, TEXT("[UNamiThirdPersonCamera] 切换到自定义模式"));
		break;
	}
	
	// 同步所有配置到 Feature
	SyncQuickConfigToFeature();
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
			FRotator ControlRot = OwnerPawn->GetControlRotation();
			// 使用0-360度归一化，避免180/-180跳变问题
			ControlRot = FNamiCameraMath::NormalizeRotatorTo360(ControlRot);
			LastValidControlRotation = ControlRot; // 缓存归一化后的旋转
			return ControlRot;
		}

		// 尝试从 PlayerController 获取
		APlayerController *PC = CameraComp->GetOwnerPlayerController();
		if (IsValid(PC))
		{
			FRotator ControlRot = PC->GetControlRotation();
			// 使用0-360度归一化，避免180/-180跳变问题
			ControlRot = FNamiCameraMath::NormalizeRotatorTo360(ControlRot);
			LastValidControlRotation = ControlRot; // 缓存归一化后的旋转
			return ControlRot;
		}
	}

	// 后备：返回上一次有效的旋转，避免相机突然跳转
	// 如果从未获取过有效旋转，则返回零旋转
	// 确保缓存的旋转也是归一化的
	return FNamiCameraMath::NormalizeRotatorTo360(LastValidControlRotation);
}
