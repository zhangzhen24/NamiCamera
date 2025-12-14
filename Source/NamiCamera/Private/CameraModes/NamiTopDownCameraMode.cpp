// Copyright Qiu, Inc. All Rights Reserved.

#include "CameraModes/NamiTopDownCameraMode.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UNamiTopDownCameraMode::UNamiTopDownCameraMode()
{
	// 参考 EnhancedCameraSystem 的 TopCameraMode 默认值
	CameraPitch = -55.0f;
	CameraYaw = 0.0f;
	CameraRoll = 0.0f;

	// 锁定 Pitch 和 Roll
	bIgnorePitch = true;
	bIgnoreRoll = true;

	// 鼠标追踪默认关闭
	bEnableMouseTracking = false;
	MouseTrackingStrength = 0.3f;
	MaxMouseTrackingOffset = 500.0f;

	// 弹簧臂配置
	SpringArm.SpringArmLength = 950.0f;
	SpringArm.bDoCollisionTest = false; // 禁用碰撞检测（俯视角通常不需要）
	SpringArm.bInheritPitch = false;     // 不继承 Pitch
	SpringArm.bInheritRoll = false;      // 不继承 Roll
	SpringArm.bInheritYaw = true;        // 允许继承 Yaw（可旋转）

	// 平滑设置
	BlendConfig.BlendTime = 0.5f;
	BlendConfig.BlendType = ENamiCameraBlendType::EaseInOut;
}

FNamiCameraView UNamiTopDownCameraMode::CalculateView_Implementation(float DeltaTime)
{
	// 首先调用父类逻辑（处理弹簧臂、跟随等）
	FNamiCameraView View = Super::CalculateView_Implementation(DeltaTime);

	// 如果启用鼠标追踪，调整 Pivot
	if (bEnableMouseTracking)
	{
		View.PivotLocation = TraceMousePosition(View.PivotLocation);

		// 需要重新计算 SpringArm，因为 Pivot 位置改变了
		FTransform InitialTransform;
		InitialTransform.SetLocation(View.PivotLocation);
		InitialTransform.SetRotation(FRotator(CameraPitch, CameraYaw, CameraRoll).Quaternion());

		// 收集忽略 Actor
		TArray<AActor*> IgnoreActors;
		for (const FNamiFollowTarget& Target : Targets)
		{
			if (Target.Target.IsValid())
			{
				IgnoreActors.AddUnique(Target.Target.Get());
			}
		}

		// 重新执行 SpringArm 计算
		SpringArm.Tick(this, DeltaTime, IgnoreActors, InitialTransform, FVector::ZeroVector);
		const FTransform& CameraTransform = SpringArm.GetCameraTransform();
		View.CameraLocation = CameraTransform.GetLocation();
		View.CameraRotation = CameraTransform.Rotator();
	}

	// 构建固定的旋转
	FRotator DesiredRotation = FRotator(CameraPitch, CameraYaw, CameraRoll);

	// 如果允许部分旋转控制
	if (!bIgnorePitch)
	{
		// 可以从 View.ControlRotation 获取玩家输入的 Pitch
		DesiredRotation.Pitch = View.ControlRotation.Pitch;
	}

	if (!bIgnoreRoll)
	{
		// 可以从 View.ControlRotation 获取玩家输入的 Roll
		DesiredRotation.Roll = View.ControlRotation.Roll;
	}

	// 如果 Yaw 也使用控制旋转（可选）
	if (!bIgnorePitch && !bIgnoreRoll)
	{
		// 只有当两个都不忽略时，才使用控制旋转的 Yaw
		DesiredRotation.Yaw = View.ControlRotation.Yaw;
	}

	// 更新相机旋转为固定角度
	View.CameraRotation = DesiredRotation;

	return View;
}

FVector UNamiTopDownCameraMode::TraceMousePosition(const FVector& BasePivot) const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return BasePivot;
	}

	// 获取鼠标屏幕位置
	float MouseX, MouseY;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return BasePivot;
	}

	// 从屏幕位置转换为世界射线
	FVector WorldLocation, WorldDirection;
	if (!PC->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
	{
		return BasePivot;
	}

	// 射线检测地面
	FHitResult HitResult;
	FVector TraceStart = WorldLocation;
	FVector TraceEnd = WorldLocation + WorldDirection * 10000.0f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PC->GetPawn());

	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		// 计算偏移
		FVector Offset = HitResult.Location - BasePivot;
		Offset.Z = 0.0f; // 只在水平面偏移

		// 限制偏移距离
		if (Offset.Size() > MaxMouseTrackingOffset)
		{
			Offset = Offset.GetSafeNormal() * MaxMouseTrackingOffset;
		}

		// 应用强度系数
		Offset *= MouseTrackingStrength;

		return BasePivot + Offset;
	}

	return BasePivot;
}
