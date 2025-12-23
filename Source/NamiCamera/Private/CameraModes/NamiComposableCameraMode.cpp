// Copyright Qiu, Inc. All Rights Reserved.

#include "CameraModes/NamiComposableCameraMode.h"

#include "Calculators/Target/NamiCameraTargetCalculator.h"
#include "Calculators/Position/NamiCameraPositionCalculator.h"
#include "Calculators/Rotation/NamiCameraRotationCalculator.h"
#include "Calculators/FOV/NamiCameraFOVCalculator.h"
#include "Components/NamiCameraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiComposableCameraMode)

namespace NamiComposableCameraMode_Private
{
	/**
	 * 辅助模板函数：设置计算器（减少代码重复）
	 * @param CurrentCalculator 当前计算器的引用
	 * @param NewCalculator 新计算器
	 * @param Mode 相机模式
	 */
	template<typename T>
	void SetCalculator(TObjectPtr<T>& CurrentCalculator, T* NewCalculator, UNamiComposableCameraMode* Mode)
	{
		// 如果当前激活且有旧计算器，先停用
		if (CurrentCalculator && Mode->IsActive())
		{
			CurrentCalculator->Deactivate();
		}

		CurrentCalculator = NewCalculator;

		if (CurrentCalculator)
		{
			CurrentCalculator->Initialize(Mode);
			if (Mode->IsActive())
			{
				CurrentCalculator->Activate();
			}
		}
	}
}

UNamiComposableCameraMode::UNamiComposableCameraMode()
{
	CurrentPivotLocation = FVector::ZeroVector;
	CurrentCameraLocation = FVector::ZeroVector;
	CurrentCameraRotation = FRotator::ZeroRotator;
	CachedControlRotation = FRotator::ZeroRotator;
}

void UNamiComposableCameraMode::Initialize_Implementation(UNamiCameraComponent* InCameraComponent)
{
	Super::Initialize_Implementation(InCameraComponent);

	InitializeCalculators();
}

void UNamiComposableCameraMode::Activate_Implementation()
{
	Super::Activate_Implementation();

	// 激活所有计算器
	if (TargetCalculator)
	{
		TargetCalculator->Activate();
	}
	if (PositionCalculator)
	{
		PositionCalculator->Activate();
	}
	if (RotationCalculator)
	{
		RotationCalculator->Activate();
	}
	if (FOVCalculator)
	{
		FOVCalculator->Activate();
	}
}

void UNamiComposableCameraMode::Deactivate_Implementation()
{
	Super::Deactivate_Implementation();

	// 停用所有计算器
	if (TargetCalculator)
	{
		TargetCalculator->Deactivate();
	}
	if (PositionCalculator)
	{
		PositionCalculator->Deactivate();
	}
	if (RotationCalculator)
	{
		RotationCalculator->Deactivate();
	}
	if (FOVCalculator)
	{
		FOVCalculator->Deactivate();
	}
}

