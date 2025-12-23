// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "ModeComponents/NamiCameraModeComponent.h"
#include "NamiCameraShakeComponent.generated.h"

/**
 * 相机震动组件
 *
 * 直接激活 CameraShake 的相机抖动系统
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Shake Component"))
class NAMICAMERA_API UNamiCameraShakeComponent : public UNamiCameraModeComponent
{
	GENERATED_BODY()

public:
	UNamiCameraShakeComponent();

	// ========== UNamiCameraModeComponent ==========
	virtual void Initialize_Implementation(UNamiCameraModeBase* InCameraMode) override;
	virtual void Activate_Implementation() override;
	virtual void Deactivate_Implementation() override;

	// ========== 震动控制 ==========

	/** 启动相机抖动 */
	UFUNCTION(BlueprintCallable, Category = "Camera Shake")
	void StartShake();

	/** 停止相机抖动 */
	UFUNCTION(BlueprintCallable, Category = "Camera Shake")
	void StopShake(bool bImmediate = false);

	/** 检查抖动是否激活 */
	UFUNCTION(BlueprintPure, Category = "Camera Shake")
	bool IsShakeActive() const { return ActiveShakeInstance.IsValid(); }

public:
	// ========== 配置 ==========

	/** CameraShake 类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake")
	TSubclassOf<UCameraShakeBase> CameraShake;

	/** 抖动强度缩放 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float ShakeScale = 1.0f;

private:
	/** 当前激活的抖动实例 */
	TWeakObjectPtr<UCameraShakeBase> ActiveShakeInstance;

	/** 获取 PlayerCameraManager */
	class APlayerCameraManager* GetPlayerCameraManager() const;
};
