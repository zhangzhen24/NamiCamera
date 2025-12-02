// Copyright Epic Games, Inc. All Rights Reserved.

#include "Libraries/NamiCameraViewportMath.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiCameraViewportMath)

bool UNamiCameraViewportMath::WorldToScreenNormalized(
	const FVector& WorldLocation,
	const FVector& CameraLocation,
	const FRotator& CameraRotation,
	float FOV,
	float AspectRatio,
	FVector2D& OutScreenPosition)
{
	// 计算相机到目标的向量
	FVector ToTarget = WorldLocation - CameraLocation;

	// 获取相机坐标系
	FRotationMatrix RotMatrix(CameraRotation);
	FVector CameraForward = RotMatrix.GetUnitAxis(EAxis::X);
	FVector CameraRight = RotMatrix.GetUnitAxis(EAxis::Y);
	FVector CameraUp = RotMatrix.GetUnitAxis(EAxis::Z);

	// 检查是否在相机前方
	float ForwardDist = FVector::DotProduct(ToTarget, CameraForward);
	if (ForwardDist <= KINDA_SMALL_NUMBER)
	{
		OutScreenPosition = FVector2D(-1.0f, -1.0f);
		return false; // 在相机后方
	}

	// 计算在相机本地空间的坐标
	float RightDist = FVector::DotProduct(ToTarget, CameraRight);
	float UpDist = FVector::DotProduct(ToTarget, CameraUp);

	// 计算视锥体半角
	float HalfFOVRad = FMath::DegreesToRadians(FOV * 0.5f);
	float TanHalfFOV = FMath::Tan(HalfFOVRad);

	// 计算屏幕位置（归一化到 -1 到 1）
	float ScreenX = RightDist / (ForwardDist * TanHalfFOV * AspectRatio);
	float ScreenY = UpDist / (ForwardDist * TanHalfFOV);

	// 转换到 0-1 范围 (0,0 = 左上角, 1,1 = 右下角)
	OutScreenPosition.X = (ScreenX + 1.0f) * 0.5f;
	OutScreenPosition.Y = (1.0f - ScreenY) * 0.5f; // Y轴翻转

	// 检查是否在屏幕范围内
	return OutScreenPosition.X >= 0.0f && OutScreenPosition.X <= 1.0f &&
	       OutScreenPosition.Y >= 0.0f && OutScreenPosition.Y <= 1.0f;
}

void UNamiCameraViewportMath::ScreenToWorldRay(
	const FVector2D& ScreenPosition,
	const FVector& CameraLocation,
	const FRotator& CameraRotation,
	float FOV,
	float AspectRatio,
	FVector& OutRayOrigin,
	FVector& OutRayDirection)
{
	// 将屏幕位置从 0-1 转换到 -1 到 1
	float ScreenX = ScreenPosition.X * 2.0f - 1.0f;
	float ScreenY = 1.0f - ScreenPosition.Y * 2.0f; // Y轴翻转

	// 计算视锥体半角
	float HalfFOVRad = FMath::DegreesToRadians(FOV * 0.5f);
	float TanHalfFOV = FMath::Tan(HalfFOVRad);

	// 获取相机坐标系
	FRotationMatrix RotMatrix(CameraRotation);
	FVector CameraForward = RotMatrix.GetUnitAxis(EAxis::X);
	FVector CameraRight = RotMatrix.GetUnitAxis(EAxis::Y);
	FVector CameraUp = RotMatrix.GetUnitAxis(EAxis::Z);

	// 计算射线方向（在相机前方1单位处的点）
	OutRayDirection = CameraForward 
		+ CameraRight * (ScreenX * TanHalfFOV * AspectRatio)
		+ CameraUp * (ScreenY * TanHalfFOV);
	OutRayDirection.Normalize();

	OutRayOrigin = CameraLocation;
}

