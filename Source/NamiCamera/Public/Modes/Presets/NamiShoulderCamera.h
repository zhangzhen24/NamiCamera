// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modes/Presets/NamiThirdPersonCamera.h"
#include "NamiShoulderCamera.generated.h"

/**
 * 肩部视角相机预设
 *
 * 特点：
 * - 相机位置靠近角色肩部，提供更好的沉浸感
 * - 适合动作游戏和射击游戏
 * - 支持武器瞄准模式
 * - 更紧凑的相机跟随
 *
 * 使用场景：
 * - 动作冒险游戏
 * - 第三人称射击游戏
 * - 角色扮演游戏
 */
UCLASS(Blueprintable, BlueprintType)
class NAMICAMERA_API UNamiShoulderCamera : public UNamiThirdPersonCamera
{
    GENERATED_BODY()

public:
    UNamiShoulderCamera();

    // ========== 生命周期 ==========
    virtual void Initialize_Implementation(UNamiCameraComponent *InCameraComponent) override;
    virtual FNamiCameraView CalculateView_Implementation(float DeltaTime) override;

protected:
    // ========== 配置 ==========

    /** 肩部偏移（相对于角色中心） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoulder Camera|Offset",
              meta = (DisplayName = "肩部偏移", Tooltip = "相机相对于角色中心的水平偏移，正数表示右侧，负数表示左侧"))
    float ShoulderOffset = 50.0f;

    /** 瞄准模式启用状态 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoulder Camera|Aiming",
              meta = (DisplayName = "启用瞄准模式", InlineEditConditionToggle))
    bool bEnableAimingMode = true;

    /** 瞄准模式相机距离 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoulder Camera|Aiming",
              meta = (DisplayName = "瞄准模式距离", EditCondition = "bEnableAimingMode", ClampMin = "50.0", ClampMax = "500.0",
                      Tooltip = "进入瞄准模式时的相机距离"))
    float AimingCameraDistance = 150.0f;

    /** 瞄准模式FOV */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoulder Camera|Aiming",
              meta = (DisplayName = "瞄准模式FOV", EditCondition = "bEnableAimingMode", ClampMin = "30.0", ClampMax = "90.0",
                      Tooltip = "进入瞄准模式时的视野角度，通常比默认FOV小，提供更精确的瞄准体验"))
    float AimingFOV = 60.0f;

    /** 瞄准模式平滑时间 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoulder Camera|Aiming",
              meta = (DisplayName = "瞄准模式平滑时间", EditCondition = "bEnableAimingMode", ClampMin = "0.0", ClampMax = "0.5",
                      Tooltip = "进入/退出瞄准模式的平滑过渡时间"))
    float AimingSmoothTime = 0.1f;

    /** 是否使用角色朝向作为基准 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoulder Camera|Behavior",
              meta = (DisplayName = "使用角色朝向", Tooltip = "相机是否基于角色朝向进行定位，建议开启以获得更好的跟随效果"))
    bool bUseCharacterFacing = true;

    /** 角色朝向影响强度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shoulder Camera|Behavior",
              meta = (DisplayName = "角色朝向影响", EditCondition = "bUseCharacterFacing", ClampMin = "0.0", ClampMax = "1.0",
                      Tooltip = "角色朝向对相机位置的影响强度，0表示完全不影响，1表示完全跟随角色朝向"))
    float CharacterFacingInfluence = 0.7f;

private:
    /** 原始相机距离（用于瞄准模式切换） */
    float OriginalCameraDistance = 0.0f;

    /** 原始FOV（用于瞄准模式切换） */
    float OriginalFOV = 90.0f;

    /** 瞄准模式过渡进度 */
    float AimingTransitionProgress = 0.0f;

    /** 瞄准模式速度 */
    float AimingTransitionVelocity = 0.0f;
};