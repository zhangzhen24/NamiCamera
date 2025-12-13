// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/NamiCameraEnums.h"
#include "NamiCameraAdjustParams.generated.h"

// ============================================================================
// 参数包装结构体
// 为每个相机参数提供独立的启用控制和混合模式选择
// ============================================================================

/**
 * Float 参数包装（FOV、ArmLength）
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraFloatParam
{
	GENERATED_BODY()

	/** 是否启用此参数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter")
	bool bEnabled = false;

	/** 混合模式 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter",
		meta = (EditCondition = "bEnabled"))
	ENamiCameraAdjustBlendMode BlendMode = ENamiCameraAdjustBlendMode::Additive;

	/** 参数值（Additive 模式为偏移，Override 模式为目标值） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter",
		meta = (EditCondition = "bEnabled"))
	float Value = 0.f;
};

/**
 * Vector 参数包装（CameraOffset、PivotOffset）
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraVectorParam
{
	GENERATED_BODY()

	/** 是否启用此参数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter")
	bool bEnabled = false;

	/** 混合模式 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter",
		meta = (EditCondition = "bEnabled"))
	ENamiCameraAdjustBlendMode BlendMode = ENamiCameraAdjustBlendMode::Additive;

	/** 参数值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter",
		meta = (EditCondition = "bEnabled"))
	FVector Value = FVector::ZeroVector;
};

/**
 * Rotator 参数包装（CameraRotation）
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraRotatorParam
{
	GENERATED_BODY()

	/** 是否启用此参数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter")
	bool bEnabled = false;

	/** 混合模式 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter",
		meta = (EditCondition = "bEnabled"))
	ENamiCameraAdjustBlendMode BlendMode = ENamiCameraAdjustBlendMode::Additive;

	/** 参数值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter",
		meta = (EditCondition = "bEnabled"))
	FRotator Value = FRotator::ZeroRotator;
};

/**
 * 相机臂旋转参数包装
 *
 * Additive 模式：相对于当前臂方向的偏移
 * Override 模式：相对于角色 Mesh 朝向的目标位置
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraArmRotationParam
{
	GENERATED_BODY()

	/** 是否启用此参数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter")
	bool bEnabled = false;

	/** 混合模式 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter",
		meta = (EditCondition = "bEnabled"))
	ENamiCameraAdjustBlendMode BlendMode = ENamiCameraAdjustBlendMode::Additive;

	/**
	 * Additive 模式：相对于当前臂方向的偏移
	 *   Yaw = 水平旋转（正值向右）
	 *   Pitch = 垂直旋转（正值向上）
	 *
	 * Override 模式：相对于角色 Mesh 朝向的目标位置
	 *   Yaw = 0°：角色正前方
	 *   Yaw = 180°：角色后方（默认相机位置）
	 *   Pitch > 0：俯视
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameter",
		meta = (EditCondition = "bEnabled",
			Tooltip = "Additive: 相对于当前臂方向的偏移\nOverride: 相对于角色Mesh朝向的目标位置"))
	FRotator Value = FRotator::ZeroRotator;
};

/**
 * 相机调整参数修改标志
 * 用于追踪哪些参数被修改过
 * 注意：由于标志位超过8个，不能使用 BlueprintType
 */
namespace ENamiCameraAdjustModifiedFlags
{
	enum Type : uint32
	{
		None = 0,

		/** FOV 被修改 */
		FOV = 1 << 0,

		/** 相机臂长度被修改 */
		ArmLength = 1 << 1,

		/** 相机臂旋转被修改 */
		ArmRotation = 1 << 2,

		/** 相机位置偏移被修改 */
		CameraLocationOffset = 1 << 3,

		/** 相机旋转偏移被修改 */
		CameraRotationOffset = 1 << 4,

		/** Pivot偏移被修改 */
		PivotOffset = 1 << 5,

		/** SpringArm TargetArmLength 被修改 */
		TargetArmLength = 1 << 6,

		/** SpringArm SocketOffset 被修改 */
		SocketOffset = 1 << 7,

		/** SpringArm TargetOffset 被修改 */
		TargetOffset = 1 << 8,
	};
}

