// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/NamiCameraState.h"
#include "Data/NamiCameraView.h"
#include "NamiCameraPipelineContext.generated.h"

class APawn;
class APlayerController;
class ANamiPlayerCameraManager;

/**
 * 相机管线上下文
 * 用于在管线各阶段之间传递数据
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraPipelineContext
{
	GENERATED_BODY()

	/** 帧时间 */
	float DeltaTime = 0.0f;

	/** 所有者 Pawn */
	UPROPERTY()
	TObjectPtr<APawn> OwnerPawn = nullptr;

	/** 所有者 PlayerController */
	UPROPERTY()
	TObjectPtr<APlayerController> OwnerPC = nullptr;

	/** 所有者 PlayerCameraManager */
	UPROPERTY()
	TObjectPtr<ANamiPlayerCameraManager> CameraManager = nullptr;

	/** 应用全局效果后的视图（用于 Debug 绘制） */
	FNamiCameraView EffectView;

	/** 是否有效 */
	bool bIsValid = false;

	// ========== 新增：状态信息 ==========

	/** 基础状态（来自 Mode Stack），用于 ModeComponent 获取"原始"状态，作为混出目标 */
	FNamiCameraState BaseState;
	bool bHasBaseState = false;

	/** 重置上下文 */
	void Reset()
	{
		DeltaTime = 0.0f;
		OwnerPawn = nullptr;
		OwnerPC = nullptr;
		CameraManager = nullptr;
		EffectView = FNamiCameraView();
		bIsValid = false;

		BaseState = FNamiCameraState();
		bHasBaseState = false;
	}
};

