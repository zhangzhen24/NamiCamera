// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modes/Presets/NamiTopDownCamera.h"
#include "Features/NamiSpringArmFeature.h"
#include "Components/NamiCameraComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Libraries/NamiCameraMath.h"
#include "LogNamiCamera.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NamiTopDownCamera)

UNamiTopDownCamera::UNamiTopDownCamera()
{
    // 俯视角相机默认配置
    DefaultFOV = 90.0f;

    // 默认配置（不使用预设，让用户自由配置）
    FixedPitchAngle = 45.0f;
    CameraHeight = 1200.0f;
    CameraDirection = ENamiTopDownCameraDirection::NorthEast;  // 默认东北方向
    bLockCameraRotation = true;

    // 初始化当前相机高度和俯仰角
    CurrentCameraHeight = CameraHeight;
    CurrentPitchAngle = FixedPitchAngle;
    
    UE_LOG(LogNamiCamera, Log, TEXT("[UNamiTopDownCamera] 构造函数 - 默认方向: %d, 角度: %.1f, 高度: %.1f"), 
        (int32)CameraDirection, FixedPitchAngle, CameraHeight);
}

void UNamiTopDownCamera::Initialize_Implementation(UNamiCameraComponent *InCameraComponent)
{
    Super::Initialize_Implementation(InCameraComponent);

    // 初始化当前相机高度和俯仰角
    CurrentCameraHeight = CameraHeight;
    CurrentPitchAngle = FixedPitchAngle;
    
    // 配置 SpringArmFeature 为俯视角相机的默认值
    UNamiSpringArmFeature* SpringArmFeature = GetSpringArmFeature();
    if (SpringArmFeature)
    {
        SpringArmFeature->SpringArm.TargetArmLength = CameraHeight;
        SpringArmFeature->SpringArm.bDoCollisionTest = true;
        SpringArmFeature->SpringArm.bEnableCameraLag = false;  // 俯视角相机通常不需要 SpringArm 的滞后
    }
    
    // 应用默认预设（如果用户选择了预设）
    if (TopDownPreset != ENamiTopDownCameraPreset::Custom)
    {
        UE_LOG(LogNamiCamera, Log, TEXT("[UNamiTopDownCamera::Initialize] 应用预设：%d"), (int32)TopDownPreset);
        ApplyTopDownPreset(TopDownPreset);
    }
    else
    {
        UE_LOG(LogNamiCamera, Log, TEXT("[UNamiTopDownCamera::Initialize] 使用自定义配置 - 方向: %d (Yaw: %.1f), 角度: %.1f, 高度: %.1f"), 
            (int32)CameraDirection, GetDirectionYaw(), FixedPitchAngle, CameraHeight);
    }
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
        SmoothedPivot = FMath::VInterpTo(
            CurrentPivotLocation,
            TargetPivot,
            DeltaTime,
            SmoothFollowSpeed);
    }
    CurrentPivotLocation = SmoothedPivot;

    // 3. 确定相机旋转
    FRotator FinalRotation;
    
    if (bLockCameraRotation)
    {
        // 完全锁定模式：使用固定角度和固定方向
        CurrentPitchAngle = FixedPitchAngle;
        float DirectionYaw = GetDirectionYaw();
        FinalRotation = FRotator(-CurrentPitchAngle, DirectionYaw, 0.0f);
        
        // 调试日志
        UE_LOG(LogNamiCamera, Verbose, TEXT("[TopDownCamera] 锁定模式 - Pitch: %.1f, DirectionYaw: %.1f"), 
            CurrentPitchAngle, DirectionYaw);
    }
    else
    {
        // 非锁定模式：允许玩家控制
        // 从 PlayerController 获取控制旋转
        FRotator ControlRotation = FRotator::ZeroRotator;
        if (AActor* Target = GetPrimaryTarget())
        {
            if (APawn* Pawn = Cast<APawn>(Target))
            {
                if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
                {
                    ControlRotation = PC->GetControlRotation();
                    // 使用0-360度归一化，避免180/-180跳变问题
                    ControlRotation = FNamiCameraMath::NormalizeRotatorTo360(ControlRotation);
                }
            }
        }
        
        // 处理俯仰角
        if (bAllowPitchAdjustment)
        {
            // 允许调整俯仰角
            CurrentPitchAngle = FMath::Clamp(
                FixedPitchAngle - ControlRotation.Pitch,
                FixedPitchAngle - PitchAdjustmentRange,
                FixedPitchAngle + PitchAdjustmentRange);
        }
        else
        {
            CurrentPitchAngle = FixedPitchAngle;
        }
        
        // 处理 Yaw 旋转
        float FinalYaw = GetDirectionYaw();  // 默认使用设定的方向
        if (bAllowYawRotation)
        {
            if (RotationIncrement > 0.0f)
            {
                // 按增量旋转（如 90° 增量）
                FinalYaw = FMath::RoundToFloat(ControlRotation.Yaw / RotationIncrement) * RotationIncrement;
            }
            else
            {
                // 自由旋转
                FinalYaw = ControlRotation.Yaw;
            }
        }
        
        FinalRotation = FRotator(-CurrentPitchAngle, FinalYaw, 0.0f);
    }

    // 4. 计算俯视角相机位置
    // 俯视角相机应该在目标正上方，而不是使用 SpringArm 的默认计算
    FVector CameraLocation = SmoothedPivot;
    
    // 根据俯视角度计算位置（适合 45-60° 俯视）
    // 计算相机应该在的位置，使其能以指定角度俯视目标
    float PitchRad = FMath::DegreesToRadians(CurrentPitchAngle);
    float HorizontalDistance = CurrentCameraHeight / FMath::Tan(PitchRad);
    
    // 分离观察方向和位置方向
    // 观察方向：相机看向哪里（如东北 = 45°，表示看向东北）
    // 位置方向：相机在目标的哪个方向（观察方向 + 180°，在相反方向）
    float LookAtYaw = GetDirectionYaw();  // 相机观察的方向（用户设置的方向）
    float PositionYaw = LookAtYaw + 180.0f;  // 相机位置的方向（反向）
    
    // 根据位置方向计算水平偏移
    FRotator PositionRotation(0.0f, PositionYaw, 0.0f);
    FVector HorizontalOffset = PositionRotation.Vector() * HorizontalDistance;
    
    // 相机位置 = 目标位置 + 水平偏移 + 垂直高度
    // 如果 LookAtYaw=45°(看向东北)，PositionYaw=225°(在西南)，相机在目标的西南方向
    CameraLocation = SmoothedPivot + HorizontalOffset;
    CameraLocation.Z += CurrentCameraHeight;
    
    // 重新计算 FinalRotation，使用用户设置的观察方向
    FinalRotation = FRotator(-CurrentPitchAngle, LookAtYaw, 0.0f);
    
    // 使用0-360度归一化，避免180/-180跳变问题
    FinalRotation = FNamiCameraMath::NormalizeRotatorTo360(FinalRotation);
    
    // 调试日志
    UE_LOG(LogNamiCamera, Verbose, TEXT("[TopDownCamera] 位置计算 - Pivot: %s, LookAtYaw: %.1f°(观察), PositionYaw: %.1f°(位置), HorizontalDist: %.1f, Offset: %s, FinalLoc: %s"),
        *SmoothedPivot.ToString(), LookAtYaw, PositionYaw, HorizontalDistance, *HorizontalOffset.ToString(), *CameraLocation.ToString());

    // 5. 可选：使用 SpringArm 进行碰撞检测
    // 如果需要碰撞检测，可以使用 SpringArmFeature
    UNamiSpringArmFeature* SpringArmFeature = GetSpringArmFeature();
    if (SpringArmFeature && SpringArmFeature->SpringArm.bDoCollisionTest)
    {
        // 配置 SpringArm 进行碰撞检测
        // 注意：我们需要手动设置 Transform，因为我们不使用默认的计算方式
        FTransform InitialTransform(FinalRotation, SmoothedPivot);
        FVector OffsetLocation = FVector::ZeroVector;
        
        UWorld* World = GetWorld();
        if (World)
        {
            // 临时设置 TargetArmLength 为我们计算的距离
            float OriginalArmLength = SpringArmFeature->SpringArm.TargetArmLength;
            SpringArmFeature->SpringArm.TargetArmLength = FVector::Dist(CameraLocation, SmoothedPivot);
            
            // 获取忽略的 Actor
            TArray<AActor*> IgnoreActors;
            if (AActor* TargetActor = GetPrimaryTarget())
            {
                IgnoreActors.Add(TargetActor);
            }
            
            // 执行碰撞检测
            SpringArmFeature->SpringArm.Tick(World, DeltaTime, IgnoreActors, InitialTransform, OffsetLocation);
            
            // 获取碰撞检测后的位置
            FTransform CameraTransform = SpringArmFeature->SpringArm.GetCameraTransform();
            CameraLocation = CameraTransform.GetLocation();
            
            // 恢复原始 ArmLength
            SpringArmFeature->SpringArm.TargetArmLength = OriginalArmLength;
        }
    }

    // 6. 创建最终视图
    // 使用计算的 FinalRotation，而不是 LookAtRotation
    // 这样可以确保观察方向设置生效
    FNamiCameraView View;
    View.PivotLocation = SmoothedPivot;
    View.CameraLocation = CameraLocation;
    // 使用0-360度归一化，避免180/-180跳变问题
    View.CameraRotation = FNamiCameraMath::NormalizeRotatorTo360(FinalRotation);  // 使用计算的旋转，确保方向正确
    View.ControlLocation = SmoothedPivot;
    View.ControlRotation = FNamiCameraMath::NormalizeRotatorTo360(FinalRotation);
    View.FOV = DefaultFOV;
    
    // 调试日志：验证最终结果
    UE_LOG(LogNamiCamera, Verbose, TEXT("[TopDownCamera] 最终视图 - Location: %s, Rotation: %s (Pitch: %.1f°, Yaw: %.1f°)"),
        *View.CameraLocation.ToString(), *View.CameraRotation.ToString(), 
        View.CameraRotation.Pitch, View.CameraRotation.Yaw);

    return View;
}

