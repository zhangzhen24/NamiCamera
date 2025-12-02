// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modes/Presets/NamiThirdPersonCamera.h"
#include "NamiTopDownCamera.generated.h"

/**
 * 俯视角相机预设
 *
 * 特点：
 * - 相机从上方俯视角色，提供广阔的视野
 * - 适合策略游戏和RPG游戏
 * - 固定或可调节的俯视角角度
 * - 平滑的相机跟随
 *
 * 使用场景：
 * - 策略游戏
 * - 角色扮演游戏
 * - 模拟游戏
 * - 俯视角动作游戏
 */
UCLASS(Blueprintable, BlueprintType)
class NAMICAMERA_API UNamiTopDownCamera : public UNamiThirdPersonCamera
{
    GENERATED_BODY()

public:
    UNamiTopDownCamera();

    // ========== 生命周期 ==========
    virtual void Initialize_Implementation(UNamiCameraComponent *InCameraComponent) override;
    virtual FNamiCameraView CalculateView_Implementation(float DeltaTime) override;

protected:
    // ========== 配置 ==========

    /** 俯视角固定角度（度） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Angle",
              meta = (DisplayName = "固定俯视角", ClampMin = "0.0", ClampMax = "90.0", UIMin = "0.0", UIMax = "90.0",
                      Tooltip = "相机从上方俯视的固定角度，0度表示水平，90度表示正上方"))
    float FixedPitchAngle = 60.0f;

    /** 是否允许调整俯仰角 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Angle",
              meta = (DisplayName = "允许调整俯仰角", InlineEditConditionToggle,
                      Tooltip = "是否允许玩家通过鼠标调整相机俯仰角"))
    bool bAllowPitchAdjustment = true;

    /** 俯仰角调整范围（度） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Angle",
              meta = (DisplayName = "俯仰角调整范围", EditCondition = "bAllowPitchAdjustment", ClampMin = "5.0", ClampMax = "45.0",
                      Tooltip = "玩家可以调整俯仰角的范围，以固定角度为中心"))
    float PitchAdjustmentRange = 20.0f;

    /** 相机高度（相对于目标） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Height",
              meta = (DisplayName = "相机高度", ClampMin = "200.0", ClampMax = "2000.0", UIMin = "200.0", UIMax = "2000.0",
                      Tooltip = "相机相对于目标的高度，值越大视野越广阔"))
    float CameraHeight = 800.0f;

    /** 是否允许调整相机高度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Height",
              meta = (DisplayName = "允许调整高度", InlineEditConditionToggle,
                      Tooltip = "是否允许玩家通过鼠标滚轮调整相机高度"))
    bool bAllowHeightAdjustment = true;

    /** 相机高度调整范围 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Height",
              meta = (DisplayName = "高度调整范围", EditCondition = "bAllowHeightAdjustment", ClampMin = "100.0", ClampMax = "1000.0",
                      Tooltip = "玩家可以调整相机高度的范围"))
    float HeightAdjustmentRange = 400.0f;

    /** 鼠标滚轮灵敏度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Input",
              meta = (DisplayName = "滚轮灵敏度", EditCondition = "bAllowHeightAdjustment", ClampMin = "10.0", ClampMax = "100.0",
                      Tooltip = "鼠标滚轮调整相机高度的灵敏度"))
    float ScrollSensitivity = 50.0f;

    /** 是否锁定Yaw旋转 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Rotation",
              meta = (DisplayName = "锁定Yaw", InlineEditConditionToggle,
                      Tooltip = "是否锁定相机的水平旋转，适合固定视角游戏"))
    bool bLockYaw = false;

    /** 是否使用平滑跟随 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Smoothing",
              meta = (DisplayName = "启用平滑跟随", InlineEditConditionToggle,
                      Tooltip = "是否启用相机平滑跟随，适合慢节奏游戏"))
    bool bEnableSmoothFollowing = true;

    /** 平滑跟随时间 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Top Down Camera|Smoothing",
              meta = (DisplayName = "平滑跟随时间", EditCondition = "bEnableSmoothFollowing", ClampMin = "0.01", ClampMax = "0.2",
                      Tooltip = "相机跟随目标的平滑时间，值越大跟随越平滑但延迟越大"))
    float SmoothFollowTime = 0.05f;

private:
    /** 当前相机高度 */
    float CurrentCameraHeight = 0.0f;

    /** 当前俯仰角 */
    float CurrentPitchAngle = 0.0f;

    /** 平滑跟随速度 */
    FVector FollowVelocity = FVector::ZeroVector;
};