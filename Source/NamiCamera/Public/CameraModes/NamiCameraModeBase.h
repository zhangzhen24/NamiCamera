// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Data/NamiCameraView.h"
#include "Data/NamiBlendConfig.h"
#include "NamiCameraModeBase.generated.h"

class UNamiCameraComponent;
class UNamiCameraFeature;

// FAlphaBlend 前向声明
struct FAlphaBlend;
struct FAlphaBlendArgs;

/**
 * 相机模式默认设置
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraModeSettings
{
	GENERATED_BODY()

	/** 视野角度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings",
		meta = (UIMin = "5.0", UIMax = "170.0", ClampMin = "5.0", ClampMax = "170.0"))
	float FieldOfView = 80.0f;

	/** 最小俯仰角 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings",
		meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89", ClampMax = "89"))
	float ViewPitchMin = -89.0f;

	/** 最大俯仰角 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings",
		meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89", ClampMax = "89"))
	float ViewPitchMax = 89.0f;
};

	/**
	 * 相机模式基类
	 * 
	 * 这是所有相机模式的根基类，提供：
	 * - 生命周期管理
	 * - Feature容器和管理
	 * - 视图计算的基础框架
	 * 
	 * 设计原则：
	 * - 模式通过组合Feature来扩展功能
	 * - 模式专注于"计算视图"，Feature负责"修改视图"
	 * - 子类应该尽量重用Feature而非直接继承复杂逻辑
	 */
	UCLASS(Abstract, Blueprintable, BlueprintType)
class NAMICAMERA_API UNamiCameraModeBase : public UObject
{
	GENERATED_BODY()

	friend struct FNamiCameraModeStack;  // 允许 Stack 访问 GetCameraBlendAlpha


public:
	explicit UNamiCameraModeBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ========== 生命周期 ==========

	/**
	 * 初始化模式
	 * @param InCameraComponent 所属的相机组件
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Mode")
	void Initialize(UNamiCameraComponent* InCameraComponent);

	/**
	 * 激活模式
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Mode")
	void Activate();

	/**
	 * 停用模式
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Mode")
	void Deactivate();

	/**
	 * 更新模式（每帧调用）
	 * @param DeltaTime 帧时间
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Mode")
	void Tick(float DeltaTime);

	/**
	 * 计算相机视图
	 * 这是模式的核心方法，子类应该重写此方法
	 *
	 * @param DeltaTime 帧时间
	 * @return 计算后的相机视图
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Mode")
	FNamiCameraView CalculateView(float DeltaTime);

	/**
	 * 计算PivotLocation（阶段1）
	 * 子类应该重写此方法来提供具体的PivotLocation计算逻辑
	 *
	 * @param DeltaTime 帧时间
	 * @return 计算出的PivotLocation
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Camera Mode|Pivot Location")
	FVector CalculatePivotLocation(float DeltaTime);

	// ========== Feature 管理 ==========

	/**
	 * 添加一个Feature到模式
	 * @param Feature 要添加的Feature实例
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Mode|Features")
	void AddFeature(UNamiCameraFeature* Feature);

	/**
	 * 从模式中移除一个Feature
	 * @param Feature 要移除的Feature实例
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Mode|Features")
	void RemoveFeature(UNamiCameraFeature* Feature);

	/**
	 * 获取所有Feature
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Mode|Features")
	const TArray<UNamiCameraFeature*>& GetFeatures() const { return Features; }

	/**
	 * 通过类型获取Feature
	 */
	template<typename T>
	T* GetFeature() const
	{
		for (UNamiCameraFeature* Feature : Features)
		{
			if (T* TypedFeature = Cast<T>(Feature))
			{
				return TypedFeature;
			}
		}
		return nullptr;
	}

	/**
	 * 通过名称获取Feature
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Mode|Features")
	UNamiCameraFeature* GetFeatureByName(FName FeatureName) const;

	/**
	 * 创建并添加Feature
	 */
	template<typename T>
	T* CreateAndAddFeature()
	{
		T* NewFeature = NewObject<T>(this);
		AddFeature(NewFeature);
		return NewFeature;
	}

	// ========== 辅助函数 ==========

	/** 获取World */
	virtual UWorld* GetWorld() const override;