#if WITH_EDITOR
void UNamiTopDownCamera::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    if (PropertyChangedEvent.Property)
    {
        FName PropertyName = PropertyChangedEvent.Property->GetFName();
        
        // 当预设改变时，应用预设
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNamiTopDownCamera, TopDownPreset))
        {
            if (TopDownPreset != ENamiTopDownCameraPreset::Custom)
            {
                ApplyTopDownPreset(TopDownPreset);
            }
        }
        // 当用户手动修改配置参数时，自动切换到 Custom 模式
        else if (TopDownPreset != ENamiTopDownCameraPreset::Custom)
        {
            // 检查是否修改了预设相关的核心参数
            if (PropertyName == GET_MEMBER_NAME_CHECKED(UNamiTopDownCamera, CameraDirection) ||
                PropertyName == GET_MEMBER_NAME_CHECKED(UNamiTopDownCamera, CustomDirectionYaw) ||
                PropertyName == GET_MEMBER_NAME_CHECKED(UNamiTopDownCamera, FixedPitchAngle) ||
                PropertyName == GET_MEMBER_NAME_CHECKED(UNamiTopDownCamera, CameraHeight) ||
                PropertyName == GET_MEMBER_NAME_CHECKED(UNamiTopDownCamera, bLockCameraRotation) ||
                PropertyName == GET_MEMBER_NAME_CHECKED(UNamiTopDownCamera, bAllowPitchAdjustment) ||
                PropertyName == GET_MEMBER_NAME_CHECKED(UNamiTopDownCamera, bAllowYawRotation) ||
                PropertyName == GET_MEMBER_NAME_CHECKED(UNamiTopDownCamera, bAllowHeightAdjustment) ||
                PropertyName == GET_MEMBER_NAME_CHECKED(UNamiTopDownCamera, bEnableSmoothFollowing))
            {
                // 自动切换到 Custom 模式
                TopDownPreset = ENamiTopDownCameraPreset::Custom;
                UE_LOG(LogNamiCamera, Log, TEXT("[UNamiTopDownCamera] 检测到配置修改（%s），自动切换到自定义模式"), 
                    *PropertyName.ToString());
            }
        }
        
        // 同步当前状态
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNamiTopDownCamera, CameraHeight))
        {
            CurrentCameraHeight = CameraHeight;
        }
        else if (PropertyName == GET_MEMBER_NAME_CHECKED(UNamiTopDownCamera, FixedPitchAngle))
        {
            CurrentPitchAngle = FixedPitchAngle;
        }
    }
}
#endif

