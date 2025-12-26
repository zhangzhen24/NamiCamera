// Copyright Qiu, Inc. All Rights Reserved.

#include "Calculators/Position/NamiTopDownPositionCalculator.h"
#include "CameraModes/NamiComposableCameraMode.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiTopDownPositionCalculator)

UNamiTopDownPositionCalculator::UNamiTopDownPositionCalculator()
{
	// 默认俯视角配置
	CameraHeight = 1500.0f;
	ViewAngle = 45.0f;
	ViewDirectionYaw = 45.0f;  // 默认东北方向
	FollowSmoothSpeed = 8.0f;

	// 不使用基类的控制旋转配置
	bUseControlRotation = false;
}

void UNamiTopDownPositionCalculator::Activate_Implementation()
{
	Super::Activate_Implementation();
}

FVector UNamiTopDownPositionCalculator::CalculateCameraPosition_Implementation(
	const FVector& PivotLocation,
	const FRotator& ControlRotation,
	float DeltaTime)
{
	// 1. 计算基础偏移（根据高度和俯视角）
	const float ViewAngleRad = FMath::DegreesToRadians(ViewAngle);
	const float HorizontalDist = CameraHeight / FMath::Tan(ViewAngleRad);

	// 2. 创建未旋转的偏移（相机在 Pivot 后方 -X 轴上）
	FVector BaseOffset(-HorizontalDist, 0.0f, CameraHeight);

	// 3. 根据 ViewDirectionYaw 旋转偏移（绕 Z 轴）
	FVector RotatedOffset = BaseOffset.RotateAngleAxis(ViewDirectionYaw, FVector::UpVector);

	// 4. 计算目标相机位置
	FVector TargetPosition = PivotLocation + RotatedOffset;

	// 5. 首帧初始化
	if (!bFirstFrameProcessed)
	{
		CurrentCameraPosition = TargetPosition;
		bFirstFrameProcessed = true;
		return TargetPosition;
	}

	// 6. 应用平滑跟随
	if (FollowSmoothSpeed > 0.0f && DeltaTime > 0.0f)
	{
		CurrentCameraPosition = FMath::VInterpTo(
			CurrentCameraPosition,
			TargetPosition,
			DeltaTime,
			FollowSmoothSpeed);
	}
	else
	{
		CurrentCameraPosition = TargetPosition;
	}

	return CurrentCameraPosition;
}

AActor* UNamiTopDownPositionCalculator::GetPrimaryTarget() const
{
	if (UNamiCameraModeBase* Mode = GetCameraMode())
	{
		if (UNamiComposableCameraMode* ComposableMode = Cast<UNamiComposableCameraMode>(Mode))
		{
			return ComposableMode->GetPrimaryTarget();
		}
	}
	return nullptr;
}

FVector UNamiTopDownPositionCalculator::CalculateBaseOffset() const
{
	// 根据高度和俯视角计算水平距离
	// tan(ViewAngle) = CameraHeight / HorizontalDistance
	// => HorizontalDistance = CameraHeight / tan(ViewAngle)

	const float ViewAngleRadians = FMath::DegreesToRadians(ViewAngle);
	const float HorizontalDistance = CameraHeight / FMath::Tan(ViewAngleRadians);

	// 返回偏移向量（X 轴后退，Z 轴抬高）
	// 注意：X 轴为负，表示相机在 Pivot 的后方（未旋转前为正北方向）
	return FVector(-HorizontalDistance, 0.0f, CameraHeight);
}
