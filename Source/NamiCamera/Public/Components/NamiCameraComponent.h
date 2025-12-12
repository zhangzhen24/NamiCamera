// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Structs/Stack/NamiCameraModeStack.h"
#include "Structs/Mode/NamiCameraModeHandle.h"
#include "Structs/Mode/NamiCameraModeStackEntry.h"
#include "Structs/Pipeline/NamiCameraPipelineContext.h"
#include "Adjusts/NamiCameraAdjustParams.h"
#include "Camera/CameraComponent.h"
#include "Modes/NamiCameraModeBase.h"
#include "NamiCameraComponent.generated.h"

// 前向声明
class ANamiPlayerCameraManager;
class UNamiCameraAdjust;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPushCameraModeDelegate, UNamiCameraModeBase *, CameraModeInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPopCameraModeDelegate);

/**
 * Nami相机组件
 * 管理相机模式堆栈和混合，提供完整的相机管线处理。
 */
UCLASS(Blueprintable, BlueprintType, AutoExpandCategories = ("Settings"))
class NAMICAMERA_API UNamiCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:
	UNamiCameraComponent(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());

	// ========== UActorComponent ==========
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	// ========== End UActorComponent ==========

	// ========== UCameraComponent ==========
	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo &DesiredView) override;
	// ========== End UCameraComponent ==========

	// ========== 辅助函数 ==========

	/** 获取所有者Pawn */
	APawn *GetOwnerPawn() const;

	/** 获取所有者PlayerController */
	APlayerController *GetOwnerPlayerController() const;

	/** 获取所有者PlayerCameraManager */
	ANamiPlayerCameraManager *GetOwnerPlayerCameraManager() const;

	// ========== Debug ==========

	/** 打印当前的相机模式堆栈 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera")
	void DumpCameraModeStack(bool bPrintToScreen = true, bool bPrintToLog = true,
							 FLinearColor TextColor = FLinearColor::Green, float Duration = 0.2f) const;

	/** 获取默认相机模式类 */
	TSubclassOf<UNamiCameraModeBase> GetDefaultCameraModeClass() const { return DefaultCameraMode; }

	/** 绘制相机 Debug 信息（PivotLocation、CameraLocation、ArmLength 等） */
	void DrawDebugCameraInfo(const struct FNamiCameraView& View) const;

	// ========== Camera Mode ==========

	/** 推送相机模式 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera")
	FNamiCameraModeHandle PushCameraMode(TSubclassOf<UNamiCameraModeBase> CameraModeClass, int32 Priority = 0);

	/** 使用实例推送相机模式 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera")
	FNamiCameraModeHandle PushCameraModeUsingInstance(UNamiCameraModeBase *CameraModeInstance, int32 Priority = 0);

	/** 移除相机模式 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera")
	bool PopCameraMode(UPARAM(ref) FNamiCameraModeHandle &ModeHandle);

	/** 使用实例移除相机模式 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera")
	bool PopCameraModeInstance(UNamiCameraModeBase *CameraMode);

	/** 获取当前激活的相机模式 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera")
	UNamiCameraModeBase *GetActiveCameraMode() const;

	// ========== Camera Modifier ==========

	/** 推送CameraModifier */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera")
	UCameraModifier *PushCameraModifier(TSubclassOf<UCameraModifier> CameraModifierClass);

	/** 使用实例推送CameraModifier */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera")
	bool PushCameraModifierInstance(UCameraModifier *CameraModifierInstance);

	/** 移除CameraModifier */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera")
	bool PopCameraModifierInstance(UCameraModifier *CameraModifierInstance);

	/** 获取激活的CameraModifiers */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera")
	TArray<UCameraModifier *> GetActivateCameraModifiers() const;

	// ========== Global Features ==========

	/** 添加全局 Feature（独立于 Mode 管理） */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Features")
	void AddGlobalFeature(UNamiCameraFeature* Feature);

	/** 移除全局 Feature */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Features")
	bool RemoveGlobalFeature(UNamiCameraFeature* Feature);

	/** 通过名称查找全局 Feature */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Features")
	UNamiCameraFeature* FindGlobalFeatureByName(FName FeatureName) const;

	/** 获取所有全局 Feature */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Features")
	const TArray<UNamiCameraFeature*>& GetGlobalFeatures() const { return GlobalFeatures; }

	/** 通过 Tag 移除全局 Feature */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Features")
	void RemoveGlobalFeaturesByTag(const FGameplayTag& Tag, bool bDeactivateFirst = true);

	/** 通过 Tag 容器移除全局 Feature（匹配任意 Tag） */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Features")
	void RemoveGlobalFeaturesByTags(const FGameplayTagContainer& TagContainer, bool bDeactivateFirst = true);

	/** 移除所有 Stay 类型的全局 Feature（便捷方法） */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Features")
	void RemoveStayGlobalFeatures();

	// ========== Feature 查找（全局 + 当前激活Mode） ==========

	/**
	 * 通过类型查找 Feature（先查找全局Feature，再查找当前激活Mode的Feature）
	 * @tparam T Feature类型
	 * @return 找到的Feature，如果不存在则返回nullptr
	 */
	template<typename T>
	T* GetFeature() const
	{
		// 先查找全局Feature
		for (UNamiCameraFeature* Feature : GlobalFeatures)
		{
			if (T* TypedFeature = Cast<T>(Feature))
			{
				return TypedFeature;
			}
		}
		
		// 再查找当前激活Mode的Feature
		if (UNamiCameraModeBase* ActiveMode = GetActiveCameraMode())
		{
			return ActiveMode->template GetFeature<T>();
		}
		
		return nullptr;
	}

	/**
	 * 通过名称查找 Feature（先查找全局Feature，再查找当前激活Mode的Feature）
	 * @param FeatureName Feature名称
	 * @return 找到的Feature，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Features")
	UNamiCameraFeature* GetFeatureByName(FName FeatureName) const;

	/**
	 * 通过 Tag 查找所有匹配的 Feature（全局 + 当前激活Mode）
	 * @param Tag 要匹配的Tag
	 * @return 匹配的Feature列表
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Features")
	TArray<UNamiCameraFeature*> GetFeaturesByTag(const FGameplayTag& Tag) const;

	/**
	 * 通过 Tag 容器查找所有匹配的 Feature（匹配任意Tag，全局 + 当前激活Mode）
	 * @param TagContainer Tag容器
	 * @return 匹配的Feature列表
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Features")
	TArray<UNamiCameraFeature*> GetFeaturesByTags(const FGameplayTagContainer& TagContainer) const;

	// ========== Camera Adjust ==========

	/**
	 * 推送相机调整器（通过类创建实例）
	 * @param AdjustClass 调整器类
	 * @return 创建的调整器实例
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Adjust")
	UNamiCameraAdjust* PushCameraAdjust(TSubclassOf<UNamiCameraAdjust> AdjustClass);

	/**
	 * 推送相机调整器（使用已存在的实例）
	 * @param AdjustInstance 调整器实例
	 * @return 是否成功推送
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Adjust")
	bool PushCameraAdjustInstance(UNamiCameraAdjust* AdjustInstance);

	/**
	 * 移除相机调整器
	 * @param AdjustInstance 要移除的调整器
	 * @param bForceImmediate 是否立即移除（跳过BlendOut）
	 * @return 是否成功移除
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Adjust")
	bool PopCameraAdjust(UNamiCameraAdjust* AdjustInstance, bool bForceImmediate = false);

	/**
	 * 通过类移除相机调整器
	 * @param AdjustClass 调整器类
	 * @param bForceImmediate 是否立即移除
	 * @return 是否成功移除
	 */
	UFUNCTION(BlueprintCallable, Category = "Nami Camera|Adjust")
	bool PopCameraAdjustByClass(TSubclassOf<UNamiCameraAdjust> AdjustClass, bool bForceImmediate = false);

	/**
	 * 通过类型查找相机调整器
	 * @tparam T 调整器类型
	 * @return 找到的调整器，如果不存在则返回nullptr
	 */
	template<typename T>
	T* FindCameraAdjust() const
	{
		for (UNamiCameraAdjust* Adjust : CameraAdjustStack)
		{
			if (T* TypedAdjust = Cast<T>(Adjust))
			{
				return TypedAdjust;
			}
		}
		return nullptr;
	}

	/**
	 * 通过类查找相机调整器
	 * @param AdjustClass 调整器类
	 * @return 找到的调整器
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Adjust")
	UNamiCameraAdjust* FindCameraAdjustByClass(TSubclassOf<UNamiCameraAdjust> AdjustClass) const;

	/**
	 * 获取所有激活的相机调整器
	 * @return 调整器列表
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Adjust")
	const TArray<UNamiCameraAdjust*>& GetCameraAdjusts() const;

	/**
	 * 检查是否有指定类型的调整器处于激活状态
	 * @param AdjustClass 调整器类
	 * @return 是否存在
	 */
	UFUNCTION(BlueprintPure, Category = "Nami Camera|Adjust")
	bool HasCameraAdjust(TSubclassOf<UNamiCameraAdjust> AdjustClass) const;