UNamiSpringArmFeature* UNamiTopDownCamera::GetSpringArmFeature() const
{
    return GetFeature<UNamiSpringArmFeature>();
}

float UNamiTopDownCamera::GetDirectionYaw() const
{
    float ResultYaw = 0.0f;
    
    switch (CameraDirection)
    {
    case ENamiTopDownCameraDirection::North:
        ResultYaw = 0.0f;
        break;
        
    case ENamiTopDownCameraDirection::NorthEast:
        ResultYaw = 45.0f;
        break;
        
    case ENamiTopDownCameraDirection::East:
        ResultYaw = 90.0f;
        break;
        
    case ENamiTopDownCameraDirection::SouthEast:
        ResultYaw = 135.0f;
        break;
        
    case ENamiTopDownCameraDirection::South:
        ResultYaw = 180.0f;
        break;
        
    case ENamiTopDownCameraDirection::SouthWest:
        ResultYaw = 225.0f;
        break;
        
    case ENamiTopDownCameraDirection::West:
        ResultYaw = 270.0f;
        break;
        
    case ENamiTopDownCameraDirection::NorthWest:
        ResultYaw = 315.0f;
        break;
        
    case ENamiTopDownCameraDirection::Custom:
    default:
        ResultYaw = CustomDirectionYaw;
        break;
    }
    
    UE_LOG(LogNamiCamera, Verbose, TEXT("[TopDownCamera::GetDirectionYaw] Direction: %d, ResultYaw: %.1f"), 
        (int32)CameraDirection, ResultYaw);
    
    return ResultYaw;
}

