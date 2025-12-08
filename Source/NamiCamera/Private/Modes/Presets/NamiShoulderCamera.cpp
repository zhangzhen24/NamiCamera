// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modes/Presets/NamiShoulderCamera.h"
#include "Libraries/NamiCameraMath.h"
#include "Features/NamiSpringArmFeature.h"

UNamiShoulderCamera::UNamiShoulderCamera()
{
    // 肩部视角相机默认配置
    DefaultFOV = 85.0f;

    // 更小的俯仰角范围，适合肩部视角
    MinPitch = -89.0f;
    MaxPitch = 89.0f;

    // 更灵敏的鼠标控制
    MouseSensitivity = 1.2f;

    // 启用平滑，获得更流畅的体验
    bEnableSmoothing = true;
    CameraLocationSmoothIntensity = 0.15f;
    CameraRotationSmoothIntensity = 0.1f;

    // 初始化原始相机距离和FOV
    OriginalCameraDistance = 325.0f;  // 更紧凑的相机距离
    OriginalFOV = DefaultFOV;
}

void UNamiShoulderCamera::Initialize_Implementation(UNamiCameraComponent *InCameraComponent)
{
    Super::Initialize_Implementation(InCameraComponent);

    // 保存原始相机距离和FOV
    OriginalFOV = DefaultFOV;
    
    // 配置 SpringArmFeature 为肩部视角相机的默认值
    UNamiSpringArmFeature* SpringArmFeature = GetSpringArmFeature();
    if (SpringArmFeature)
    {
        SpringArmFeature->SpringArm.TargetArmLength = OriginalCameraDistance;  // 更紧凑的相机距离
    }
}

FNamiCameraView UNamiShoulderCamera::CalculateView_Implementation(float DeltaTime)
{
    // 1. 计算观察点（PivotLocation）
    FVector TargetPivot = CalculatePivotLocation();
    TargetPivot.Z += PivotHeightOffset;

    // 2. 获取控制旋转（相机旋转）
    FRotator ControlRotation = GetControlRotation();

    // 注意：鼠标灵敏度不应该应用在累积的控制旋转值上
    // 灵敏度应该在输入阶段应用，否则会导致旋转值不断放大
    // 移除这行代码，避免相机疯狂乱晃
    // ControlRotation.Pitch *= MouseSensitivity;
    // ControlRotation.Yaw *= MouseSensitivity;

    // 应用Pitch限制
    if (bLimitPitch)
    {
        ControlRotation.Pitch = FMath::ClampAngle(ControlRotation.Pitch, MinPitch, MaxPitch);
    }

    // 应用Yaw限制
    if (bLimitYaw)
    {
        ControlRotation.Yaw = FMath::ClampAngle(ControlRotation.Yaw, MinYaw, MaxYaw);
    }

    // 锁定Roll旋转
    if (bLockRoll)
    {
        ControlRotation.Roll = 0.0f;
    }

    // 使用0-360度归一化，避免180/-180跳变问题
    ControlRotation = FNamiCameraMath::NormalizeRotatorTo360(ControlRotation);

    // 3. 应用肩部偏移
    FVector ShoulderOffsetVector = FVector::ZeroVector;
    if (bUseCharacterFacing)
    {
        // 获取角色朝向
        FRotator CharacterRotation = GetPrimaryTarget()->GetActorRotation();
        // 计算肩部偏移方向（基于角色朝向）
        ShoulderOffsetVector = FRotationMatrix(CharacterRotation).GetUnitAxis(EAxis::Y) * ShoulderOffset;
    }
    else
    {
        // 基于相机朝向的肩部偏移
        ShoulderOffsetVector = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y) * ShoulderOffset;
    }

    // 4. 应用相机距离缩放到 SpringArmFeature
    UNamiSpringArmFeature* SpringArmFeature = GetSpringArmFeature();
    if (SpringArmFeature)
    {
        float ScaledArmLength = OriginalCameraDistance * CameraDistanceScale;
        SpringArmFeature->SpringArm.TargetArmLength = ScaledArmLength;
    }

    // 5. 创建基础视图（应用肩部偏移到 PivotLocation）
        FNamiCameraView View;
    View.PivotLocation = TargetPivot + ShoulderOffsetVector;
    View.CameraLocation = TargetPivot + ShoulderOffsetVector;  // 初始位置，将由 SpringArmFeature 修改
        View.CameraRotation = ControlRotation;
        View.ControlLocation = TargetPivot + ShoulderOffsetVector;
        View.ControlRotation = ControlRotation;
        View.FOV = DefaultFOV;

    // 6. 调用父类实现，让 Feature 系统处理（包括 SpringArmFeature）
    View = Super::CalculateView_Implementation(DeltaTime);

    // 7. 恢复原始 ArmLength（避免影响下一帧）
    if (SpringArmFeature)
    {
        SpringArmFeature->SpringArm.TargetArmLength = OriginalCameraDistance;
    }

    // 使用0-360度归一化，避免180/-180跳变问题
    View.CameraRotation = FNamiCameraMath::NormalizeRotatorTo360(View.CameraRotation);
    View.ControlRotation = FNamiCameraMath::NormalizeRotatorTo360(View.ControlRotation);

    LastCameraRotation = View.CameraRotation;

        return View;
}