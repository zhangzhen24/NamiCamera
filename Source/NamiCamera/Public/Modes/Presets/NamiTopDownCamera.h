// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modes/Templates/NamiFollowCameraMode.h"
#include "NamiTopDownCamera.generated.h"

/**
 * 俯视角相机观察方向（世界空间）
 * 定义相机看向哪个方向
 */
UENUM(BlueprintType)
enum class ENamiTopDownCameraDirection : uint8
{
	/** 看向北方（相机在南方，向北看） */
	North UMETA(DisplayName = "北 (0°)"),
	
	/** 看向东北（相机在西南方，向东北看）- 经典等轴测视角 */
	NorthEast UMETA(DisplayName = "东北 (45°)"),
	
	/** 看向东方（相机在西方，向东看） */
	East UMETA(DisplayName = "东 (90°)"),
	
	/** 看向东南（相机在西北方，向东南看） */
	SouthEast UMETA(DisplayName = "东南 (135°)"),
	
	/** 看向南方（相机在北方，向南看） */
	South UMETA(DisplayName = "南 (180°)"),
	
	/** 看向西南（相机在东北方，向西南看） */
	SouthWest UMETA(DisplayName = "西南 (225°)"),
	
	/** 看向西方（相机在东方，向西看） */
	West UMETA(DisplayName = "西 (270°)"),
	
	/** 看向西北（相机在东南方，向西北看） */
	NorthWest UMETA(DisplayName = "西北 (315°)"),
	
	/** 自定义方向 */
	Custom UMETA(DisplayName = "自定义")
};

/**
 * 俯视角相机预设类型
 */
UENUM(BlueprintType)
enum class ENamiTopDownCameraPreset : uint8
{
	/** MOBA 风格：45° 固定角度，完全锁定旋转，适合《英雄联盟》、《Dota 2》风格 */
	MOBA UMETA(DisplayName = "MOBA 风格"),
	
	/** ARPG 风格：55° 固定角度，锁定旋转，适合《暗黑破坏神》、《流放之路》风格 */
	ARPG UMETA(DisplayName = "ARPG 风格"),
	
	/** RTS 风格：50° 固定角度，自由缩放，适合《星际争霸》、《帝国时代》风格 */
	RTS UMETA(DisplayName = "RTS 风格"),
	
	/** 策略 RPG 风格：50° 角度，可旋转（90° 增量），适合《博德之门3》、《神界：原罪》风格 */
	TacticalRPG UMETA(DisplayName = "策略 RPG 风格"),
	
	/** 自定义：用户自定义所有参数 */
	Custom UMETA(DisplayName = "自定义")
};

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
 * - MOBA 游戏（《英雄联盟》、《Dota 2》）
 * - ARPG 游戏（《暗黑破坏神》、《流放之路》）
 * - RTS 游戏（《星际争霸》、《帝国时代》）
 * - 策略 RPG（《博德之门3》、《神界：原罪》）
 */
UCLASS(Blueprintable, BlueprintType)
class NAMICAMERA_API UNamiTopDownCamera : public UNamiFollowCameraMode
{
    GENERATED_BODY()

public:
    UNamiTopDownCamera();

    // ========== 生命周期 ==========
    virtual void Initialize_Implementation(UNamiCameraComponent *InCameraComponent) override;
    virtual FNamiCameraView CalculateView_Implementation(float DeltaTime) override;
    
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
    
    // ========== 辅助方法 ==========
    
    /**
     * 获取 SpringArmFeature（便捷方法）
     */
    UFUNCTION(BlueprintPure, Category = "Top Down Camera|SpringArm")
    class UNamiSpringArmFeature* GetSpringArmFeature() const;
    
    /**
     * 获取当前观察方向对应的 Yaw 角度
     */
    UFUNCTION(BlueprintPure, Category = "Top Down Camera|Direction")
    float GetDirectionYaw() const;

    // ========== 预设配置 ==========
    
    /**
     * 应用俯视角相机预设
     * 根据预设类型快速配置相机参数
     */
    UFUNCTION(BlueprintCallable, Category = "Top Down Camera|Preset")
    void ApplyTopDownPreset(ENamiTopDownCameraPreset Preset);
    
