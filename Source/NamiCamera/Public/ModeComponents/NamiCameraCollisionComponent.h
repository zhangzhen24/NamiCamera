// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModeComponents/NamiCameraModeComponent.h"
#include "Components/NamiSpringArm.h"
#include "NamiCameraCollisionComponent.generated.h"

/**
 * 相机碰撞组件
 *
 * 为相机模式添加碰撞检测功能：
 * - 碰撞检测（防止相机穿透墙壁）
 * - 相机滞后（平滑相机移动）
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Collision Component"))
class NAMICAMERA_API UNamiCameraCollisionComponent : public UNamiCameraModeComponent
{
	GENERATED_BODY()

public:
	UNamiCameraCollisionComponent();

	// ========== UNamiCameraModeComponent ==========
	virtual void Initialize_Implementation(UNamiCameraModeBase* InCameraMode) override;
	virtual void Activate_Implementation() override;
	virtual void ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime) override;

	// ========== 辅助函数 ==========

	/**
	 * 获取要忽略碰撞的 Actor 列表
	 * 默认返回相机 Owner
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Collision")
	TArray<AActor*> GetIgnoreActors() const;

public:
	// ========== 配置 ==========

	/**
	 * 弹簧臂配置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	FNamiSpringArm SpringArm;

protected:
	/** 是否已初始化 */
	bool bSpringArmInitialized = false;
};
