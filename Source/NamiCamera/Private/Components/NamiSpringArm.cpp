// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/NamiSpringArm.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "Engine/World.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "DrawDebugHelpers.h"
#include "Libraries/NamiCameraMath.h"

//////////////////////////////////////////////////////////////////////////
// FNamiSpringArm

void FNamiSpringArm::ApplyRotationLag(FRotator &InOutDesiredRot, float DeltaTime)
{
	// 使用0-360度归一化，避免180/-180跳变问题
	FRotator NormalizedDesiredRot = FNamiCameraMath::NormalizeRotatorTo360(InOutDesiredRot);
	FRotator NormalizedPreviousRot = FNamiCameraMath::NormalizeRotatorTo360(PreviousDesiredRot);
	
	if (bUseCameraLagSubstepping && DeltaTime > CameraLagMaxTimeStep && CameraRotationLagSpeed > 0.f)
	{
		// 计算角度差值（使用0-360度范围，避免跳变）
		FRotator ArmRotStep;
		ArmRotStep.Pitch = FNamiCameraMath::FindDeltaAngle360(NormalizedPreviousRot.Pitch, NormalizedDesiredRot.Pitch) * (1.f / DeltaTime);
		ArmRotStep.Yaw = FNamiCameraMath::FindDeltaAngle360(NormalizedPreviousRot.Yaw, NormalizedDesiredRot.Yaw) * (1.f / DeltaTime);
		ArmRotStep.Roll = FNamiCameraMath::FindDeltaAngle360(NormalizedPreviousRot.Roll, NormalizedDesiredRot.Roll) * (1.f / DeltaTime);
		
		FRotator LerpTarget = NormalizedPreviousRot;
		float RemainingTime = DeltaTime;

		while (RemainingTime > KINDA_SMALL_NUMBER)
		{
			const float LerpAmount = FMath::Min(CameraLagMaxTimeStep, RemainingTime);
			// 优化：直接应用步进，最后归一化一次
			LerpTarget.Pitch = LerpTarget.Pitch + ArmRotStep.Pitch * LerpAmount;
			LerpTarget.Yaw = LerpTarget.Yaw + ArmRotStep.Yaw * LerpAmount;
			LerpTarget.Roll = LerpTarget.Roll + ArmRotStep.Roll * LerpAmount;
			// 确保 LerpTarget 在0-360度范围（只归一化一次）
			LerpTarget = FNamiCameraMath::NormalizeRotatorTo360(LerpTarget);
			RemainingTime -= LerpAmount;

			// 使用四元数插值（QInterpTo会自动处理最短路径）
			InOutDesiredRot = FRotator(FMath::QInterpTo(FQuat(NormalizedPreviousRot), FQuat(LerpTarget), LerpAmount, CameraRotationLagSpeed));
			NormalizedPreviousRot = FNamiCameraMath::NormalizeRotatorTo360(InOutDesiredRot);
		}
	}
	else
	{
		// 使用四元数插值（QInterpTo会自动处理最短路径）
		InOutDesiredRot = FRotator(FMath::QInterpTo(FQuat(NormalizedPreviousRot), FQuat(NormalizedDesiredRot), DeltaTime, CameraRotationLagSpeed));
	}

	// 保存归一化后的值，确保下一帧计算时使用0-360度范围
	PreviousDesiredRot = FNamiCameraMath::NormalizeRotatorTo360(InOutDesiredRot);
}

