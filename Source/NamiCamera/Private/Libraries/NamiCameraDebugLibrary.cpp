// Copyright Epic Games, Inc. All Rights Reserved.

#include "Libraries/NamiCameraDebugLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Structs/State/NamiCameraFramingTypes.h"

void UNamiCameraDebugLibrary::DrawCameraView(
	const FNamiCameraView& View,
	UWorld* World,
	float Duration,
	float Thickness,
	bool bDrawPivot,
	bool bDrawCamera,
	bool bDrawDirection,
	bool bDrawDistance)
{
	if (!IsValid(World))
	{
		return;
	}

	const float DrawDuration = Duration > 0.0f ? Duration : -1.0f;
	const float SphereSize = 10.0f;
	const float ArrowSize = 50.0f;
	const float TextScale = 1.0f;

	// 绘制观察点（PivotLocation）
	if (bDrawPivot)
	{
		// 观察点用绿色球体表示
		DrawDebugSphere(World, View.PivotLocation, SphereSize, 12, FColor::Green, false, DrawDuration, 0, Thickness);
		
	}

	// 绘制相机位置
	if (bDrawCamera)
	{
		// 相机位置用蓝色球体表示
		DrawDebugSphere(World, View.CameraLocation, SphereSize, 12, FColor::Blue, false, DrawDuration, 0, Thickness);
		
	}

	// 绘制从相机到观察点的连线
	if (bDrawPivot && bDrawCamera)
	{
		FVector Direction = View.PivotLocation - View.CameraLocation;
		float Distance = Direction.Size();
		
		// 绘制连线（红色）
		DrawDebugLine(World, View.CameraLocation, View.PivotLocation, FColor::Red, false, DrawDuration, 0, Thickness);
		
	}

	// 绘制相机方向
	if (bDrawDirection && bDrawCamera)
	{
		FVector Forward = View.CameraRotation.Vector();
		FVector Right = FRotator(0.0f, View.CameraRotation.Yaw + 90.0f, 0.0f).Vector();
		FVector Up = FRotator(View.CameraRotation.Pitch - 90.0f, View.CameraRotation.Yaw, 0.0f).Vector();
		
		// 前方向（红色箭头）
		DrawDebugDirectionalArrow(World, View.CameraLocation, 
			View.CameraLocation + Forward * ArrowSize, 
			ArrowSize * 0.3f, FColor::Red, false, DrawDuration, 0, Thickness);
		
		// 右方向（绿色箭头）
		DrawDebugDirectionalArrow(World, View.CameraLocation, 
			View.CameraLocation + Right * ArrowSize * 0.5f, 
			ArrowSize * 0.15f, FColor::Green, false, DrawDuration, 0, Thickness);
		
		// 上方向（蓝色箭头）
		DrawDebugDirectionalArrow(World, View.CameraLocation, 
			View.CameraLocation + Up * ArrowSize * 0.5f, 
			ArrowSize * 0.15f, FColor::Blue, false, DrawDuration, 0, Thickness);
		
	}

}

void UNamiCameraDebugLibrary::DrawCameraViewSimple(
	const FNamiCameraView& View,
	UWorld* World,
	float Duration)
{
	DrawCameraView(View, World, Duration, 2.0f, true, true, true, true);
}

FString UNamiCameraDebugLibrary::GetCameraViewString(const FNamiCameraView& View)
{
	return FString::Printf(
		TEXT("CameraView:\n")
		TEXT("  PivotLocation: %s\n")
		TEXT("  CameraLocation: %s\n")
		TEXT("  CameraRotation: P=%.2f Y=%.2f R=%.2f\n")
		TEXT("  FOV: %.2f\n")
		TEXT("  Distance: %.2f cm"),
		*View.PivotLocation.ToString(),
		*View.CameraLocation.ToString(),
		View.CameraRotation.Pitch,
		View.CameraRotation.Yaw,
		View.CameraRotation.Roll,
		View.FOV,
		(View.PivotLocation - View.CameraLocation).Size()
	);
}

