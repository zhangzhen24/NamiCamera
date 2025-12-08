// Copyright Epic Games, Inc. All Rights Reserved.

#include "Features/NamiToricSpaceFeature.h"
#include "Modes/Templates/NamiFollowCameraMode.h"
#include "Libraries/NamiCameraMath.h"
#include "GameFramework/Actor.h"
#include "Components/NamiCameraComponent.h"
#include "Structs/Follow/NamiFollowTarget.h"
#include "Math/UnrealMathUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiToricSpaceFeature)

UNamiToricSpaceFeature::UNamiToricSpaceFeature()
{
	FeatureName = TEXT("ToricSpace");
	Priority = 35;  // 在 VelocityPrediction 之后，SpringArm 之前执行
}

void UNamiToricSpaceFeature::Initialize_Implementation(UNamiCameraModeBase* InCameraMode)
{
	Super::Initialize_Implementation(InCameraMode);

	if (!IsEnabled() || !bAutoSetPrimaryTarget)
	{
		return;
	}

	// 自动设置主角为主目标
	UNamiCameraModeBase* Mode = GetCameraMode();
	if (!IsValid(Mode))
	{
		return;
	}

	UNamiFollowCameraMode* FollowMode = Cast<UNamiFollowCameraMode>(Mode);
	if (!IsValid(FollowMode))
	{
		return;
	}

	// 获取相机组件
	UNamiCameraComponent* CameraComp = Mode->GetCameraComponent();
	if (!IsValid(CameraComp))
	{
		return;
	}

	// 获取主角（Owner Pawn）
	APawn* OwnerPawn = CameraComp->GetOwnerPawn();
	if (IsValid(OwnerPawn))
	{
		// 检查是否已经设置了主目标
		AActor* CurrentPrimary = FollowMode->GetPrimaryTarget();
		if (!IsValid(CurrentPrimary) || CurrentPrimary != OwnerPawn)
		{
			// 自动设置主角为主目标
			FollowMode->SetPrimaryTarget(OwnerPawn);
		}
	}
}

void UNamiToricSpaceFeature::Activate_Implementation()
{
	Super::Activate_Implementation();

	// 激活时再次检查并设置主角（以防初始化时主角还未准备好）
	if (bAutoSetPrimaryTarget)
	{
		UNamiCameraModeBase* Mode = GetCameraMode();
		if (IsValid(Mode))
		{
			UNamiFollowCameraMode* FollowMode = Cast<UNamiFollowCameraMode>(Mode);
			if (IsValid(FollowMode))
			{
				UNamiCameraComponent* CameraComp = Mode->GetCameraComponent();
				if (IsValid(CameraComp))
				{
					APawn* OwnerPawn = CameraComp->GetOwnerPawn();
					if (IsValid(OwnerPawn))
					{
						AActor* CurrentPrimary = FollowMode->GetPrimaryTarget();
						if (!IsValid(CurrentPrimary) || CurrentPrimary != OwnerPawn)
						{
							FollowMode->SetPrimaryTarget(OwnerPawn);
						}
					}
				}
			}
		}
	}
}