bool UNamiCameraViewportMath::CalculateCameraForSingleTarget(
	const FVector& TargetWorldLocation,
	const FVector2D& DesiredScreenPosition,
	float CameraDistance,
	float CameraHeight,
	float FOV,
	float AspectRatio,
	const FVector& BaseDirection,
	FVector& OutCameraLocation,
	FRotator& OutCameraRotation)
{
	// 将屏幕位置从 0-1 转换到 -1 到 1
	float ScreenX = DesiredScreenPosition.X * 2.0f - 1.0f;
	float ScreenY = 1.0f - DesiredScreenPosition.Y * 2.0f; // Y轴翻转

	// 计算视锥体半角
	float HalfFOVRad = FMath::DegreesToRadians(FOV * 0.5f);
	float TanHalfFOV = FMath::Tan(HalfFOVRad);

	// 使用基础方向作为相机朝向的参考
	FVector CameraForward = BaseDirection.GetSafeNormal();
	if (CameraForward.IsNearlyZero())
	{
		CameraForward = FVector::ForwardVector;
	}

	// 计算相机的右向量和上向量
	FVector WorldUp = FVector::UpVector;
	FVector CameraRight = FVector::CrossProduct(WorldUp, CameraForward).GetSafeNormal();
	if (CameraRight.IsNearlyZero())
	{
		CameraRight = FVector::RightVector;
	}
	FVector CameraUp = FVector::CrossProduct(CameraForward, CameraRight).GetSafeNormal();

	// 计算相机位置
	// 相机在目标后方，距离为 CameraDistance
	OutCameraLocation = TargetWorldLocation - CameraForward * CameraDistance;
	OutCameraLocation.Z += CameraHeight;

	// 根据期望的屏幕位置调整相机位置
	// 如果目标应该在屏幕左侧(ScreenX < 0)，相机需要向右偏移
	float HorizontalOffset = -ScreenX * TanHalfFOV * AspectRatio * CameraDistance;
	float VerticalOffset = -ScreenY * TanHalfFOV * CameraDistance;

	OutCameraLocation += CameraRight * HorizontalOffset;
	OutCameraLocation += CameraUp * VerticalOffset;

	// 计算相机朝向（看向目标）
	FVector ToTarget = TargetWorldLocation - OutCameraLocation;
	OutCameraRotation = ToTarget.Rotation();

	return true;
}