	/** 获取相机组件 */
	UFUNCTION(BlueprintPure, Category = "Camera Mode")
	FORCEINLINE UNamiCameraComponent* GetCameraComponent() const { return CameraComponent.Get(); }

	/** 设置相机组件 */
	FORCEINLINE void SetCameraComponent(UNamiCameraComponent* NewCameraComponent);

	/** 是否激活 */
	UFUNCTION(BlueprintPure, Category = "Camera Mode")
	bool IsActive() const { return State == ENamiCameraModeState::Active; }

	/** 获取状态 */
	UFUNCTION(BlueprintPure, Category = "Camera Mode")
	ENamiCameraModeState GetState() const { return State; }

	/** 获取当前视图 */
	UFUNCTION(BlueprintPure, Category = "Camera Mode")
	const FNamiCameraView& GetCurrentView() const { return CurrentView; }

	/** 获取视图（供 Stack 使用，与 EnhancedCameraSystem 保持一致） */
	const FNamiCameraView& GetView() const { return CurrentView; }

	/** 获取混合权重 */
	UFUNCTION(BlueprintPure, Category = "Camera Mode")
	float GetBlendWeight() const { return BlendWeight; }

	/** 设置混合权重 */
	UFUNCTION(BlueprintCallable, Category = "Camera Mode")
	void SetBlendWeight(float InWeight);

	/** 获取 FAlphaBlendArgs 引用（供 Stack 使用，与 EnhancedCameraSystem 兼容） */
	FAlphaBlendArgs& GetBlendAlpha() { return BlendStack; }

	/** 获取混合 Alpha 的引用（用于外部设置淡出） */
	FAlphaBlend& GetBlendAlphaRef() { return CameraBlendAlpha; }


public:
	// ========== 配置 ==========

	/** 混合配置（用户友好配置） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Mode|Blending")
	FNamiBlendConfig BlendConfig;

	/** 混合参数（内部使用，与 EnhancedCameraSystem 兼容） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Mode|Blending", meta = (ExposeOnSpawn, DisplayAfter = "BlendConfig"))
	FAlphaBlendArgs BlendStack;

	/** 默认FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Mode",
		meta = (ClampMin = "30.0", ClampMax = "120.0"))
	float DefaultFOV = 90.0f;

	/** 优先级（用于模式堆栈排序） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Mode")
	int32 Priority = 0;

protected:

	/** 相机组件 */
	UPROPERTY(BlueprintReadOnly, Category = "Camera Mode", meta = (AllowPrivateAccess = "true"))
	TWeakObjectPtr<UNamiCameraComponent> CameraComponent;

	/** Feature列表 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Camera Mode|Features")
	TArray<TObjectPtr<UNamiCameraFeature>> Features;

	/** Feature 查找 Map（优化查找性能，按 FeatureName 索引） */
	mutable TMap<FName, UNamiCameraFeature*> FeatureMap;

	/** 是否需要重新构建 FeatureMap */
	mutable bool bFeatureMapDirty = true;

	/** 重建 FeatureMap（延迟构建，只在需要时重建） */
	void RebuildFeatureMap() const;

	/** 当前视图 */
	FNamiCameraView CurrentView;

	/** FAlphaBlend 实例（用于管理混合权重） */
	UPROPERTY(BlueprintReadOnly, Category = "Camera Mode", meta = (AllowPrivateAccess = "true"))
	FAlphaBlend CameraBlendAlpha;

	/** 混合权重 */
	float BlendWeight = 0.0f;

	/** 当前状态 */
	ENamiCameraModeState State = ENamiCameraModeState::None;

	/** 是否已激活（用于判断是否需要更新） */
	bool bIsActivated = false;

	/**
	 * 更新混合权重（使用 FAlphaBlend）
	 * 应该在每帧的 Tick 中调用
	 */
	void UpdateBlending(float DeltaTime);

	/**
	 * 应用所有Feature到视图
	 */
	void ApplyFeaturesToView(FNamiCameraView& InOutView, float DeltaTime);

	/**
	 * 更新所有Feature
	 */
	void UpdateFeatures(float DeltaTime);

	/**
	 * 对Feature按优先级排序
	 */
	void SortFeatures();

};

