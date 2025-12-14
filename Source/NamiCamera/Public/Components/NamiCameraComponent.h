// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"

// NamiCamera module headers (sorted alphabetically)
#include "CameraAdjust/NamiCameraAdjustParams.h"
#include "CameraModes/NamiCameraModeBase.h"
#include "Data/NamiCameraModeHandle.h"
#include "Data/NamiCameraModeStack.h"
#include "Data/NamiCameraModeStackEntry.h"
#include "Data/NamiCameraPipelineContext.h"

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
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Debug")
	void DumpCameraModeStack(bool bPrintToScreen = true, bool bPrintToLog = true,
							 FLinearColor TextColor = FLinearColor::Green, float Duration = 0.2f) const;

	/** 获取默认相机模式类 */
	TSubclassOf<UNamiCameraModeBase> GetDefaultCameraModeClass() const { return DefaultCameraMode; }

#if WITH_EDITOR
	/** Draw debug camera info (editor only) */
	void DrawDebugCameraInfo(const struct FNamiCameraView& View) const;
#endif

	// ========== Camera Mode ==========

	/** 推送相机模式 */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Modes")
	FNamiCameraModeHandle PushCameraMode(TSubclassOf<UNamiCameraModeBase> CameraModeClass, int32 Priority = 0);

	/** 使用实例推送相机模式 */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Modes")
	FNamiCameraModeHandle PushCameraModeUsingInstance(UNamiCameraModeBase *CameraModeInstance, int32 Priority = 0);

	/** 移除相机模式 */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Modes")
	bool PopCameraMode(UPARAM(ref) FNamiCameraModeHandle &ModeHandle);

	/** 使用实例移除相机模式 */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Modes")
	bool PopCameraModeInstance(UNamiCameraModeBase *CameraMode);

	/** 获取当前激活的相机模式 */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Modes")
	UNamiCameraModeBase *GetActiveCameraMode() const;

	// ========== Camera Modifier ==========

	/** 推送CameraModifier */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Modifiers")
	UCameraModifier *PushCameraModifier(TSubclassOf<UCameraModifier> CameraModifierClass);

	/** 使用实例推送CameraModifier */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Modifiers")
	bool PushCameraModifierInstance(UCameraModifier *CameraModifierInstance);

	/** 移除CameraModifier */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Modifiers")
	bool PopCameraModifierInstance(UCameraModifier *CameraModifierInstance);

	/** 获取激活的CameraModifiers */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Modifiers")
	TArray<UCameraModifier *> GetActivateCameraModifiers() const;

	// ========== Global Features ==========

	/** 添加全局 Feature（独立于 Mode 管理） */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Features")
	void AddGlobalFeature(UNamiCameraFeature* Feature);

	/** 移除全局 Feature */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Features")
	bool RemoveGlobalFeature(UNamiCameraFeature* Feature);

	/** 通过名称查找全局 Feature */
	UFUNCTION(BlueprintPure, Category = "NamiCamera|Features")
	UNamiCameraFeature* FindGlobalFeatureByName(FName FeatureName) const;

	/** 获取所有全局 Feature */
	UFUNCTION(BlueprintPure, Category = "NamiCamera|Features")
	const TArray<UNamiCameraFeature*>& GetGlobalFeatures() const { return GlobalFeatures; }

	/** 通过 Tag 移除全局 Feature */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Features")
	void RemoveGlobalFeaturesByTag(const FGameplayTag& Tag, bool bDeactivateFirst = true);

	/** 通过 Tag 容器移除全局 Feature（匹配任意 Tag） */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Features")
	void RemoveGlobalFeaturesByTags(const FGameplayTagContainer& TagContainer, bool bDeactivateFirst = true);

	/** 移除所有 Stay 类型的全局 Feature（便捷方法） */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Features")
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
	UFUNCTION(BlueprintPure, Category = "NamiCamera|Features")
	UNamiCameraFeature* GetFeatureByName(FName FeatureName) const;

	/**
	 * 通过 Tag 查找所有匹配的 Feature（全局 + 当前激活Mode）
	 * @param Tag 要匹配的Tag
	 * @return 匹配的Feature列表
	 */
	UFUNCTION(BlueprintPure, Category = "NamiCamera|Features")
	TArray<UNamiCameraFeature*> GetFeaturesByTag(const FGameplayTag& Tag) const;

	/**
	 * 通过 Tag 容器查找所有匹配的 Feature（匹配任意Tag，全局 + 当前激活Mode）
	 * @param TagContainer Tag容器
	 * @return 匹配的Feature列表
	 */
	UFUNCTION(BlueprintPure, Category = "NamiCamera|Features")
	TArray<UNamiCameraFeature*> GetFeaturesByTags(const FGameplayTagContainer& TagContainer) const;

	// ========== Camera Adjust ==========

	/**
	 * 推送相机调整器（通过类创建实例）
	 * @param AdjustClass 调整器类
	 * @param DuplicatePolicy 同类重复处理策略（默认保持现有，防止跳变）
	 * @return 创建的调整器实例，如果策略为KeepExisting且已存在则返回现有实例
	 */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Adjustments")
	UNamiCameraAdjust* PushCameraAdjust(TSubclassOf<UNamiCameraAdjust> AdjustClass,
		ENamiCameraAdjustDuplicatePolicy DuplicatePolicy = ENamiCameraAdjustDuplicatePolicy::KeepExisting);

	/**
	 * 推送相机调整器（使用已存在的实例）
	 * @param AdjustInstance 调整器实例
	 * @param DuplicatePolicy 同类重复处理策略（默认保持现有，防止跳变）
	 * @return 是否成功推送
	 */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Adjustments")
	bool PushCameraAdjustInstance(UNamiCameraAdjust* AdjustInstance,
		ENamiCameraAdjustDuplicatePolicy DuplicatePolicy = ENamiCameraAdjustDuplicatePolicy::KeepExisting);

	/**
	 * 移除相机调整器
	 * @param AdjustInstance 要移除的调整器
	 * @param bForceImmediate 是否立即移除（跳过BlendOut）
	 * @return 是否成功移除
	 */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Adjustments")
	bool PopCameraAdjust(UNamiCameraAdjust* AdjustInstance, bool bForceImmediate = false);

	/**
	 * 通过类移除相机调整器
	 * @param AdjustClass 调整器类
	 * @param bForceImmediate 是否立即移除
	 * @return 是否成功移除
	 */
	UFUNCTION(BlueprintCallable, Category = "NamiCamera|Adjustments")
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
	UFUNCTION(BlueprintPure, Category = "NamiCamera|Adjustments")
	UNamiCameraAdjust* FindCameraAdjustByClass(TSubclassOf<UNamiCameraAdjust> AdjustClass) const;

	/**
	 * 获取所有激活的相机调整器
	 * @return 调整器列表
	 */
	UFUNCTION(BlueprintPure, Category = "NamiCamera|Adjustments")
	const TArray<UNamiCameraAdjust*>& GetCameraAdjusts() const;

	/**
	 * 检查是否有指定类型的调整器处于激活状态
	 * @param AdjustClass 调整器类
	 * @return 是否存在
	 */
	UFUNCTION(BlueprintPure, Category = "NamiCamera|Adjustments")
	bool HasCameraAdjust(TSubclassOf<UNamiCameraAdjust> AdjustClass) const;