FNamiViewportConstraintResult UNamiCameraViewportMath::CalculateCameraForTwoTargets(
	const FVector& Target1Location,
	const FVector2D& Target1ScreenPosition,
	const FVector& Target2Location,
	const FVector2D& Target2ScreenPosition,
	float CameraHeight,
	FVector2D FOVRange,
	float AspectRatio)
{
	FNamiViewportConstraintResult Result;

	// 计算两个目标的中点
	FVector MidPoint = (Target1Location + Target2Location) * 0.5f;

	// 计算两个目标之间的距离
	float TargetDistance = FVector::Dist(Target1Location, Target2Location);
	if (TargetDistance < KINDA_SMALL_NUMBER)
	{
		// 两个目标位置相同，使用单目标算法
		Result.bSuccess = CalculateCameraForSingleTarget(
			Target1Location,
			(Target1ScreenPosition + Target2ScreenPosition) * 0.5f,
			500.0f,
			CameraHeight,
			(FOVRange.X + FOVRange.Y) * 0.5f,
			AspectRatio,
			FVector::ForwardVector,
			Result.CameraLocation,
			Result.CameraRotation);
		Result.FOV = (FOVRange.X + FOVRange.Y) * 0.5f;
		Result.ConstraintSatisfaction = Result.bSuccess ? 1.0f : 0.0f;
		return Result;
	}

	// 计算期望的屏幕距离（对角线）
	FVector2D ScreenDelta = Target2ScreenPosition - Target1ScreenPosition;
	float DesiredScreenDistance = ScreenDelta.Size();
	if (DesiredScreenDistance < KINDA_SMALL_NUMBER)
	{
		DesiredScreenDistance = 0.5f; // 默认屏幕距离
	}

	// 计算相机方向：垂直于两目标连线
	FVector Target1ToTarget2 = (Target2Location - Target1Location).GetSafeNormal();
	FVector CameraDirection = FVector::CrossProduct(FVector::UpVector, Target1ToTarget2).GetSafeNormal();
	
	// 确保相机方向有效
	if (CameraDirection.IsNearlyZero())
	{
		CameraDirection = FVector::BackwardVector;
	}

	// 根据屏幕位置决定相机应该在哪一侧
	// 如果Target1在屏幕左侧(x < 0.5)，Target2在右侧，相机应该在后方
	FVector2D ScreenCenter = (Target1ScreenPosition + Target2ScreenPosition) * 0.5f;
	if (ScreenCenter.X > 0.5f)
	{
		// 整体偏右，相机需要向左调整
		CameraDirection = -CameraDirection;
	}

	// 估算相机距离
	// 使用中间FOV开始计算
	float FOV = FMath::Clamp((FOVRange.X + FOVRange.Y) * 0.5f, FOVRange.X, FOVRange.Y);
	float HalfFOVRad = FMath::DegreesToRadians(FOV * 0.5f);
	float TanHalfFOV = FMath::Tan(HalfFOVRad);

	// 相机距离：使目标在屏幕上占据期望的距离
	float CameraDistance = TargetDistance / (DesiredScreenDistance * 2.0f * TanHalfFOV * AspectRatio);
	CameraDistance = FMath::Clamp(CameraDistance, 200.0f, 3000.0f);

	// 计算初始相机位置
	Result.CameraLocation = MidPoint - CameraDirection * CameraDistance;
	Result.CameraLocation.Z = MidPoint.Z + CameraHeight;

	// 根据期望的屏幕中心调整相机位置
	FVector2D DesiredCenter = (Target1ScreenPosition + Target2ScreenPosition) * 0.5f;
	FVector2D CenterOffset = FVector2D(0.5f, 0.5f) - DesiredCenter;

	// 获取相机坐标系进行偏移
	FVector ToMidPoint = MidPoint - Result.CameraLocation;
	FRotator TempRotation = ToMidPoint.Rotation();
	FRotationMatrix RotMatrix(TempRotation);
	FVector CameraRight = RotMatrix.GetUnitAxis(EAxis::Y);
	FVector CameraUp = RotMatrix.GetUnitAxis(EAxis::Z);

	// 应用中心偏移
	Result.CameraLocation += CameraRight * (CenterOffset.X * CameraDistance * TanHalfFOV * AspectRatio * 2.0f);
	Result.CameraLocation += CameraUp * (-CenterOffset.Y * CameraDistance * TanHalfFOV * 2.0f);

	// 计算最终相机朝向
	FVector FinalToMidPoint = MidPoint - Result.CameraLocation;
	Result.CameraRotation = FinalToMidPoint.Rotation();
	Result.FOV = FOV;
	Result.bSuccess = true;

	// 评估约束满足程度
	FVector2D ActualPos1, ActualPos2;
	bool bVisible1 = WorldToScreenNormalized(Target1Location, Result.CameraLocation, Result.CameraRotation, FOV, AspectRatio, ActualPos1);
	bool bVisible2 = WorldToScreenNormalized(Target2Location, Result.CameraLocation, Result.CameraRotation, FOV, AspectRatio, ActualPos2);

	if (bVisible1 && bVisible2)
	{
		float Error1 = FVector2D::Distance(ActualPos1, Target1ScreenPosition);
		float Error2 = FVector2D::Distance(ActualPos2, Target2ScreenPosition);
		Result.ConstraintSatisfaction = 1.0f - FMath::Clamp((Error1 + Error2) * 0.5f, 0.0f, 1.0f);
	}
	else
	{
		Result.ConstraintSatisfaction = 0.0f;
	}

	return Result;
}

