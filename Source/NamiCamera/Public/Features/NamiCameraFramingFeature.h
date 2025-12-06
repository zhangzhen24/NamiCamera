// Copyright Epic Games, Inc. All Rights Reserved.
//
// 多目标构图 Feature：根据一组目标计算中心和推荐距离，更新视图的 Pivot/Camera 以保持目标在画面内。

#pragma once

#include "CoreMinimal.h"
#include "Features/NamiCameraFeature.h"
#include "Structs/State/NamiCameraFramingTypes.h"
#include "NamiCameraFramingFeature.generated.h"

UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class NAMICAMERA_API UNamiCameraFramingFeature : public UNamiCameraFeature
{
	GENERATED_BODY()

public:
	UNamiCameraFramingFeature();

	virtual void Activate_Implementation() override;
	virtual void Deactivate_Implementation() override;
	virtual void ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime) override;

	/** 设置构图目标（Actor 列表） */
	UFUNCTION(BlueprintCallable, Category = "Camera Framing")
	void SetTargets(const TArray<AActor*>& InTargets);

	/** 清空构图目标 */
	UFUNCTION(BlueprintCallable, Category = "Camera Framing")
	void ClearTargets();

	/** 构图配置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Framing")
	FNamiCameraFramingConfig FramingConfig;

	/** 额外半径填充（厘米），避免目标贴边 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Framing")
	float ExtraRadiusPadding = 50.0f;

protected:
	/** 目标列表（弱引用） */
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> Targets;

	/** 上一帧的中心与距离，用于平滑 */
	FVector LastCenter = FVector::ZeroVector;
	float LastDistance = 300.0f;

	/** 计算有效目标位置集合 */
	void GatherTargets(TArray<FVector>& OutPositions) const;
};