void UNamiToricSpaceFeature::Update_Implementation(float DeltaTime)
{
	Super::Update_Implementation(DeltaTime);

	if (!IsEnabled())
	{
		return;
	}

	// 获取所有有效目标
	TArray<FNamiFollowTarget> ValidTargets = GetAllValidTargets();

	if (ValidTargets.Num() == 0)
	{
		return;
	}

	// 检查是否有足够的目标
	if (bRequireTwoTargets && ValidTargets.Num() < 2)
	{
		return;
	}

	// 平滑角度参数
	if (AngleSmoothTime > 0.0f && DeltaTime > 0.0f)
	{
		CurrentTheta = FNamiCameraMath::SmoothDampAngle(CurrentTheta, ToricTheta, ThetaVelocity, AngleSmoothTime, DeltaTime);
		CurrentPhi = FNamiCameraMath::SmoothDampAngle(CurrentPhi, ToricPhi, PhiVelocity, AngleSmoothTime, DeltaTime);
	}
	else
	{
		CurrentTheta = ToricTheta;
		CurrentPhi = ToricPhi;
	}

	// 计算目标大小（用于自动半径）
	float TargetSize = 0.0f;
	float TargetRadius = ToricRadius;

	if (bEnableMultiTargetMode && ValidTargets.Num() >= 2)
	{
		// 多目标模式：使用边界框大小
		TargetSize = CalculateMultiTargetSize(ValidTargets);
	}
	else if (ValidTargets.Num() >= 2)
	{
		// 双目标模式：使用目标距离
		AActor* Target1 = GetTarget1();
		AActor* Target2 = GetTarget2();
		if (IsValid(Target1) && IsValid(Target2))
		{
			TargetSize = FVector::Dist(Target1->GetActorLocation(), Target2->GetActorLocation());
		}
	}

	// 自动计算半径
	if (bAutoCalculateRadius && TargetSize > 0.0f)
	{
		TargetRadius = TargetSize * RadiusScaleFactor;
		TargetRadius = FMath::Clamp(TargetRadius, MinRadius, MaxRadius);
	}

	// 平滑半径
	if (RadiusSmoothTime > 0.0f && DeltaTime > 0.0f)
	{
		CurrentRadius = FNamiCameraMath::SmoothDamp(CurrentRadius, TargetRadius, RadiusVelocity, RadiusSmoothTime, DeltaTime);
	}
	else
	{
		CurrentRadius = TargetRadius;
	}

	// 平滑高度
	if (HeightSmoothTime > 0.0f && DeltaTime > 0.0f)
	{
		CurrentHeight = FNamiCameraMath::SmoothDamp(CurrentHeight, ToricHeight, HeightVelocity, HeightSmoothTime, DeltaTime);
	}
	else
	{
		CurrentHeight = ToricHeight;
	}

	// 保存目标位置（用于下一帧检测变化）
	if (ValidTargets.Num() >= 1)
	{
		LastTarget1Location = ValidTargets[0].GetLocation();
	}
	if (ValidTargets.Num() >= 2)
	{
		LastTarget2Location = ValidTargets[1].GetLocation();
	}
}

