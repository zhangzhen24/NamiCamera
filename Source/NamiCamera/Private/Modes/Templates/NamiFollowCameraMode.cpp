// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modes/Templates/NamiFollowCameraMode.h"
#include "Libraries/NamiCameraMath.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/NamiCameraComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiFollowCameraMode)

UNamiFollowCameraMode::UNamiFollowCameraMode()
{
}

void UNamiFollowCameraMode::Activate_Implementation()
{
	Super::Activate_Implementation();

	bInitialized = false;
	CameraVelocity = FVector::ZeroVector;
	YawVelocity = 0.0f;
	PitchVelocity = 0.0f;
}

FNamiCameraView UNamiFollowCameraMode::CalculateView_Implementation(float DeltaTime)
{
	FNamiCameraView View;

	// 1. 计算枢轴点
	FVector TargetPivot = CalculatePivotLocation();
	
	// 应用PivotLocationOffset（视点偏移）
	FVector PivotOffset = PivotLocationOffset;
	if (!PivotOffset.IsNearlyZero())
	{
		// 应用控制器旋转（如果启用）
		if (bPivotOffsetUseTargetRotation)
		{
			// 获取控制器旋转（ControlRotation）
			FRotator ControlRotation = FRotator::ZeroRotator;
			UNamiCameraComponent* CameraComp = GetCameraComponent();
			if (IsValid(CameraComp))
			{
				// 优先从 Owner Pawn 获取
				APawn* OwnerPawn = CameraComp->GetOwnerPawn();
				if (IsValid(OwnerPawn))
				{
					ControlRotation = OwnerPawn->GetControlRotation();
				}
				else
				{
					// 尝试从 PlayerController 获取
					APlayerController* PC = CameraComp->GetOwnerPlayerController();
					if (IsValid(PC))
					{
						ControlRotation = PC->GetControlRotation();
					}
				}
			}
			
			// 如果获取到了有效的ControlRotation，立即归一化，然后再进行后续计算
			if (!ControlRotation.IsNearlyZero())
			{
				// 立即归一化，确保获取的 ControlRotation 是归一化后的旋转
				// 使用0-360度归一化，避免180/-180跳变问题
				FRotator NormalizedControlRotation = FNamiCameraMath::NormalizeRotatorTo360(ControlRotation);
				
				if (bPivotOffsetUseYawOnly)
				{
					// 只使用Yaw，Pitch和Roll设为0
					NormalizedControlRotation.Pitch = 0.0f;
					NormalizedControlRotation.Roll = 0.0f;
				}
				
				// 使用归一化后的 ControlRotation 旋转 PivotOffset
				PivotOffset = NormalizedControlRotation.RotateVector(PivotOffset);
			}
		}
		TargetPivot += PivotOffset;
	}
	
	// 叠加PivotHeightOffset（如果PivotLocationOffset.Z为0或很小）
	if (FMath::IsNearlyZero(PivotLocationOffset.Z, 1.0f))
	{
		TargetPivot.Z += PivotHeightOffset;
	}

	// 2. 计算相机位置
	FVector TargetCameraLocation = CalculateCameraLocation(TargetPivot);

	// 3. 计算相机旋转
	FRotator TargetRotation = CalculateCameraRotation(TargetCameraLocation, TargetPivot);

	// 4. 应用平滑
	if (!bInitialized)
	{
		// 首次不平滑，直接设置
		CurrentPivotLocation = TargetPivot;
		CurrentCameraLocation = TargetCameraLocation;
		CurrentCameraRotation = TargetRotation;
		bInitialized = true;
	}
	else if (DeltaTime > 0.0f)
	{
		// 性能优化：如果平滑被禁用，直接设置目标值，跳过计算
		if (!bEnableSmoothing)
		{
			CurrentPivotLocation = TargetPivot;
			CurrentCameraLocation = TargetCameraLocation;
			CurrentCameraRotation = TargetRotation;
		}
		else
		{
			// PivotLocation 不再进行平滑处理，直接使用计算值
				CurrentPivotLocation = TargetPivot;

			// 平滑相机位置
			if (bEnableCameraLocationSmoothing && CameraLocationSmoothIntensity > 0.0f)
			{
				// 将平滑强度映射到实际平滑时间
				float SmoothTime = FNamiCameraMath::MapSmoothIntensity(CameraLocationSmoothIntensity);
				CurrentCameraLocation = FNamiCameraMath::SmoothDamp(
					CurrentCameraLocation,
					TargetCameraLocation,
					CameraVelocity,
					SmoothTime,
					DeltaTime);
			}
			else
			{
				CurrentCameraLocation = TargetCameraLocation;
			}

			// 平滑旋转
			if (bEnableCameraRotationSmoothing && CameraRotationSmoothIntensity > 0.0f)
			{
				// 将平滑强度映射到实际平滑时间
				float SmoothTime = FNamiCameraMath::MapSmoothIntensity(CameraRotationSmoothIntensity);
				CurrentCameraRotation = FNamiCameraMath::SmoothDampRotator(
					CurrentCameraRotation,
					TargetRotation,
					YawVelocity,
					PitchVelocity,
					SmoothTime,
					SmoothTime,
					DeltaTime);
			}
			else
			{
				CurrentCameraRotation = TargetRotation;
			}
		}
	}

	// 设置视图
	View.PivotLocation = CurrentPivotLocation;
	View.CameraLocation = CurrentCameraLocation;
	View.CameraRotation = CurrentCameraRotation;

	// 应用动态FOV
	if (bEnableDynamicFOV)
	{
		// 计算目标FOV
		float TargetFOV = BaseFOV;

		// 获取主要目标的速度（如果有）
		AActor *PrimaryTarget = GetPrimaryTarget();
		if (IsValid(PrimaryTarget))
		{
			// 尝试获取Pawn或Character的速度
			APawn *PawnTarget = Cast<APawn>(PrimaryTarget);
			if (PawnTarget)
			{
				float Speed = PawnTarget->GetVelocity().Size();
				// 根据速度调整FOV
				TargetFOV += Speed * SpeedFOVFactor;
			}
		}

		// 限制FOV范围
		TargetFOV = FMath::Clamp(TargetFOV, MinDynamicFOV, MaxDynamicFOV);

		// 平滑过渡FOV变化
		if (DeltaTime > 0.0f)
		{
			float MaxChange = DynamicFOVChangeRate * DeltaTime;
			CurrentDynamicFOV = FMath::FInterpTo(CurrentDynamicFOV, TargetFOV, DeltaTime, DynamicFOVChangeRate);
		}
		else
		{
			CurrentDynamicFOV = TargetFOV;
		}

		View.FOV = CurrentDynamicFOV;
	}
	else
	{
		// 使用默认FOV
		View.FOV = DefaultFOV;
	}

	return View;
}

