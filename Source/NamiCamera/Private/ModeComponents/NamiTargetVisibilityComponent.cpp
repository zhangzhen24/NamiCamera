// Copyright Qiu, Inc. All Rights Reserved.

#include "ModeComponents/NamiTargetVisibilityComponent.h"
#include "CameraModes/NamiCameraModeBase.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiTargetVisibilityComponent)

UNamiTargetVisibilityComponent::UNamiTargetVisibilityComponent()
{
	ComponentName = TEXT("TargetVisibility");
	Priority = 50; // 中等优先级
}

void UNamiTargetVisibilityComponent::Initialize_Implementation(UNamiCameraModeBase* InCameraMode)
{
	Super::Initialize_Implementation(InCameraMode);

	CurrentVisibilityState = ENamiTargetVisibilityState::Visible;
	CurrentOcclusionRatio = 0.0f;
	CurrentAdjustmentOffset = FVector::ZeroVector;
}

void UNamiTargetVisibilityComponent::Activate_Implementation()
{
	Super::Activate_Implementation();

	// 重置状态
	CurrentVisibilityState = ENamiTargetVisibilityState::Visible;
	CurrentOcclusionRatio = 0.0f;
	CurrentAdjustmentOffset = FVector::ZeroVector;
	LastOcclusionCheckTime = 0.0f;
}

void UNamiTargetVisibilityComponent::Update_Implementation(float DeltaTime)
{
	Super::Update_Implementation(DeltaTime);

	if (!LockOnProvider || !LockOnProvider->HasLockedTarget())
	{
		CurrentVisibilityState = ENamiTargetVisibilityState::Visible;
		CurrentOcclusionRatio = 0.0f;
		return;
	}

	UNamiCameraModeBase* Mode = GetCameraMode();
	if (!Mode)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 获取相机和目标位置
	const FVector CameraLocation = Mode->GetLastCameraLocation();
	const FVector TargetLocation = LockOnProvider->GetLockedLocation();

	// 遮挡检测（按间隔执行）
	if (VisibilityConfig.bEnableOcclusionCheck)
	{
		const float CurrentTime = World->GetTimeSeconds();
		if (CurrentTime - LastOcclusionCheckTime >= VisibilityConfig.OcclusionCheckInterval)
		{
			PerformOcclusionCheck(CameraLocation, TargetLocation);
			LastOcclusionCheckTime = CurrentTime;
		}
	}

	// 屏幕边界检测
	if (VisibilityConfig.bEnableScreenBoundsCheck)
	{
		if (!PerformScreenBoundsCheck(TargetLocation))
		{
			CurrentVisibilityState = ENamiTargetVisibilityState::OffScreen;
		}
	}

	// 更新可见性状态
	UpdateVisibilityState();
}

void UNamiTargetVisibilityComponent::ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime)
{
	Super::ApplyToView_Implementation(InOutView, DeltaTime);

	// 如果有遮挡且需要调整相机
	if (VisibilityConfig.bAdjustCameraOnOcclusion && CurrentOcclusionRatio > 0.0f)
	{
		FVector TargetAdjustment = CalculateCameraAdjustment(DeltaTime);

		// 平滑过渡到目标调整量
		CurrentAdjustmentOffset = FMath::VInterpTo(
			CurrentAdjustmentOffset,
			TargetAdjustment,
			DeltaTime,
			VisibilityConfig.AdjustmentSmoothSpeed
		);

		// 应用调整
		InOutView.CameraLocation += CurrentAdjustmentOffset;
	}
	else
	{
		// 逐渐恢复原位
		CurrentAdjustmentOffset = FMath::VInterpTo(
			CurrentAdjustmentOffset,
			FVector::ZeroVector,
			DeltaTime,
			VisibilityConfig.AdjustmentSmoothSpeed
		);

		if (!CurrentAdjustmentOffset.IsNearlyZero(1.0f))
		{
			InOutView.CameraLocation += CurrentAdjustmentOffset;
		}
	}
}

void UNamiTargetVisibilityComponent::SetLockOnProvider(TScriptInterface<INamiLockOnTargetProvider> InProvider)
{
	LockOnProvider = InProvider;
}

