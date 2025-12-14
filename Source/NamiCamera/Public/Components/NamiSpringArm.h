// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "NamiSpringArm.generated.h"

/**
 * SpringArm
 */
USTRUCT(Blueprintable)
struct NAMICAMERA_API FNamiSpringArm
{
	GENERATED_BODY()

	
public:
	FNamiSpringArm();

	/** 重置动态状态 */
	void Initialize();

	/** 更新SpringArm */
	void Tick(const UObject* WorldContext, float DeltaTime, const AActor* IgnoreActor, const FTransform& InitialTransform, const FVector OffsetLocation);
	void Tick(const UObject* WorldContext, float DeltaTime, const TArray<AActor*>& IgnoreActors, const FTransform& InitialTransform, const FVector OffsetLocation);

	/** 返回当前相机Transform */
	const FTransform& GetCameraTransform() const;

	/** 弹簧臂长度（相机到Pivot的距离，无碰撞时） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera, 
		meta=( ClampMin="0.0", ClampMax="10000.0", UIMin="0.0", UIMax="10000.0"))
	float SpringArmLength = 0.0f;

	/** 查询探针大小（单位：Unreal单位） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision, meta=(EditCondition="bDoCollisionTest"))
	float ProbeSize = 12.0f;

	/** 查询探针的碰撞通道（默认为ECC_Camera） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision, meta=(EditCondition="bDoCollisionTest"))
	TEnumAsByte<ECollisionChannel> ProbeChannel = ECC_Camera;

	/** 如果为true，使用ProbeChannel和ProbeSize进行碰撞测试，防止相机穿透关卡 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision)
	bool bDoCollisionTest = true;

	/** 碰撞检测频率（0.0表示每帧检测，1.0表示每秒检测一次） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision, meta=(EditCondition="bDoCollisionTest", ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float CollisionCheckFrequency = 0.0f;

	/** 碰撞检测结果缓存时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision, meta=(EditCondition="bDoCollisionTest", ClampMin="0.0", ClampMax="0.5", UIMin="0.0", UIMax="0.5"))
	float CollisionCacheTime = 0.0f;

	/** 是否使用平滑过渡从碰撞位置恢复到期望位置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision, meta=(EditCondition="bDoCollisionTest", InlineEditConditionToggle))
	bool bEnableSmoothCollisionRecovery = true;

	/** 碰撞恢复平滑时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision, meta=(EditCondition="bDoCollisionTest && bEnableSmoothCollisionRecovery", ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float CollisionRecoverySmoothTime = 0.1f;

	/** 是否忽略静态物体 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision, meta=(EditCondition="bDoCollisionTest"))
	bool bIgnoreStaticObjects = false;

	/** 是否忽略动态物体 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision, meta=(EditCondition="bDoCollisionTest"))
	bool bIgnoreDynamicObjects = false;

	/** 是否从父组件继承Pitch？如果使用绝对旋转则无效 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraSettings)
	bool bInheritPitch = true;

	/** 是否从父组件继承Yaw？如果使用绝对旋转则无效 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraSettings)
	bool bInheritYaw = true;

	/** 是否从父组件继承Roll？如果使用绝对旋转则无效 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraSettings)
	bool bInheritRoll = true;

	/** 获取未应用碰撞测试位移的相机位置 */
	FVector GetUnfixedCameraPosition() const;

	/** 是否应用了碰撞测试位移？ */
	bool IsCollisionFixApplied() const;

private:
	/** 更新期望的Arm位置，如果进行了追踪则调用BlendLocations进行混合 */
	void UpdateDesiredArmLocation(const UWorld* WorldContext, const TArray<const AActor*>& IgnoreActors, const FTransform& InitialTransform, const FVector OffsetLocation,
	                              bool bDoTrace);

	/** 更新期望的Arm位置，如果进行了追踪则调用BlendLocations进行混合 */
	void UpdateDesiredArmLocation(const UObject* WorldContext, float DeltaTime, const TArray<AActor*>& IgnoreActors,
	                              const FTransform& InitialTransform, const FVector OffsetLocation, bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag);

	/** 允许子类混合追踪命中位置与期望的Arm位置；默认返回 bHitSomething ? TraceHitLocation : DesiredArmLocation */
	FVector BlendLocations(const FVector& DesiredArmLocation, const FVector& TraceHitLocation, bool bHitSomething);

