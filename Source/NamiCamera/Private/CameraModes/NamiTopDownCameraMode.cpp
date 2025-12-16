// Copyright Qiu, Inc. All Rights Reserved.

#include "CameraModes/NamiTopDownCameraMode.h"
#include "CameraFeatures/NamiCameraEdgeScrollFeature.h"
#include "CameraFeatures/NamiCameraKeyboardPanFeature.h"
#include "CameraFeatures/NamiCameraMouseDragPanFeature.h"
#include "Components/NamiCameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UNamiTopDownCameraMode::UNamiTopDownCameraMode()
{
	// 俯视角配置
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

	// 相机距离配置（替代 SpringArm）
	CameraDistance = 950.0f;

	// 平滑设置
	BlendConfig.BlendTime = 0.5f;
	BlendConfig.BlendType = ENamiCameraBlendType::EaseInOut;

	// 缩放配置（ARPG风格）
	bEnableMouseWheelZoom = true;
	MinZoomDistance = 400.0f;
	MaxZoomDistance = 2000.0f;
	ZoomSpeed = 150.0f;
	ZoomSmoothSpeed = 10.0f;
	CurrentZoomDistance = CameraDistance;
	TargetZoomDistance = CameraDistance;

	// 平移配置（ARPG风格）
	PanOffset = FVector::ZeroVector;
	MaxPanDistance = 1000.0f;
	PanReturnSpeed = 5.0f;
	bAutoReturnToCharacter = true;

	// 自动设置配置
	bAutoSetOwnerAsPrimaryTarget = true;

	// 清除父类的 CameraOffset（我们使用 CameraDistance 和固定角度）
	CameraOffset = FVector::ZeroVector;
}

void UNamiTopDownCameraMode::Activate_Implementation()
{
	Super::Activate_Implementation();

	// 初始化缩放距离
	CurrentZoomDistance = CameraDistance;
	TargetZoomDistance = CameraDistance;

	// 如果启用自动设置目标，将相机所属的Pawn设为PrimaryTarget
	if (bAutoSetOwnerAsPrimaryTarget)
	{
		if (UNamiCameraComponent* CameraComp = GetCameraComponent())
		{
			if (APawn* OwnerPawn = CameraComp->GetOwnerPawn())
			{
				// 只在还没有目标的情况下自动设置
				if (GetPrimaryTarget() == nullptr)
				{
					SetPrimaryTarget(OwnerPawn);
					UE_LOG(LogTemp, Log, TEXT("TopDownCameraMode: Auto-set primary target to %s"), *OwnerPawn->GetName());
				}
			}
		}
	}
}

