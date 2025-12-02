// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modes/Presets/NamiShoulderCamera.h"
#include "Libraries/NamiCameraMath.h"

UNamiShoulderCamera::UNamiShoulderCamera()
{
    // 肩部视角相机默认配置
    DefaultFOV = 85.0f;

    // 更紧凑的相机距离
    SpringArm.TargetArmLength = 325.0f;

    // 更小的俯仰角范围，适合肩部视角
    MinPitch = -89.0f;
    MaxPitch = 89.0f;

    // 更灵敏的鼠标控制
    MouseSensitivity = 1.2f;

    // 启用平滑，获得更流畅的体验
    bEnableSmoothing = true;
    PivotSmoothIntensity = 0.0f;
    CameraLocationSmoothIntensity = 0.15f;
    CameraRotationSmoothIntensity = 0.1f;

    // 初始化原始相机距离和FOV
    OriginalCameraDistance = SpringArm.TargetArmLength;
    OriginalFOV = DefaultFOV;
}

void UNamiShoulderCamera::Initialize_Implementation(UNamiCameraComponent *InCameraComponent)
{
    Super::Initialize_Implementation(InCameraComponent);

    // 保存原始相机距离和FOV
    OriginalCameraDistance = SpringArm.TargetArmLength;
    OriginalFOV = DefaultFOV;
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

    // 4. 准备 SpringArm 的输入
    // 应用相机距离缩放因子
    float ScaledArmLength = SpringArm.TargetArmLength * CameraDistanceScale;
    SpringArm.TargetArmLength = ScaledArmLength;

    // 应用肩部偏移到初始变换
    FTransform InitialTransform(ControlRotation, TargetPivot + ShoulderOffsetVector);
    FVector OffsetLocation = FVector::ZeroVector;

    // 5. 准备忽略的Actor列表
    TArray<AActor *> IgnoreActors;
    AActor *PrimaryTarget = GetPrimaryTarget();
    if (IsValid(PrimaryTarget))
    {
        IgnoreActors.Add(PrimaryTarget);
        // 忽略附加的Actor
        PrimaryTarget->ForEachAttachedActors([&IgnoreActors](AActor *Actor)
                                             { IgnoreActors.AddUnique(Actor); return true; });
    }

    // 6. 调用 SpringArm.Tick 计算相机位置
    UWorld *World = GetWorld();
    if (IsValid(World))
    {
        SpringArm.Tick(World, DeltaTime, IgnoreActors, InitialTransform, OffsetLocation);

        // 7. 从 SpringArm 获取最终相机Transform
        FTransform CameraTransform = SpringArm.GetCameraTransform();
        FRotator SpringArmRotation = CameraTransform.Rotator();

        // 8. 创建 View
        FNamiCameraView View;
        View.PivotLocation = TargetPivot;
        View.CameraLocation = CameraTransform.GetLocation();
        View.CameraRotation = SpringArmRotation;
        View.ControlLocation = CameraTransform.GetLocation();
        View.ControlRotation = SpringArmRotation;
        View.FOV = DefaultFOV;

        // 9. 应用平滑（使用父类的平滑逻辑）
        if (!bInitialized)
        {
            CurrentPivotLocation = View.PivotLocation;
            CurrentCameraLocation = View.CameraLocation;
            CurrentCameraRotation = View.CameraRotation;
            bInitialized = true;
        }
        else if (DeltaTime > 0.0f)
        {
            // 平滑枢轴点
            if (bEnableSmoothing && bEnablePivotSmoothing && PivotSmoothIntensity > 0.0f)
            {
                // 将平滑强度映射到实际平滑时间
                float SmoothTime = FNamiCameraMath::MapSmoothIntensity(PivotSmoothIntensity);
                CurrentPivotLocation = FNamiCameraMath::SmoothDamp(
                    CurrentPivotLocation,
                    View.PivotLocation,
                    PivotVelocity,
                    SmoothTime,
                    DeltaTime);
            }
            else
            {
                CurrentPivotLocation = View.PivotLocation;
            }

            // 平滑相机位置
            if (bEnableSmoothing && bEnableCameraLocationSmoothing && CameraLocationSmoothIntensity > 0.0f)
            {
                // 将平滑强度映射到实际平滑时间
                float SmoothTime = FNamiCameraMath::MapSmoothIntensity(CameraLocationSmoothIntensity);
                CurrentCameraLocation = FNamiCameraMath::SmoothDamp(
                    CurrentCameraLocation,
                    View.CameraLocation,
                    CameraVelocity,
                    SmoothTime,
                    DeltaTime);
            }
            else
            {
                CurrentCameraLocation = View.CameraLocation;
            }

            // 平滑旋转
            if (bEnableSmoothing && bEnableCameraRotationSmoothing && CameraRotationSmoothIntensity > 0.0f)
            {
                // 将平滑强度映射到实际平滑时间
                float SmoothTime = FNamiCameraMath::MapSmoothIntensity(CameraRotationSmoothIntensity);
                CurrentCameraRotation = FNamiCameraMath::SmoothDampRotator(
                    CurrentCameraRotation,
                    View.CameraRotation,
                    YawVelocity,
                    PitchVelocity,
                    SmoothTime,
                    SmoothTime,
                    DeltaTime);
            }
            else
            {
                CurrentCameraRotation = View.CameraRotation;
            }
        }

        // 10. 使用平滑后的值
        View.PivotLocation = CurrentPivotLocation;
        View.CameraLocation = CurrentCameraLocation;
        View.CameraRotation = CurrentCameraRotation;
        View.ControlLocation = CurrentCameraLocation;
        View.ControlRotation = CurrentCameraRotation;

        LastCameraRotation = View.CameraRotation;

        return View;
    }
    else
    {
        // World 无效时的后备方案：直接使用 PivotLocation 作为相机位置
        FNamiCameraView View;
        View.PivotLocation = TargetPivot;
        View.CameraLocation = TargetPivot + ShoulderOffsetVector;
        View.CameraRotation = ControlRotation;
        View.ControlLocation = TargetPivot + ShoulderOffsetVector;
        View.ControlRotation = ControlRotation;
        View.FOV = DefaultFOV;
        return View;
    }
}