void FNamiSpringArm::ApplyLocationLag(FVector &InOutDesiredLoc, const FVector &ArmOrigin, UWorld *World, float DeltaTime)
{
	if (bUseCameraLagSubstepping && DeltaTime > CameraLagMaxTimeStep && CameraLagSpeed > 0.f)
	{
		const FVector ArmMovementStep = (InOutDesiredLoc - PreviousDesiredLoc) * (1.f / DeltaTime);
		FVector LerpTarget = PreviousDesiredLoc;

		float RemainingTime = DeltaTime;
		while (RemainingTime > KINDA_SMALL_NUMBER)
		{
			const float LerpAmount = FMath::Min(CameraLagMaxTimeStep, RemainingTime);
			LerpTarget += ArmMovementStep * LerpAmount;
			RemainingTime -= LerpAmount;

			InOutDesiredLoc = FMath::VInterpTo(PreviousDesiredLoc, LerpTarget, LerpAmount, CameraLagSpeed);
			PreviousDesiredLoc = InOutDesiredLoc;
		}
	}
	else
	{
		InOutDesiredLoc = FMath::VInterpTo(PreviousDesiredLoc, InOutDesiredLoc, DeltaTime, CameraLagSpeed);
	}

	// 应用最大距离限制
	bool bClampedDist = false;
	if (CameraLagMaxDistance > 0.f)
	{
		const FVector FromOrigin = InOutDesiredLoc - ArmOrigin;
		if (FromOrigin.SizeSquared() > FMath::Square(CameraLagMaxDistance))
		{
			InOutDesiredLoc = ArmOrigin + FromOrigin.GetClampedToMaxSize(CameraLagMaxDistance);
			bClampedDist = true;
		}
	}

	// 绘制调试标记
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDrawDebugLagMarkers && World)
	{
		DrawDebugSphere(World, ArmOrigin, 5.f, 8, FColor::Green);
		DrawDebugSphere(World, InOutDesiredLoc, 5.f, 8, FColor::Yellow);

		const FVector ToOrigin = ArmOrigin - InOutDesiredLoc;
		DrawDebugDirectionalArrow(World, InOutDesiredLoc, InOutDesiredLoc + ToOrigin * 0.5f, 7.5f, bClampedDist ? FColor::Red : FColor::Green);
		DrawDebugDirectionalArrow(World, InOutDesiredLoc + ToOrigin * 0.5f, ArmOrigin, 7.5f, bClampedDist ? FColor::Red : FColor::Green);
	}
#endif

	PreviousArmOrigin = ArmOrigin;
	PreviousDesiredLoc = InOutDesiredLoc;
}

FVector FNamiSpringArm::CalculateDesiredCameraLocation(const FVector &ArmOrigin, const FRotator &DesiredRot, const FVector &OffsetLocation) const
{
	FVector DesiredLoc = ArmOrigin;
	DesiredLoc -= DesiredRot.Vector() * SpringArmLength;
	DesiredLoc += FRotationMatrix(DesiredRot).TransformVector(OffsetLocation);
	return DesiredLoc;
}

FVector FNamiSpringArm::PerformCollisionTrace(const UWorld *World, const FVector &ArmOrigin, const FVector &DesiredLoc, const TArray<AActor *> &IgnoreActors)
{
	// 性能优化：早期退出条件
	if (!World || !bDoCollisionTest || SpringArmLength == 0.0f)
	{
		UnfixedCameraPosition = DesiredLoc;
		bIsCameraFixed = false;
		return DesiredLoc;
	}

	bIsCameraFixed = true;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpringArm), false);
	if (IgnoreActors.Num() > 0)
	{
		QueryParams.AddIgnoredActors(IgnoreActors);
	}

	FHitResult Result;
	World->SweepSingleByChannel(Result, ArmOrigin, DesiredLoc, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeSize), QueryParams);

	UnfixedCameraPosition = DesiredLoc;
	FVector ResultLoc = BlendLocations(DesiredLoc, Result.Location, Result.bBlockingHit);

	if (ResultLoc == DesiredLoc)
	{
		bIsCameraFixed = false;
	}

	return ResultLoc;
}

