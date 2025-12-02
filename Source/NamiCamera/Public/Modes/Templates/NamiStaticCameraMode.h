// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modes/NamiCameraModeBase.h"
#include "NamiStaticCameraMode.generated.h"

/**
 * 静态相机模式模板
 * 
 * 用途：
 * - 过场动画中的固定镜头
 * - 固定视角房间（如恐怖游戏）
 * - 沿预设轨道移动的镜头
 * - 观察点相机
 * 
 * 特点：
 * - 相机位置不随角色移动而改变（除非手动设置）
 * - 可以注视某个目标（LookAt）
 * - 可以设置固定位置或使用轨道Feature
 * 
 * 继承指南：
 * - 如果只需要简单的固定镜头，直接使用此类
 * - 如果需要轨道运动，添加 RailFeature
 * - 如果需要复杂的注视逻辑，添加 LookAtFeature
 */
UCLASS(Blueprintable, BlueprintType)
class NAMICAMERA_API UNamiStaticCameraMode : public UNamiCameraModeBase
{
	GENERATED_BODY()

public:
	UNamiStaticCameraMode();

	// ========== 生命周期 ==========
	virtual void Activate_Implementation() override;
	virtual FNamiCameraView CalculateView_Implementation(float DeltaTime) override;

	// ========== 位置设置 ==========

	/**
	 * 设置相机位置
	 */
	UFUNCTION(BlueprintCallable, Category = "Static Camera")
	void SetCameraLocation(const FVector& Location);

	/**
	 * 设置相机旋转
	 */
	UFUNCTION(BlueprintCallable, Category = "Static Camera")
	void SetCameraRotation(const FRotator& Rotation);

	/**
	 * 设置相机变换
	 */
	UFUNCTION(BlueprintCallable, Category = "Static Camera")
	void SetCameraTransform(const FTransform& Transform);

	/**
	 * 从Actor获取变换
	 */
	UFUNCTION(BlueprintCallable, Category = "Static Camera")
	void SetCameraFromActor(AActor* Actor);

	// ========== 注视目标 ==========

	/**
	 * 设置注视目标
	 */
	UFUNCTION(BlueprintCallable, Category = "Static Camera|LookAt")
	void SetLookAtTarget(AActor* Target);

	/**
	 * 设置注视位置
	 */
	UFUNCTION(BlueprintCallable, Category = "Static Camera|LookAt")
	void SetLookAtLocation(const FVector& Location);

	/**
	 * 清除注视目标
	 */
	UFUNCTION(BlueprintCallable, Category = "Static Camera|LookAt")
	void ClearLookAtTarget();

public:
	// ========== 配置 ==========

	/** 相机位置（世界坐标） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Static Camera")
	FVector CameraLocation = FVector::ZeroVector;

	/** 相机旋转（如果没有LookAt目标） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Static Camera")
	FRotator CameraRotation = FRotator::ZeroRotator;

	/** 注视目标Actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Static Camera|LookAt")
	TWeakObjectPtr<AActor> LookAtTarget;

	/** 注视位置（如果没有LookAtTarget） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Static Camera|LookAt")
	FVector LookAtLocation = FVector::ZeroVector;

	/** 是否使用注视目标 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Static Camera|LookAt")
	bool bUseLookAt = false;

	/** 注视平滑时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Static Camera|LookAt",
		meta = (EditCondition = "bUseLookAt", ClampMin = "0.0", ClampMax = "2.0"))
	float LookAtSmoothTime = 0.15f;

protected:
	/**
	 * 计算注视旋转
	 */
	FRotator CalculateLookAtRotation() const;

	/** 当前平滑后的旋转 */
	FRotator SmoothedRotation;
	float YawVelocity = 0.0f;
	float PitchVelocity = 0.0f;
};