FNamiViewportConstraintResult UNamiCameraViewportMath::CalculateCameraForMultipleTargets(
	const TArray<FNamiViewportTargetConstraint>& Constraints,
	const FVector& InitialCameraLocation,
	float InitialFOV,
	FVector2D CameraHeightRange,
	FVector2D FOVRange,
	float AspectRatio,
	int32 MaxIterations)
{
	FNamiViewportConstraintResult Result;
	Result.CameraLocation = InitialCameraLocation;
	Result.FOV = InitialFOV;

	if (Constraints.Num() == 0)
	{
		Result.bSuccess = false;
		return Result;
	}

	// 计算所有目标的中心点
	FVector TargetsCenter = CalculateWeightedCenter(Constraints);

	// 初始相机朝向
	FVector ToCenter = TargetsCenter - Result.CameraLocation;
	if (ToCenter.IsNearlyZero())
	{
		ToCenter = FVector::ForwardVector;
	}
	Result.CameraRotation = ToCenter.Rotation();

	// 迭代优化
	float StepSize = 50.0f;
	float BestQuality = 0.0f;
	FVector BestLocation = Result.CameraLocation;
	FRotator BestRotation = Result.CameraRotation;
	float BestFOV = Result.FOV;

	for (int32 Iteration = 0; Iteration < MaxIterations; ++Iteration)
	{
		// 评估当前质量
		float CurrentQuality = EvaluateFramingQuality(Constraints, Result.CameraLocation, Result.CameraRotation, Result.FOV, AspectRatio);

		if (CurrentQuality > BestQuality)
		{
			BestQuality = CurrentQuality;
			BestLocation = Result.CameraLocation;
			BestRotation = Result.CameraRotation;
			BestFOV = Result.FOV;
		}

		// 如果质量足够好，提前退出
		if (CurrentQuality > 0.95f)
		{
			break;
		}

		// 计算梯度并调整
		// 尝试不同方向的移动
		TArray<FVector> Directions = {
			FVector(1, 0, 0), FVector(-1, 0, 0),
			FVector(0, 1, 0), FVector(0, -1, 0),
			FVector(0, 0, 1), FVector(0, 0, -1)
		};

		FVector BestDirection = FVector::ZeroVector;
		float BestNewQuality = CurrentQuality;

		for (const FVector& Dir : Directions)
		{
			FVector TestLocation = Result.CameraLocation + Dir * StepSize;
			TestLocation.Z = FMath::Clamp(TestLocation.Z, TargetsCenter.Z + CameraHeightRange.X, TargetsCenter.Z + CameraHeightRange.Y);

			FVector TestToCenter = TargetsCenter - TestLocation;
			FRotator TestRotation = TestToCenter.IsNearlyZero() ? Result.CameraRotation : TestToCenter.Rotation();

			float TestQuality = EvaluateFramingQuality(Constraints, TestLocation, TestRotation, Result.FOV, AspectRatio);

			if (TestQuality > BestNewQuality)
			{
				BestNewQuality = TestQuality;
				BestDirection = Dir;
			}
		}

		// 应用最佳移动
		if (!BestDirection.IsNearlyZero())
		{
			Result.CameraLocation += BestDirection * StepSize;
			Result.CameraLocation.Z = FMath::Clamp(Result.CameraLocation.Z, TargetsCenter.Z + CameraHeightRange.X, TargetsCenter.Z + CameraHeightRange.Y);

			FVector NewToCenter = TargetsCenter - Result.CameraLocation;
			if (!NewToCenter.IsNearlyZero())
			{
				Result.CameraRotation = NewToCenter.Rotation();
			}
		}

		// 尝试调整FOV
		for (float FOVOffset : {-5.0f, 5.0f})
		{
			float TestFOV = FMath::Clamp(Result.FOV + FOVOffset, FOVRange.X, FOVRange.Y);
			float TestQuality = EvaluateFramingQuality(Constraints, Result.CameraLocation, Result.CameraRotation, TestFOV, AspectRatio);

			if (TestQuality > BestNewQuality)
			{
				BestNewQuality = TestQuality;
				Result.FOV = TestFOV;
			}
		}

		// 减小步长
		StepSize *= 0.95f;
		if (StepSize < 1.0f)
		{
			StepSize = 1.0f;
		}
	}

	// 使用最佳结果
	Result.CameraLocation = BestLocation;
	Result.CameraRotation = BestRotation;
	Result.FOV = BestFOV;
	Result.ConstraintSatisfaction = BestQuality;
	Result.bSuccess = BestQuality > 0.5f;

	return Result;
}

