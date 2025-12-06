// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Structs/State/NamiCameraStateFlags.h"
#include "Enums/NamiCameraEnums.h"
#include "Structs/State/NamiCameraFramingTypes.h"
#include "NamiCameraState.generated.h"

/**
 * 相机状态
 * 
 * 核心数据结构，包含：
 * - 输入参数：可调整的"旋钮"（吊臂长度、旋转等）
 * - 输出结果：计算得出的最终值（相机位置、旋转）
 * - 修改追踪：记录哪些参数被修改过
 * 
 * 设计理念：
 * - 分离"参数"和"结果"
 * - 支持在不同阶段修改（修改参数 vs 修改结果）
 * - 通过 ChangedFlags 实现精确的按需混合
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraState
{
	GENERATED_BODY()

public:
	FNamiCameraState();

	// ==================== 输入参数 ====================
	// 这些是可调整的"旋钮"，修改它们会影响最终的相机位置计算
	
	/** 
	 * 枢轴点位置（跟随目标的位置）
	 * 通常是角色的某个骨骼或根位置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FVector PivotLocation;
	
	/** 
	 * 枢轴点旋转（通常来自玩家输入）
	 * 决定相机围绕枢轴点的朝向
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FRotator PivotRotation;
	
	/** 
	 * 吊臂长度（相机到枢轴点的距离）
	 * 负值会导致相机在枢轴点前方
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input",
			  meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ArmLength;
	
	/** 
	 * 吊臂旋转偏移（在 PivotRotation 基础上额外旋转）
	 * 
	 * 旋转原理：
	 * - 旋转中心：PivotLocation（角色位置）
	 * - 旋转半径：ArmLength（吊臂长度）
	 * - 基础旋转：PivotRotation（控制旋转）
	 * - 旋转偏移：ArmRotation（额外旋转）
	 * 
	 * 四元数组合：FinalQuat = PivotQuat * ArmQuat
	 * - 先应用 ArmQuat（局部空间旋转）
	 * - 再用 PivotQuat 转到世界空间
	 * - 避免万向锁问题
	 * 
	 * 影响范围：
	 * - 主要影响：相机位置（让相机沿以角色为中心的圆弧移动）
	 * - 间接影响：相机朝向（相机朝向会跟随吊臂方向，因为相机默认看向角色）
	 * 
	 * Pitch: 俯仰（正值让相机向上移动，形成俯视角）
	 * Yaw: 偏航（正值让相机向右绕行）
	 * Roll: 翻滚（不常用）
	 * 
	 * 与 CameraRotationOffset 的区别：
	 * - ArmRotation：调整吊臂方向，既影响位置也影响朝向（间接）
	 * - CameraRotationOffset：只调整相机朝向，不影响位置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Arm")
	FRotator ArmRotation;
	
	/** 
	 * 吊臂末端偏移（相机本地空间）
	 * 在吊臂末端应用的额外偏移
	 * 常用于肩部视角的左右偏移
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Arm")
	FVector ArmOffset;
	
	// ==================== 控制旋转参数 ====================
	// 代表玩家的视角意图，可被玩家输入打断
	
	/** 
	 * 控制旋转偏移（玩家视角意图）
	 * 
	 * 修改 ControlRotation，代表临时改变玩家"想看哪里"
	 * 区别于 CameraRotationOffset：
	 * - ControlRotationOffset：修改玩家视角意图，可被玩家输入打断
	 * - CameraRotationOffset：纯视觉效果，不受玩家输入影响
	 * 
	 * 适用场景：
	 * - 技能释放时引导玩家看向目标
	 * - 演出镜头的视角控制
	 * - 过场动画的强制视角
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Control")
	FRotator ControlRotationOffset;
	
	// ==================== 相机参数 ====================
	// 直接作用于相机的参数（在吊臂计算之后应用）
	
	/** 
	 * 相机位置偏移（相机本地空间）
	 * 在吊臂计算完成后，额外的相机位置偏移
	 * 用于微调相机位置，如技能特效时的相机抖动
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Camera")
	FVector CameraLocationOffset;
	
	/** 
	 * 相机旋转偏移
	 * 在吊臂计算完成后，额外的相机旋转偏移
	 * 只影响相机朝向，不影响相机位置
	 * 用于相机视角微调，如看向特定目标
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Camera")
	FRotator CameraRotationOffset;
	
	/** 
	 * 视野角度（FOV）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Camera",
			  meta = (ClampMin = "5.0", ClampMax = "170.0", UIMin = "5.0", UIMax = "170.0"))
	float FieldOfView;

	// ==================== 输出结果 ====================
	// 这些是从输入参数计算得出的最终值
	
	/** 
	 * 最终相机世界位置（计算得出）
	 * 计算流程:
	 * 1. 从 PivotLocation 出发
	 * 2. 按 (PivotRotation + ArmRotation) 方向，向后延伸 ArmLength
	 * 3. 在吊臂末端应用 ArmOffset（吊臂本地空间）
	 * 4. 应用 CameraLocationOffset（相机本地空间）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	FVector CameraLocation;
	
	/** 
	 * 最终相机世界旋转（计算得出）
	 * 计算流程:
	 * 1. 基础旋转 = PivotRotation + ArmRotation（吊臂方向决定相机朝向）
	 * 2. 应用 CameraRotationOffset（额外的相机旋转偏移）
	 * 
	 * 注意：ArmRotation 通过影响吊臂方向间接影响相机旋转
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	FRotator CameraRotation;

	// ==================== 修改追踪 ====================
	
	/** 
	 * 修改标志
	 * 记录哪些参数被修改过，用于混合时只处理被修改的部分
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Tracking")
	FNamiCameraStateFlags ChangedFlags;

	// ==================== 核心方法 ====================
	
	/**
	 * 从输入参数计算输出结果
	 * 计算 CameraLocation 和 CameraRotation
	 */
	void ComputeOutput();
	
	/**
	 * 应用另一个状态的修改
	 * 仅应用 Other.ChangedFlags 标记为 true 的参数
	 * 
	 * @param Other 要应用的状态
	 * @param BlendMode 混合模式
	 * @param Weight 混合权重（仅用于 Additive 模式）
	 */
	void ApplyChanged(const FNamiCameraState& Other, ENamiCameraBlendMode BlendMode, float Weight = 1.0f);
	
	/**
	 * 线性插值到另一个状态
	 * 
	 * @param To 目标状态
	 * @param Alpha 插值因子（0=this, 1=To）
	 */
	void Lerp(const FNamiCameraState& To, float Alpha);
	
	/**
	 * 仅对被修改的参数进行插值
	 * 
	 * @param To 目标状态
	 * @param Alpha 插值因子
	 * @param Mask 要插值的参数掩码
	 */
	void LerpChanged(const FNamiCameraState& To, float Alpha, const FNamiCameraStateFlags& Mask);
	
	/**
	 * 重置为默认值
	 */
	void Reset();
	
	/**
	 * 清除所有修改标志
	 */
	void ClearChangedFlags();
	
	/**
	 * 设置所有修改标志
	 */
	void SetAllChangedFlags();

	// ==================== 多目标构图（可选）====================

	/** 应用多目标构图计算结果到输入参数（会设置 ChangedFlags） */
	void ApplyFramingResult(const FNamiCameraFramingResult& Result);

	// ==================== 便捷设置方法（自动设置 ChangedFlags）====================
	
	void SetPivotLocation(const FVector& InValue);
	void SetPivotRotation(const FRotator& InValue);
	void SetArmLength(float InValue);
	void SetArmRotation(const FRotator& InValue);
	void SetArmOffset(const FVector& InValue);
	void SetControlRotationOffset(const FRotator& InValue);
	void SetCameraLocationOffset(const FVector& InValue);
	void SetCameraRotationOffset(const FRotator& InValue);
	void SetFieldOfView(float InValue);
	void SetCameraLocation(const FVector& InValue);
	void SetCameraRotation(const FRotator& InValue);

	// ==================== 便捷叠加方法（Additive，自动设置 ChangedFlags）====================
	
	void AddPivotLocation(const FVector& InDelta);
	void AddPivotRotation(const FRotator& InDelta);
	void AddArmLength(float InDelta);
	void AddArmRotation(const FRotator& InDelta);
	void AddArmOffset(const FVector& InDelta);
	void AddControlRotationOffset(const FRotator& InDelta);
	void AddCameraLocationOffset(const FVector& InDelta);
	void AddCameraRotationOffset(const FRotator& InDelta);
	void AddFieldOfView(float InDelta);
	void AddCameraLocation(const FVector& InDelta);
	void AddCameraRotation(const FRotator& InDelta);
};

