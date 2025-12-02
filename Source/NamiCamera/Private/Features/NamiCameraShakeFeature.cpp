// Copyright Epic Games, Inc. All Rights Reserved.

#include "Features/NamiCameraShakeFeature.h"
#include "Math/UnrealMathUtility.h"

UNamiCameraShakeFeature::UNamiCameraShakeFeature()
{
    // 设置默认优先级，确保抖动Feature在其他Feature之后应用
    Priority = 10;
}

void UNamiCameraShakeFeature::Initialize_Implementation(UNamiCameraModeBase *InCameraMode)
{
    Super::Initialize_Implementation(InCameraMode);

    // 初始化损伤值和抖动参数
    CurrentDamageValue = 0.0f;
    ShakeIntensity = 0.0f;
    TranslationShakeTime = 0.0f;
    RotationShakeTime = 0.0f;
}

void UNamiCameraShakeFeature::Update_Implementation(float DeltaTime)
{
    Super::Update_Implementation(DeltaTime);

    // 更新损伤值
    UpdateDamageValue(DeltaTime);

    // 更新抖动时间
    TranslationShakeTime += DeltaTime;
    RotationShakeTime += DeltaTime;

    // 计算当前抖动强度
    ShakeIntensity = CalculateShakeIntensity();
}

void UNamiCameraShakeFeature::ApplyToView_Implementation(FNamiCameraView &InOutView, float DeltaTime)
{
    Super::ApplyToView_Implementation(InOutView, DeltaTime);

    // 只有当抖动强度大于0时才应用抖动
    if (ShakeIntensity > 0.0f)
    {
        // 生成并应用位移抖动
        if (bEnableTranslationShake)
        {
            FVector TranslationShake = GenerateTranslationShake(DeltaTime);
            InOutView.CameraLocation += TranslationShake;
            InOutView.ControlLocation += TranslationShake;
        }

        // 生成并应用旋转抖动
        if (bEnableRotationShake)
        {
            FRotator RotationShake = GenerateRotationShake(DeltaTime);
            InOutView.CameraRotation += RotationShake;
            InOutView.ControlRotation += RotationShake;
        }
    }
}

void UNamiCameraShakeFeature::AddDamageValue(float DamageValue)
{
    // 只添加正数损伤值
    if (DamageValue > 0.0f)
    {
        CurrentDamageValue += DamageValue;
        // 限制最大损伤值
        CurrentDamageValue = FMath::Min(CurrentDamageValue, 1.0f);
    }
}

void UNamiCameraShakeFeature::SetDamageValue(float DamageValue)
{
    // 确保损伤值在[0, 1]范围内
    CurrentDamageValue = FMath::Clamp(DamageValue, 0.0f, 1.0f);
}

void UNamiCameraShakeFeature::UpdateDamageValue(float DeltaTime)
{
    // 损伤值随时间衰减
    if (CurrentDamageValue > 0.0f)
    {
        CurrentDamageValue -= DamageDecayRate * DeltaTime;
        // 确保损伤值不小于0
        CurrentDamageValue = FMath::Max(CurrentDamageValue, 0.0f);
    }
}

float UNamiCameraShakeFeature::CalculateShakeIntensity() const
{
    // 使用指数曲线计算抖动强度，获得更好的视觉效果
    // 当损伤值为0时，抖动强度为0
    // 当损伤值为1时，抖动强度为1
    // 中间值使用指数曲线，使抖动感觉更自然
    return FMath::Pow(CurrentDamageValue, ShakeIntensityExponent);
}

FVector UNamiCameraShakeFeature::GenerateTranslationShake(float DeltaTime) const
{
    // 使用正弦函数生成平滑的位移抖动
    // 不同轴使用不同的频率，产生更自然的抖动效果
    float Intensity = ShakeIntensity * MaxTranslationShake;

    float X = FMath::Sin(TranslationShakeTime * TranslationShakeFrequency * 2.0f * PI) * Intensity;
    float Y = FMath::Sin(TranslationShakeTime * TranslationShakeFrequency * 3.0f * PI + 1.0f) * Intensity;
    float Z = FMath::Sin(TranslationShakeTime * TranslationShakeFrequency * 1.5f * PI + 2.0f) * Intensity;

    return FVector(X, Y, Z);
}

FRotator UNamiCameraShakeFeature::GenerateRotationShake(float DeltaTime) const
{
    // 使用正弦函数生成平滑的旋转抖动
    float Intensity = ShakeIntensity * MaxRotationShake;

    float Pitch = FMath::Sin(RotationShakeTime * RotationShakeFrequency * 2.5f * PI) * Intensity;
    float Yaw = FMath::Sin(RotationShakeTime * RotationShakeFrequency * 1.8f * PI + 1.5f) * Intensity;
    float Roll = FMath::Sin(RotationShakeTime * RotationShakeFrequency * 3.2f * PI + 0.5f) * Intensity;

    return FRotator(Pitch, Yaw, Roll);
}