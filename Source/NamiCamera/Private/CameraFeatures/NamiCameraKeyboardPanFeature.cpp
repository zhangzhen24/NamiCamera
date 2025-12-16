// Copyright Qiu, Inc. All Rights Reserved.

#include "CameraFeatures/NamiCameraKeyboardPanFeature.h"
#include "CameraModes/NamiTopDownCameraMode.h"
#include "Components/NamiCameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"

UNamiCameraKeyboardPanFeature::UNamiCameraKeyboardPanFeature()
{
	PanSpeed = 800.0f;
	bUseRelativeToCamera = true;
	CurrentPanInput = FVector2D::ZeroVector;
}

void UNamiCameraKeyboardPanFeature::Activate_Implementation()
{
	Super::Activate_Implementation();
	BindInputActions();
}

void UNamiCameraKeyboardPanFeature::Deactivate_Implementation()
{
	UnbindInputActions();
	CurrentPanInput = FVector2D::ZeroVector;
	Super::Deactivate_Implementation();
}

void UNamiCameraKeyboardPanFeature::BindInputActions()
{
	UNamiCameraComponent* CameraComp = GetCameraMode()->GetCameraComponent();
	APlayerController* PC = CameraComp->GetOwnerPlayerController();
	if (!PC)
	{
		return;
	}

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PC->InputComponent);
	if (!Input || !Pan2DAction)
	{
		return;
	}

	// 绑定2D平移输入
	Input->BindAction(Pan2DAction, ETriggerEvent::Triggered, this,
		&UNamiCameraKeyboardPanFeature::OnPan2DInput);

	// 绑定Completed事件以清除输入
	Input->BindAction(Pan2DAction, ETriggerEvent::Completed, this,
		&UNamiCameraKeyboardPanFeature::OnPan2DCompleted);
}

void UNamiCameraKeyboardPanFeature::UnbindInputActions()
{
	// Enhanced Input System没有直接的Unbind方法
	// 输入绑定会在InputComponent销毁时自动清理
	// 这里清空输入状态
	CurrentPanInput = FVector2D::ZeroVector;
}

void UNamiCameraKeyboardPanFeature::OnPan2DInput(const FInputActionValue& Value)
{
	// 获取2D输入向量
	CurrentPanInput = Value.Get<FVector2D>();
}

void UNamiCameraKeyboardPanFeature::OnPan2DCompleted(const FInputActionValue& Value)
{
	// 清除输入状态
	CurrentPanInput = FVector2D::ZeroVector;
}

void UNamiCameraKeyboardPanFeature::Update_Implementation(float DeltaTime)
{
	Super::Update_Implementation(DeltaTime);

	if (!IsEnabled() || CurrentPanInput.IsNearlyZero())
	{
		return;
	}

	// 只对TopDownCameraMode生效
	UNamiTopDownCameraMode* TopDownMode = Cast<UNamiTopDownCameraMode>(GetCameraMode());
	if (!TopDownMode)
	{
		return;
	}

	// 计算平移方向
	const FVector PanDirection = CalculatePanDirection(CurrentPanInput);
	const FVector PanDelta = PanDirection * PanSpeed * DeltaTime;

	// 应用到TopDownMode
	TopDownMode->AddPanOffset(PanDelta);
}

FVector UNamiCameraKeyboardPanFeature::CalculatePanDirection(const FVector2D& Input) const
{
	if (bUseRelativeToCamera)
	{
		// 相对于相机Yaw方向平移
		const FRotator CameraRot = GetCameraMode()->GetCurrentView().CameraRotation;
		const FRotator YawRotation(0.0f, CameraRot.Yaw, 0.0f);
		const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Input.Y = W/S (前后), Input.X = A/D (左右)
		return (Forward * Input.Y + Right * Input.X).GetSafeNormal();
	}
	else
	{
		// 世界空间平移（北/南/东/西）
		// Input.Y = 前后 (X轴), Input.X = 左右 (Y轴)
		return FVector(Input.Y, Input.X, 0.0f).GetSafeNormal();
	}
}
