// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Data/NamiCameraEnums.h"
#include "NamiCameraAdjustCurveBinding.generated.h"

/**
 * 曲线绑定结构
 * 定义如何将输入源映射到输出值
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraAdjustCurveBinding
{
	GENERATED_BODY()

	FNamiCameraAdjustCurveBinding()
		: InputSource(ENamiCameraAdjustInputSource::None)
		, Curve(nullptr)
		, InputMin(0.f)
		, InputMax(1.f)
		, OutputScale(1.f)
		, OutputOffset(0.f)
		, bClampInput(true)
	{
	}

	/** 输入源类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	ENamiCameraAdjustInputSource InputSource;

	/** 映射曲线（输入0-1，输出由曲线定义） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve")
	UCurveFloat* Curve;

	/** 输入最小值（映射到曲线的0） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float InputMin;

	/** 输入最大值（映射到曲线的1） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float InputMax;

	/** 输出缩放系数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	float OutputScale;

	/** 输出偏移量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	float OutputOffset;

	/** 是否将输入值限制在[InputMin, InputMax]范围内 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bClampInput;

	/**
	 * 计算归一化的输入值（0-1）
	 * @param RawInput 原始输入值
	 * @return 归一化后的值
	 */
	float NormalizeInput(float RawInput) const
	{
		if (FMath::IsNearlyEqual(InputMax, InputMin))
		{
			return 0.f;
		}

		float NormalizedInput = (RawInput - InputMin) / (InputMax - InputMin);

		if (bClampInput)
		{
			NormalizedInput = FMath::Clamp(NormalizedInput, 0.f, 1.f);
		}

		return NormalizedInput;
	}

	/**
	 * 评估曲线并返回最终输出值
	 * @param RawInput 原始输入值
	 * @return 经过曲线映射和缩放后的输出值
	 */
	float Evaluate(float RawInput) const
	{
		if (InputSource == ENamiCameraAdjustInputSource::None)
		{
			return OutputOffset;
		}

		float NormalizedInput = NormalizeInput(RawInput);

		float CurveOutput = 1.f;
		if (Curve)
		{
			CurveOutput = Curve->GetFloatValue(NormalizedInput);
		}
		else
		{
			// 没有曲线时使用线性映射
			CurveOutput = NormalizedInput;
		}

		return CurveOutput * OutputScale + OutputOffset;
	}

	/** 检查此绑定是否有效（有输入源） */
	bool IsValid() const
	{
		return InputSource != ENamiCameraAdjustInputSource::None;
	}
};

/**
 * 完整的曲线驱动配置
 * 包含多个参数的曲线绑定
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraAdjustCurveConfig
{
	GENERATED_BODY()

	/** FOV曲线绑定 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FOV")
	FNamiCameraAdjustCurveBinding FOVBinding;

	/** 相机臂长度曲线绑定 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpringArm")
	FNamiCameraAdjustCurveBinding ArmLengthBinding;

	/** 相机位置X偏移曲线绑定 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset")
	FNamiCameraAdjustCurveBinding CameraOffsetXBinding;

	/** 相机位置Y偏移曲线绑定 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset")
	FNamiCameraAdjustCurveBinding CameraOffsetYBinding;

	/** 相机位置Z偏移曲线绑定 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset")
	FNamiCameraAdjustCurveBinding CameraOffsetZBinding;

	/** 检查是否有任何有效的曲线绑定 */
	bool HasValidBindings() const
	{
		return FOVBinding.IsValid() ||
			   ArmLengthBinding.IsValid() ||
			   CameraOffsetXBinding.IsValid() ||
			   CameraOffsetYBinding.IsValid() ||
			   CameraOffsetZBinding.IsValid();
	}
};
