// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModeComponents/NamiCameraModeComponent.h"
#include "Interfaces/NamiLockOnTargetProvider.h"
#include "NamiTargetVisibilityComponent.generated.h"

/**
 * 目标可见性状态
 */
UENUM(BlueprintType)
enum class ENamiTargetVisibilityState : uint8
{
	Visible,           // 完全可见
	PartiallyOccluded, // 部分遮挡
	FullyOccluded,     // 完全遮挡
	OffScreen          // 超出屏幕
};

/**
 * 目标可见性配置
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiTargetVisibilityConfig
{
	GENERATED_BODY()

	/** 是否启用遮挡检测 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion")
	bool bEnableOcclusionCheck = true;

	/** 遮挡检测间隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion", meta = (ClampMin = "0.0"))
	float OcclusionCheckInterval = 0.1f;

	/** 遮挡检测射线数量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion", meta = (ClampMin = "1"))
	int32 OcclusionRayCount = 5;

	/** 遮挡检测射线扩散半径 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion", meta = (ClampMin = "0.0"))
	float OcclusionRaySpread = 50.0f;

	/** 遮挡检测碰撞通道 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion")
	TEnumAsByte<ECollisionChannel> OcclusionChannel = ECC_Visibility;

	/** 是否启用屏幕边界检测 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Bounds")
	bool bEnableScreenBoundsCheck = true;

	/** 屏幕安全边距（0-0.5） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Screen Bounds", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float ScreenSafeMargin = 0.1f;

	/** 是否在遮挡时调整相机 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjustment")
	bool bAdjustCameraOnOcclusion = true;

	/** 调整模式：0=拉近，1=抬高，2=侧移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjustment", meta = (ClampMin = "0", ClampMax = "2"))
	int32 AdjustmentMode = 0;

	/** 最大距离调整量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjustment", meta = (ClampMin = "0.0"))
	float MaxDistanceAdjustment = 100.0f;

	/** 最大高度调整量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjustment", meta = (ClampMin = "0.0"))
	float MaxHeightAdjustment = 50.0f;

	/** 调整平滑速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjustment", meta = (ClampMin = "0.0"))
	float AdjustmentSmoothSpeed = 5.0f;
};

/**
 * 目标可见性组件
 *
 * 功能：
 * - 检测锁定目标是否被遮挡
 * - 检测目标是否在屏幕内
 * - 遮挡时自动调整相机
 * - 提供可见性状态供其他系统使用
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Target Visibility Component"))
class NAMICAMERA_API UNamiTargetVisibilityComponent : public UNamiCameraModeComponent
{
	GENERATED_BODY()

public:
	UNamiTargetVisibilityComponent();

	// ========== UNamiCameraModeComponent ==========
	virtual void Initialize_Implementation(UNamiCameraModeBase* InCameraMode) override;
	virtual void Activate_Implementation() override;
	virtual void Update_Implementation(float DeltaTime) override;
	virtual void ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime) override;

	// ========== 公共接口 ==========

	/**
	 * 设置锁定目标提供者
	 */
	UFUNCTION(BlueprintCallable, Category = "Target Visibility")
	void SetLockOnProvider(TScriptInterface<INamiLockOnTargetProvider> InProvider);

	/**
	 * 获取当前可见性状态
	 */
	UFUNCTION(BlueprintPure, Category = "Target Visibility")
	ENamiTargetVisibilityState GetVisibilityState() const { return CurrentVisibilityState; }

	/**
	 * 获取遮挡比例（0 = 完全可见，1 = 完全遮挡）
	 */
	UFUNCTION(BlueprintPure, Category = "Target Visibility")
	float GetOcclusionRatio() const { return CurrentOcclusionRatio; }

	/**
	 * 目标是否可见
	 */
	UFUNCTION(BlueprintPure, Category = "Target Visibility")
	bool IsTargetVisible() const { return CurrentVisibilityState == ENamiTargetVisibilityState::Visible; }

	/**
	 * 目标是否在屏幕内
	 */
	UFUNCTION(BlueprintPure, Category = "Target Visibility")
	bool IsTargetOnScreen() const { return CurrentVisibilityState != ENamiTargetVisibilityState::OffScreen; }

public:
	/**
	 * 可见性检测配置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Visibility")
	FNamiTargetVisibilityConfig VisibilityConfig;

protected:
	// ========== 内部方法 ==========

	/**
	 * 执行遮挡检测
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Target Visibility|Internal")
	void PerformOcclusionCheck(const FVector& CameraLocation, const FVector& TargetLocation);

	/**
	 * 执行屏幕边界检测
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Target Visibility|Internal")
	bool PerformScreenBoundsCheck(const FVector& TargetLocation);

	/**
	 * 计算相机调整量
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Target Visibility|Internal")
	FVector CalculateCameraAdjustment(float DeltaTime);

	/**
	 * 更新可见性状态
	 */
	void UpdateVisibilityState();

protected:
	/** 锁定目标提供者 */
	TScriptInterface<INamiLockOnTargetProvider> LockOnProvider;

	/** 当前可见性状态 */
	UPROPERTY(BlueprintReadOnly, Category = "Target Visibility")
	ENamiTargetVisibilityState CurrentVisibilityState = ENamiTargetVisibilityState::Visible;

	/** 当前遮挡比例 */
	UPROPERTY(BlueprintReadOnly, Category = "Target Visibility")
	float CurrentOcclusionRatio = 0.0f;

	/** 当前相机调整偏移 */
	FVector CurrentAdjustmentOffset = FVector::ZeroVector;

	/** 上次遮挡检测时间 */
	float LastOcclusionCheckTime = 0.0f;

	/** 命中的射线数量（用于计算部分遮挡） */
	int32 OccludedRayCount = 0;
};
