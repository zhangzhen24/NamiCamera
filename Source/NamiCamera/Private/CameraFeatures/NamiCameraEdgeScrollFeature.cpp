// Copyright Qiu, Inc. All Rights Reserved.

#include "CameraFeatures/NamiCameraEdgeScrollFeature.h"
#include "CameraModes/NamiTopDownCameraMode.h"
#include "Components/NamiCameraComponent.h"
#include "GameFramework/PlayerController.h"

UNamiCameraEdgeScrollFeature::UNamiCameraEdgeScrollFeature()
{
	EdgeThreshold = 50.0f;
	ScrollSpeed = 500.0f;
	bEnableHorizontalScroll = true;
	bEnableVerticalScroll = true;
}

void UNamiCameraEdgeScrollFeature::Update_Implementation(float DeltaTime)
{
	Super::Update_Implementation(DeltaTime);

	if (!IsEnabled())
	{
		return;
	}

	// 只对TopDownCameraMode生效
	UNamiTopDownCameraMode* TopDownMode = Cast<UNamiTopDownCameraMode>(GetCameraMode());
	if (!TopDownMode)
	{
		return;
	}

	// 检查鼠标是否在边缘
	FVector2D ScrollDirection;
	if (IsMouseAtScreenEdge(ScrollDirection))
	{
		// 转换屏幕方向到世界空间
		const FRotator CameraRot = GetCameraMode()->GetCurrentView().CameraRotation;
		const FRotator YawRotation(0.0f, CameraRot.Yaw, 0.0f);
		const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// 计算世界空间的平移方向
		const FVector WorldDirection = (Forward * ScrollDirection.Y + Right * ScrollDirection.X).GetSafeNormal();
		const FVector PanDelta = WorldDirection * ScrollSpeed * DeltaTime;

		// 应用到TopDownMode
		TopDownMode->AddPanOffset(PanDelta);
	}
}

bool UNamiCameraEdgeScrollFeature::IsMouseAtScreenEdge(FVector2D& OutDirection) const
{
	APlayerController* PC = GetCameraMode()->GetCameraComponent()->GetOwnerPlayerController();
	if (!PC)
	{
		return false;
	}

	// 获取视口大小
	int32 ViewportX, ViewportY;
	PC->GetViewportSize(ViewportX, ViewportY);

	if (ViewportX <= 0 || ViewportY <= 0)
	{
		return false;
	}

	// 获取鼠标位置
	float MouseX, MouseY;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	OutDirection = FVector2D::ZeroVector;

	// 检查水平边缘
	if (bEnableHorizontalScroll)
	{
		if (MouseX < EdgeThreshold)
		{
			// 鼠标在左边缘，向左滚动
			OutDirection.X = -1.0f;
		}
		else if (MouseX > ViewportX - EdgeThreshold)
		{
			// 鼠标在右边缘，向右滚动
			OutDirection.X = 1.0f;
		}
	}

	// 检查垂直边缘
	if (bEnableVerticalScroll)
	{
		if (MouseY < EdgeThreshold)
		{
			// 鼠标在上边缘，向前滚动
			OutDirection.Y = 1.0f;
		}
		else if (MouseY > ViewportY - EdgeThreshold)
		{
			// 鼠标在下边缘，向后滚动
			OutDirection.Y = -1.0f;
		}
	}

	return !OutDirection.IsNearlyZero();
}

FVector2D UNamiCameraEdgeScrollFeature::GetEdgeScrollDirection() const
{
	FVector2D Direction;
	IsMouseAtScreenEdge(Direction);
	return Direction;
}
