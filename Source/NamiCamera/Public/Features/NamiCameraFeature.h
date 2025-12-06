// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Structs/View/NamiCameraView.h"
#include "NamiCameraFeature.generated.h"

class UNamiCameraModeBase;

/**
 * 相机功能基类
 * 用于模块化扩展相机模式的功能
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class NAMICAMERA_API UNamiCameraFeature : public UObject
{
	GENERATED_BODY()

public:
	UNamiCameraFeature();

	// ========== 生命周期 ==========

	/**
	 * 初始化功能
	 * @param InCameraMode 所属的相机模式
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Feature")
	void Initialize(UNamiCameraModeBase* InCameraMode);

	/**
	 * 激活功能
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Feature")
	void Activate();

	/**
	 * 停用功能
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Feature")
	void Deactivate();

	/**
	 * 每帧更新
	 * @param DeltaTime 帧时间
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Feature")
	void Update(float DeltaTime);

	// ========== 视图应用 ==========

	/**
	 * 应用功能到视图
	 * @param InOutView 输入输出的视图
	 * @param DeltaTime 帧时间
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Feature")
	void ApplyToView(UPARAM(ref) FNamiCameraView& InOutView, float DeltaTime);

	// ========== 辅助函数 ==========

	/** 获取World */
	virtual UWorld* GetWorld() const override;

	/** 是否启用 */
	UFUNCTION(BlueprintPure, Category = "Camera Feature")
	bool IsEnabled() const { return bEnabled; }

	/** 设置启用状态 */
	UFUNCTION(BlueprintCallable, Category = "Camera Feature")
	void SetEnabled(bool bInEnabled) { bEnabled = bInEnabled; }

	/** 获取所属的相机模式 */
	UFUNCTION(BlueprintPure, Category = "Camera Feature")
	UNamiCameraModeBase* GetCameraMode() const { return CameraMode.Get(); }

	// ========== GameplayTags ==========

	/**
	 * 添加 Tag
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Feature|Tags")
	void AddTag(const FGameplayTag& Tag) { Tags.AddTag(Tag); }

	/**
	 * 移除 Tag
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Feature|Tags")
	void RemoveTag(const FGameplayTag& Tag) { Tags.RemoveTag(Tag); }

	/**
	 * 检查是否包含 Tag
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Feature|Tags")
	bool HasTag(const FGameplayTag& Tag) const { return Tags.HasTag(Tag); }

	/**
	 * 检查是否包含任意 Tag
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Feature|Tags")
	bool HasAnyTag(const FGameplayTagContainer& TagContainer) const { return Tags.HasAny(TagContainer); }

	/**
	 * 检查是否包含所有 Tag
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Feature|Tags")
	bool HasAllTags(const FGameplayTagContainer& TagContainer) const { return Tags.HasAll(TagContainer); }

public:
	/** 功能名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Feature")
	FName FeatureName = NAME_None;

	/** 优先级（数值越大优先级越高） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Feature")
	int32 Priority = 0;

	/** 是否启用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Feature")
	bool bEnabled = true;

	/** Gameplay Tags（用于标记和分类 Feature，类似 GAS 系统） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Feature")
	FGameplayTagContainer Tags;

protected:
	/** 所属的相机模式 */
	UPROPERTY()
	TWeakObjectPtr<UNamiCameraModeBase> CameraMode;
};

