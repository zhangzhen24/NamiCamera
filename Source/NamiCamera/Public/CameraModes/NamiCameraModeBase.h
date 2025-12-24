// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/NamiBlendConfig.h"
#include "Core/NamiCameraView.h"
#include "UObject/Object.h"
#include "NamiCameraModeBase.generated.h"

class UNamiCameraComponent;
class UNamiCameraModeComponent;

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
 * - ModeComponent 容器和管理
 * - 视图计算的基础框架
 *
 * 设计原则：
 * - 模式通过组合 ModeComponent 来扩展功能
 * - 模式专注于"计算视图"，ModeComponent 负责"修改视图"
 * - 子类应该尽量重用 ModeComponent 而非直接继承复杂逻辑
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

	// ========== ModeComponent 管理 ==========

	/**
	 * 添加一个组件到模式
	 * @param Component 要添加的组件实例
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Mode|Components")
	void AddComponent(UNamiCameraModeComponent* Component);

	/**
	 * 从模式中移除一个组件
	 * @param Component 要移除的组件实例
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Mode|Components")
	void RemoveComponent(UNamiCameraModeComponent* Component);

	/**
	 * 获取所有组件
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Mode|Components")
	const TArray<UNamiCameraModeComponent*>& GetComponents() const { return ModeComponents; }

	/**
	 * 通过类型获取组件
	 */
	template<typename T>
	T* GetComponent() const
	{
		for (UNamiCameraModeComponent* Component : ModeComponents)
		{
			if (T* TypedComponent = Cast<T>(Component))
			{
				return TypedComponent;
			}
		}
		return nullptr;
	}

	/**
	 * 通过名称获取组件
	 */
	UFUNCTION(BlueprintPure, Category = "Camera Mode|Components")
	UNamiCameraModeComponent* GetComponentByName(FName ComponentName) const;

	/**
	 * 创建并添加组件
	 */
	template<typename T>
	T* CreateAndAddComponent()
	{
		T* NewComponent = NewObject<T>(this);
		AddComponent(NewComponent);
		return NewComponent;
	}

	// ========== 辅助函数 ==========

	/** 获取World */
	virtual UWorld* GetWorld() const override;

	/** 获取相机组件 */
	UFUNCTION(BlueprintPure, Category = "Camera Mode")
	FORCEINLINE UNamiCameraComponent* GetCameraComponent() const { return CameraComponent.Get(); }

	/** 设置相机组件 */
	void SetCameraComponent(UNamiCameraComponent* NewCameraComponent);

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

	/** 获取上一次计算的相机位置 */
	UFUNCTION(BlueprintPure, Category = "Camera Mode")
	FVector GetLastCameraLocation() const { return CurrentView.CameraLocation; }

	/** 获取拥有者 Actor（通过相机组件获取 Pawn） */
	UFUNCTION(BlueprintPure, Category = "Camera Mode")
	AActor* GetOwnerActor() const;

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

	/** 组件列表 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Camera Mode|Components")
	TArray<TObjectPtr<UNamiCameraModeComponent>> ModeComponents;

	/** 组件查找 Map（优化查找性能，按 ComponentName 索引） */
	mutable TMap<FName, UNamiCameraModeComponent*> ComponentMap;

	/** 是否需要重新构建 ComponentMap */
	mutable bool bComponentMapDirty = true;

	/** 重建 ComponentMap（延迟构建，只在需要时重建） */
	void RebuildComponentMap() const;

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
	 * 应用所有组件到视图
	 */
	void ApplyComponentsToView(FNamiCameraView& InOutView, float DeltaTime);

	/**
	 * 更新所有组件
	 */
	void UpdateComponents(float DeltaTime);

	/**
	 * 对组件按优先级排序
	 */
	void SortComponents();

};

