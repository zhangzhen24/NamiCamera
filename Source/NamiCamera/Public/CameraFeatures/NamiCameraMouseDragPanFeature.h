// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CameraFeatures/NamiCameraFeature.h"
#include "InputActionValue.h"
#include "NamiCameraMouseDragPanFeature.generated.h"

class UInputAction;

/**
 * 鼠标拖拽平移Feature
 *
 * 按住中键拖拽鼠标平移相机
 * 支持Enhanced Input System
 */
UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="鼠标拖拽平移"))
class NAMICAMERA_API UNamiCameraMouseDragPanFeature : public UNamiCameraFeature
{
	GENERATED_BODY()

public:
	UNamiCameraMouseDragPanFeature();

	//~ Begin UNamiCameraFeature Interface
	virtual void Activate_Implementation() override;
	virtual void Deactivate_Implementation() override;
	virtual void Update_Implementation(float DeltaTime) override;
	//~ End UNamiCameraFeature Interface

	/** 拖拽灵敏度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mouse Drag",
		meta=(ClampMin="0.1", UIMin="0.1",
		Tooltip="鼠标拖拽的灵敏度，值越大移动越快"))
	float DragSensitivity;

	/** 是否反转拖拽方向 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mouse Drag",
		meta=(Tooltip="true: 鼠标向右拖拽，相机向左移动（反向）"))
	bool bInvertDragDirection;

	/** Enhanced Input Action (中键) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mouse Drag|Input",
		meta=(Tooltip="用于触发拖拽的Input Action（通常是中键）"))
	TObjectPtr<UInputAction> DragAction;

	/** 检查是否正在拖拽 */
	UFUNCTION(BlueprintPure, Category="Mouse Drag")
	bool IsDragging() const { return bIsDragging; }

protected:
	/** 绑定输入动作 */
	void BindInputActions();

	/** 解绑输入动作 */
	void UnbindInputActions();

	/** 输入回调：拖拽开始 */
	void OnDragStarted(const FInputActionValue& Value);

	/** 输入回调：拖拽完成 */
	void OnDragCompleted(const FInputActionValue& Value);

	/** 转换屏幕空间增量到世界空间平移 */
	FVector ConvertScreenDeltaToWorldPan(const FVector2D& ScreenDelta) const;

private:
	/** 是否正在拖拽 */
	bool bIsDragging;

	/** 上一帧的鼠标位置 */
	FVector2D LastMousePosition;
};
