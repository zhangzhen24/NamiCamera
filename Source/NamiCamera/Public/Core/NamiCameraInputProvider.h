// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NamiCameraInputProvider.generated.h"

/**
 * 相机输入提供者接口
 * 
 * 用于解耦相机模块和输入系统
 * 输入系统实现此接口，相机模块通过接口获取输入信息
 * 
 * 设计目的：
 * - 降低耦合：相机模块不直接依赖输入系统
 * - 易于测试：可以创建 Mock 实现
 * - 易于扩展：可以支持不同的输入源（键盘、手柄、AI等）
 */
UINTERFACE(MinimalAPI, BlueprintType)
class UNamiCameraInputProvider : public UInterface
{
	GENERATED_BODY()
};

class NAMICAMERA_API INamiCameraInputProvider
{
	GENERATED_BODY()

public:
	/**
	 * 获取相机输入向量
	 * 
	 * @return 输入向量 (X=Yaw, Y=Pitch)
	 *         X: 水平输入，范围 [-1.0, 1.0]，负值=向左，正值=向右
	 *         Y: 垂直输入，范围 [-1.0, 1.0]，负值=向下，正值=向上
	 * 
	 * 输入大小可通过 FVector2D::Size() 获取
	 * 输入方向可通过向量本身获取
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera Input")
	FVector2D GetCameraInputVector() const;
};

