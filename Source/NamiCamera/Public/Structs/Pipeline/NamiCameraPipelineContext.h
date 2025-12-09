// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Structs/View/NamiCameraView.h"
#include "Structs/State/NamiCameraState.h"
#include "Enums/NamiCameraEnums.h"
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

	/** 基础状态（来自 Mode Stack），用于 Feature 获取"原始"状态，作为混出目标 */
	FNamiCameraState BaseState;
	bool bHasBaseState = false;

	/** 玩家输入状态（来自 Input Feature），用于打断时获取玩家输入值 */
	FNamiCameraState PlayerInputState;
	bool bHasPlayerInputState = false;

	/** 玩家输入信息 */
	struct FPlayerInputInfo
	{
		FVector2D InputVector = FVector2D::ZeroVector;  // (X=Yaw, Y=Pitch)
		float InputMagnitude = 0.0f;
		bool bHasInput = false;

		void Update(const FVector2D& InVector)
		{
			InputVector = InVector;
			InputMagnitude = InVector.Size();
			bHasInput = InputMagnitude > KINDA_SMALL_NUMBER;
		}

		void Reset()
		{
			InputVector = FVector2D::ZeroVector;
			InputMagnitude = 0.0f;
			bHasInput = false;
		}
	} PlayerInput;

	// ========== 新增：参数更新控制 ==========

	/** 参数更新控制，定义每个参数的更新模式 */
	struct FParamUpdateControl
	{
		// 枢轴点参数
		ENamiCameraParamUpdateMode PivotLocation = ENamiCameraParamUpdateMode::Normal;
		ENamiCameraParamUpdateMode PivotRotation = ENamiCameraParamUpdateMode::Normal;

		// 吊臂参数
		ENamiCameraParamUpdateMode ArmLength = ENamiCameraParamUpdateMode::Normal;
		ENamiCameraParamUpdateMode ArmRotation = ENamiCameraParamUpdateMode::Normal;
		ENamiCameraParamUpdateMode ArmOffset = ENamiCameraParamUpdateMode::Normal;

		// 相机参数
		ENamiCameraParamUpdateMode CameraLocationOffset = ENamiCameraParamUpdateMode::Normal;
		ENamiCameraParamUpdateMode CameraRotationOffset = ENamiCameraParamUpdateMode::Normal;
		ENamiCameraParamUpdateMode FieldOfView = ENamiCameraParamUpdateMode::Normal;

		// 输出结果
		ENamiCameraParamUpdateMode CameraLocation = ENamiCameraParamUpdateMode::Normal;
		ENamiCameraParamUpdateMode CameraRotation = ENamiCameraParamUpdateMode::Normal;

		void Reset()
		{
			PivotLocation = ENamiCameraParamUpdateMode::Normal;
			PivotRotation = ENamiCameraParamUpdateMode::Normal;
			ArmLength = ENamiCameraParamUpdateMode::Normal;
			ArmRotation = ENamiCameraParamUpdateMode::Normal;
			ArmOffset = ENamiCameraParamUpdateMode::Normal;
			CameraLocationOffset = ENamiCameraParamUpdateMode::Normal;
			CameraRotationOffset = ENamiCameraParamUpdateMode::Normal;
			FieldOfView = ENamiCameraParamUpdateMode::Normal;
			CameraLocation = ENamiCameraParamUpdateMode::Normal;
			CameraRotation = ENamiCameraParamUpdateMode::Normal;
		}

		void SetRotationParamsMode(ENamiCameraParamUpdateMode Mode)
		{
			PivotRotation = Mode;
			ArmRotation = Mode;
			CameraRotationOffset = Mode;
			CameraRotation = Mode;
		}

		void SetLocationParamsMode(ENamiCameraParamUpdateMode Mode)
		{
			PivotLocation = Mode;
			ArmLength = Mode;
			ArmOffset = Mode;
			CameraLocationOffset = Mode;
			CameraLocation = Mode;
		}
	} ParamUpdateControl;

	/** 保存的参数值（用于 Frozen/Preserved 模式） */
	struct FPreservedValues
	{
		FRotator PivotRotation;
		FRotator ArmRotation;
		FRotator CameraRotationOffset;
		FRotator CameraRotation;
		FVector PivotLocation;
		float ArmLength;
		FVector ArmOffset;
		FVector CameraLocationOffset;
		float FieldOfView;
		FVector CameraLocation;

		void PreserveFromState(const FNamiCameraState& State);
		void RestoreRotationToState(FNamiCameraState& OutState) const;
		void RestoreLocationToState(FNamiCameraState& OutState) const;
		void Reset()
		{
			PivotRotation = FRotator::ZeroRotator;
			ArmRotation = FRotator::ZeroRotator;
			CameraRotationOffset = FRotator::ZeroRotator;
			CameraRotation = FRotator::ZeroRotator;
			PivotLocation = FVector::ZeroVector;
			ArmLength = 0.0f;
			ArmOffset = FVector::ZeroVector;
			CameraLocationOffset = FVector::ZeroVector;
			FieldOfView = 0.0f;
			CameraLocation = FVector::ZeroVector;
		}
	} PreservedValues;

	bool bHasPreservedValues = false;

	// ========== 新增：输入控制状态 ==========

	/** 输入控制状态，管理当前 Feature 的输入控制模式 */
	ENamiCameraInputControlState InputControlState = ENamiCameraInputControlState::PlayerInput;

	/** 是否已触发打断 */
	bool bIsInterrupted = false;

	/** 打断触发时间（用于调试） */
	float InterruptTime = 0.0f;

	/** 重置上下文 */
	void Reset()
	{
		DeltaTime = 0.0f;
		OwnerPawn = nullptr;
		OwnerPC = nullptr;
		CameraManager = nullptr;
		EffectView = FNamiCameraView();
		bIsValid = false;

		// 重置新增字段
		BaseState = FNamiCameraState();
		bHasBaseState = false;
		PlayerInputState = FNamiCameraState();
		bHasPlayerInputState = false;
		PlayerInput.Reset();
		ParamUpdateControl.Reset();
		bHasPreservedValues = false;
		PreservedValues.Reset();
		InputControlState = ENamiCameraInputControlState::PlayerInput;
		bIsInterrupted = false;
		InterruptTime = 0.0f;
	}
};

