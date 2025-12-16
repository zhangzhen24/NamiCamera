// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CameraModes/NamiFollowCameraMode.h"
#include "NamiTopDownCameraMode.generated.h"

/**
 * 俯视角相机模式
 *
 * 特点：
 * - 固定的俯视角度，适合 MOBA、RTS、ARPG 等游戏类型
 * - 锁定 Pitch 和 Roll，只允许 Yaw 旋转
 * - 可选的鼠标位置追踪功能
 * - 内置距离控制（不依赖 SpringArm 碰撞检测）
 *
 * 架构说明：
 * - 直接继承 FollowCameraMode，不经过 SpringArmCameraMode
 * - 使用简单的距离计算代替 SpringArm，更适合俯视角场景
 * - 不需要碰撞检测，保持稳定的相机距离
 */
UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="顶视角相机模式"))
class NAMICAMERA_API UNamiTopDownCameraMode : public UNamiFollowCameraMode
{
	GENERATED_BODY()

public:
	UNamiTopDownCameraMode();

	//~ Begin UNamiCameraModeBase Interface
	virtual FNamiCameraView CalculateView_Implementation(float DeltaTime) override;
	//~ End UNamiCameraModeBase Interface

	// ========== Zoom Control Methods ==========

	/** 向内缩放（靠近目标） */
	UFUNCTION(BlueprintCallable, Category="NamiCamera|TopDown|Zoom")
	void ZoomIn(float Amount);

	/** 向外缩放（远离目标） */
	UFUNCTION(BlueprintCallable, Category="NamiCamera|TopDown|Zoom")
	void ZoomOut(float Amount);

	/** 设置目标缩放距离 */
	UFUNCTION(BlueprintCallable, Category="NamiCamera|TopDown|Zoom")
	void SetTargetZoomDistance(float Distance);

	/** 获取当前缩放距离 */
	UFUNCTION(BlueprintPure, Category="NamiCamera|TopDown|Zoom")
	float GetCurrentZoomDistance() const { return CurrentZoomDistance; }

	/** 获取目标缩放距离 */
	UFUNCTION(BlueprintPure, Category="NamiCamera|TopDown|Zoom")
	float GetTargetZoomDistance() const { return TargetZoomDistance; }

	// ========== Pan Control Methods ==========

	/** 添加平移偏移 */
	UFUNCTION(BlueprintCallable, Category="NamiCamera|TopDown|Pan",
		meta=(Tooltip="向当前PanOffset添加偏移量"))
	void AddPanOffset(const FVector& Offset);

	/** 重置平移偏移 */
	UFUNCTION(BlueprintCallable, Category="NamiCamera|TopDown|Pan",
		meta=(Tooltip="立即将PanOffset重置为零"))
	void ResetPanOffset();

