// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Structs/Stack/NamiCameraModeStack.h"
#include "Structs/Mode/NamiCameraModeHandle.h"
#include "Structs/Mode/NamiCameraModeStackEntry.h"
#include "Camera/CameraComponent.h"
#include "NamiCameraComponent.generated.h"

class UNamiCameraModeBase;
class ANamiPlayerCameraManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPushCameraModeDelegate, UNamiCameraModeBase *, CameraModeInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPopCameraModeDelegate);

/**
 * Nami相机组件
 * 管理相机模式堆栈和混合
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

	/** 通知相机模式初始化 */
	UFUNCTION()
	void NotifyCameraModeInitialize(UNamiCameraModeBase *CameraModeInstance);

	/** 更新混合堆栈 */
	void UpdateBlendingStack();

	/** 从池中查找或添加相机模式实例 */
	UNamiCameraModeBase *FindOrAddCameraModeInstanceInPool(TSubclassOf<UNamiCameraModeBase> CameraModeClass);

	/** 在指定索引处移除相机模式 */
	bool PullCameraModeAtIndex(int32 Index);

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
};
