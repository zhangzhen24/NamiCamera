// Copyright Qiu, Inc. All Rights Reserved.

#include "CameraFeatures/NamiCameraMouseDragPanFeature.h"
#include "CameraModes/NamiTopDownCameraMode.h"
#include "Components/NamiCameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"

UNamiCameraMouseDragPanFeature::UNamiCameraMouseDragPanFeature()
{
	DragSensitivity = 1.0f;
	bInvertDragDirection = false;
	bIsDragging = false;
	LastMousePosition = FVector2D::ZeroVector;
}

void UNamiCameraMouseDragPanFeature::Activate_Implementation()
{
	Super::Activate_Implementation();
	BindInputActions();
}

void UNamiCameraMouseDragPanFeature::Deactivate_Implementation()
{
	UnbindInputActions();
	bIsDragging = false;
	Super::Deactivate_Implementation();
}

void UNamiCameraMouseDragPanFeature::BindInputActions()
{
	UNamiCameraComponent* CameraComp = GetCameraMode()->GetCameraComponent();
	APlayerController* PC = CameraComp->GetOwnerPlayerController();
	if (!PC)
	{
		return;
	}

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PC->InputComponent);
	if (!Input || !DragAction)
	{
		return;
	}

	// 绑定拖拽开始事件（按下中键）
	Input->BindAction(DragAction, ETriggerEvent::Started, this,
		&UNamiCameraMouseDragPanFeature::OnDragStarted);

	// 绑定拖拽完成事件（松开中键）
	Input->BindAction(DragAction, ETriggerEvent::Completed, this,
		&UNamiCameraMouseDragPanFeature::OnDragCompleted);
}

void UNamiCameraMouseDragPanFeature::UnbindInputActions()
{
	// Enhanced Input System没有直接的Unbind方法
	// 输入绑定会在InputComponent销毁时自动清理
	// 这里清空拖拽状态
	bIsDragging = false;
}

void UNamiCameraMouseDragPanFeature::OnDragStarted(const FInputActionValue& Value)
{
	bIsDragging = true;

	// 记录开始拖拽时的鼠标位置
	APlayerController* PC = GetCameraMode()->GetCameraComponent()->GetOwnerPlayerController();
	if (PC)
	{
		PC->GetMousePosition(LastMousePosition.X, LastMousePosition.Y);
	}
}

void UNamiCameraMouseDragPanFeature::OnDragCompleted(const FInputActionValue& Value)
{
	bIsDragging = false;
}

void UNamiCameraMouseDragPanFeature::Update_Implementation(float DeltaTime)
{
	Super::Update_Implementation(DeltaTime);

	if (!IsEnabled() || !bIsDragging)
	{
		return;
	}

	// 只对TopDownCameraMode生效
	UNamiTopDownCameraMode* TopDownMode = Cast<UNamiTopDownCameraMode>(GetCameraMode());
	if (!TopDownMode)
	{
		return;
	}

	APlayerController* PC = GetCameraMode()->GetCameraComponent()->GetOwnerPlayerController();
	if (!PC)
	{
		return;
	}

	// 获取当前鼠标位置
	float CurrentX, CurrentY;
	if (!PC->GetMousePosition(CurrentX, CurrentY))
	{
		return;
	}

	// 计算鼠标移动增量
	FVector2D CurrentPos(CurrentX, CurrentY);
	FVector2D DragDelta = CurrentPos - LastMousePosition;

	// 反转方向（可选）
	if (bInvertDragDirection)
	{
		DragDelta *= -1.0f;
	}

	// 如果有移动，应用平移
	if (!DragDelta.IsNearlyZero())
	{
		// 转换屏幕空间增量到世界空间平移
		const FVector PanDelta = ConvertScreenDeltaToWorldPan(DragDelta);
		TopDownMode->AddPanOffset(PanDelta);
	}

	// 更新上一帧鼠标位置
	LastMousePosition = CurrentPos;
}

FVector UNamiCameraMouseDragPanFeature::ConvertScreenDeltaToWorldPan(const FVector2D& ScreenDelta) const
{
	// 根据相机高度缩放灵敏度
	// 相机越高，同样的屏幕移动应该对应更大的世界空间移动
	const float CameraHeight = FMath::Max(100.0f, GetCameraMode()->GetCurrentView().CameraLocation.Z);
	const float ScaleFactor = DragSensitivity * CameraHeight / 1000.0f;

	// 转换到相机坐标系（只考虑Yaw）
	const FRotator CameraRot = GetCameraMode()->GetCurrentView().CameraRotation;
	const FRotator YawRotation(0.0f, CameraRot.Yaw, 0.0f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// 屏幕X移动 -> 世界Right方向
	// 屏幕Y移动 -> 世界Forward方向（反向，因为屏幕Y向下是正）
	return (Right * ScreenDelta.X - Forward * ScreenDelta.Y) * ScaleFactor;
}
