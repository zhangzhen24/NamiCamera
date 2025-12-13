// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NamiCameraStateFlags.generated.h"

/**
 * 相机状态修改标志
 * 
 * 用于追踪哪些相机参数被修改过
 * 这样在混合时可以只处理真正被修改的部分
 */
USTRUCT(BlueprintType)
struct NAMICAMERA_API FNamiCameraStateFlags
{
	GENERATED_BODY()

public:
	FNamiCameraStateFlags() = default;
	explicit FNamiCameraStateFlags(bool bDefaultValue);

	// ========== 枢轴点参数 ==========
	
	/** 枢轴点位置 */
	uint8 bPivotLocation:1 = false;
	
	/** 枢轴点旋转 */
	uint8 bPivotRotation:1 = false;
	
	// ========== 吊臂参数 ==========
	
	/** 吊臂长度 */
	uint8 bArmLength:1 = false;
	
	/** 吊臂旋转（围绕枢轴点） */
	uint8 bArmRotation:1 = false;
	
	/** 吊臂偏移 */
	uint8 bArmOffset:1 = false;

	// ========== 相机参数 ==========
	
	/** 相机位置偏移 */
	uint8 bCameraLocationOffset:1 = false;
	
	/** 相机旋转偏移（纯视觉效果，不受玩家输入影响） */
	uint8 bCameraRotationOffset:1 = false;
	
	/** 视野角度 */
	uint8 bFieldOfView:1 = false;

	// ========== 输出结果标志 ==========
	
	/** 最终相机位置 */
	uint8 bCameraLocation:1 = false;
	
	/** 最终相机旋转 */
	uint8 bCameraRotation:1 = false;

	// ========== 工具方法 ==========
	
	/** 清除所有标志 */
	void Clear();
	
	/** 设置所有标志 */
	void SetAll(bool bValue);
	
	/** 设置所有输入参数标志 */
	void SetAllInput(bool bValue);
	
	/** 设置所有输出结果标志 */
	void SetAllOutput(bool bValue);
	
	/** 是否有任何输入参数被修改 */
	bool HasAnyInputFlag() const;
	
	/** 是否有任何输出结果被修改 */
	bool HasAnyOutputFlag() const;
	
	/** 是否有任何标志被设置 */
	bool HasAnyFlag() const;
	
	/** 合并另一个标志（OR 运算） */
	FNamiCameraStateFlags& operator|=(const FNamiCameraStateFlags& Other);
	
	/** 合并两个标志（OR 运算） */
	FNamiCameraStateFlags operator|(const FNamiCameraStateFlags& Other) const;
	
	/** 交集（AND 运算） */
	FNamiCameraStateFlags operator&(const FNamiCameraStateFlags& Other) const;
	
	/** 返回所有标志都设置的静态实例 */
	static const FNamiCameraStateFlags& All();
	
	/** 返回所有输入标志都设置的静态实例 */
	static const FNamiCameraStateFlags& AllInput();
	
	/** 返回所有输出标志都设置的静态实例 */
	static const FNamiCameraStateFlags& AllOutput();
};