void UNamiToricSpaceFeature::ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime)
{
	Super::ApplyToView_Implementation(InOutView, DeltaTime);

	if (!IsEnabled())
	{
		return;
	}

	// 获取所有有效目标
	TArray<FNamiFollowTarget> ValidTargets = GetAllValidTargets();

	if (ValidTargets.Num() == 0)
	{
		return;
	}

	// 多目标模式：使用所有目标的中心
	if (bEnableMultiTargetMode && ValidTargets.Num() >= 2)
	{
		// 计算多目标中心
		FVector MultiTargetCenter = CalculateMultiTargetCenter(ValidTargets);
		
		// 计算多目标大小（用于自动半径）
		float MultiTargetSize = CalculateMultiTargetSize(ValidTargets);
		
		// 如果自动计算半径，使用多目标大小
		float TargetRadius = CurrentRadius;
		if (bAutoCalculateRadius)
		{
			TargetRadius = MultiTargetSize * RadiusScaleFactor;
			TargetRadius = FMath::Clamp(TargetRadius, MinRadius, MaxRadius);
			
			// 平滑半径
			if (RadiusSmoothTime > 0.0f && DeltaTime > 0.0f)
			{
				CurrentRadius = FNamiCameraMath::SmoothDamp(CurrentRadius, TargetRadius, RadiusVelocity, RadiusSmoothTime, DeltaTime);
			}
			else
			{
				CurrentRadius = TargetRadius;
			}
		}

		// 获取主目标和第一个次要目标（用于180度规则和聚焦点计算）
		AActor* PrimaryTarget = nullptr;
		AActor* SecondaryTarget = nullptr;
		
		for (const FNamiFollowTarget& T : ValidTargets)
		{
			if (T.TargetType == ENamiFollowTargetType::Primary && !IsValid(PrimaryTarget))
			{
				PrimaryTarget = T.Target.Get();
			}
			else if (T.TargetType != ENamiFollowTargetType::Primary && !IsValid(SecondaryTarget))
			{
				SecondaryTarget = T.Target.Get();
			}
		}

		// 如果没有找到，使用前两个目标
		if (!IsValid(PrimaryTarget) && ValidTargets.Num() > 0)
		{
			PrimaryTarget = ValidTargets[0].Target.Get();
		}
		if (!IsValid(SecondaryTarget) && ValidTargets.Num() > 1)
		{
			SecondaryTarget = ValidTargets[1].Target.Get();
		}

		// 应用180度规则（如果有两个明确目标）
		float AdjustedTheta = CurrentTheta;
		if (bEnforce180DegreeRule && IsValid(PrimaryTarget) && IsValid(SecondaryTarget))
		{
			AdjustedTheta = Apply180DegreeRule(
				CurrentTheta,
				PrimaryTarget->GetActorLocation(),
				SecondaryTarget->GetActorLocation(),
				FocusTarget);
		}

		// 计算复曲面空间中的相机位置
		// 对于多目标，使用中心点和主-次目标的方向
		FVector Target1Loc = IsValid(PrimaryTarget) ? PrimaryTarget->GetActorLocation() : MultiTargetCenter;
		FVector Target2Loc = IsValid(SecondaryTarget) ? SecondaryTarget->GetActorLocation() : MultiTargetCenter;
		
		FVector ToricCameraLocation = CalculateToricSpaceCameraLocation(
			Target1Loc,
			Target2Loc,
			AdjustedTheta,
			CurrentPhi,
			CurrentRadius,
			CurrentHeight);

		// 更新相机位置
		InOutView.CameraLocation = ToricCameraLocation;

		// 计算聚焦点（使用多目标中心或根据 FocusTarget）
		FVector FocusPoint;
		if (IsValid(PrimaryTarget) && IsValid(SecondaryTarget))
		{
			FocusPoint = CalculateFocusPoint(Target1Loc, Target2Loc);
		}
		else
		{
			// 使用多目标中心
			FocusPoint = MultiTargetCenter;
		}

		// 更新 PivotLocation（聚焦点）
		InOutView.PivotLocation = FocusPoint;

		// 计算相机朝向（看向聚焦点）
		FVector ToFocus = FocusPoint - ToricCameraLocation;
		if (!ToFocus.IsNearlyZero())
		{
			InOutView.CameraRotation = ToFocus.Rotation();
		}
	}
	else if (ValidTargets.Num() >= 2)
	{
		// 双目标模式（原有逻辑）
		AActor* Target1 = GetTarget1();
		AActor* Target2 = GetTarget2();

		if (!IsValid(Target1) || !IsValid(Target2))
		{
			return;
		}

		FVector Target1Loc = Target1->GetActorLocation();
		FVector Target2Loc = Target2->GetActorLocation();

		// 应用180度规则（如果需要）
		float AdjustedTheta = CurrentTheta;
		if (bEnforce180DegreeRule)
		{
			AdjustedTheta = Apply180DegreeRule(CurrentTheta, Target1Loc, Target2Loc, FocusTarget);
		}

		// 计算复曲面空间中的相机位置
		FVector ToricCameraLocation = CalculateToricSpaceCameraLocation(
			Target1Loc,
			Target2Loc,
			AdjustedTheta,
			CurrentPhi,
			CurrentRadius,
			CurrentHeight);

		// 更新相机位置
		InOutView.CameraLocation = ToricCameraLocation;

		// 计算聚焦点
		FVector FocusPoint = CalculateFocusPoint(Target1Loc, Target2Loc);

		// 更新 PivotLocation（聚焦点）
		InOutView.PivotLocation = FocusPoint;

		// 计算相机朝向（看向聚焦点）
		FVector ToFocus = FocusPoint - ToricCameraLocation;
		if (!ToFocus.IsNearlyZero())
		{
			InOutView.CameraRotation = ToFocus.Rotation();
		}
	}
}

bool UNamiToricSpaceFeature::HasTwoTargets() const
{
	UNamiCameraModeBase* Mode = GetCameraMode();
	if (!IsValid(Mode))
	{
		return false;
	}

	UNamiFollowCameraMode* FollowMode = Cast<UNamiFollowCameraMode>(Mode);
	if (!IsValid(FollowMode))
	{
		return false;
	}

	// 获取所有有效目标
	int32 ValidTargetCount = 0;
	for (const FNamiFollowTarget& T : FollowMode->GetTargets())
	{
		if (T.IsValid())
		{
			ValidTargetCount++;
		}
	}

	return ValidTargetCount >= 2;
}

AActor* UNamiToricSpaceFeature::GetTarget1() const
{
	UNamiCameraModeBase* Mode = GetCameraMode();
	if (!IsValid(Mode))
	{
		return nullptr;
	}

	UNamiFollowCameraMode* FollowMode = Cast<UNamiFollowCameraMode>(Mode);
	if (!IsValid(FollowMode))
	{
		return nullptr;
	}

	// 获取主目标（通常是角色）
	return FollowMode->GetPrimaryTarget();
}