FVector FNamiSpringArm::PerformCollisionTrace(const UWorld *World, const FVector &ArmOrigin, const FVector &DesiredLoc, const TArray<const AActor *> &IgnoreActors)
{
	// 性能优化：早期退出条件
	if (!World || !bDoCollisionTest || SpringArmLength == 0.0f)
	{
		UnfixedCameraPosition = DesiredLoc;
		bIsCameraFixed = false;
		return DesiredLoc;
	}

	float CurrentTime = World->GetTimeSeconds();

	// 检查是否需要执行碰撞检测
	bool bShouldPerformTrace = true;

	// 频率控制
	if (CollisionCheckFrequency > 0.0f)
	{
		if (CurrentTime - LastCollisionCheckTime < CollisionCheckFrequency)
		{
			bShouldPerformTrace = false;
		}
	}

	// 缓存检查
	if (CollisionCacheTime > 0.0f)
	{
		if (CurrentTime < CollisionCacheExpireTime)
		{
			bShouldPerformTrace = false;
		}
	}

	FHitResult Result;
	bool bHitSomething = false;
	FVector TraceHitLocation = DesiredLoc;

	if (bShouldPerformTrace)
	{
		// 更新上次检查时间
		LastCollisionCheckTime = CurrentTime;

		bIsCameraFixed = true;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpringArm), false);
		QueryParams.AddIgnoredActors(IgnoreActors);

		// 注意：在UE 5.3中，FCollisionQueryParams不支持AddIgnoredObjectTypes方法
		// 移除碰撞过滤代码，使用默认碰撞检测行为

		// 执行碰撞检测
		World->SweepSingleByChannel(Result, ArmOrigin, DesiredLoc, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeSize), QueryParams);

		bHitSomething = Result.bBlockingHit;
		TraceHitLocation = Result.Location;

		// 绘制调试信息
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bDrawDebugCollision)
		{
			// 绘制射线
			DrawDebugLine(World, ArmOrigin, DesiredLoc, bHitSomething ? FColor::Red : FColor::Green, false, -1.0f, 0, 2.0f);
			
			// 绘制碰撞点
			if (bHitSomething)
			{
				DrawDebugSphere(World, TraceHitLocation, ProbeSize, 12, FColor::Red, false, -1.0f);
			}
			
			// 绘制探针球体
			DrawDebugSphere(World, DesiredLoc, ProbeSize, 12, FColor::Yellow, false, -1.0f);
		}
#endif

		// 缓存结果
		if (CollisionCacheTime > 0.0f)
		{
			CachedCollisionLocation = TraceHitLocation;
			bCachedCollisionHit = bHitSomething;
			CollisionCacheExpireTime = CurrentTime + CollisionCacheTime;
		}
	}
	else
	{
		// 使用缓存结果
		bHitSomething = bCachedCollisionHit;
		TraceHitLocation = CachedCollisionLocation;
	}

	UnfixedCameraPosition = DesiredLoc;
	FVector ResultLoc = BlendLocations(DesiredLoc, TraceHitLocation, bHitSomething);

	if (ResultLoc == DesiredLoc)
	{
		bIsCameraFixed = false;
	}

	return ResultLoc;
}

void FNamiSpringArm::UpdateCameraTransform(const FVector &FinalLocation, const FRotator &FinalRotation)
{
	CameraTransform.SetLocation(FinalLocation);
	CameraTransform.SetRotation(FinalRotation.Quaternion());
	StateIsValid = true;
}

void FNamiSpringArm::UpdateDesiredArmLocation(const UWorld *WorldContext, const TArray<const AActor *> &IgnoreActors, const FTransform &InitialTransform, const FVector OffsetLocation,
											  bool bDoTrace)
{
	FVector PivotLocation = InitialTransform.GetLocation();
	FRotator DesiredRot = InitialTransform.Rotator();

	ensureMsgf(!bDoTrace || WorldContext != nullptr, TEXT("World is required for spring arm to trace against"));
	const bool bShouldTrace = bDoTrace && WorldContext != nullptr;

	FVector ArmOrigin = PivotLocation;
	FVector DesiredLoc = CalculateDesiredCameraLocation(ArmOrigin, DesiredRot, OffsetLocation);

	FVector ResultLoc;
	if (bShouldTrace)
	{
		ResultLoc = PerformCollisionTrace(WorldContext, ArmOrigin, DesiredLoc, IgnoreActors);
	}
	else
	{
		ResultLoc = DesiredLoc;
		bIsCameraFixed = false;
		UnfixedCameraPosition = ResultLoc;
	}

	UpdateCameraTransform(ResultLoc, DesiredRot);
}

