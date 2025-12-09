// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Features/NamiCameraFeature.h"
#include "Structs/State/NamiCameraState.h"
#include "Enums/NamiCameraEnums.h"
#include "NamiCameraInputFeature.generated.h"

/**
 * 相机输入处理 Feature
 * 负责处理玩家输入，修改 ArmRotation 等参数。统一管理玩家输入处理逻辑，支持不同的输入模式和输入衰减混合。
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

	/** 输入模式。NoInput：完全忽略玩家输入。WithInput：允许玩家输入影响相机。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input",
			  meta = (Tooltip = "NoInput：完全忽略玩家输入\nWithInput：允许玩家输入影响相机"))
	ENamiCameraInputMode InputMode = ENamiCameraInputMode::WithInput;

	/** 输入权重（用于混合）。0.0=完全忽略输入，1.0=完全应用输入。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input",
			  meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0",
					  Tooltip = "输入权重（用于混合）。0.0 = 完全忽略输入，1.0 = 完全应用输入。"))
	float InputWeight = 1.0f;

	/** 是否启用输入平滑。启用后，玩家输入会经过平滑处理，避免相机抖动。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input",
			  meta = (Tooltip = "启用后，玩家输入会经过平滑处理，避免相机抖动。"))
	bool bEnableInputSmoothing = true;

	/** 输入平滑时间（秒）。输入平滑的响应时间，值越大越平滑但响应越慢。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Input",
			  meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0",
					  EditCondition = "bEnableInputSmoothing",
					  Tooltip = "输入平滑的响应时间（秒）。值越大越平滑但响应越慢。推荐值：0.05-0.2。"))
	float InputSmoothTime = 0.1f;

protected:
	/** 输入速度（用于平滑） */
	FVector2D InputVelocity = FVector2D::ZeroVector;
};

