// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NamiCameraCalculatorBase.generated.h"

class UNamiCameraModeBase;

/**
 * 相机计算器公共基类
 *
 * 为所有计算器类型（Target、Position、Rotation、FOV）提供公共功能：
 * - 生命周期管理 (Initialize, Activate, Deactivate)
 * - 相机模式引用
 * - World 访问
 *
 * 计算器是 Mode 的内部实现细节，用于计算相机视图的各个组成部分。
 * 普通用户无需直接使用，高级用户可通过自定义 Mode 来使用不同的计算器组合。
 *
 * 继承层次：
 * UNamiCameraCalculatorBase
 *   ├─ UNamiCameraTargetCalculator    (计算 PivotLocation)
 *   ├─ UNamiCameraPositionCalculator  (计算 CameraLocation)
 *   ├─ UNamiCameraRotationCalculator  (计算 CameraRotation)
 *   └─ UNamiCameraFOVCalculator       (计算 FOV)
 */
UCLASS(Abstract, Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew)
class NAMICAMERA_API UNamiCameraCalculatorBase : public UObject
{
	GENERATED_BODY()

public:
	UNamiCameraCalculatorBase();

	// ========== 生命周期 ==========

	/**
	 * 初始化计算器
	 * @param InCameraMode 所属的相机模式
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Calculator")
	void Initialize(UNamiCameraModeBase* InCameraMode);

	/**
	 * 激活计算器
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Calculator")
	void Activate();

	/**
	 * 停用计算器
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Calculator")
	void Deactivate();

	// ========== 状态查询 ==========

	/** 是否已初始化 */
	UFUNCTION(BlueprintPure, Category = "Camera Calculator")
	bool IsInitialized() const { return bInitialized; }

	/** 是否激活中 */
	UFUNCTION(BlueprintPure, Category = "Camera Calculator")
	bool IsActive() const { return bIsActive; }

	// ========== 辅助函数 ==========

	/** 获取 World */
	virtual UWorld* GetWorld() const override;

	/** 获取所属的相机模式 */
	UFUNCTION(BlueprintPure, Category = "Camera Calculator")
	UNamiCameraModeBase* GetCameraMode() const { return CameraMode.Get(); }

protected:
	/** 所属的相机模式 */
	UPROPERTY()
	TWeakObjectPtr<UNamiCameraModeBase> CameraMode;

	/** 是否已初始化 */
	UPROPERTY(BlueprintReadOnly, Category = "Camera Calculator")
	uint8 bInitialized : 1;

	/** 是否激活中 */
	UPROPERTY(BlueprintReadOnly, Category = "Camera Calculator")
	uint8 bIsActive : 1;
};
