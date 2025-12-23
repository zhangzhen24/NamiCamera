// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Data/NamiCameraView.h"
#include "NamiCameraModeComponent.generated.h"

class UNamiCameraModeBase;
struct FNamiCameraPipelineContext;

/**
 * 相机模式组件基类
 *
 * 用于模块化扩展相机模式的功能。模式组件是持久生效的相机效果，
 * 通过 AddComponent() 添加到相机模式后立即生效。
 *
 * 典型用例：
 * - 相机震动 (ShakeComponent)
 * - 碰撞检测 (CollisionComponent)
 * - 锁定目标 (LockOnComponent)
 * - 目标可见性 (VisibilityComponent)
 *
 * 使用方式：
 * 1. 继承此类并重写 ApplyToView() 方法
 * 2. 通过 CameraComponent->AddComponent() 添加
 * 3. 通过 CameraComponent->RemoveComponent() 移除
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class NAMICAMERA_API UNamiCameraModeComponent : public UObject
{
	GENERATED_BODY()

public:
	UNamiCameraModeComponent();

	// ========== 生命周期 ==========

	/**
	 * 初始化组件
	 * @param InCameraMode 所属的相机模式
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Mode Component")
	void Initialize(UNamiCameraModeBase* InCameraMode);

	/**
	 * 激活组件
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Mode Component")
	void Activate();

	/**
	 * 停用组件
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Mode Component")
	void Deactivate();

	/**
	 * 每帧更新
	 * @param DeltaTime 帧时间
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Mode Component")
	void Update(float DeltaTime);

	// ========== 视图应用 ==========

	/**
	 * 应用组件效果到视图
	 * @param InOutView 输入输出的视图
	 * @param DeltaTime 帧时间
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Mode Component")
	void ApplyToView(UPARAM(ref) FNamiCameraView& InOutView, float DeltaTime);

	/**
	 * 应用组件效果到视图（带 Context）
	 * @param InOutView 输入输出的视图
	 * @param DeltaTime 帧时间
	 * @param Context 管线上下文
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Mode Component")
	void ApplyToViewWithContext(
		UPARAM(ref) FNamiCameraView& InOutView,
		float DeltaTime,
		UPARAM(ref) FNamiCameraPipelineContext& Context);

	// ========== 辅助函数 ==========

	/** 获取 World */
	virtual UWorld* GetWorld() const override;

	/** 是否启用 */
	UFUNCTION(BlueprintPure, Category = "Camera Mode Component")
	bool IsEnabled() const { return bEnabled; }

	/** 设置启用状态 */
	UFUNCTION(BlueprintCallable, Category = "Camera Mode Component")
	void SetEnabled(bool bInEnabled) { bEnabled = bInEnabled; }

	/** 获取所属的相机模式 */
	UFUNCTION(BlueprintPure, Category = "Camera Mode Component")
	UNamiCameraModeBase* GetCameraMode() const { return CameraMode.Get(); }

	// ========== GameplayTags ==========

	/** 添加 Tag */
	UFUNCTION(BlueprintCallable, Category = "Camera Mode Component|Tags")
	void AddTag(const FGameplayTag& Tag) { Tags.AddTag(Tag); }

	/** 移除 Tag */
	UFUNCTION(BlueprintCallable, Category = "Camera Mode Component|Tags")
	void RemoveTag(const FGameplayTag& Tag) { Tags.RemoveTag(Tag); }

	/** 检查是否包含 Tag */
	UFUNCTION(BlueprintPure, Category = "Camera Mode Component|Tags")
	bool HasTag(const FGameplayTag& Tag) const { return Tags.HasTag(Tag); }

	/** 检查是否包含任意 Tag */
	UFUNCTION(BlueprintPure, Category = "Camera Mode Component|Tags")
	bool HasAnyTag(const FGameplayTagContainer& TagContainer) const { return Tags.HasAny(TagContainer); }

	/** 检查是否包含所有 Tag */
	UFUNCTION(BlueprintPure, Category = "Camera Mode Component|Tags")
	bool HasAllTags(const FGameplayTagContainer& TagContainer) const { return Tags.HasAll(TagContainer); }

public:
	/** 组件名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Mode Component")
	FName ComponentName = NAME_None;

	/** 优先级（数值越大优先级越高） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Mode Component")
	int32 Priority = 0;

	/** 是否启用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Mode Component")
	bool bEnabled = true;

	/** Gameplay Tags（用于标记和分类组件） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Mode Component")
	FGameplayTagContainer Tags;

protected:
	/** 所属的相机模式 */
	UPROPERTY()
	TWeakObjectPtr<UNamiCameraModeBase> CameraMode;
};