AActor* UNamiToricSpaceFeature::GetTarget2() const
{
	UNamiCameraModeBase* Mode = GetCameraMode();
	if (!IsValid(Mode))
	{
		return nullptr;
	}

	UNamiFollowCameraMode* FollowMode = Cast<UNamiFollowCameraMode>(Mode);
	if (!IsValid(FollowMode))
	{
		return nullptr;
	}

	// 获取第一个非主目标（通常是敌人）
	for (const FNamiFollowTarget& T : FollowMode->GetTargets())
	{
		if (T.IsValid() && T.TargetType != ENamiFollowTargetType::Primary)
		{
			return T.Target.Get();
		}
	}

	// 如果没有找到非主目标，返回第二个目标（如果有）
	int32 TargetIndex = 0;
	for (const FNamiFollowTarget& T : FollowMode->GetTargets())
	{
		if (T.IsValid())
		{
			if (TargetIndex == 1)
			{
				return T.Target.Get();
			}
			TargetIndex++;
		}
	}

	return nullptr;
}

FVector UNamiToricSpaceFeature::CalculateToricSpaceCameraLocation(
	const FVector& Target1Location,
	const FVector& Target2Location,
	float Theta,
	float Phi,
	float Radius,
	float Height) const
{
	// 计算两个目标的中点
	FVector MidPoint = (Target1Location + Target2Location) * 0.5f;

	// 计算目标连线方向
	FVector TargetDirection = (Target2Location - Target1Location).GetSafeNormal();
	if (TargetDirection.IsNearlyZero())
	{
		// 如果两个目标位置相同，使用默认方向
		TargetDirection = FVector::ForwardVector;
	}

	// 计算垂直于目标连线的方向（相机围绕的轴）
	FVector PerpendicularAxis = FVector::CrossProduct(FVector::UpVector, TargetDirection).GetSafeNormal();
	if (PerpendicularAxis.IsNearlyZero())
	{
		// 如果目标连线是垂直的，使用默认方向
		PerpendicularAxis = FVector::RightVector;
	}

	// 计算另一个垂直方向（完成右手坐标系）
	FVector OtherPerpendicular = FVector::CrossProduct(TargetDirection, PerpendicularAxis).GetSafeNormal();

	// 将角度转换为弧度
	float ThetaRad = FMath::DegreesToRadians(Theta);
	float PhiRad = FMath::DegreesToRadians(Phi);

	// 在复曲面空间中计算相机位置
	// 水平圆上的位置
	FVector HorizontalOffset = 
		PerpendicularAxis * FMath::Cos(ThetaRad) * Radius +
		OtherPerpendicular * FMath::Sin(ThetaRad) * Radius;

	// 垂直偏移（考虑 Phi 角度）
	FVector VerticalOffset = FVector::UpVector * (Height + Radius * FMath::Sin(PhiRad));

	// 最终相机位置
	FVector CameraLocation = MidPoint + HorizontalOffset + VerticalOffset;

	return CameraLocation;
}

float UNamiToricSpaceFeature::Apply180DegreeRule(
	float Theta,
	const FVector& Target1Location,
	const FVector& Target2Location,
	float InFocusTarget) const
{
	// 计算聚焦点
	FVector FocusPoint = FMath::Lerp(Target1Location, Target2Location, InFocusTarget);

	// 计算目标连线方向
	FVector TargetDirection = (Target2Location - Target1Location).GetSafeNormal();
	if (TargetDirection.IsNearlyZero())
	{
		return Theta;
	}

	// 计算垂直于目标连线的方向
	FVector PerpendicularAxis = FVector::CrossProduct(FVector::UpVector, TargetDirection).GetSafeNormal();
	if (PerpendicularAxis.IsNearlyZero())
	{
		return Theta;
	}

	// 计算从聚焦点到目标1的方向
	FVector ToTarget1 = (Target1Location - FocusPoint).GetSafeNormal();
	if (ToTarget1.IsNearlyZero())
	{
		return Theta;
	}

	// 计算从聚焦点到目标2的方向
	FVector ToTarget2 = (Target2Location - FocusPoint).GetSafeNormal();
	if (ToTarget2.IsNearlyZero())
	{
		return Theta;
	}

	// 根据聚焦目标决定相机应该在"哪一侧"
	// 如果聚焦目标1，相机应该在目标1的"后方"（相对于目标2）
	// 如果聚焦目标2，相机应该在目标2的"后方"（相对于目标1）
	
	// 计算期望的相机方向（从聚焦点指向相机）
	FVector ExpectedCameraDirection;
	if (InFocusTarget < 0.5f)
	{
		// 聚焦目标1，相机应该在目标1的后方
		ExpectedCameraDirection = -ToTarget1;
	}
	else if (InFocusTarget > 0.5f)
	{
		// 聚焦目标2，相机应该在目标2的后方
		ExpectedCameraDirection = -ToTarget2;
	}
	else
	{
		// 聚焦中点，相机应该在垂直于连线的方向
		ExpectedCameraDirection = PerpendicularAxis;
	}

	// 计算当前 Theta 对应的相机方向
	float ThetaRad = FMath::DegreesToRadians(Theta);
	FVector OtherPerpendicular = FVector::CrossProduct(TargetDirection, PerpendicularAxis).GetSafeNormal();
	FVector CurrentCameraDirection = 
		PerpendicularAxis * FMath::Cos(ThetaRad) +
		OtherPerpendicular * FMath::Sin(ThetaRad);

	// 检查当前方向是否与期望方向一致（点积）
	float Dot = FVector::DotProduct(CurrentCameraDirection, ExpectedCameraDirection);

	// 如果点积为负，说明相机在"错误的一侧"，需要调整 Theta
	if (Dot < 0.0f)
	{
		// 调整到相反方向
		Theta = FMath::Fmod(Theta + 180.0f, 360.0f);
	}

	return Theta;
}

