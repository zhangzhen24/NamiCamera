// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Features/NamiCameraFeature.h"
#include "NamiCameraShakeFeature.generated.h"

/**
 * 相机抖动Feature
 * 基于损伤值的相机抖动系统，支持位移和旋转抖动
 */
UCLASS(Blueprintable, BlueprintType)
class NAMICAMERA_API UNamiCameraShakeFeature : public UNamiCameraFeature
{
    GENERATED_BODY()

public:
    UNamiCameraShakeFeature();

    // ========== UNamiCameraFeature ==========
    virtual void Initialize_Implementation(UNamiCameraModeBase* InCameraMode) override;
    virtual void Update_Implementation(float DeltaTime) override;
    virtual void ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime) override;
    // ========== End UNamiCameraFeature ==========

    /** 添加损伤值 */
    UFUNCTION(BlueprintCallable, Category = "Nami Camera|Shake")
    void AddDamageValue(float DamageValue);

    /** 设置损伤值 */
    UFUNCTION(BlueprintCallable, Category = "Nami Camera|Shake")
    void SetDamageValue(float DamageValue);

    /** 获取当前损伤值 */
    UFUNCTION(BlueprintPure, Category = "Nami Camera|Shake")
    float GetDamageValue() const { return CurrentDamageValue; }

    /** 获取当前抖动强度 */
    UFUNCTION(BlueprintPure, Category = "Nami Camera|Shake")
    float GetShakeIntensity() const { return ShakeIntensity; }

protected:
    /** 损伤值衰减速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake Settings", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float DamageDecayRate = 1.0f;

    /** 抖动强度曲线指数（2.0=平方，3.0=立方） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake Settings", meta = (ClampMin = "1.0", ClampMax = "5.0"))
    float ShakeIntensityExponent = 2.0f;

    /** 最大位移抖动幅度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake Settings", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float MaxTranslationShake = 10.0f;

    /** 最大旋转抖动幅度（度） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake Settings", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float MaxRotationShake = 1.0f;

    /** 是否启用位移抖动 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake Settings")
    bool bEnableTranslationShake = true;

    /** 是否启用旋转抖动 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake Settings")
    bool bEnableRotationShake = true;

    /** 位移抖动频率（Hz） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake Settings", meta = (ClampMin = "0.1", ClampMax = "20.0"))
    float TranslationShakeFrequency = 5.0f;

    /** 旋转抖动频率（Hz） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shake Settings", meta = (ClampMin = "0.1", ClampMax = "20.0"))
    float RotationShakeFrequency = 5.0f;

private:
    /** 当前损伤值 */
    float CurrentDamageValue = 0.0f;

    /** 当前抖动强度 */
    float ShakeIntensity = 0.0f;

    /** 位移抖动时间偏移 */
    float TranslationShakeTime = 0.0f;

    /** 旋转抖动时间偏移 */
    float RotationShakeTime = 0.0f;

    /** 更新损伤值 */
    void UpdateDamageValue(float DeltaTime);

    /** 计算抖动强度 */
    float CalculateShakeIntensity() const;

    /** 生成位移抖动 */
    FVector GenerateTranslationShake(float DeltaTime) const;

    /** 生成旋转抖动 */
    FRotator GenerateRotationShake(float DeltaTime) const;
};