bool UNamiCameraViewportMath::IsTargetWithinBounds(
	const FVector& TargetLocation,
	const FVector& CameraLocation,
	const FRotator& CameraRotation,
	float FOV,
	float AspectRatio,
	const FNamiViewportBoundsConstraint& BoundsConstraint)
{
	FVector2D ScreenPosition;
	bool bVisible = WorldToScreenNormalized(TargetLocation, CameraLocation, CameraRotation, FOV, AspectRatio, ScreenPosition);

	if (!bVisible && BoundsConstraint.bMustBeVisible)
	{
		return false;
	}

	if (!bVisible)
	{
		return true; // 不要求必须可见，且不可见时认为满足约束
	}

	return ScreenPosition.X >= BoundsConstraint.MinScreenPosition.X &&
	       ScreenPosition.X <= BoundsConstraint.MaxScreenPosition.X &&
	       ScreenPosition.Y >= BoundsConstraint.MinScreenPosition.Y &&
	       ScreenPosition.Y <= BoundsConstraint.MaxScreenPosition.Y;
}

float UNamiCameraViewportMath::CalculateMinFOVForTargets(
	const TArray<FVector>& Targets,
	const FVector& CameraLocation,
	const FRotator& CameraRotation,
	FVector2D BoundsMin,
	FVector2D BoundsMax,
	float AspectRatio,
	FVector2D FOVRange)
{
	if (Targets.Num() == 0)
	{
		return FOVRange.X;
	}

	// 二分查找最小FOV
	float MinFOV = FOVRange.X;
	float MaxFOV = FOVRange.Y;

	for (int32 i = 0; i < 20; ++i) // 二分查找迭代
	{
		float MidFOV = (MinFOV + MaxFOV) * 0.5f;
		bool bAllVisible = true;

		for (const FVector& Target : Targets)
		{
			FVector2D ScreenPos;
			bool bVisible = WorldToScreenNormalized(Target, CameraLocation, CameraRotation, MidFOV, AspectRatio, ScreenPos);

			if (!bVisible || 
			    ScreenPos.X < BoundsMin.X || ScreenPos.X > BoundsMax.X ||
			    ScreenPos.Y < BoundsMin.Y || ScreenPos.Y > BoundsMax.Y)
			{
				bAllVisible = false;
				break;
			}
		}

		if (bAllVisible)
		{
			MaxFOV = MidFOV; // 可以用更小的FOV
		}
		else
		{
			MinFOV = MidFOV; // 需要更大的FOV
		}
	}

	return MaxFOV;
}

float UNamiCameraViewportMath::CalculateIdealCameraDistance(
	const TArray<FVector>& Targets,
	const FRotator& CameraRotation,
	float FOV,
	float AspectRatio,
	FVector2D ScreenPadding,
	FVector2D DistanceRange)
{
	if (Targets.Num() == 0)
	{
		return DistanceRange.X;
	}

	// 计算目标的包围盒
	FBox BoundingBox(ForceInit);
	for (const FVector& Target : Targets)
	{
		BoundingBox += Target;
	}

	FVector Center = BoundingBox.GetCenter();
	FVector Extent = BoundingBox.GetExtent();

	// 计算所需距离以包含所有目标
	float HalfFOVRad = FMath::DegreesToRadians(FOV * 0.5f);
	float TanHalfFOV = FMath::Tan(HalfFOVRad);

	// 考虑水平和垂直方向
	float HorizontalExtent = FMath::Max(Extent.X, Extent.Y);
	float VerticalExtent = Extent.Z;

	// 考虑屏幕边距
	float EffectiveScreenWidth = 1.0f - ScreenPadding.X * 2.0f;
	float EffectiveScreenHeight = 1.0f - ScreenPadding.Y * 2.0f;

	float HorizontalDistance = HorizontalExtent / (TanHalfFOV * AspectRatio * EffectiveScreenWidth);
	float VerticalDistance = VerticalExtent / (TanHalfFOV * EffectiveScreenHeight);

	float IdealDistance = FMath::Max(HorizontalDistance, VerticalDistance);
	return FMath::Clamp(IdealDistance, DistanceRange.X, DistanceRange.Y);
}