FNamiCameraView UNamiComposableCameraMode::CalculateView_Implementation(float DeltaTime)
{
	FNamiCameraView View;
	View.FOV = DefaultFOV;

	// 获取控制旋转（缓存以供策略使用）
	CachedControlRotation = GetControlRotation();

	// 标准化 Pitch 到 [-180, 180] 范围，防止相机翻转
	// GetControlRotation() 可能返回 [0, 360) 格式的旋转（如 270° 而不是 -90°）
	CachedControlRotation.Pitch = FRotator::NormalizeAxis(CachedControlRotation.Pitch);

	// ========== 1. 目标计算器 → PivotLocation ==========
	FVector PivotLocation = FVector::ZeroVector;
	if (TargetCalculator)
	{
		TargetCalculator->CalculateTargetLocation(DeltaTime, PivotLocation);
	}
	else if (UNamiCameraComponent* CameraComp = GetCameraComponent())
	{
		// 后备：使用相机组件 Owner 位置
		if (AActor* Owner = CameraComp->GetOwner())
		{
			PivotLocation = Owner->GetActorLocation();
		}
	}

	// 应用视点偏移
	if (!PivotOffset.IsNearlyZero())
	{
		FVector Offset = PivotOffset;
		if (bPivotOffsetUseControlRotation)
		{
			FRotator RotationToUse = CachedControlRotation;
			if (bPivotOffsetUseYawOnly)
			{
				RotationToUse.Pitch = 0.0f;
				RotationToUse.Roll = 0.0f;
			}
			Offset = RotationToUse.RotateVector(Offset);
		}
		PivotLocation += Offset;
	}

	CurrentPivotLocation = PivotLocation;
	View.PivotLocation = PivotLocation;

	// ========== 2. 位置计算器 → CameraLocation ==========
	if (PositionCalculator)
	{
		CurrentCameraLocation = PositionCalculator->CalculateCameraPosition(PivotLocation, CachedControlRotation, DeltaTime);
	}
	else
	{
		// 后备：简单偏移
		CurrentCameraLocation = PivotLocation + FVector(-300.0f, 0.0f, 100.0f);
	}
	View.CameraLocation = CurrentCameraLocation;

	// ========== 3. 旋转计算器 → CameraRotation ==========
	if (RotationCalculator)
	{
		CurrentCameraRotation = RotationCalculator->CalculateCameraRotation(
			CurrentCameraLocation,
			PivotLocation,
			CachedControlRotation,
			DeltaTime);
	}
	else
	{
		// 后备：看向 PivotLocation
		FVector Direction = PivotLocation - CurrentCameraLocation;
		if (!Direction.IsNearlyZero())
		{
			CurrentCameraRotation = Direction.Rotation();
		}
	}
	View.CameraRotation = CurrentCameraRotation;

	// ========== 4. FOV 计算器 → FOV ==========
	if (FOVCalculator)
	{
		View.FOV = FOVCalculator->CalculateFOV(CurrentCameraLocation, PivotLocation, DeltaTime);
	}

	// 同步控制位置和旋转
	View.ControlLocation = View.CameraLocation;
	View.ControlRotation = View.CameraRotation;

	return View;
}

void UNamiComposableCameraMode::SetPrimaryTarget(AActor* Target)
{
	if (TargetCalculator)
	{
		TargetCalculator->SetPrimaryTarget(Target);
	}
}

AActor* UNamiComposableCameraMode::GetPrimaryTarget() const
{
	if (TargetCalculator)
	{
		return TargetCalculator->GetPrimaryTarget();
	}
	return nullptr;
}

void UNamiComposableCameraMode::SetTargetCalculator(UNamiCameraTargetCalculator* InCalculator)
{
	NamiComposableCameraMode_Private::SetCalculator(TargetCalculator, InCalculator, this);
}

void UNamiComposableCameraMode::SetPositionCalculator(UNamiCameraPositionCalculator* InCalculator)
{
	NamiComposableCameraMode_Private::SetCalculator(PositionCalculator, InCalculator, this);
}

void UNamiComposableCameraMode::SetRotationCalculator(UNamiCameraRotationCalculator* InCalculator)
{
	NamiComposableCameraMode_Private::SetCalculator(RotationCalculator, InCalculator, this);
}

void UNamiComposableCameraMode::SetFOVCalculator(UNamiCameraFOVCalculator* InCalculator)
{
	NamiComposableCameraMode_Private::SetCalculator(FOVCalculator, InCalculator, this);
}

FRotator UNamiComposableCameraMode::GetControlRotation_Implementation() const
{
	if (UNamiCameraComponent* CameraComp = GetCameraComponent())
	{
		// 优先从 Owner Pawn 获取
		if (APawn* OwnerPawn = CameraComp->GetOwnerPawn())
		{
			return OwnerPawn->GetControlRotation();
		}
		// 尝试从 PlayerController 获取
		if (APlayerController* PC = CameraComp->GetOwnerPlayerController())
		{
			return PC->GetControlRotation();
		}
	}
	return FRotator::ZeroRotator;
}

void UNamiComposableCameraMode::InitializeCalculators()
{
	if (bCalculatorsInitialized)
	{
		return;
	}

	// 初始化所有计算器
	if (TargetCalculator)
	{
		TargetCalculator->Initialize(this);
	}
	if (PositionCalculator)
	{
		PositionCalculator->Initialize(this);
	}
	if (RotationCalculator)
	{
		RotationCalculator->Initialize(this);
	}
	if (FOVCalculator)
	{
		FOVCalculator->Initialize(this);
	}

	bCalculatorsInitialized = true;
}
