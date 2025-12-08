// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Features/NamiCameraFeature.h"
#include "Modes/Templates/NamiFollowCameraMode.h"
#include "NamiToricSpaceFeature.generated.h"

/**
 * 复曲面空间相机 Feature
 * 
 * 基于 Toric Space（复曲面空间）理论实现的相机系统，常用于双人对话、战斗等场景。
 * 
 * 核心概念：
 * - 相机在一个围绕两个目标的复曲面（torus-like space）上移动
 * - 提供连续的参数化空间，避免相机跳变
 * - 自动满足180度规则（180-degree rule）和反拍（reverse shot）语法
 * 
 * 工作原理：
 * - 检测是否有两个主要目标（如角色和敌人）
 * - 使用复曲面参数（Theta, Phi, Radius）计算相机位置
 * - 在复曲面空间中平滑过渡，实现电影化的镜头语言
 * 
 * 参考：
 * - Toric Space (SIGGRAPH 2015) - Manipulation SolidFrame
 * - 常用于游戏中的双人对话、战斗锁定等场景
 * 
 * 适用场景：
 * - 战斗相机（角色 vs 敌人）
 * - 对话相机（两个角色）
 * - 任何需要围绕两个目标进行相机控制的场景
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class NAMICAMERA_API UNamiToricSpaceFeature : public UNamiCameraFeature
{
	GENERATED_BODY()

public:
	UNamiToricSpaceFeature();

	// ========== Feature 生命周期 ==========
	virtual void Initialize_Implementation(UNamiCameraModeBase* InCameraMode) override;
	virtual void Activate_Implementation() override;
	virtual void Update_Implementation(float DeltaTime) override;
	virtual void ApplyToView_Implementation(FNamiCameraView& InOutView, float DeltaTime) override;

	// ========== 配置 ==========

	/** 复曲面水平角度（度，0-360）- 相机围绕目标的水平角度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Parameters",
		meta = (DisplayName = "水平角度 (Theta)",
		ClampMin = "0.0", ClampMax = "360.0", UIMin = "0.0", UIMax = "360.0",
		Tooltip = "相机在复曲面上的水平角度（Theta）。\n0° = 相机在目标连线的右侧\n90° = 相机在目标连线的后方\n180° = 相机在目标连线的左侧\n270° = 相机在目标连线的前方"))
	float ToricTheta = 90.0f;

	/** 复曲面垂直角度（度，-90到90）- 相机的俯仰角度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Parameters",
		meta = (DisplayName = "垂直角度 (Phi)",
		ClampMin = "-90.0", ClampMax = "90.0", UIMin = "-90.0", UIMax = "90.0",
		Tooltip = "相机在复曲面上的垂直角度（Phi）。\n正值 = 俯视角度\n负值 = 仰视角度\n0 = 水平视角"))
	float ToricPhi = 0.0f;

	/** 复曲面半径（单位：cm）- 相机到目标连线的距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Parameters",
		meta = (DisplayName = "半径 (Radius)",
		ClampMin = "100.0", ClampMax = "5000.0", UIMin = "100.0", UIMax = "5000.0",
		Tooltip = "相机到目标连线中点的距离（半径）。值越大，相机距离越远。"))
	float ToricRadius = 500.0f;

	/** 相机高度偏移（单位：cm）- 相对于目标连线中点的高度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Parameters",
		meta = (DisplayName = "高度偏移",
		Tooltip = "相机相对于目标连线中点的垂直高度偏移。"))
	float ToricHeight = 100.0f;

	/** 是否自动计算半径（基于目标距离） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Auto",
		meta = (DisplayName = "自动计算半径",
		Tooltip = "如果启用，半径会根据两个目标之间的距离自动调整。"))
	bool bAutoCalculateRadius = false;

	/** 半径缩放系数（仅在自动计算时使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Auto",
		meta = (DisplayName = "半径缩放系数", EditCondition = "bAutoCalculateRadius",
		ClampMin = "0.5", ClampMax = "5.0",
		Tooltip = "自动计算半径时的缩放系数。值越大，相机距离越远。"))
	float RadiusScaleFactor = 1.5f;

	/** 最小半径（自动计算时的下限） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Auto",
		meta = (DisplayName = "最小半径", EditCondition = "bAutoCalculateRadius",
		ClampMin = "100.0",
		Tooltip = "自动计算半径时的最小值。"))
	float MinRadius = 200.0f;

	/** 最大半径（自动计算时的上限） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Auto",
		meta = (DisplayName = "最大半径", EditCondition = "bAutoCalculateRadius",
		ClampMin = "100.0",
		Tooltip = "自动计算半径时的最大值。"))
	float MaxRadius = 2000.0f;

	/** 是否启用180度规则（自动约束相机在"正确的一侧"） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Rules",
		meta = (DisplayName = "启用180度规则",
		Tooltip = "如果启用，自动确保相机始终在目标的正确一侧，满足电影180度规则。"))
	bool bEnforce180DegreeRule = true;

	/** 聚焦目标（0=目标1，1=目标2，0.5=两个目标的中点） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Focus",
		meta = (DisplayName = "聚焦目标",
		ClampMin = "0.0", ClampMax = "1.0",
		Tooltip = "相机聚焦的目标。\n0.0 = 聚焦目标1（通常是角色）\n1.0 = 聚焦目标2（通常是敌人）\n0.5 = 聚焦两个目标的中点"))
	float FocusTarget = 0.5f;

	/** 角度平滑时间（秒）- Theta 和 Phi 变化的平滑时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Smoothing",
		meta = (DisplayName = "角度平滑时间",
		ClampMin = "0.0", UIMin = "0.0",
		Tooltip = "Theta 和 Phi 角度变化的平滑时间（秒）。0 = 无平滑。"))
	float AngleSmoothTime = 0.2f;

	/** 半径平滑时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Smoothing",
		meta = (DisplayName = "半径平滑时间",
		ClampMin = "0.0", UIMin = "0.0",
		Tooltip = "半径变化的平滑时间（秒）。0 = 无平滑。"))
	float RadiusSmoothTime = 0.3f;

	/** 高度平滑时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Smoothing",
		meta = (DisplayName = "高度平滑时间",
		ClampMin = "0.0", UIMin = "0.0",
		Tooltip = "高度变化的平滑时间（秒）。0 = 无平滑。"))
	float HeightSmoothTime = 0.2f;

	/** 需要两个目标才启用（如果只有一个目标，不应用复曲面空间） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Conditions",
		meta = (DisplayName = "需要两个目标",
		Tooltip = "如果启用，只有当存在两个有效目标时才应用复曲面空间。否则使用默认相机计算。"))
	bool bRequireTwoTargets = true;

	/** 自动设置主角为主目标（如果启用，会在初始化时自动将相机组件的Owner设置为PrimaryTarget） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Auto Setup",
		meta = (DisplayName = "自动设置主角",
		Tooltip = "如果启用，在Feature初始化时自动将相机组件的Owner（主角）设置为PrimaryTarget。"))
	bool bAutoSetPrimaryTarget = true;

	/** 多目标模式：使用所有目标的边界框中心作为复曲面中心 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Multi-Target",
		meta = (DisplayName = "启用多目标模式",
		Tooltip = "如果启用，当有多个目标时，使用所有目标的边界框中心作为复曲面空间的计算中心。\n适合战斗场景中的多敌人锁定。"))
	bool bEnableMultiTargetMode = true;

	/** 多目标时使用加权中心（而非简单平均） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toric Space|Multi-Target",
		meta = (DisplayName = "使用加权中心", EditCondition = "bEnableMultiTargetMode",
		Tooltip = "如果启用，多目标时使用权重计算中心点。否则使用简单平均。"))
	bool bUseWeightedCenter = true;

protected:
	/** 检测是否有两个有效目标 */
	bool HasTwoTargets() const;

	/** 获取目标1（通常是角色/主目标） */
	AActor* GetTarget1() const;

	/** 获取目标2（通常是敌人/次要目标） */
	AActor* GetTarget2() const;

	/** 计算复曲面空间中的相机位置 */
	FVector CalculateToricSpaceCameraLocation(
		const FVector& Target1Location,
		const FVector& Target2Location,
		float Theta,
		float Phi,
		float Radius,
		float Height) const;

	/** 应用180度规则，调整 Theta 确保相机在"正确的一侧" */
	float Apply180DegreeRule(float Theta, const FVector& Target1Location, const FVector& Target2Location, float InFocusTarget) const;

	/** 计算聚焦点位置（基于 FocusTarget 参数） */
	FVector CalculateFocusPoint(const FVector& Target1Location, const FVector& Target2Location) const;

	/** 获取所有有效目标 */
	TArray<FNamiFollowTarget> GetAllValidTargets() const;

	/** 计算多目标的中心点（边界框中心或加权中心） */
	FVector CalculateMultiTargetCenter(const TArray<FNamiFollowTarget>& Targets) const;

	/** 计算多目标的边界框大小（用于自动半径计算） */
	float CalculateMultiTargetSize(const TArray<FNamiFollowTarget>& Targets) const;

	// ========== 内部状态 ==========

	/** 当前平滑后的 Theta */
	float CurrentTheta = 90.0f;

	/** 当前平滑后的 Phi */
	float CurrentPhi = 0.0f;

	/** 当前平滑后的半径 */
	float CurrentRadius = 500.0f;

	/** 当前平滑后的高度 */
	float CurrentHeight = 100.0f;

	/** Theta 平滑速度 */
	float ThetaVelocity = 0.0f;

	/** Phi 平滑速度 */
	float PhiVelocity = 0.0f;

	/** 半径平滑速度 */
	float RadiusVelocity = 0.0f;

	/** 高度平滑速度 */
	float HeightVelocity = 0.0f;

	/** 上一帧的目标1位置（用于检测变化） */
	FVector LastTarget1Location = FVector::ZeroVector;

	/** 上一帧的目标2位置（用于检测变化） */
	FVector LastTarget2Location = FVector::ZeroVector;
};

