// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/NamiSpringArm.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "Engine/World.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "DrawDebugHelpers.h"

//////////////////////////////////////////////////////////////////////////
// FNamiSpringArm

// 添加新的私有成员变量到FNamiSpringArm结构体
struct FNamiSpringArmPrivate
{
	/** 上次碰撞检测时间 */
	float LastCollisionCheckTime = 0.0f;

	/** 碰撞检测结果缓存 */
	FVector CachedCollisionLocation = FVector::ZeroVector;
	bool bCachedCollisionHit = false;
	float CollisionCacheExpireTime = 0.0f;

	/** 碰撞恢复平滑速度 */
	FVector CollisionRecoveryVelocity = FVector::ZeroVector;

	/** 当前碰撞恢复位置 */
	FVector CurrentCollisionRecoveryLocation = FVector::ZeroVector;
};

// 使用静态线程本地存储来保存私有数据
static thread_local TMap<const FNamiSpringArm *, FNamiSpringArmPrivate> SpringArmPrivateData;

FNamiSpringArmPrivate &GetPrivateData(const FNamiSpringArm *SpringArm)
{
	return SpringArmPrivateData.FindOrAdd(SpringArm);
}

void FNamiSpringArm::ApplyRotationLag(FRotator &InOutDesiredRot, float DeltaTime)
{
	if (bUseCameraLagSubstepping && DeltaTime > CameraLagMaxTimeStep && CameraRotationLagSpeed > 0.f)
	{
		const FRotator ArmRotStep = (InOutDesiredRot - PreviousDesiredRot).GetNormalized() * (1.f / DeltaTime);
		FRotator LerpTarget = PreviousDesiredRot;
		float RemainingTime = DeltaTime;

		while (RemainingTime > KINDA_SMALL_NUMBER)
		{
			const float LerpAmount = FMath::Min(CameraLagMaxTimeStep, RemainingTime);
			LerpTarget += ArmRotStep * LerpAmount;
			RemainingTime -= LerpAmount;

			InOutDesiredRot = FRotator(FMath::QInterpTo(FQuat(PreviousDesiredRot), FQuat(LerpTarget), LerpAmount, CameraRotationLagSpeed));
			PreviousDesiredRot = InOutDesiredRot;
		}
	}
	else
	{
		InOutDesiredRot = FRotator(FMath::QInterpTo(FQuat(PreviousDesiredRot), FQuat(InOutDesiredRot), DeltaTime, CameraRotationLagSpeed));
	}

	PreviousDesiredRot = InOutDesiredRot;
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
	DesiredLoc -= DesiredRot.Vector() * TargetArmLength;
	DesiredLoc += FRotationMatrix(DesiredRot).TransformVector(OffsetLocation);
	return DesiredLoc;
}

FVector FNamiSpringArm::PerformCollisionTrace(const UWorld *World, const FVector &ArmOrigin, const FVector &DesiredLoc, const TArray<AActor *> &IgnoreActors)
{
	if (!World || TargetArmLength == 0.0f)
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
	if (!World || TargetArmLength == 0.0f)
	{
		UnfixedCameraPosition = DesiredLoc;
		bIsCameraFixed = false;
		return DesiredLoc;
	}

	FNamiSpringArmPrivate &PrivateData = GetPrivateData(this);
	float CurrentTime = World->GetTimeSeconds();

	// 检查是否需要执行碰撞检测
	bool bShouldPerformTrace = true;

	// 频率控制
	if (CollisionCheckFrequency > 0.0f)
	{
		if (CurrentTime - PrivateData.LastCollisionCheckTime < CollisionCheckFrequency)
		{
			bShouldPerformTrace = false;
		}
	}

	// 缓存检查
	if (CollisionCacheTime > 0.0f)
	{
		if (CurrentTime < PrivateData.CollisionCacheExpireTime)
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
		PrivateData.LastCollisionCheckTime = CurrentTime;

		bIsCameraFixed = true;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpringArm), false);
		QueryParams.AddIgnoredActors(IgnoreActors);

		// 注意：在UE 5.3中，FCollisionQueryParams不支持AddIgnoredObjectTypes方法
		// 移除碰撞过滤代码，使用默认碰撞检测行为

		// 执行碰撞检测
		World->SweepSingleByChannel(Result, ArmOrigin, DesiredLoc, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeSize), QueryParams);

		bHitSomething = Result.bBlockingHit;
		TraceHitLocation = Result.Location;

		// 缓存结果
		if (CollisionCacheTime > 0.0f)
		{
			PrivateData.CachedCollisionLocation = TraceHitLocation;
			PrivateData.bCachedCollisionHit = bHitSomething;
			PrivateData.CollisionCacheExpireTime = CurrentTime + CollisionCacheTime;
		}
	}
	else
	{
		// 使用缓存结果
		bHitSomething = PrivateData.bCachedCollisionHit;
		TraceHitLocation = PrivateData.CachedCollisionLocation;
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
	: UnfixedCameraPosition(), PreviousDesiredLoc(), PreviousArmOrigin(), PreviousDesiredRot(), bEnableCameraLag(false), bEnableCameraRotationLag(false), bUseCameraLagSubstepping(true), bDrawDebugLagMarkers(0), CameraLagSpeed(10.f), CameraRotationLagSpeed(10.f), CameraLagMaxTimeStep(1.f / 60.f), CameraLagMaxDistance(0.f), bClampToMaxPhysicsDeltaTime(false)
{
}

void FNamiSpringArm::Initialize()
{
	bIsCameraFixed = false;
	StateIsValid = false;

	// 初始化私有数据
	FNamiSpringArmPrivate &PrivateData = GetPrivateData(this);
	PrivateData.LastCollisionCheckTime = 0.0f;
	PrivateData.CachedCollisionLocation = FVector::ZeroVector;
	PrivateData.bCachedCollisionHit = false;
	PrivateData.CollisionCacheExpireTime = 0.0f;
	PrivateData.CollisionRecoveryVelocity = FVector::ZeroVector;
	PrivateData.CurrentCollisionRecoveryLocation = FVector::ZeroVector;
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