protected:
	/** 默认相机模式 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	TSubclassOf<UNamiCameraModeBase> DefaultCameraMode;

	/** 推送相机模式委托 */
	UPROPERTY(BlueprintAssignable)
	FOnPushCameraModeDelegate OnPushCameraMode;

	/** 移除相机模式委托 */
	UPROPERTY(BlueprintAssignable)
	FOnPopCameraModeDelegate OnPopCameraMode;

	/** 所有者Pawn */
	UPROPERTY(BlueprintReadOnly, Category = "Nami Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<APawn> OwnerPawn;

	/** 所有者PlayerController */
	UPROPERTY(BlueprintReadOnly, Category = "Nami Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<APlayerController> OwnerPlayerController;

	/** 所有者PlayerCameraManager */
	UPROPERTY(BlueprintReadOnly, Category = "Nami Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ANamiPlayerCameraManager> OwnerPlayerCameraManager;

	/** 缓存的 PlayerController（避免每帧查找） */
	mutable TWeakObjectPtr<APlayerController> CachedPlayerController;

	/** 缓存的 PlayerCameraManager（避免每帧查找） */
	mutable TWeakObjectPtr<ANamiPlayerCameraManager> CachedPlayerCameraManager;

	/** 通知相机模式初始化 */
	UFUNCTION()
	void NotifyCameraModeInitialize(UNamiCameraModeBase *CameraModeInstance);

	/** 更新混合堆栈 */
	void UpdateBlendingStack();

	/** 从池中查找或添加相机模式实例 */
	UNamiCameraModeBase *FindOrAddCameraModeInstanceInPool(TSubclassOf<UNamiCameraModeBase> CameraModeClass);

	/** 在指定索引处移除相机模式 */
	bool PullCameraModeAtIndex(int32 Index);

	// ========== 相机管线处理（重构后的阶段方法） ==========

	/**
	 * 阶段 0：预处理层
	 * 检查组件有效性，初始化上下文
	 */
	bool PreProcessPipeline(float DeltaTime, FNamiCameraPipelineContext& OutContext);

	/**
	 * 阶段 1：模式计算层
	 * 更新模式堆栈，计算并混合所有模式的视图
	 */
	bool ProcessModeStack(float DeltaTime, const FNamiCameraPipelineContext& Context, FNamiCameraView& OutBaseView);

	/**
	 * 阶段 2：全局效果层
	 * 应用所有全局 Features 到视图
	 */
	void ProcessGlobalFeatures(float DeltaTime, FNamiCameraPipelineContext& Context, FNamiCameraView& InOutView);

	/**
	 * 阶段 2.5：相机调整层
	 * 处理所有 CameraAdjust 的参数修改
	 */
	void ProcessCameraAdjusts(float DeltaTime, FNamiCameraPipelineContext& Context, FNamiCameraView& InOutView);

	/**
	 * 计算所有激活的 CameraAdjust 的合并参数
	 * @param DeltaTime 帧时间
	 * @param CurrentArmRotation 当前臂旋转（用于 Override 模式计算）
	 * @return 合并后的调整参数
	 */
	FNamiCameraAdjustParams CalculateCombinedAdjustParams(float DeltaTime, const FRotator& CurrentArmRotation);

	/**
	 * 将调整参数应用到视图
	 * @param Params 调整参数
	 * @param InOutView 要修改的视图
	 */
	void ApplyAdjustParamsToView(const FNamiCameraAdjustParams& Params, FNamiCameraView& InOutView);

	/**
	 * 清理已完全停用的调整器
	 */
	void CleanupInactiveCameraAdjusts();

	/**
	 * 阶段 3：控制器同步层
	 * 同步视图的 ControlLocation/Rotation 到 PlayerController
	 */
	void ProcessControllerSync(float DeltaTime, const FNamiCameraPipelineContext& Context, const FNamiCameraView& InView);

	/**
	 * 阶段 4：平滑混合层
	 * 从当前位置平滑过渡到目标位置
	 */
	void ProcessSmoothing(float DeltaTime, const FNamiCameraView& InView, FMinimalViewInfo& OutPOV);

	/**
	 * 阶段 5：后处理层
	 * Debug 绘制、日志输出、组件同步
	 */
	void PostProcessPipeline(float DeltaTime, const FNamiCameraPipelineContext& Context, FMinimalViewInfo& InOutPOV);

	// ========== 平滑混合层 ==========

	/** 当前实际视图（平滑后的结果） */
	FMinimalViewInfo CurrentActualView;

	/** 是否已初始化当前视图 */
	bool bHasInitializedCurrentView = false;

	/** 位置混合速度（单位：cm/s，0 = 瞬切，>0 = 平滑） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Blending",
			  meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "10000.0",
					  Tooltip = "位置混合速度。0 = 瞬切，>0 = 平滑过渡（单位：cm/s）"))
	float LocationBlendSpeed = 0.0f;

	/** 旋转混合速度（单位：度/s，0 = 瞬切，>0 = 平滑） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Blending",
			  meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1000.0",
					  Tooltip = "旋转混合速度。0 = 瞬切，>0 = 平滑过渡（单位：度/s）"))
	float RotationBlendSpeed = 0.0f;

	/** FOV 混合速度（单位：度/s，0 = 瞬切，>0 = 平滑） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Blending",
			  meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1000.0",
					  Tooltip = "FOV 混合速度。0 = 瞬切，>0 = 平滑过渡（单位：度/s）"))
	float FOVBlendSpeed = 0.0f;

	FVector CurrentControlLocation;
	FRotator CurrentControlRotation;
	bool bHasInitializedControl = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Blending",
			  meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "20000.0"))
	float ControlLocationBlendSpeed = 1200.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Blending",
			  meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "3600.0"))
	float ControlRotationBlendSpeed = 360.0f;

private:
	/** 相机模式实例池 */
	UPROPERTY()
	TArray<TObjectPtr<UNamiCameraModeBase>> CameraModeInstancePool;

	/** 相机模式优先级堆栈 */
	UPROPERTY()
	TArray<FNamiCameraModeStackEntry> CameraModePriorityStack;

	/** 混合堆栈 */
	UPROPERTY()
	FNamiCameraModeStack BlendingStack;

	/** 全局 Feature 列表（独立于 Mode 管理） */
	UPROPERTY()
	TArray<TObjectPtr<UNamiCameraFeature>> GlobalFeatures;

	/** 相机调整器堆栈（按优先级排序） */
	UPROPERTY()
	TArray<TObjectPtr<UNamiCameraAdjust>> CameraAdjustStack;
};