    // ========== 便捷配置方法 ==========
    
    /**
     * 设置观察方向
     * @param Direction 观察方向
     */
    UFUNCTION(BlueprintCallable, Category = "Top Down Camera|Quick Config")
    void SetCameraDirection(ENamiTopDownCameraDirection Direction);
    
    /**
     * 设置相机高度
     * @param Height 相机高度
     */
    UFUNCTION(BlueprintCallable, Category = "Top Down Camera|Quick Config")
    void SetCameraHeight(float Height);
    
    /**
     * 设置俯视角度
     * @param Angle 俯视角度（20-80°）
     */
    UFUNCTION(BlueprintCallable, Category = "Top Down Camera|Quick Config")
    void SetPitchAngle(float Angle);
    
    /**
     * 设置是否锁定相机旋转
     * @param bLock 是否锁定
     */
    UFUNCTION(BlueprintCallable, Category = "Top Down Camera|Quick Config")
    void SetLockRotation(bool bLock);

protected:
    // ========== 1. 快捷配置 ==========
    
    /** 
     * 俯视角相机预设类型
     * 快速应用经典游戏风格的相机配置
     * 注意：应用预设会覆盖当前的所有配置（包括观察方向）
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 快捷配置",
              meta = (DisplayName = "相机预设",
                      Tooltip = "快速应用经典游戏风格的相机配置（会覆盖所有设置）：\n• MOBA 风格：45° 东北方向\n• ARPG 风格：55° 东北方向\n• RTS 风格：50° 西北方向\n• 策略 RPG 风格：可旋转（90° 增量）\n• 自定义：保持当前设置"))
    ENamiTopDownCameraPreset TopDownPreset = ENamiTopDownCameraPreset::Custom;
    
    /** 
     * 相机观察方向（世界空间）
     * 决定相机看向哪个方向
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 快捷配置",
              meta = (DisplayName = "观察方向",
                      Tooltip = "相机看向的方向（世界空间）：\n• 东北(45°)：最常见，适合 MOBA、ARPG（相机在西南，看向东北）\n• 西北(315°)：适合 RTS 游戏（相机在东南，看向西北）\n• 其他方向：根据游戏需求选择"))
    ENamiTopDownCameraDirection CameraDirection = ENamiTopDownCameraDirection::NorthEast;
    
    /** 
     * 自定义观察方向的 Yaw 角度
     * 仅当观察方向为 Custom 时有效
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 快捷配置",
              meta = (DisplayName = "自定义方向角度", 
                      EditCondition = "CameraDirection == ENamiTopDownCameraDirection::Custom",
                      ClampMin = "0.0", ClampMax = "360.0", UIMin = "0.0", UIMax = "360.0",
                      Tooltip = "自定义相机观察方向的 Yaw 角度：\n• 0° = 北\n• 90° = 东\n• 180° = 南\n• 270° = 西"))
    float CustomDirectionYaw = 45.0f;

    /** 
     * 俯视角固定角度（度）
     * 控制相机从多高的角度俯视
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 快捷配置",
              meta = (DisplayName = "俯视角度", 
                      ClampMin = "20.0", ClampMax = "80.0", UIMin = "20.0", UIMax = "80.0",
                      Tooltip = "相机俯视的角度（Pitch）：\n• 45°：标准 MOBA 视角\n• 55°：ARPG 视角（更俯视）\n• 60°：接近正俯视\n• 推荐范围：40-60°"))
    float FixedPitchAngle = 45.0f;

    /** 
     * 相机高度（相对于目标）
     * 控制相机距离地面的高度
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 快捷配置",
              meta = (DisplayName = "相机高度", 
                      ClampMin = "300.0", ClampMax = "2000.0", UIMin = "300.0", UIMax = "2000.0",
                      Tooltip = "相机距离目标的垂直高度：\n• 800-1000：ARPG（紧凑视野）\n• 1200-1500：MOBA（标准视野）\n• 1500+：RTS（广阔视野）"))
    float CameraHeight = 1200.0f;
    
    /** 
     * 是否完全锁定相机旋转
     * 开启后相机角度完全固定
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. 快捷配置",
              meta = (DisplayName = "锁定相机旋转",
                      Tooltip = "开启后相机角度完全固定，不会跟随鼠标旋转\n• 适合：MOBA、ARPG、RTS\n• 提供一致的空间感知"))
    bool bLockCameraRotation = true;

    // ========== 2. 高级设置 ==========
    
    /** 允许俯仰角调整（仅在未锁定旋转时有效） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 高级设置|俯仰调整",
              meta = (DisplayName = "允许俯仰调整", EditCondition = "!bLockCameraRotation",
                      Tooltip = "允许玩家通过鼠标调整相机的俯仰角度"))
    bool bAllowPitchAdjustment = false;

    /** 俯仰角调整范围（度） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 高级设置|俯仰调整",
              meta = (DisplayName = "俯仰调整范围", EditCondition = "!bLockCameraRotation && bAllowPitchAdjustment", 
                      ClampMin = "5.0", ClampMax = "30.0", UIMin = "5.0", UIMax = "30.0",
                      Tooltip = "以固定角度为中心的俯仰角调整范围\n推荐值：10-20°"))
    float PitchAdjustmentRange = 15.0f;

    /** 允许相机高度调整（缩放） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 高级设置|高度调整",
              meta = (DisplayName = "允许高度调整",
                      Tooltip = "允许玩家通过鼠标滚轮调整相机高度（缩放功能）"))
    bool bAllowHeightAdjustment = true;

    /** 相机高度调整范围 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 高级设置|高度调整",
              meta = (DisplayName = "高度调整范围", EditCondition = "bAllowHeightAdjustment", 
                      ClampMin = "200.0", ClampMax = "1000.0", UIMin = "200.0", UIMax = "1000.0",
                      Tooltip = "相机高度的调整范围（基础高度 ± 此值）\n推荐值：300-500"))
    float HeightAdjustmentRange = 500.0f;

    /** 鼠标滚轮灵敏度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 高级设置|高度调整",
              meta = (DisplayName = "滚轮灵敏度", EditCondition = "bAllowHeightAdjustment", 
                      ClampMin = "10.0", ClampMax = "100.0", UIMin = "10.0", UIMax = "100.0",
                      Tooltip = "鼠标滚轮调整高度的灵敏度\n推荐值：30-70"))
    float ScrollSensitivity = 50.0f;

    /** 允许相机偏航旋转 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 高级设置|旋转设置",
              meta = (DisplayName = "允许偏航旋转", EditCondition = "!bLockCameraRotation",
                      Tooltip = "允许玩家水平旋转相机（Yaw）"))
    bool bAllowYawRotation = false;
    
    /** 旋转增量（度）- 0 为自由旋转，非零为增量旋转（例如 90° 增量） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 高级设置|旋转设置",
              meta = (DisplayName = "旋转增量", EditCondition = "!bLockCameraRotation && bAllowYawRotation",
                      ClampMin = "0.0", ClampMax = "90.0", UIMin = "0.0", UIMax = "90.0",
                      Tooltip = "相机旋转的角度增量：\n• 0 = 自由旋转\n• 90 = 每次旋转 90°（策略 RPG 风格）\n• 45 = 每次旋转 45°"))
    float RotationIncrement = 0.0f;

    /** 启用平滑跟随 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 高级设置|平滑跟随",
              meta = (DisplayName = "启用平滑跟随",
                      Tooltip = "启用平滑的相机跟随效果，适合慢节奏游戏"))
    bool bEnableSmoothFollowing = true;

    /** 平滑跟随速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. 高级设置|平滑跟随",
              meta = (DisplayName = "平滑跟随速度", EditCondition = "bEnableSmoothFollowing", 
                      ClampMin = "1.0", ClampMax = "20.0", UIMin = "1.0", UIMax = "20.0",
                      Tooltip = "相机平滑跟随的速度，值越大跟随越快\n推荐值：快速 15-20，平衡 8-12，慢速 3-6"))
    float SmoothFollowSpeed = 10.0f;

private:
    /** 当前相机高度 */
    float CurrentCameraHeight = 0.0f;

    /** 当前俯仰角 */
    float CurrentPitchAngle = 0.0f;

    /** 平滑跟随速度 */
    FVector FollowVelocity = FVector::ZeroVector;
};