	/** 应用旋转滞后 */
	void ApplyRotationLag(FRotator& InOutDesiredRot, float DeltaTime);

	/** 应用位置滞后 */
	void ApplyLocationLag(FVector& InOutDesiredLoc, const FVector& ArmOrigin, UWorld* World, float DeltaTime);

	/** 计算期望的相机位置（基于Pivot、旋转、偏移和Arm长度） */
	FVector CalculateDesiredCameraLocation(const FVector& ArmOrigin, const FRotator& DesiredRot, const FVector& OffsetLocation) const;

	/** 执行碰撞检测并返回最终位置 */
	FVector PerformCollisionTrace(const UWorld* World, const FVector& ArmOrigin, const FVector& DesiredLoc, const TArray<AActor*>& IgnoreActors);

	/** 执行碰撞检测并返回最终位置（使用const AActor*数组） */
	FVector PerformCollisionTrace(const UWorld* World, const FVector& ArmOrigin, const FVector& DesiredLoc, const TArray<const AActor*>& IgnoreActors);

	/** 更新相机变换 */
	void UpdateCameraTransform(const FVector& FinalLocation, const FRotator& FinalRotation);

	FTransform CameraTransform;

	/** 应用碰撞测试位移时的临时变量，用于通知是否正在应用以及应用了多少 */
	bool bIsCameraFixed = false;
	bool StateIsValid = false;
	FVector UnfixedCameraPosition;

#pragma region Camera Lag

public:
	/** 使用相机延迟时的临时变量，用于记录之前的相机位置 */
	FVector PreviousDesiredLoc;
	FVector PreviousArmOrigin;
	/** 延迟相机旋转的临时变量，用于之前的旋转 */
	FRotator PreviousDesiredRot;

	/** 如果为true，相机会滞后于目标位置以平滑其移动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Lag)
	uint32 bEnableCameraLag : 1;

	/** 如果为true，相机会滞后于目标旋转以平滑其移动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Lag)
	uint32 bEnableCameraRotationLag : 1;

	/** 如果bUseCameraLagSubstepping为true，子步进相机阻尼以便很好地处理波动的帧率（尽管这会带来成本） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag, AdvancedDisplay)
	uint32 bUseCameraLagSubstepping : 1;

	/** 如果为true且启用了相机位置延迟，在相机目标（绿色）和延迟位置（黄色）处绘制标记 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Lag)
	uint32 bDrawDebugLagMarkers : 1;

	/** 如果bEnableCameraLag为true，控制相机到达目标位置的速度。低值较慢（更多延迟），高值较快（更少延迟），而零是即时（无延迟） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Lag, meta=(editcondition="bEnableCameraLag", ClampMin="0.0", ClampMax="1000.0", UIMin = "0.0", UIMax = "1000.0"))
	float CameraLagSpeed;

	/** 如果bEnableCameraRotationLag为true，控制相机到达目标位置的速度。低值较慢（更多延迟），高值较快（更少延迟），而零是即时（无延迟） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Lag, meta=(editcondition = "bEnableCameraRotationLag", ClampMin="0.0", ClampMax="1000.0", UIMin = "0.0", UIMax = "1000.0"))
	float CameraRotationLagSpeed;

	/** 子步进相机延迟时使用的最大时间步长 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag, AdvancedDisplay,
		meta=(editcondition = "bUseCameraLagSubstepping", ClampMin="0.005", ClampMax="0.5", UIMin = "0.005", UIMax = "0.5"))
	float CameraLagMaxTimeStep;

	/** 相机目标可能滞后于当前位置的最大距离。如果设置为零，则不强制执行最大距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Lag, meta=(editcondition="bEnableCameraLag", ClampMin="0.0", UIMin = "0.0"))
	float CameraLagMaxDistance;

	/** 如果为true且视图目标正在使用物理模拟，则使用与物理系统相同的最大时间步长上限。防止在Chaos Physics内限制增量时间时出现相机抖动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag)
	uint32 bClampToMaxPhysicsDeltaTime : 1;

#pragma endregion

#pragma region Collision Debug

public:
	/** 是否绘制碰撞检测调试信息（射线、碰撞点等） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraCollision)
	uint32 bDrawDebugCollision : 1;

#pragma endregion

private:
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