void UNamiTargetVisibilityComponent::PerformOcclusionCheck_Implementation(
	const FVector& CameraLocation,
	const FVector& TargetLocation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	OccludedRayCount = 0;
	const int32 TotalRays = VisibilityConfig.OcclusionRayCount;

	// 设置碰撞查询参数
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(LockOnProvider->GetLockedTargetActor());

	// 添加忽略的 Actor
	UNamiCameraModeBase* Mode = GetCameraMode();
	if (Mode && Mode->GetOwnerActor())
	{
		QueryParams.AddIgnoredActor(Mode->GetOwnerActor());
	}

	// 计算射线方向
	const FVector Direction = (TargetLocation - CameraLocation).GetSafeNormal();

	// 中心射线
	FHitResult HitResult;
	if (World->LineTraceSingleByChannel(
		HitResult,
		CameraLocation,
		TargetLocation,
		VisibilityConfig.OcclusionChannel,
		QueryParams))
	{
		OccludedRayCount++;
	}

	// 如果需要多条射线检测
	if (TotalRays > 1)
	{
		// 计算垂直于目标方向的两个轴
		FVector RightAxis = FVector::CrossProduct(Direction, FVector::UpVector);
		if (RightAxis.IsNearlyZero())
		{
			RightAxis = FVector::CrossProduct(Direction, FVector::RightVector);
		}
		RightAxis.Normalize();

		const FVector UpAxis = FVector::CrossProduct(RightAxis, Direction).GetSafeNormal();

		// 发射周围的射线
		const int32 SurroundingRays = TotalRays - 1;
		for (int32 i = 0; i < SurroundingRays; ++i)
		{
			const float Angle = (2.0f * PI * i) / SurroundingRays;
			const FVector Offset = (FMath::Cos(Angle) * RightAxis + FMath::Sin(Angle) * UpAxis) *
				VisibilityConfig.OcclusionRaySpread;

			const FVector EndPoint = TargetLocation + Offset;

			if (World->LineTraceSingleByChannel(
				HitResult,
				CameraLocation,
				EndPoint,
				VisibilityConfig.OcclusionChannel,
				QueryParams))
			{
				OccludedRayCount++;
			}
		}
	}

	// 计算遮挡比例
	CurrentOcclusionRatio = static_cast<float>(OccludedRayCount) / static_cast<float>(TotalRays);
}

bool UNamiTargetVisibilityComponent::PerformScreenBoundsCheck_Implementation(const FVector& TargetLocation)
{
	UNamiCameraModeBase* Mode = GetCameraMode();
	if (!Mode)
	{
		return true;
	}

	// 获取玩家控制器
	AActor* OwnerActor = Mode->GetOwnerActor();
	if (!OwnerActor)
	{
		return true;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerActor->GetInstigatorController());
	if (!PC)
	{
		// 尝试从 Pawn 获取
		if (APawn* Pawn = Cast<APawn>(OwnerActor))
		{
			PC = Cast<APlayerController>(Pawn->GetController());
		}
	}

	if (!PC)
	{
		return true;
	}

	// 将世界位置转换为屏幕位置
	FVector2D ScreenPosition;
	if (!PC->ProjectWorldLocationToScreen(TargetLocation, ScreenPosition))
	{
		return false; // 在屏幕后方
	}

	// 获取屏幕尺寸
	int32 ScreenWidth, ScreenHeight;
	PC->GetViewportSize(ScreenWidth, ScreenHeight);

	if (ScreenWidth <= 0 || ScreenHeight <= 0)
	{
		return true;
	}

	// 计算归一化屏幕位置
	const float NormalizedX = ScreenPosition.X / ScreenWidth;
	const float NormalizedY = ScreenPosition.Y / ScreenHeight;

	// 检查是否在安全区内
	const float Margin = VisibilityConfig.ScreenSafeMargin;
	return NormalizedX >= Margin && NormalizedX <= (1.0f - Margin) &&
		   NormalizedY >= Margin && NormalizedY <= (1.0f - Margin);
}

FVector UNamiTargetVisibilityComponent::CalculateCameraAdjustment_Implementation(float DeltaTime)
{
	FVector Adjustment = FVector::ZeroVector;

	UNamiCameraModeBase* Mode = GetCameraMode();
	if (!Mode || !LockOnProvider || !LockOnProvider->HasLockedTarget())
	{
		return Adjustment;
	}

	const FVector CameraLocation = Mode->GetLastCameraLocation();
	const FVector TargetLocation = LockOnProvider->GetLockedLocation();
	const FVector ToTarget = TargetLocation - CameraLocation;

	// 根据调整模式计算调整量
	switch (VisibilityConfig.AdjustmentMode)
	{
	case 0: // 拉近相机
		{
			const float AdjustAmount = VisibilityConfig.MaxDistanceAdjustment * CurrentOcclusionRatio;
			Adjustment = ToTarget.GetSafeNormal() * AdjustAmount;
		}
		break;

	case 1: // 抬高相机
		{
			const float AdjustAmount = VisibilityConfig.MaxHeightAdjustment * CurrentOcclusionRatio;
			Adjustment = FVector::UpVector * AdjustAmount;
		}
		break;

	case 2: // 侧移相机
		{
			const FVector SideDirection = FVector::CrossProduct(ToTarget.GetSafeNormal(), FVector::UpVector);
			const float AdjustAmount = VisibilityConfig.MaxDistanceAdjustment * CurrentOcclusionRatio;
			Adjustment = SideDirection * AdjustAmount;
		}
		break;
	}

	return Adjustment;
}

void UNamiTargetVisibilityComponent::UpdateVisibilityState()
{
	// 如果已经是 OffScreen，保持该状态
	if (CurrentVisibilityState == ENamiTargetVisibilityState::OffScreen)
	{
		return;
	}

	// 根据遮挡比例更新状态
	if (CurrentOcclusionRatio >= 0.9f)
	{
		CurrentVisibilityState = ENamiTargetVisibilityState::FullyOccluded;
	}
	else if (CurrentOcclusionRatio >= 0.3f)
	{
		CurrentVisibilityState = ENamiTargetVisibilityState::PartiallyOccluded;
	}
	else
	{
		CurrentVisibilityState = ENamiTargetVisibilityState::Visible;
	}
}
