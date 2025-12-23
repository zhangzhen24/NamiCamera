// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NamiComposableCameraMode.h"
#include "Calculators/FOV/NamiFramingFOVCalculator.h"
#include "Calculators/Position/NamiEllipseOrbitPositionCalculator.h"
#include "Calculators/Rotation/NamiLookAtRotationCalculator.h"
#include "Calculators/Target/NamiDualFocusTargetCalculator.h"
#include "Interfaces/NamiLockOnTargetProvider.h"
#include "ModeComponents/NamiCameraLockOnComponent.h"
#include "NamiDualFocusCameraMode.generated.h"

/**
 * 双焦点相机模式（组合式架构）
 *
 * 预配置的组合式相机，包含：
 * - DualFocusTargetCalculator: 双焦点目标计算（玩家+锁定目标）
 * - EllipseOrbitPositionCalculator: 椭圆轨道位置计算
 * - LookAtRotationCalculator: 看向焦点旋转计算
 * - FramingFOVCalculator: 构图同框 FOV 计算
 * - LockOnComponent: 锁定目标管理组件
 *
 * 适用于：鬼泣风格战斗相机
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Dual Focus Camera"))
class NAMICAMERA_API UNamiDualFocusCameraMode : public UNamiComposableCameraMode
{
	GENERATED_BODY()

public:
	UNamiDualFocusCameraMode();

	// ========== 生命周期 ==========

	virtual void Initialize_Implementation(UNamiCameraComponent* InCameraComponent) override;
	virtual void Activate_Implementation() override;
	virtual FNamiCameraView CalculateView_Implementation(float DeltaTime) override;

	// ========== 锁定目标管理 ==========

	/**
	 * 设置锁定目标提供者
	 */
	UFUNCTION(BlueprintCallable, Category = "Dual Focus Camera")
	void SetLockOnProvider(TScriptInterface<INamiLockOnTargetProvider> Provider);

	/**
	 * 获取锁定目标提供者
	 */
	UFUNCTION(BlueprintPure, Category = "Dual Focus Camera")
	TScriptInterface<INamiLockOnTargetProvider> GetLockOnProvider() const;

	/**
	 * 是否有有效的锁定目标
	 */
	UFUNCTION(BlueprintPure, Category = "Dual Focus Camera")
	bool HasValidLockedTarget() const;

	// ========== 轨道控制 ==========

	/**
	 * 添加输入到轨道角度
	 */
	UFUNCTION(BlueprintCallable, Category = "Dual Focus Camera|Input")
	void AddOrbitInput(float DeltaAngle);

	// ========== 计算器访问 ==========

	UFUNCTION(BlueprintPure, Category = "Dual Focus Camera|Calculators")
	UNamiDualFocusTargetCalculator* GetDualFocusTargetCalculator() const;

	UFUNCTION(BlueprintPure, Category = "Dual Focus Camera|Calculators")
	UNamiEllipseOrbitPositionCalculator* GetEllipseOrbitPositionCalculator() const;

	UFUNCTION(BlueprintPure, Category = "Dual Focus Camera|Calculators")
	UNamiLookAtRotationCalculator* GetLookAtRotationCalculator() const;

	UFUNCTION(BlueprintPure, Category = "Dual Focus Camera|Calculators")
	UNamiFramingFOVCalculator* GetFramingFOVCalculator() const;

	UFUNCTION(BlueprintPure, Category = "Dual Focus Camera|Components")
	UNamiCameraLockOnComponent* GetLockOnComponent() const;

protected:
	/** 创建默认计算器 */
	void CreateDefaultCalculators();

	/** 创建并注册 LockOn 组件 */
	void SetupLockOnComponent();

	/** 同步锁定目标到所有计算器 */
	void SyncLockOnProviderToCalculators();

	/** 处理玩家输入 */
	void ProcessPlayerInput(float DeltaTime);

	/** 锁定目标提供者 */
	UPROPERTY()
	TScriptInterface<INamiLockOnTargetProvider> CachedLockOnProvider;

	/** 锁定组件（通过 CameraComponent 注册） */
	UPROPERTY()
	TObjectPtr<UNamiCameraLockOnComponent> LockOnComponent;
};
