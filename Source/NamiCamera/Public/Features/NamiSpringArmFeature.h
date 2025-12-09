// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Features/NamiCameraFeature.h"
#include "Components/NamiSpringArm.h"
#include "NamiSpringArmFeature.generated.h"

/**
 * SpringArm Feature
 * 
 * 为相机模式添加弹簧臂功能，包括：
 * - 碰撞检测
 * - 相机滞后（Camera Lag）
 * - 旋转滞后（Rotation Lag）
 * 
 * 工作原理：
 * - 在 ApplyToView() 中，使用 SpringArm 计算相机位置
 * - SpringArm 会处理碰撞检测，防止相机穿透关卡
 * - 支持相机滞后，使相机平滑跟随目标
 * 
 * 适用场景：
 * - 第三人称相机
 * - 跟随相机
 * - 任何需要碰撞检测的相机
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class NAMICAMERA_API UNamiSpringArmFeature : public UNamiCameraFeature
{
	GENERATED_BODY()

public:
	UNamiSpringArmFeature();

	// ========== Feature 生命周期 ==========
	virtual void Initialize_Implementation(UNamiCameraModeBase* InCameraMode) override;
	virtual void Activate_Implementation() override;
	virtual void Update_Implementation(float DeltaTime) override;
	virtual void ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime) override;

	// ========== 配置 ==========

	/** SpringArm配置（弹簧臂） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm", 
		meta = (ShowInnerProperties, Tooltip = "SpringArm配置（弹簧臂）"))
	FNamiSpringArm SpringArm;

	/** 是否自动从相机模式获取目标Actor（用于忽略碰撞） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm",
		meta = (Tooltip = "是否自动从相机模式获取目标Actor（用于忽略碰撞）"))
	bool bAutoGetTarget = true;

	/** 手动指定的忽略Actor列表（如果bAutoGetTarget为false时使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm",
		meta = (EditCondition = "!bAutoGetTarget",
				Tooltip = "手动指定的忽略Actor列表（如果bAutoGetTarget为false时使用）"))
	TArray<TObjectPtr<AActor>> IgnoreActors;

protected:
	/** 获取需要忽略的Actor列表 */
	TArray<AActor*> GetIgnoreActors() const;
};

