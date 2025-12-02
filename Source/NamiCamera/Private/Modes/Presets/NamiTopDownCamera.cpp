// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modes/Presets/NamiTopDownCamera.h"
#include "Libraries/NamiCameraMath.h"

UNamiTopDownCamera::UNamiTopDownCamera()
{
    // 俯视角相机默认配置
    DefaultFOV = 90.0f;

    // 更高的相机距离
    SpringArm.TargetArmLength = 800.0f;

    // 固定俯视角
    MinPitch = -89.0f;
    MaxPitch = -10.0f; // 限制为俯视角度

    // 禁用俯仰角限制，因为俯视角相机使用自己的角度控制
    bLimitPitch = false;

    // 启用平滑，获得更流畅的体验
    bEnableSmoothing = true;
    PivotSmoothIntensity = 0.5f;
    CameraLocationSmoothIntensity = 0.5f;
    CameraRotationSmoothIntensity = 0.3f;

    // 初始化当前相机高度和俯仰角
    CurrentCameraHeight = CameraHeight;
    CurrentPitchAngle = FixedPitchAngle;
}

void UNamiTopDownCamera::Initialize_Implementation(UNamiCameraComponent *InCameraComponent)
{
    Super::Initialize_Implementation(InCameraComponent);

    // 初始化当前相机高度和俯仰角
    CurrentCameraHeight = CameraHeight;
    CurrentPitchAngle = FixedPitchAngle;
}

FNamiCameraView UNamiTopDownCamera::CalculateView_Implementation(float DeltaTime)
{
    // 1. 计算观察点（PivotLocation）
    FVector TargetPivot = CalculatePivotLocation();
    TargetPivot.Z += PivotHeightOffset;

    // 2. 应用平滑跟随
    FVector SmoothedPivot = TargetPivot;
    if (bEnableSmoothFollowing)
    {
        SmoothedPivot = FNamiCameraMath::SmoothDamp(
            CurrentPivotLocation,
            TargetPivot,
            FollowVelocity,
            SmoothFollowTime,
            DeltaTime);
    }

    // 3. 获取控制旋转（相机旋转）
    FRotator ControlRotation = GetControlRotation();

    // 注意：鼠标灵敏度不应该应用在累积的控制旋转值上
    // 灵敏度应该在输入阶段应用，否则会导致旋转值不断放大
    // 移除这行代码，避免相机疯狂乱晃
    // ControlRotation.Pitch *= MouseSensitivity;
    // ControlRotation.Yaw *= MouseSensitivity;

    // 4. 处理俯视角角度
    if (bAllowPitchAdjustment)
    {
        // 允许玩家调整俯仰角，在固定角度基础上调整
        CurrentPitchAngle = FMath::Clamp(
            FixedPitchAngle - ControlRotation.Pitch, // 反转Pitch，因为鼠标向上移动应该让相机更俯视
            FixedPitchAngle - PitchAdjustmentRange,
            FixedPitchAngle + PitchAdjustmentRange);
    }
    else
    {
        // 使用固定俯视角
        CurrentPitchAngle = FixedPitchAngle;
    }

    // 5. 处理Yaw旋转
    FRotator FinalRotation;
    if (bLockYaw)
    {
        // 锁定Yaw，使用固定水平旋转
        FinalRotation = FRotator(-CurrentPitchAngle, 0.0f, 0.0f);
    }
    else
    {
        // 允许Yaw旋转
        FinalRotation = FRotator(-CurrentPitchAngle, ControlRotation.Yaw, 0.0f);
    }

    // 6. 准备 SpringArm 的输入
    // 应用相机距离缩放因子
    float ScaledArmLength = CurrentCameraHeight * CameraDistanceScale;
    SpringArm.TargetArmLength = ScaledArmLength;

    FTransform InitialTransform(FinalRotation, SmoothedPivot);
    FVector OffsetLocation = FVector::ZeroVector;

    // 7. 准备忽略的Actor列表
    TArray<AActor *> IgnoreActors;
    AActor *PrimaryTarget = GetPrimaryTarget();
    if (IsValid(PrimaryTarget))
    {
        IgnoreActors.Add(PrimaryTarget);
        // 忽略附加的Actor
        PrimaryTarget->ForEachAttachedActors([&IgnoreActors](AActor *Actor)
                                             { IgnoreActors.AddUnique(Actor); return true; });
    }

    // 8. 调用 SpringArm.Tick 计算相机位置
    UWorld *World = GetWorld();
    if (IsValid(World))
    {
        SpringArm.Tick(World, DeltaTime, IgnoreActors, InitialTransform, OffsetLocation);

        // 9. 从 SpringArm 获取最终相机Transform
        FTransform CameraTransform = SpringArm.GetCameraTransform();
        FRotator SpringArmRotation = CameraTransform.Rotator();

        // 10. 创建 View
        FNamiCameraView View;
        View.PivotLocation = SmoothedPivot;
        View.CameraLocation = CameraTransform.GetLocation();
        View.CameraRotation = SpringArmRotation;
        View.ControlLocation = CameraTransform.GetLocation();
        View.ControlRotation = SpringArmRotation;
        View.FOV = DefaultFOV;

        // 11. 应用平滑（使用父类的平滑逻辑）
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

        // 12. 使用平滑后的值
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
        View.CameraLocation = TargetPivot + FVector(0.0f, 0.0f, CurrentCameraHeight);
        View.CameraRotation = FRotator(-CurrentPitchAngle, ControlRotation.Yaw, 0.0f);
        View.ControlLocation = TargetPivot + FVector(0.0f, 0.0f, CurrentCameraHeight);
        View.ControlRotation = FRotator(-CurrentPitchAngle, ControlRotation.Yaw, 0.0f);
        View.FOV = DefaultFOV;
        return View;
    }
}