void FNamiSpringArm::UpdateDesiredArmLocation(const UObject *WorldContext, float DeltaTime, const TArray<AActor *> &IgnoreActors, const FTransform &InitialTransform,
											  const FVector OffsetLocation, bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag)
{
	UWorld *World = WorldContext ? WorldContext->GetWorld() : nullptr;

	FVector PivotLocation = InitialTransform.GetLocation();
	FRotator DesiredRot = InitialTransform.Rotator();

	ensureMsgf(!bDoTrace || WorldContext != nullptr, TEXT("World is required for spring arm to trace against"));
	const bool bShouldTrace = bDoTrace && WorldContext != nullptr;

	// 限制DeltaTime到物理系统最大值
	if (bClampToMaxPhysicsDeltaTime)
	{
		DeltaTime = FMath::Min(DeltaTime, UPhysicsSettings::Get()->MaxPhysicsDeltaTime);
	}

	// 应用旋转滞后
	if (bDoRotationLag)
	{
		ApplyRotationLag(DesiredRot, DeltaTime);
	}
	else
	{
		PreviousDesiredRot = DesiredRot;
	}

	// 计算Arm原点
	FVector ArmOrigin = PivotLocation;

	// 先计算期望的相机位置（基于ArmOrigin和旋转）
	FVector DesiredLoc = CalculateDesiredCameraLocation(ArmOrigin, DesiredRot, OffsetLocation);

	// 应用位置滞后（平滑期望的相机位置）
	if (bDoLocationLag)
	{
		ApplyLocationLag(DesiredLoc, ArmOrigin, World, DeltaTime);
	}
	else
	{
		PreviousArmOrigin = ArmOrigin;
		PreviousDesiredLoc = DesiredLoc;
	}

	// 执行碰撞检测
	FVector ResultLoc;
	if (bShouldTrace)
	{
		ResultLoc = PerformCollisionTrace(World, ArmOrigin, DesiredLoc, IgnoreActors);
	}
	else
	{
		ResultLoc = DesiredLoc;
		bIsCameraFixed = false;
		UnfixedCameraPosition = ResultLoc;
	}

	// 更新相机变换
	UpdateCameraTransform(ResultLoc, DesiredRot);
}

FVector FNamiSpringArm::BlendLocations(const FVector &DesiredArmLocation, const FVector &TraceHitLocation, bool bHitSomething)
{
	return bHitSomething ? TraceHitLocation : DesiredArmLocation;
}

FNamiSpringArm::FNamiSpringArm()
	: UnfixedCameraPosition(), PreviousDesiredLoc(), PreviousArmOrigin(), PreviousDesiredRot(), bEnableCameraLag(false), bEnableCameraRotationLag(false), bUseCameraLagSubstepping(true), bDrawDebugLagMarkers(0), CameraLagSpeed(10.f), CameraRotationLagSpeed(10.f), CameraLagMaxTimeStep(1.f / 60.f), CameraLagMaxDistance(0.f), bClampToMaxPhysicsDeltaTime(false), bDrawDebugCollision(false)
{
}

void FNamiSpringArm::Initialize()
{
	bIsCameraFixed = false;
	StateIsValid = false;

	// 初始化私有数据
	LastCollisionCheckTime = 0.0f;
	CachedCollisionLocation = FVector::ZeroVector;
	bCachedCollisionHit = false;
	CollisionCacheExpireTime = 0.0f;
	CollisionRecoveryVelocity = FVector::ZeroVector;
	CurrentCollisionRecoveryLocation = FVector::ZeroVector;
}

void FNamiSpringArm::Tick(const UObject *WorldContext, float DeltaTime, const AActor *IgnoreActor, const FTransform &InitialTransform, const FVector OffsetLocation)
{
	TArray<AActor *> IgnoreActorArray;
	if (IgnoreActor)
	{
		IgnoreActorArray.Add(const_cast<AActor *>(IgnoreActor));
	}
	Tick(WorldContext, DeltaTime, IgnoreActorArray, InitialTransform, OffsetLocation);
}

void FNamiSpringArm::Tick(const UObject *WorldContext, float DeltaTime, const TArray<AActor *> &IgnoreActors, const FTransform &InitialTransform, const FVector OffsetLocation)
{
	UpdateDesiredArmLocation(WorldContext, DeltaTime, IgnoreActors, InitialTransform, OffsetLocation, bDoCollisionTest, bEnableCameraLag, bEnableCameraRotationLag);
}

const FTransform &FNamiSpringArm::GetCameraTransform() const
{
	ensure(StateIsValid);
	return CameraTransform;
}

FVector FNamiSpringArm::GetUnfixedCameraPosition() const
{
	ensure(StateIsValid);
	return UnfixedCameraPosition;
}

bool FNamiSpringArm::IsCollisionFixApplied() const
{
	return bIsCameraFixed;
}