void UNamiTopDownCamera::SetCameraDirection(ENamiTopDownCameraDirection Direction)
{
    CameraDirection = Direction;
    UE_LOG(LogNamiCamera, Log, TEXT("[UNamiTopDownCamera] 设置观察方向：%d (Yaw: %.1f°)"), 
        (int32)Direction, GetDirectionYaw());
}

void UNamiTopDownCamera::SetCameraHeight(float Height)
{
    CameraHeight = FMath::Clamp(Height, 300.0f, 2000.0f);
    CurrentCameraHeight = CameraHeight;
    UE_LOG(LogNamiCamera, Log, TEXT("[UNamiTopDownCamera] 设置相机高度：%.1f"), CameraHeight);
}

void UNamiTopDownCamera::SetPitchAngle(float Angle)
{
    FixedPitchAngle = FMath::Clamp(Angle, 20.0f, 80.0f);
    CurrentPitchAngle = FixedPitchAngle;
    UE_LOG(LogNamiCamera, Log, TEXT("[UNamiTopDownCamera] 设置俯视角度：%.1f°"), FixedPitchAngle);
}

void UNamiTopDownCamera::SetLockRotation(bool bLock)
{
    bLockCameraRotation = bLock;
    UE_LOG(LogNamiCamera, Log, TEXT("[UNamiTopDownCamera] %s相机旋转"), 
        bLock ? TEXT("锁定") : TEXT("解锁"));
}

