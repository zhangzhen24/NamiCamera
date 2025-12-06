// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modifiers/NamiCameraEffectModifierBase.h"
#include "Camera/CameraShakeBase.h"
#include "NamiCameraShakeModifier.generated.h"

/**
 * Nami 相机震动修改器
 * 
 * 专门用于管理相机震动
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Nami Camera Shake Modifier"))
class NAMICAMERA_API UNamiCameraShakeModifier : public UNamiCameraEffectModifierBase
{
	GENERATED_BODY()

public:
	UNamiCameraShakeModifier();

	// ========== UNamiCameraEffectModifierBase 接口 ==========
	
	virtual bool ApplyEffect(struct FMinimalViewInfo& InOutPOV, float Weight, float DeltaTime) override;
	virtual bool ShouldKeepActive(float Weight) const override;
	virtual void AddedToCamera(APlayerCameraManager* Camera) override;

	// ========== UNamiCameraEffectModifierBase 重写 ==========
	
	virtual void ActivateEffect(bool bResetTimer = false) override;
	virtual void DeactivateEffect(bool bForceImmediate = false) override;

	// ========== 配置 ==========
	
	/** 相机震动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Shake")
	TSubclassOf<UCameraShakeBase> CameraShake;
	
	/** 震动强度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Shake")
	float ShakeScale = 1.0f;
	
	/** 是否在效果开始时播放震动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Shake")
	bool bPlayShakeOnStart = true;
	
	/** 是否循环播放震动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Shake")
	bool bLoopShake = false;

protected:
	/** 相机震动实例 */
	TWeakObjectPtr<UCameraShakeBase> ActiveShakeInstance;
	
	/** 震动待启动标志（当 ActivateEffect 时 CameraOwner 未设置时使用） */
	bool bPendingShakeStart = false;
};