	/** 检查是否正在被平移 */
	UFUNCTION(BlueprintPure, Category="NamiCamera|TopDown|Pan",
		meta=(Tooltip="检查是否有平移Feature正在活动"))
	bool IsBeingPanned() const;

protected:
	/** 相机俯仰角度（负值表示向下看） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown",
		meta=(ClampMin="-89.0", ClampMax="0.0", UIMin="-89.0", UIMax="0.0",
		Tooltip="相机俯视角度，范围 [-89, 0]，推荐值: -55"))
	float CameraPitch;

	/** 相机偏航角度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown",
		meta=(ClampMin="-180.0", ClampMax="180.0", UIMin="-180.0", UIMax="180.0",
		Tooltip="相机水平旋转角度"))
	float CameraYaw;

	/** 相机横滚角度（通常保持为 0） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown",
		meta=(ClampMin="-180.0", ClampMax="180.0", UIMin="-180.0", UIMax="180.0",
		Tooltip="相机倾斜角度，通常保持为 0"))
	float CameraRoll;

	/** 是否忽略 Pitch 输入（锁定俯仰角） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown",
		meta=(Tooltip="是否锁定俯仰角，防止玩家改变相机上下视角"))
	bool bIgnorePitch;

	/** 是否忽略 Roll 输入（锁定横滚角） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown",
		meta=(Tooltip="是否锁定横滚角，保持相机水平"))
	bool bIgnoreRoll;

	/** 是否启用鼠标位置追踪 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Advanced",
		meta=(Tooltip="是否根据鼠标位置微调相机焦点"))
	bool bEnableMouseTracking;

	/** 鼠标追踪影响强度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Advanced",
		meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0",
		EditCondition="bEnableMouseTracking",
		Tooltip="鼠标位置对相机焦点的影响程度"))
	float MouseTrackingStrength;

	/** 鼠标追踪最大偏移距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Advanced",
		meta=(ClampMin="0.0", UIMin="0.0",
		EditCondition="bEnableMouseTracking",
		Tooltip="鼠标追踪允许的最大偏移距离"))
	float MaxMouseTrackingOffset;

	// ========== Distance & Zoom Configuration ==========

	/** 初始相机距离（Pivot 到相机的距离） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Zoom",
		meta=(ClampMin="100.0", UIMin="100.0",
		Tooltip="相机与目标点的初始距离"))
	float CameraDistance;

	/** 是否启用鼠标滚轮缩放 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Zoom",
		meta=(Tooltip="是否允许使用鼠标滚轮缩放相机距离"))
	bool bEnableMouseWheelZoom;

	/** 最小缩放距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Zoom",
		meta=(ClampMin="100.0", UIMin="100.0",
		EditCondition="bEnableMouseWheelZoom",
		Tooltip="相机可以缩放到的最近距离"))
	float MinZoomDistance;

	/** 最大缩放距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Zoom",
		meta=(ClampMin="100.0", UIMin="100.0",
		EditCondition="bEnableMouseWheelZoom",
		Tooltip="相机可以缩放到的最远距离"))
	float MaxZoomDistance;

	/** 缩放速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Zoom",
		meta=(ClampMin="1.0", UIMin="1.0",
		EditCondition="bEnableMouseWheelZoom",
		Tooltip="鼠标滚轮缩放的速度"))
	float ZoomSpeed;

	/** 缩放平滑速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Zoom",
		meta=(ClampMin="0.1", UIMin="0.1",
		EditCondition="bEnableMouseWheelZoom",
		Tooltip="缩放插值的平滑速度，值越大缩放越快"))
	float ZoomSmoothSpeed;

	// ========== Pan Offset System ==========

	/** 平移偏移量（用于手动平移相机） */
	UPROPERTY(BlueprintReadOnly, Category="NamiCamera|TopDown|Pan",
		meta=(Tooltip="当前的相机平移偏移量"))
	FVector PanOffset;

	/** 最大平移距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Pan",
		meta=(ClampMin="0.0", UIMin="0.0",
		Tooltip="相机可以平移离开角色的最大距离"))
	float MaxPanDistance;

	/** 平移回弹速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Pan",
		meta=(ClampMin="0.1", UIMin="0.1",
		Tooltip="停止平移后，相机返回角色中心的速度"))
	float PanReturnSpeed;

	/** 是否自动返回角色中心 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Pan",
		meta=(Tooltip="停止平移后，是否自动将相机平滑移回角色中心"))
	bool bAutoReturnToCharacter;

	// ========== Auto Target Setup ==========

	/** 激活时是否自动设置玩家主角为跟随目标 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NamiCamera|TopDown|Setup",
		meta=(Tooltip="激活相机模式时，自动将拥有此相机的Pawn设为PrimaryTarget"))
	bool bAutoSetOwnerAsPrimaryTarget;

protected:
	//~ Begin UNamiCameraModeBase Interface
	virtual void Activate_Implementation() override;
	virtual void Tick_Implementation(float DeltaTime) override;
	//~ End UNamiCameraModeBase Interface

	/** 更新缩放逻辑 */
	void UpdateZoom(float DeltaTime);

	/** 更新平移偏移逻辑 */
	void UpdatePanOffset(float DeltaTime);

	/** 追踪鼠标位置（如果启用） */
	FVector TraceMousePosition(const FVector& BasePivot) const;

private:
	/** 当前缩放距离（实际值） */
	float CurrentZoomDistance;

	/** 目标缩放距离（目标值） */
	float TargetZoomDistance;
};
