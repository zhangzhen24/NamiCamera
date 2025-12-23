// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NamiCameraModeComponent.h"
#include "Components/NamiSpringArm.h"
#include "NamiCameraSpringArmComponent.generated.h"

/**
 * 弹簧臂组件
 *
 * 为相机模式添加弹簧臂功能：
 * - 碰撞检测（防止相机穿透墙壁）
 * - 相机滞后（平滑相机移动）
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Spring Arm Component"))
class NAMICAMERA_API UNamiCameraSpringArmComponent : public UNamiCameraModeComponent
{
	GENERATED_BODY()

public:
	UNamiCameraSpringArmComponent();

	// ========== 生命周期 ==========

	virtual void Initialize_Implementation(UNamiCameraModeBase* InCameraMode) override;
	virtual void Activate_Implementation() override;
	virtual void ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime) override;

	// ========== 辅助函数 ==========

	/**
	 * 获取要忽略碰撞的 Actor 列表
	 * 默认返回相机 Owner
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Spring Arm")
	TArray<AActor*> GetIgnoreActors() const;

public:
	// ========== 配置 ==========

	/**
	 * 弹簧臂配置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spring Arm")
	FNamiSpringArm SpringArm;

protected:
	/** 是否已初始化 */
	bool bSpringArmInitialized = false;
};