FNamiCameraView UNamiTopDownCameraMode::CalculateView_Implementation(float DeltaTime)
{
	// 1. 计算 Pivot 位置（目标点）
	FVector BasePivot = CalculatePivotLocation(DeltaTime);
	FVector TargetPivot = ApplyPivotLocationOffset(BasePivot);

	// 2. 应用 PanOffset
	if (!PanOffset.IsNearlyZero())
	{
		FVector ModifiedPivot = TargetPivot + PanOffset;

		// 限制最大偏移距离
		const FVector OffsetDelta = ModifiedPivot - TargetPivot;
		if (OffsetDelta.Size() > MaxPanDistance)
		{
			ModifiedPivot = TargetPivot + OffsetDelta.GetSafeNormal() * MaxPanDistance;
			// 更新 PanOffset 以反映限制后的值
			PanOffset = ModifiedPivot - TargetPivot;
		}

		TargetPivot = ModifiedPivot;
	}

	// 3. 应用鼠标追踪
	if (bEnableMouseTracking)
	{
		TargetPivot = TraceMousePosition(TargetPivot);
	}

	// 4. 构建固定的旋转角度
	FRotator DesiredRotation = FRotator(CameraPitch, CameraYaw, CameraRoll);

	// 5. 根据旋转和距离计算相机位置
	// 相机位置 = Pivot - 旋转方向 * 距离
	FVector CameraDirection = DesiredRotation.Vector();
	FVector CameraLocation = TargetPivot - CameraDirection * CurrentZoomDistance;

	// 6. 构建视图
	FNamiCameraView View;
	View.PivotLocation = TargetPivot;
	View.CameraLocation = CameraLocation;
	View.CameraRotation = DesiredRotation;
	View.ControlLocation = TargetPivot;
	View.ControlRotation = DesiredRotation;
	View.FOV = DefaultFOV;

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

void UNamiTopDownCameraMode::Tick_Implementation(float DeltaTime)
{
	Super::Tick_Implementation(DeltaTime);

	// 更新缩放
	UpdateZoom(DeltaTime);

	// 更新平移偏移
	UpdatePanOffset(DeltaTime);
}

void UNamiTopDownCameraMode::UpdateZoom(float DeltaTime)
{
	if (!bEnableMouseWheelZoom)
	{
		return;
	}

	// 平滑插值到目标距离
	if (FMath::Abs(CurrentZoomDistance - TargetZoomDistance) > 0.1f)
	{
		CurrentZoomDistance = FMath::FInterpTo(
			CurrentZoomDistance,
			TargetZoomDistance,
			DeltaTime,
			ZoomSmoothSpeed
		);
	}
}

void UNamiTopDownCameraMode::ZoomIn(float Amount)
{
	if (!bEnableMouseWheelZoom)
	{
		return;
	}

	// 向内缩放（减小距离）
	TargetZoomDistance = FMath::Clamp(
		TargetZoomDistance - Amount * ZoomSpeed,
		MinZoomDistance,
		MaxZoomDistance
	);
}

void UNamiTopDownCameraMode::ZoomOut(float Amount)
{
	if (!bEnableMouseWheelZoom)
	{
		return;
	}

	// 向外缩放（增加距离）
	TargetZoomDistance = FMath::Clamp(
		TargetZoomDistance + Amount * ZoomSpeed,
		MinZoomDistance,
		MaxZoomDistance
	);
}

void UNamiTopDownCameraMode::SetTargetZoomDistance(float Distance)
{
	if (!bEnableMouseWheelZoom)
	{
		return;
	}

	// 直接设置目标距离（带范围限制）
	TargetZoomDistance = FMath::Clamp(
		Distance,
		MinZoomDistance,
		MaxZoomDistance
	);
}

void UNamiTopDownCameraMode::UpdatePanOffset(float DeltaTime)
{
	// 不在平移时自动返回角色中心
	if (bAutoReturnToCharacter && !IsBeingPanned() && PanOffset.Size() > 1.0f)
	{
		PanOffset = FMath::VInterpTo(
			PanOffset,
			FVector::ZeroVector,
			DeltaTime,
			PanReturnSpeed
		);
	}
}

void UNamiTopDownCameraMode::AddPanOffset(const FVector& Offset)
{
	PanOffset += Offset;

	// 立即限制在最大距离内
	if (PanOffset.Size() > MaxPanDistance)
	{
		PanOffset = PanOffset.GetSafeNormal() * MaxPanDistance;
	}
}

void UNamiTopDownCameraMode::ResetPanOffset()
{
	PanOffset = FVector::ZeroVector;
}

bool UNamiTopDownCameraMode::IsBeingPanned() const
{
	// 检查所有平移Feature是否活动
	const TArray<UNamiCameraFeature*>& CameraFeatures = GetFeatures();

	for (const UNamiCameraFeature* Feature : CameraFeatures)
	{
		if (!Feature || !Feature->IsEnabled())
		{
			continue;
		}

		// 检查边缘滚动
		if (const UNamiCameraEdgeScrollFeature* EdgeScroll = Cast<UNamiCameraEdgeScrollFeature>(Feature))
		{
			FVector2D Direction;
			if (EdgeScroll->IsMouseAtScreenEdge(Direction))
			{
				return true;
			}
		}

		// 检查键盘平移
		if (const UNamiCameraKeyboardPanFeature* KeyboardPan = Cast<UNamiCameraKeyboardPanFeature>(Feature))
		{
			if (KeyboardPan->HasActivePanInput())
			{
				return true;
			}
		}

		// 检查鼠标拖拽
		if (const UNamiCameraMouseDragPanFeature* MouseDrag = Cast<UNamiCameraMouseDragPanFeature>(Feature))
		{
			if (MouseDrag->IsDragging())
			{
				return true;
			}
		}
	}

	return false;
}
