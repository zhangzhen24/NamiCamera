// Copyright Qiu, Inc. All Rights Reserved.

#include "Math/NamiCameraMath.h"
#include "Math/UnrealMathUtility.h"

float FNamiCameraMath::SmoothDamp(float Current, float Target, float &OutVelocity, float SmoothTime, float DeltaTime, float MaxSpeed)
{
	if (SmoothTime <= 0.0f)
	{
		OutVelocity = 0.0f;
		return Target;
	}

	// 将SmoothTime转换为InterpSpeed
	// SmoothTime越小，InterpSpeed越大，相机响应越快
	// 这符合用户的预期：平滑时间越小，响应越快；平滑时间越大，越平滑但响应越慢
	float InterpSpeed = 1.0f / SmoothTime;

	// 使用FMath::FInterpTo进行平滑，这个函数的效果更符合用户预期
	float SmoothedValue = FMath::FInterpTo(Current, Target, DeltaTime, InterpSpeed);

	// 计算速度
	OutVelocity = (SmoothedValue - Current) / DeltaTime;

	// 限制最大速度
	if (MaxSpeed < INFINITY && FMath::Abs(OutVelocity) > MaxSpeed)
	{
		OutVelocity = FMath::Sign(OutVelocity) * MaxSpeed;
		// 重新计算SmoothedValue，确保不超过最大速度
		SmoothedValue = Current + OutVelocity * DeltaTime;
	}

	return SmoothedValue;
}

float FNamiCameraMath::SmoothDampAngle(float CurrentAngleDeg, float TargetAngleDeg, float &OutVelocity, float SmoothTime, float DeltaTime, float MaxSpeed)
{
	// 找到最短角度差（处理360度环绕）
	float DeltaAngle = FMath::FindDeltaAngleDegrees(CurrentAngleDeg, TargetAngleDeg);
	float AdjustedTarget = CurrentAngleDeg + DeltaAngle;

	return SmoothDamp(CurrentAngleDeg, AdjustedTarget, OutVelocity, SmoothTime, DeltaTime, MaxSpeed);
}

FVector FNamiCameraMath::SmoothDamp(const FVector &Current, const FVector &Target, FVector &OutVelocity, float SmoothTime, float DeltaTime, float MaxSpeed)
{
	// 如果平滑时间为0，直接返回目标
	if (SmoothTime <= 0.0f)
	{
		OutVelocity = FVector::ZeroVector;
		return Target;
	}

	// 如果距离很小，直接返回目标
	FVector Delta = Target - Current;
	if (Delta.IsNearlyZero())
	{
		OutVelocity = FVector::ZeroVector;
		return Target;
	}

	// 将SmoothTime转换为InterpSpeed
	// SmoothTime越小，InterpSpeed越大，相机响应越快
	float InterpSpeed = 1.0f / SmoothTime;

	// 使用UE内置的平滑插值函数，第四个参数是InterpSpeed
	return FMath::VInterpTo(Current, Target, DeltaTime, InterpSpeed);
}

FRotator FNamiCameraMath::SmoothDamp(const FRotator &Current, const FRotator &Target, FRotator &OutVelocity, float SmoothTime, float DeltaTime, float MaxSpeed)
{
	// 如果平滑时间为0，直接返回目标
	if (SmoothTime <= 0.0f)
	{
		OutVelocity = FRotator::ZeroRotator;
		return Target;
	}

	// 分别处理每个旋转分量，自动处理360度环绕
	float PitchVelocity = OutVelocity.Pitch;
	float YawVelocity = OutVelocity.Yaw;
	float RollVelocity = OutVelocity.Roll;

	float NewPitch = SmoothDampAngle(Current.Pitch, Target.Pitch, PitchVelocity, SmoothTime, DeltaTime, MaxSpeed);
	float NewYaw = SmoothDampAngle(Current.Yaw, Target.Yaw, YawVelocity, SmoothTime, DeltaTime, MaxSpeed);
	float NewRoll = SmoothDampAngle(Current.Roll, Target.Roll, RollVelocity, SmoothTime, DeltaTime, MaxSpeed);

	// 更新OutVelocity
	OutVelocity.Pitch = PitchVelocity;
	OutVelocity.Yaw = YawVelocity;
	OutVelocity.Roll = RollVelocity;

	FRotator NewRotation(NewPitch, NewYaw, NewRoll);
	NewRotation.Normalize();
	return NewRotation;
}

FRotator FNamiCameraMath::SmoothDampRotator(
	const FRotator &Current,
	const FRotator &Target,
	float &OutYawVelocity,
	float &OutPitchVelocity,
	float YawSmoothTime,
	float PitchSmoothTime,
	float DeltaTime)
{
	float NewPitch = Current.Pitch;
	float NewYaw = Current.Yaw;
	float NewRoll = Target.Roll; // Roll通常不需要平滑

	if (PitchSmoothTime > 0.0f)
	{
		NewPitch = SmoothDampAngle(Current.Pitch, Target.Pitch, OutPitchVelocity, PitchSmoothTime, DeltaTime);
	}
	else
	{
		float PitchDelta = FMath::FindDeltaAngleDegrees(Current.Pitch, Target.Pitch);
		NewPitch = Current.Pitch + PitchDelta;
		OutPitchVelocity = 0.0f;
	}

	if (YawSmoothTime > 0.0f)
	{
		NewYaw = SmoothDampAngle(Current.Yaw, Target.Yaw, OutYawVelocity, YawSmoothTime, DeltaTime);
	}
	else
	{
		float YawDelta = FMath::FindDeltaAngleDegrees(Current.Yaw, Target.Yaw);
		NewYaw = Current.Yaw + YawDelta;
		OutYawVelocity = 0.0f;
	}

	FRotator NewRotation(NewPitch, NewYaw, NewRoll);
	NewRotation.Normalize();
	return NewRotation;
}

FQuat FNamiCameraMath::SmoothDamp(const FQuat &Current, const FQuat &Target, float &OutAngularVelocity, float SmoothTime, float DeltaTime, float MaxSpeed)
{
	float AngularDistance = Current.AngularDistance(Target);

	// 如果角度差很小，直接返回目标
	if (AngularDistance <= 0.0001f)
	{
		OutAngularVelocity = 0.0f;
		return Target;
	}

	// 对角度距离进行平滑
	float SmoothAngularDistance = SmoothDamp(AngularDistance, 0.0f, OutAngularVelocity, SmoothTime, DeltaTime, MaxSpeed);

	// 使用球面插值
	float t = 1.0f - (SmoothAngularDistance / AngularDistance);
	return FQuat::Slerp(Current, Target, t);
}

float FNamiCameraMath::MapSmoothIntensity(float SmoothIntensity)
{
	// 将0.0-1.0的平滑强度映射到0.0-2.0的实际平滑时间
	// 使用非线性映射，使调整更加直观
	// 映射公式：ActualSmoothTime = 2.0 * SmoothIntensity^2
	// 这个公式会让平滑强度在0-0.5之间变化比较明显，0.5-1.0之间变化逐渐平缓

	// 首先将平滑强度限制在0.0-1.0范围内
	float ClampedIntensity = FMath::Clamp(SmoothIntensity, 0.0f, 1.0f);

	// 使用幂函数进行非线性映射
	// 0.0 -> 0.0秒
	// 0.5 -> 0.5秒
	// 1.0 -> 2.0秒
	float ActualSmoothTime = 2.0f * FMath::Square(ClampedIntensity);

	return ActualSmoothTime;
}
