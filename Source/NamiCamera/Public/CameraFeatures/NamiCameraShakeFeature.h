// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CameraFeatures/NamiCameraFeature.h"
#include "Camera/CameraShakeBase.h"
#include "NamiCameraShakeFeature.generated.h"

/**
 * 相机抖动Feature
 * 直接激活 CameraShake 的相机抖动系统
 */
UCLASS(Blueprintable, BlueprintType)
class NAMICAMERA_API UNamiCameraShakeFeature : public UNamiCameraFeature
{
    GENERATED_BODY()

public:
    UNamiCameraShakeFeature();

    // ========== UNamiCameraFeature ==========
    virtual void Initialize_Implementation(UNamiCameraModeBase* InCameraMode) override;
    virtual void Activate_Implementation() override;
    virtual void Deactivate_Implementation() override;
    // ========== End UNamiCameraFeature ==========

    /** 启动相机抖动 */
    UFUNCTION(BlueprintCallable, Category = "Nami Camera|Shake")
    void StartShake();

    /** 停止相机抖动 */
    UFUNCTION(BlueprintCallable, Category = "Nami Camera|Shake")
    void StopShake(bool bImmediate = false);

    /** 检查抖动是否激活 */
    UFUNCTION(BlueprintPure, Category = "Nami Camera|Shake")
    bool IsShakeActive() const { return ActiveShakeInstance.IsValid(); }

    /** CameraShake 类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake Settings")
    TSubclassOf<UCameraShakeBase> CameraShake;

    /** 抖动强度缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake Settings", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float ShakeScale = 1.0f;

protected:

private:
    /** 当前激活的抖动实例 */
    TWeakObjectPtr<UCameraShakeBase> ActiveShakeInstance;

    /** 获取 PlayerCameraManager */
    class APlayerCameraManager* GetPlayerCameraManager() const;
};