FVector UNamiToricSpaceFeature::CalculateFocusPoint(
	const FVector& Target1Location,
	const FVector& Target2Location) const
{
	// 根据 FocusTarget 参数计算聚焦点
	// 0.0 = 目标1，1.0 = 目标2，0.5 = 中点
	return FMath::Lerp(Target1Location, Target2Location, FocusTarget);
}

TArray<FNamiFollowTarget> UNamiToricSpaceFeature::GetAllValidTargets() const
{
	TArray<FNamiFollowTarget> Result;
	
	UNamiCameraModeBase* Mode = GetCameraMode();
	if (!IsValid(Mode))
	{
		return Result;
	}

	UNamiFollowCameraMode* FollowMode = Cast<UNamiFollowCameraMode>(Mode);
	if (!IsValid(FollowMode))
	{
		return Result;
	}

	// 获取所有有效目标
	for (const FNamiFollowTarget& T : FollowMode->GetTargets())
	{
		if (T.IsValid())
		{
			Result.Add(T);
		}
	}

	return Result;
}

FVector UNamiToricSpaceFeature::CalculateMultiTargetCenter(const TArray<FNamiFollowTarget>& Targets) const
{
	if (Targets.Num() == 0)
	{
		return FVector::ZeroVector;
	}

	if (bUseWeightedCenter)
	{
		// 加权中心
		FVector WeightedSum = FVector::ZeroVector;
		float TotalWeight = 0.0f;

		for (const FNamiFollowTarget& T : Targets)
		{
			if (T.IsValid())
			{
				FVector Location = T.GetLocation();
				WeightedSum += Location * T.Weight;
				TotalWeight += T.Weight;
			}
		}

		if (TotalWeight > KINDA_SMALL_NUMBER)
		{
			return WeightedSum / TotalWeight;
		}
	}
	else
	{
		// 简单平均
		FVector Sum = FVector::ZeroVector;
		int32 Count = 0;

		for (const FNamiFollowTarget& T : Targets)
		{
			if (T.IsValid())
			{
				Sum += T.GetLocation();
				Count++;
			}
		}

		if (Count > 0)
		{
			return Sum / static_cast<float>(Count);
		}
	}

	return FVector::ZeroVector;
}

float UNamiToricSpaceFeature::CalculateMultiTargetSize(const TArray<FNamiFollowTarget>& Targets) const
{
	if (Targets.Num() == 0)
	{
		return 0.0f;
	}

	if (Targets.Num() == 1)
	{
		return 100.0f; // 默认大小
	}

	// 计算边界框
	FVector MinBounds = Targets[0].GetLocation();
	FVector MaxBounds = MinBounds;

	for (const FNamiFollowTarget& T : Targets)
	{
		if (T.IsValid())
		{
			FVector Location = T.GetLocation();
			MinBounds = MinBounds.ComponentMin(Location);
			MaxBounds = MaxBounds.ComponentMax(Location);
		}
	}

	// 返回边界框的对角线长度（作为"大小"）
	FVector Size = MaxBounds - MinBounds;
	return Size.Size();
}

