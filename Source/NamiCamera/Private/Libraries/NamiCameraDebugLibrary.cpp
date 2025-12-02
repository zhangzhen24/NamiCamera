// Copyright Epic Games, Inc. All Rights Reserved.

#include "Libraries/NamiCameraDebugLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"

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
		
		// 观察点标签
		if (bDrawDistance)
		{
			DrawDebugString(World, View.PivotLocation + FVector(0, 0, SphereSize * 2), 
				FString::Printf(TEXT("Pivot\n%s"), *View.PivotLocation.ToString()), 
				nullptr, FColor::Green, DrawDuration, false, TextScale);
		}
	}

	// 绘制相机位置
	if (bDrawCamera)
	{
		// 相机位置用蓝色球体表示
		DrawDebugSphere(World, View.CameraLocation, SphereSize, 12, FColor::Blue, false, DrawDuration, 0, Thickness);
		
		// 相机位置标签
		if (bDrawDistance)
		{
			DrawDebugString(World, View.CameraLocation + FVector(0, 0, SphereSize * 2), 
				FString::Printf(TEXT("Camera\n%s"), *View.CameraLocation.ToString()), 
				nullptr, FColor::Blue, DrawDuration, false, TextScale);
		}
	}

	// 绘制从相机到观察点的连线
	if (bDrawPivot && bDrawCamera)
	{
		FVector Direction = View.PivotLocation - View.CameraLocation;
		float Distance = Direction.Size();
		
		// 绘制连线（红色）
		DrawDebugLine(World, View.CameraLocation, View.PivotLocation, FColor::Red, false, DrawDuration, 0, Thickness);
		
		// 绘制距离信息
		if (bDrawDistance && Distance > 0.0f)
		{
			FVector MidPoint = (View.CameraLocation + View.PivotLocation) * 0.5f;
			DrawDebugString(World, MidPoint, 
				FString::Printf(TEXT("Distance: %.1f cm"), Distance), 
				nullptr, FColor::Yellow, DrawDuration, false, TextScale);
		}
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
		
		// 相机旋转信息
		if (bDrawDistance)
		{
			DrawDebugString(World, View.CameraLocation + Forward * ArrowSize * 1.2f, 
				FString::Printf(TEXT("Rotation\nP:%.1f Y:%.1f R:%.1f"), 
					View.CameraRotation.Pitch, View.CameraRotation.Yaw, View.CameraRotation.Roll), 
				nullptr, FColor::Cyan, DrawDuration, false, TextScale);
		}
	}

	// 绘制FOV信息
	if (bDrawDistance && bDrawCamera)
	{
		DrawDebugString(World, View.CameraLocation + FVector(0, 0, -SphereSize * 2), 
			FString::Printf(TEXT("FOV: %.1f°"), View.FOV), 
			nullptr, FColor::Magenta, DrawDuration, false, TextScale);
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