namespace
{
	void DrawRect(UWorld* World, const FVector& Origin, const FVector& X, const FVector& Y, const FVector& Extent, const FColor& Color, float Duration, float Thickness)
	{
		FVector P1 = Origin + X * Extent.X + Y * Extent.Y;
		FVector P2 = Origin - X * Extent.X + Y * Extent.Y;
		FVector P3 = Origin - X * Extent.X - Y * Extent.Y;
		FVector P4 = Origin + X * Extent.X - Y * Extent.Y;

		DrawDebugLine(World, P1, P2, Color, false, Duration, 0, Thickness);
		DrawDebugLine(World, P2, P3, Color, false, Duration, 0, Thickness);
		DrawDebugLine(World, P3, P4, Color, false, Duration, 0, Thickness);
		DrawDebugLine(World, P4, P1, Color, false, Duration, 0, Thickness);
	}
}

void UNamiCameraDebugLibrary::DrawFramingDebug(
	UWorld* World,
	const FVector& CameraLocation,
	const FRotator& CameraRotation,
	float FOV,
	const FVector& FramingCenter,
	float ArmLength,
	const FVector2D& SafeZone,
	const TArray<FVector>& Targets,
	float Duration,
	float Thickness)
{
	if (!IsValid(World))
	{
		return;
	}

	const float DrawDuration = Duration > 0.0f ? Duration : -1.0f;

	// 绘制目标与中心
	DrawDebugSphere(World, FramingCenter, 10.0f, 12, FColor::Green, false, DrawDuration, 0, Thickness);
	for (const FVector& Pos : Targets)
	{
		DrawDebugSphere(World, Pos, 8.0f, 12, FColor::Yellow, false, DrawDuration, 0, Thickness);
		DrawDebugLine(World, Pos, FramingCenter, FColor::Yellow, false, DrawDuration, 0, Thickness);
	}

	// 绘制相机到中心
	DrawDebugLine(World, CameraLocation, FramingCenter, FColor::Cyan, false, DrawDuration, 0, Thickness);

	// 以当前 FOV 和距离估算屏幕矩形并绘制安全区
	const float Aspect = 16.0f / 9.0f;
	const float Distance = FMath::Max(ArmLength, 100.0f);
	const float HalfFovRad = FMath::DegreesToRadians(FMath::Max(1.0f, FOV * 0.5f));
	const float HalfHeight = Distance * FMath::Tan(HalfFovRad);
	const float HalfWidth = HalfHeight * Aspect;

	const FVector Forward = CameraRotation.Vector();
	const FVector Right = FRotationMatrix(CameraRotation).GetScaledAxis(EAxis::Y);
	const FVector Up = FRotationMatrix(CameraRotation).GetScaledAxis(EAxis::Z);

	const FVector PlaneOrigin = CameraLocation + Forward * Distance;

	// 外框（近似视口）
	DrawRect(World, PlaneOrigin, Right, Up, FVector(HalfWidth, HalfHeight, 0), FColor::Blue, DrawDuration, Thickness);

	// 安全区（按 SafeZone 比例缩小）
	const FVector SafeExtent = FVector(HalfWidth * (1.0f - SafeZone.X * 2.0f), HalfHeight * (1.0f - SafeZone.Y * 2.0f), 0);
	DrawRect(World, PlaneOrigin, Right, Up, SafeExtent, FColor::Green, DrawDuration, Thickness);

	// 文字
	DrawDebugString(World, PlaneOrigin + Up * (HalfHeight + 20.0f), FString::Printf(TEXT("FOV: %.1f"), FOV),
		nullptr, FColor::Magenta, DrawDuration, false, 1.0f);
	DrawDebugString(World, FramingCenter + FVector(0, 0, 30.0f), TEXT("Framing Center"), nullptr, FColor::Green, DrawDuration, false, 1.0f);
}

