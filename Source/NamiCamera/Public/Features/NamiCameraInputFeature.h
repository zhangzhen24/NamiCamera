// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Features/NamiCameraFeature.h"
#include "Structs/State/NamiCameraState.h"
#include "Enums/NamiCameraEnums.h"
#include "NamiCameraInputFeature.generated.h"

/**
 * 相机输入处理Feature
 * 
 * 用于阶段2：State计算管线
 * 负责处理玩家输入，修改ArmRotation等参数
 * 
 * 设计目的：
 * - 统一管理玩家输入处理逻辑
 * - 支持不同的输入模式（NoInput/WithInput）
 * - 支持输入衰减和混合
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class NAMICAMERA_API UNamiCameraInputFeature : public UNamiCameraFeature
{
	GENERATED_BODY()

public:
	UNamiCameraInputFeature();

	// ========== 重写 ==========

	virtual void Initialize_Implementation(UNamiCameraModeBase* InCameraMode) override;

	// ========== 输入处理 ==========

	/**
	 * 处理玩家输入并修改State
	 * 子类可以重写此方法来提供具体的输入处理逻辑
	 * 
	 * @param InOutState 输入输出的State（会被修改）
	 * @param DeltaTime 帧时间
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Input")
	void ProcessPlayerInput(UPARAM(ref) FNamiCameraState& InOutState, float DeltaTime);

	// ========== 配置 ==========

	/**
	 * 输入模式
	 * NoInput: 完全忽略玩家输入
	 * WithInput: 允许玩家输入影响相机
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input",
			  meta = (DisplayName = "输入模式"))
	ENamiCameraInputMode InputMode = ENamiCameraInputMode::WithInput;

	/**
	 * 输入权重（用于混合）
	 * 0.0 = 完全忽略输入，1.0 = 完全应用输入
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input",
			  meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0",
					  DisplayName = "输入权重"))
	float InputWeight = 1.0f;

	/**
	 * 是否启用输入平滑
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input",
			  meta = (DisplayName = "启用输入平滑"))
	bool bEnableInputSmoothing = true;

	/**
	 * 输入平滑时间（秒）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input",
			  meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0",
					  DisplayName = "输入平滑时间", EditCondition = "bEnableInputSmoothing"))
	float InputSmoothTime = 0.1f;

protected:
	/** 输入速度（用于平滑） */
	FVector2D InputVelocity = FVector2D::ZeroVector;
};

