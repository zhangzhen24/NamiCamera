// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CameraFeatures/NamiCameraFeature.h"
#include "InputActionValue.h"
#include "NamiCameraKeyboardPanFeature.generated.h"

class UInputAction;

/**
 * 键盘平移Feature
 *
 * 使用WASD或箭头键平移相机
 * 支持Enhanced Input System
 */
UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="键盘平移"))
class NAMICAMERA_API UNamiCameraKeyboardPanFeature : public UNamiCameraFeature
{
	GENERATED_BODY()

public:
	UNamiCameraKeyboardPanFeature();

	//~ Begin UNamiCameraFeature Interface
	virtual void Activate_Implementation() override;
	virtual void Deactivate_Implementation() override;
	virtual void Update_Implementation(float DeltaTime) override;
	//~ End UNamiCameraFeature Interface

	/** 平移速度（单位/秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Keyboard Pan",
		meta=(ClampMin="0.0", UIMin="0.0",
		Tooltip="键盘平移的速度"))
	float PanSpeed;

	/** 是否相对于相机方向平移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Keyboard Pan",
		meta=(Tooltip="true: W向相机前方平移, false: W向世界北方平移"))
	bool bUseRelativeToCamera;

	/** Enhanced Input Action (2D向量: WASD组合) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Keyboard Pan|Input",
		meta=(Tooltip="2D Input Action用于WASD或箭头键输入"))
	TObjectPtr<UInputAction> Pan2DAction;

	/** 检查是否有活动的平移输入 */
	UFUNCTION(BlueprintPure, Category="Keyboard Pan")
	bool HasActivePanInput() const { return !CurrentPanInput.IsNearlyZero(); }

	/** 获取当前平移输入 */
	UFUNCTION(BlueprintPure, Category="Keyboard Pan")
	FVector2D GetCurrentPanInput() const { return CurrentPanInput; }

protected:
	/** 绑定输入动作 */
	void BindInputActions();

	/** 解绑输入动作 */
	void UnbindInputActions();

	/** 输入回调：平移输入 */
	void OnPan2DInput(const FInputActionValue& Value);

	/** 输入回调：平移结束 */
	void OnPan2DCompleted(const FInputActionValue& Value);

	/** 计算平移方向（2D输入 -> 3D世界方向） */
	FVector CalculatePanDirection(const FVector2D& Input) const;

private:
	/** 当前平移输入向量 */
	FVector2D CurrentPanInput;
};
