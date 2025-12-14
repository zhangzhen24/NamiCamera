// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/UnrealMathUtility.h"

/**
 * Nami相机数学库
 * 提供相机相关的数学工具函数，包括平滑插值、角度处理等
 */
class NAMICAMERA_API FNamiCameraMath
{
public:
	/**
	 * 平滑阻尼（Smooth Damp）
	 * 使用临界阻尼平滑算法，使值平滑过渡到目标值
	 * 
	 * @param Current 当前值
	 * @param Target 目标值
	 * @param OutVelocity 输出速度（用于跟踪平滑状态，需要持续传递）
	 * @param SmoothTime 平滑时间（秒），值越大过渡越平滑
	 * @param DeltaTime 帧时间
	 * @param MaxSpeed 最大速度限制（可选，默认无限制）
	 * @return 平滑后的新值
	 */
	static float SmoothDamp(float Current, float Target, float& OutVelocity, float SmoothTime, float DeltaTime, float MaxSpeed = INFINITY);

	/**
	 * 平滑阻尼角度（Smooth Damp Angle）
	 * 处理360度环绕的角度平滑，自动选择最短路径
	 * 
	 * @param CurrentAngleDeg 当前角度（度）
	 * @param TargetAngleDeg 目标角度（度）
	 * @param OutVelocity 输出角速度（用于跟踪平滑状态，需要持续传递）
	 * @param SmoothTime 平滑时间（秒），值越大过渡越平滑
	 * @param DeltaTime 帧时间
	 * @param MaxSpeed 最大角速度限制（可选，默认无限制）
	 * @return 平滑后的新角度（度）
	 */
	static float SmoothDampAngle(float CurrentAngleDeg, float TargetAngleDeg, float& OutVelocity, float SmoothTime, float DeltaTime, float MaxSpeed = INFINITY);

	/**
	 * 平滑阻尼向量（Smooth Damp Vector）
	 * 对向量的三个分量分别进行平滑
	 * 
	 * @param Current 当前向量
	 * @param Target 目标向量
	 * @param OutVelocity 输出速度向量（用于跟踪平滑状态，需要持续传递）
	 * @param SmoothTime 平滑时间（秒），值越大过渡越平滑
	 * @param DeltaTime 帧时间
	 * @param MaxSpeed 最大速度限制（可选，默认无限制）
	 * @return 平滑后的新向量
	 */
	static FVector SmoothDamp(const FVector& Current, const FVector& Target, FVector& OutVelocity, float SmoothTime, float DeltaTime, float MaxSpeed = INFINITY);

	/**
	 * 平滑阻尼旋转（Smooth Damp Rotator）
	 * 对旋转的Pitch、Yaw、Roll分别进行平滑，自动处理360度环绕
	 * 
	 * @param Current 当前旋转
	 * @param Target 目标旋转
	 * @param OutVelocity 输出角速度（用于跟踪平滑状态，需要持续传递）
	 * @param SmoothTime 平滑时间（秒），值越大过渡越平滑
	 * @param DeltaTime 帧时间
	 * @param MaxSpeed 最大角速度限制（可选，默认无限制）
	 * @return 平滑后的新旋转
	 */
	static FRotator SmoothDamp(const FRotator& Current, const FRotator& Target, FRotator& OutVelocity, float SmoothTime, float DeltaTime, float MaxSpeed = INFINITY);

	/**
	 * 平滑阻尼旋转（独立配置Pitch和Yaw）
	 * 允许Pitch和Yaw使用不同的平滑时间
	 * 
	 * @param Current 当前旋转
	 * @param Target 目标旋转
	 * @param OutYawVelocity 输出Yaw角速度
	 * @param OutPitchVelocity 输出Pitch角速度
	 * @param YawSmoothTime Yaw平滑时间（秒），0表示立即
	 * @param PitchSmoothTime Pitch平滑时间（秒），0表示立即
	 * @param DeltaTime 帧时间
	 * @return 平滑后的新旋转
	 */
	static FRotator SmoothDampRotator(
		const FRotator& Current,
		const FRotator& Target,
		float& OutYawVelocity,
		float& OutPitchVelocity,
		float YawSmoothTime,
		float PitchSmoothTime,
		float DeltaTime);