/**
 * 相机调整参数结构
 * 定义 CameraAdjust 可以修改的所有相机属性
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraAdjustParams
{
	GENERATED_BODY()

	FNamiCameraAdjustParams() = default;

	// ========== 视图参数（在视图计算之后应用） ==========

	/** FOV 偏移量（Additive模式使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "View")
	float FOVOffset = 0.f;

	/** FOV 乘数（内部计算使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "View")
	float FOVMultiplier = 1.f;

	/** FOV 目标���（Override模式使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "View")
	float FOVTarget = 90.f;

	/** 相机位置偏移（世界空间或相对空间，取决于配置） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "View")
	FVector CameraLocationOffset = FVector::ZeroVector;

	/** 相机旋转偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "View")
	FRotator CameraRotationOffset = FRotator::ZeroRotator;

	/** Pivot位置偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "View")
	FVector PivotOffset = FVector::ZeroVector;

	// ========== SpringArm参数（在SpringArm计算之前应用） ==========

	/** 目标臂长偏移量（Additive模式使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm")
	float TargetArmLengthOffset = 0.f;

	/** 目标臂长乘数（内部计算使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm")
	float TargetArmLengthMultiplier = 1.f;

	/** 目标臂长目标值（Override模式使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm")
	float TargetArmLengthTarget = 300.f;

	/** 臂旋转偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm")
	FRotator ArmRotationOffset = FRotator::ZeroRotator;

	/** Socket偏移增量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm")
	FVector SocketOffsetDelta = FVector::ZeroVector;

	/** Target偏移增量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm")
	FVector TargetOffsetDelta = FVector::ZeroVector;

	// ========== 每参数混合模式 ==========

	/** FOV 混合模式 */
	UPROPERTY()
	ENamiCameraAdjustBlendMode FOVBlendMode = ENamiCameraAdjustBlendMode::Additive;

	/** 臂长混合模式 */
	UPROPERTY()
	ENamiCameraAdjustBlendMode ArmLengthBlendMode = ENamiCameraAdjustBlendMode::Additive;

	/** 臂旋转混合模式 */
	UPROPERTY()
	ENamiCameraAdjustBlendMode ArmRotationBlendMode = ENamiCameraAdjustBlendMode::Additive;

	/** 相机位置偏移混合模式 */
	UPROPERTY()
	ENamiCameraAdjustBlendMode CameraOffsetBlendMode = ENamiCameraAdjustBlendMode::Additive;

	/** 相机旋转偏移混合模式 */
	UPROPERTY()
	ENamiCameraAdjustBlendMode CameraRotationBlendMode = ENamiCameraAdjustBlendMode::Additive;

	/** Pivot 偏移混合模式 */
	UPROPERTY()
	ENamiCameraAdjustBlendMode PivotOffsetBlendMode = ENamiCameraAdjustBlendMode::Additive;

	// ========== 修改标志 ==========

	/** 记录哪些参数被修改过 */
	UPROPERTY()
	uint32 ModifiedFlags = 0;

	// ========== 辅助方法 ==========

	/** 标记FOV已修改 */
	void MarkFOVModified()
	{
		ModifiedFlags |= ENamiCameraAdjustModifiedFlags::FOV;
	}

	/** 标记臂长已修改 */
	void MarkArmLengthModified()
	{
		ModifiedFlags |= ENamiCameraAdjustModifiedFlags::ArmLength;
	}

	/** 标记臂旋转已修改 */
	void MarkArmRotationModified()
	{
		ModifiedFlags |= ENamiCameraAdjustModifiedFlags::ArmRotation;
	}

	/** 标记相机位置偏移已修改 */
	void MarkCameraLocationOffsetModified()
	{
		ModifiedFlags |= ENamiCameraAdjustModifiedFlags::CameraLocationOffset;
	}

	/** 标记相机旋转偏移已修改 */
	void MarkCameraRotationOffsetModified()
	{
		ModifiedFlags |= ENamiCameraAdjustModifiedFlags::CameraRotationOffset;
	}

	/** 标记Pivot偏移已修改 */
	void MarkPivotOffsetModified()
	{
		ModifiedFlags |= ENamiCameraAdjustModifiedFlags::PivotOffset;
	}

	/** 标记目标臂长已修改 */
	void MarkTargetArmLengthModified()
	{
		ModifiedFlags |= ENamiCameraAdjustModifiedFlags::TargetArmLength;
	}

	/** 标记Socket偏移已修改 */
	void MarkSocketOffsetModified()
	{
		ModifiedFlags |= ENamiCameraAdjustModifiedFlags::SocketOffset;
	}

	/** 标记Target偏移已修改 */
	void MarkTargetOffsetModified()
	{
		ModifiedFlags |= ENamiCameraAdjustModifiedFlags::TargetOffset;
	}

	/** 检查特定标志是否被设置 */
	bool HasFlag(ENamiCameraAdjustModifiedFlags::Type Flag) const
	{
		return (ModifiedFlags & static_cast<uint32>(Flag)) != 0;
	}

	/** 检查是否有任何SpringArm相关的修改 */
	bool HasPreSpringArmModifications() const
	{
		const uint32 SpringArmFlags =
			ENamiCameraAdjustModifiedFlags::TargetArmLength |
			ENamiCameraAdjustModifiedFlags::SocketOffset |
			ENamiCameraAdjustModifiedFlags::TargetOffset |
			ENamiCameraAdjustModifiedFlags::ArmRotation;

		return (ModifiedFlags & SpringArmFlags) != 0;
	}

	/** 检查是否有任何视图相关的修改 */
	bool HasPostViewModifications() const
	{
		const uint32 ViewFlags =
			ENamiCameraAdjustModifiedFlags::FOV |
			ENamiCameraAdjustModifiedFlags::CameraLocationOffset |
			ENamiCameraAdjustModifiedFlags::CameraRotationOffset |
			ENamiCameraAdjustModifiedFlags::PivotOffset;

		return (ModifiedFlags & ViewFlags) != 0;
	}

	/** 重置所有参数到默认值 */
	void Reset()
	{
		*this = FNamiCameraAdjustParams();
	}

	/** 线性插值两个参数结构 */
	static FNamiCameraAdjustParams Lerp(
		const FNamiCameraAdjustParams& A,
		const FNamiCameraAdjustParams& B,
		float Alpha);

	/** 根据权重缩放所有偏移值 */
	FNamiCameraAdjustParams ScaleByWeight(float Weight) const;

	/**
	 * 根据每个参数的 BlendMode 进行缩放
	 * Additive 模式的参数会被缩放，Override 模式的参数保持不变
	 * @param Weight 缩放权重
	 * @return 缩放后的参数
	 */
	FNamiCameraAdjustParams ScaleAdditiveParamsByWeight(float Weight) const;

	/** 合并两个参数（叠加模式） */
	static FNamiCameraAdjustParams Combine(
		const FNamiCameraAdjustParams& A,
		const FNamiCameraAdjustParams& B);
};
