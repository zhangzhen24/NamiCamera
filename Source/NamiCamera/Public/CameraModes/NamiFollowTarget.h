// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NamiFollowTarget.generated.h"

/**
 * 跟随目标数据
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiFollowTarget
{
	GENERATED_BODY()

	FNamiFollowTarget()
		: Target(nullptr)
		, TargetLocation(FVector::ZeroVector)
		, Weight(1.0f)
		, TargetType(ENamiFollowTargetType::Secondary)
	{
	}

	FNamiFollowTarget(AActor* InTarget, float InWeight = 1.0f, ENamiFollowTargetType InType = ENamiFollowTargetType::Secondary)
		: Target(InTarget)
		, TargetLocation(FVector::ZeroVector)
		, Weight(InWeight)
		, TargetType(InType)
	{
	}

	/** 目标Actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Target")
	TWeakObjectPtr<AActor> Target;

	/** 目标位置（如果Target为空则使用此值） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Target")
	FVector TargetLocation;

	/** 权重 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Target",
		meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float Weight;

	/** 目标类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow Target")
	ENamiFollowTargetType TargetType;

	/** 获取世界位置 */
	FVector GetLocation() const
	{
		if (Target.IsValid())
		{
			return Target->GetActorLocation();
		}
		return TargetLocation;
	}

	/** 是否有效 */
	bool IsValid() const
	{
		return Target.IsValid() || !TargetLocation.IsNearlyZero();
	}
};