void UNamiFollowCameraMode::SetPrimaryTarget(AActor *Target)
{
	// 移除现有的主要目标
	Targets.RemoveAll([](const FNamiFollowTarget &T)
					  { return T.TargetType == ENamiFollowTargetType::Primary; });

	// 添加新的主要目标
	if (IsValid(Target))
	{
		FNamiFollowTarget NewTarget(Target, 1.0f, ENamiFollowTargetType::Primary);
		Targets.Insert(NewTarget, 0); // 插入到开头
	}
}

AActor *UNamiFollowCameraMode::GetPrimaryTarget() const
{
	for (const FNamiFollowTarget &T : Targets)
	{
		if (T.TargetType == ENamiFollowTargetType::Primary && T.Target.IsValid())
		{
			return T.Target.Get();
		}
	}
	return nullptr;
}

void UNamiFollowCameraMode::AddTarget(AActor *Target, float Weight, ENamiFollowTargetType TargetType)
{
	if (!IsValid(Target))
	{
		return;
	}

	// 检查是否已存在
	for (FNamiFollowTarget &T : Targets)
	{
		if (T.Target == Target)
		{
			T.Weight = Weight;
			T.TargetType = TargetType;
			return;
		}
	}

	// 添加新目标
	Targets.Add(FNamiFollowTarget(Target, Weight, TargetType));
}