float UNamiCameraViewportMath::CalculateScreenDistance(
	const FVector& Location1,
	const FVector& Location2,
	const FVector& CameraLocation,
	const FRotator& CameraRotation,
	float FOV,
	float AspectRatio)
{
	FVector2D ScreenPos1, ScreenPos2;

	bool bVisible1 = WorldToScreenNormalized(Location1, CameraLocation, CameraRotation, FOV, AspectRatio, ScreenPos1);
	bool bVisible2 = WorldToScreenNormalized(Location2, CameraLocation, CameraRotation, FOV, AspectRatio, ScreenPos2);

	if (!bVisible1 || !bVisible2)
	{
		return -1.0f; // 至少一个不可见
	}

	return FVector2D::Distance(ScreenPos1, ScreenPos2);
}

FBox2D UNamiCameraViewportMath::CalculateScreenBoundingBox(
	const TArray<FVector>& Targets,
	const FVector& CameraLocation,
	const FRotator& CameraRotation,
	float FOV,
	float AspectRatio)
{
	FBox2D ScreenBox(ForceInit);

	for (const FVector& Target : Targets)
	{
		FVector2D ScreenPos;
		if (WorldToScreenNormalized(Target, CameraLocation, CameraRotation, FOV, AspectRatio, ScreenPos))
		{
			ScreenBox += ScreenPos;
		}
	}

	return ScreenBox;
}

FVector UNamiCameraViewportMath::CalculateTargetsCenter(const TArray<FVector>& Targets)
{
	if (Targets.Num() == 0)
	{
		return FVector::ZeroVector;
	}

	FVector Sum = FVector::ZeroVector;
	for (const FVector& Target : Targets)
	{
		Sum += Target;
	}

	return Sum / static_cast<float>(Targets.Num());
}

FVector UNamiCameraViewportMath::CalculateWeightedCenter(const TArray<FNamiViewportTargetConstraint>& Constraints)
{
	if (Constraints.Num() == 0)
	{
		return FVector::ZeroVector;
	}

	FVector WeightedSum = FVector::ZeroVector;
	float TotalWeight = 0.0f;

	for (const FNamiViewportTargetConstraint& Constraint : Constraints)
	{
		WeightedSum += Constraint.GetWorldLocation() * Constraint.Weight;
		TotalWeight += Constraint.Weight;
	}

	if (TotalWeight <= KINDA_SMALL_NUMBER)
	{
		return CalculateTargetsCenter(TArray<FVector>()); // 返回空
	}

	return WeightedSum / TotalWeight;
}

float UNamiCameraViewportMath::EvaluateFramingQuality(
	const TArray<FNamiViewportTargetConstraint>& Constraints,
	const FVector& CameraLocation,
	const FRotator& CameraRotation,
	float FOV,
	float AspectRatio)
{
	if (Constraints.Num() == 0)
	{
		return 0.0f;
	}

	float TotalError = 0.0f;
	float TotalWeight = 0.0f;

	for (const FNamiViewportTargetConstraint& Constraint : Constraints)
	{
		FVector2D ActualScreenPosition;
		bool bVisible = WorldToScreenNormalized(
			Constraint.GetWorldLocation(),
			CameraLocation,
			CameraRotation,
			FOV,
			AspectRatio,
			ActualScreenPosition);

		if (!bVisible)
		{
			if (Constraint.bHardConstraint)
			{
				return 0.0f; // 硬约束不可见，质量为0
			}
			TotalError += Constraint.Weight * 1.0f; // 最大误差
		}
		else
		{
			float Error = FVector2D::Distance(ActualScreenPosition, Constraint.DesiredScreenPosition);
			TotalError += Constraint.Weight * FMath::Min(Error, 1.0f);
		}

		TotalWeight += Constraint.Weight;
	}

	if (TotalWeight <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	// 返回质量分数 (0-1)
	return 1.0f - (TotalError / TotalWeight);
}

FRotator UNamiCameraViewportMath::CalculateLookAtRotation(
	const FVector& CameraLocation,
	const FVector& TargetLocation)
{
	FVector Direction = TargetLocation - CameraLocation;
	if (Direction.IsNearlyZero())
	{
		return FRotator::ZeroRotator;
	}
	return Direction.Rotation();
}

float UNamiCameraViewportMath::GetDefaultAspectRatio()
{
	// 尝试从游戏视口获取
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		if (ViewportSize.Y > 0.0f)
		{
			return ViewportSize.X / ViewportSize.Y;
		}
	}

	// 默认16:9
	return 16.0f / 9.0f;
}