protected:
	/** Default camera mode used when component initializes */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings",
		meta = (AllowPrivateAccess = "true",
				Tooltip = "相机组件初始化时使用的默认相机模式类"))
	TSubclassOf<UNamiCameraModeBase> DefaultCameraMode;

	/** 推送相机模式委托 */
	UPROPERTY(BlueprintAssignable)
	FOnPushCameraModeDelegate OnPushCameraMode;

	/** 移除相机模式委托 */
	UPROPERTY(BlueprintAssignable)
	FOnPopCameraModeDelegate OnPopCameraMode;

	/** Cached owner pawn reference */
	UPROPERTY(BlueprintReadOnly, Category = "NamiCamera",
		meta = (AllowPrivateAccess = "true",
				Tooltip = "拥有此相机组件的Pawn"))
	TObjectPtr<APawn> OwnerPawn;

	/** Cached owner player controller reference */
	UPROPERTY(BlueprintReadOnly, Category = "NamiCamera",
		meta = (AllowPrivateAccess = "true",
				Tooltip = "拥有此相机组件的玩家控制器"))
	TObjectPtr<APlayerController> OwnerPlayerController;

	/** Cached owner player camera manager reference */
	UPROPERTY(BlueprintReadOnly, Category = "NamiCamera",
		meta = (AllowPrivateAccess = "true",
				Tooltip = "拥有此相机组件的玩家相机管理器"))
	TObjectPtr<ANamiPlayerCameraManager> OwnerPlayerCameraManager;


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
	FNamiCameraAdjustParams CalculateCombinedAdjustParams(float DeltaTime, const FRotator& CurrentArmRotation, const FNamiCameraView& CurrentView);

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
	 * 检测是否有玩家相机旋转输入
	 * @param Threshold 输入阈值（鼠标移动超过此值视为有输入）
	 * @return 是否检测到输入
	 */
	bool DetectPlayerCameraInput(float Threshold) const;

	/**
	 * 同步臂旋转到 PlayerController 的 ControlRotation
	 * 用于输入打断时，确保玩家从当前臂旋转位置接管控制
	 * @param ArmRotation 当前臂旋转
	 */
	void SyncArmRotationToControlRotation(const FRotator& ArmRotation);

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

	/** Location blending speed (cm/s, 0 = instant, >0 = smooth) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Blending",
			  meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "10000.0",
					  Tooltip = "相机位置混合速度。0 = 瞬切，>0 = 平滑过渡（单位：cm/s）"))
	float LocationBlendSpeed = 0.0f;

	/** Rotation blending speed (degrees/s, 0 = instant, >0 = smooth) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Blending",
			  meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1000.0",
					  Tooltip = "相机旋转混合速度。0 = 瞬切，>0 = 平滑过渡（单位：度/s）"))
	float RotationBlendSpeed = 0.0f;

	/** FOV blending speed (degrees/s, 0 = instant, >0 = smooth) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Blending",
			  meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1000.0",
					  Tooltip = "视野角混合速度。0 = 瞬切，>0 = 平滑过渡（单位：度/s）"))
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

	// ========== 输入打断调试 ==========
	/** 输入打断后的帧计数器（用于调试日志） */
	int32 InputInterruptDebugFrameCounter = 0;
	/** 打断前保存的视图信息 */
	FNamiCameraView InputInterruptSavedView;

	// ========== 输入打断同步 ==========
	/** 是否需要同步 ControlRotation（在 CalculateCombinedAdjustParams 中设置，ProcessCameraAdjusts 中应用） */
	bool bPendingControlRotationSync = false;
	/** 待同步的 ControlRotation */
	FRotator PendingControlRotation;
};