	/**
	 * 逆插值（Inverse Lerp）
	 * 计算值在[min, max]范围内的归一化位置[0, 1]
	 * 
	 * @param Value 输入值
	 * @param Min 最小值
	 * @param Max 最大值
	 * @return 归一化位置[0, 1]
	 */
	static FORCEINLINE float InverseLerp(float Value, float Min, float Max)
	{
		if (FMath::IsNearlyEqual(Min, Max))
		{
			return 0.0f;
		}
		return FMath::Clamp((Value - Min) / (Max - Min), 0.0f, 1.0f);
	}

	/**
	 * 平滑阻尼四元数（Smooth Damp Quaternion）
	 * 使用球面插值进行旋转平滑
	 * 
	 * @param Current 当前四元数
	 * @param Target 目标四元数
	 * @param OutAngularVelocity 输出角速度（用于跟踪平滑状态，需要持续传递）
	 * @param SmoothTime 平滑时间（秒），值越大过渡越平滑
	 * @param DeltaTime 帧时间
	 * @param MaxSpeed 最大角速度限制（可选，默认无限制）
	 * @return 平滑后的新四元数
	 */
	static FQuat SmoothDamp(const FQuat& Current, const FQuat& Target, float& OutAngularVelocity, float SmoothTime, float DeltaTime, float MaxSpeed = INFINITY);

	/**
	 * 角度归一化（Angle Normalize）
	 * 将角度归一化到[-180, 180]范围
	 * 
	 * @param AngleDeg 输入角度（度）
	 * @return 归一化后的角度（度）
	 */
	static FORCEINLINE float NormalizeAngle(float AngleDeg)
	{
		AngleDeg += 180.0f;
		AngleDeg = FMath::Fmod(AngleDeg, 360.0f);
		return AngleDeg - 180.0f;
	}

	/**
	 * 角度归一化到0-360度范围
	 * 避免-180到180的跳变问题，确保角度值在[0, 360)范围内
	 * 
	 * @param AngleDeg 输入角度（度）
	 * @return 归一化后的角度（度），范围[0, 360)
	 */
	static FORCEINLINE float NormalizeAngleTo360(float AngleDeg)
	{
		AngleDeg = FMath::Fmod(AngleDeg, 360.0f);
		if (AngleDeg < 0.0f)
		{
			AngleDeg += 360.0f;
		}
		return AngleDeg;
	}

	/**
	 * 旋转体归一化到0-360度范围
	 * 将Pitch、Yaw、Roll都归一化到[0, 360)范围
	 * 
	 * @param Rot 输入旋转
	 * @return 归一化后的旋转，所有分量都在[0, 360)范围
	 */
	static FORCEINLINE FRotator NormalizeRotatorTo360(const FRotator& Rot)
	{
		FRotator Result;
		Result.Pitch = NormalizeAngleTo360(Rot.Pitch);
		Result.Yaw = NormalizeAngleTo360(Rot.Yaw);
		Result.Roll = NormalizeAngleTo360(Rot.Roll);
		return Result;
	}

	/**
	 * 角度差值计算（基于0-360度范围）
	 * 返回从Current到Target的最短路径差值
	 * 
	 * @param CurrentDeg 当前角度（度），应该在[0, 360)范围
	 * @param TargetDeg 目标角度（度），应该在[0, 360)范围
	 * @return 最短路径的差值（度），范围[-180, 180]
	 */
	static FORCEINLINE float FindDeltaAngle360(float CurrentDeg, float TargetDeg)
	{
		// 确保输入在0-360度范围
		CurrentDeg = NormalizeAngleTo360(CurrentDeg);
		TargetDeg = NormalizeAngleTo360(TargetDeg);
		
		// 计算差值
		float Delta = TargetDeg - CurrentDeg;
		
		// 处理跨越0度的情况，选择最短路径
		if (Delta > 180.0f)
		{
			Delta -= 360.0f;
		}
		else if (Delta < -180.0f)
		{
			Delta += 360.0f;
		}
		
		return Delta;
	}

	/**
	 * 平滑强度映射
	 * 将0.0-1.0的平滑强度映射到0.0-2.0的实际平滑时间
	 * 使用非线性映射，使调整更加直观
	 * 
	 * @param SmoothIntensity 平滑强度（0.0-1.0）
	 * @return 实际平滑时间（0.0-2.0）
	 */
	static float MapSmoothIntensity(float SmoothIntensity);

};