void UNamiTopDownCamera::ApplyTopDownPreset(ENamiTopDownCameraPreset Preset)
{
    TopDownPreset = Preset;
    
    switch (Preset)
    {
    case ENamiTopDownCameraPreset::MOBA:
        // MOBA 风格：45° 固定角度，完全锁定，看向东北（经典等轴测）
        FixedPitchAngle = 45.0f;
        CameraHeight = 1200.0f;
        CameraDirection = ENamiTopDownCameraDirection::NorthEast;  // 看向东北
        bLockCameraRotation = true;
        bAllowPitchAdjustment = false;
        bAllowYawRotation = false;
        bAllowHeightAdjustment = true;
        HeightAdjustmentRange = 400.0f;
        bEnableSmoothFollowing = true;
        SmoothFollowSpeed = 12.0f;
        UE_LOG(LogNamiCamera, Log, TEXT("[UNamiTopDownCamera] 应用预设：MOBA 风格 - 看向东北(45°), 角度: 45°, 高度: 1200"));
        break;
        
    case ENamiTopDownCameraPreset::ARPG:
        // ARPG 风格：55° 固定角度，锁定旋转，看向东北（暗黑破坏神风格）
        FixedPitchAngle = 55.0f;
        CameraHeight = 900.0f;
        CameraDirection = ENamiTopDownCameraDirection::NorthEast;  // 看向东北
        bLockCameraRotation = true;
        bAllowPitchAdjustment = false;
        bAllowYawRotation = false;
        bAllowHeightAdjustment = true;
        HeightAdjustmentRange = 300.0f;
        bEnableSmoothFollowing = true;
        SmoothFollowSpeed = 15.0f;
        UE_LOG(LogNamiCamera, Log, TEXT("[UNamiTopDownCamera] 应用预设：ARPG 风格 - 看向东北"));
        break;
        
    case ENamiTopDownCameraPreset::RTS:
        // RTS 风格：50° 固定角度，自由缩放，看向西北（星际争霸风格）
        FixedPitchAngle = 50.0f;
        CameraHeight = 1500.0f;
        CameraDirection = ENamiTopDownCameraDirection::NorthWest;  // 看向西北
        bLockCameraRotation = true;
        bAllowPitchAdjustment = false;
        bAllowYawRotation = false;
        bAllowHeightAdjustment = true;
        HeightAdjustmentRange = 800.0f;
        bEnableSmoothFollowing = true;
        SmoothFollowSpeed = 10.0f;
        UE_LOG(LogNamiCamera, Log, TEXT("[UNamiTopDownCamera] 应用预设：RTS 风格 - 看向西北"));
        break;
        
    case ENamiTopDownCameraPreset::TacticalRPG:
        // 策略 RPG 风格：50° 角度，可旋转（90° 增量），默认看向东北
        FixedPitchAngle = 50.0f;
        CameraHeight = 800.0f;
        CameraDirection = ENamiTopDownCameraDirection::NorthEast;  // 默认看向东北
        bLockCameraRotation = false;
        bAllowPitchAdjustment = false;
        bAllowYawRotation = true;
        RotationIncrement = 90.0f;
        bAllowHeightAdjustment = true;
        HeightAdjustmentRange = 400.0f;
        bEnableSmoothFollowing = true;
        SmoothFollowSpeed = 12.0f;
        UE_LOG(LogNamiCamera, Log, TEXT("[UNamiTopDownCamera] 应用预设：策略 RPG 风格 - 可旋转"));
        break;
        
    case ENamiTopDownCameraPreset::Custom:
    default:
        // 自定义：不修改任何参数
        UE_LOG(LogNamiCamera, Log, TEXT("[UNamiTopDownCamera] 切换到自定义模式"));
        break;
    }
    
    // 同步到当前状态
    CurrentCameraHeight = CameraHeight;
    CurrentPitchAngle = FixedPitchAngle;
    
    // 更新 SpringArmFeature
    UNamiSpringArmFeature* SpringArmFeature = GetSpringArmFeature();
    if (SpringArmFeature)
    {
        SpringArmFeature->SpringArm.TargetArmLength = CameraHeight;
    }
}