void UNamiFollowCameraMode::RemoveTarget(AActor *Target)
{
	Targets.RemoveAll([Target](const FNamiFollowTarget &T)
					  { return T.Target == Target; });
}

void UNamiFollowCameraMode::ClearAllTargets()
{
	Targets.Empty();
}

void UNamiFollowCameraMode::SetCustomPivotLocation(const FVector &Location)
{
	CustomPivotLocation = Location;
	bUseCustomPivotLocation = true;
}

void UNamiFollowCameraMode::ClearCustomPivotLocation()
{
	bUseCustomPivotLocation = false;
	CustomPivotLocation = FVector::ZeroVector;
}

FVector UNamiFollowCameraMode::CalculatePivotLocation_Implementation(float DeltaTime)
{
	// 调用本类的CalculatePivotLocation() const方法
	return CalculatePivotLocation();
}

FVector UNamiFollowCameraMode::CalculatePivotLocation() const
{
	// 使用自定义枢轴点
	if (bUseCustomPivotLocation)
	{
		return CustomPivotLocation;
	}

	// 收集有效目标
	TArray<FNamiFollowTarget> ValidTargets;
	for (const FNamiFollowTarget &T : Targets)
	{
		if (T.IsValid())
		{
			ValidTargets.Add(T);
		}
	}

	if (ValidTargets.Num() == 0)
	{
		// 如果没有目标，尝试使用相机组件的位置（如果有）
		// 这样可以避免相机固定在原点
		if (GetCameraComponent())
		{
			AActor *Owner = GetCameraComponent()->GetOwner();
			if (IsValid(Owner))
			{
				return Owner->GetActorLocation();
			}
		}

		// 如果连相机组件都没有，返回当前位置（可能是初始化时的位置）
		return CurrentPivotLocation;
	}

	// 只有一个目标
	if (ValidTargets.Num() == 1)
	{
		return ValidTargets[0].GetLocation();
	}

	// 多个目标 - 加权平均
	FVector WeightedSum = FVector::ZeroVector;
	float TotalWeight = 0.0f;

	for (const FNamiFollowTarget &T : ValidTargets)
	{
		WeightedSum += T.GetLocation() * T.Weight;
		TotalWeight += T.Weight;
	}

	if (TotalWeight > KINDA_SMALL_NUMBER)
	{
		return WeightedSum / TotalWeight;
	}

	return ValidTargets[0].GetLocation();
}

FVector UNamiFollowCameraMode::CalculateCameraLocation_Implementation(const FVector &InPivotLocation) const
{
	FVector Offset = CameraOffset;

	// 应用目标旋转
	if (bUseTargetRotation)
	{
		FRotator TargetRotation = GetPrimaryTargetRotation();
		if (bUseYawOnly)
		{
			TargetRotation.Pitch = 0.0f;
			TargetRotation.Roll = 0.0f;
		}
		Offset = TargetRotation.RotateVector(Offset);
	}

	return InPivotLocation + Offset;
}

FRotator UNamiFollowCameraMode::CalculateCameraRotation_Implementation(
	const FVector &InCameraLocation,
	const FVector &InPivotLocation) const
{
	FVector Direction = InPivotLocation - InCameraLocation;
	if (Direction.IsNearlyZero())
	{
		return CurrentCameraRotation;
	}
	// 使用0-360度归一化，避免180/-180跳变问题
	FRotator Rotation = Direction.Rotation();
	return FNamiCameraMath::NormalizeRotatorTo360(Rotation);
}

FRotator UNamiFollowCameraMode::GetPrimaryTargetRotation() const
{
	AActor *Primary = GetPrimaryTarget();
	if (IsValid(Primary))
	{
		return Primary->GetActorRotation();
	}
	return FRotator::ZeroRotator;
}

