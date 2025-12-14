// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"

/**
 * NamiCamera 性能统计
 *
 * 本文件声明了 NamiCamera 插件的所有性能统计项。
 * 在游戏控制台中输入 'stat NamiCamera' 即可查看这些统计数据。
 *
 * 统计分组：
 * - STATGROUP_NamiCamera: 相机相关性能指标的主统计组
 *
 * 可用的统计项：
 * - STAT_NamiCamera_GetCameraView: 跟踪主相机视图计算的性能
 * - STAT_NamiCamera_ModeStack: 跟踪相机模式堆栈处理耗时
 * - STAT_NamiCamera_CameraAdjust: 跟踪相机调整计算耗时
 * - STAT_NamiCamera_GlobalFeatures: 跟踪全局特性应用耗时
 * - STAT_NamiCamera_Smoothing: 跟踪相机平滑处理耗时
 */

// ============================================================================
// 统计分组
// ============================================================================

/** NamiCamera 性能分析的主统计组 */
DECLARE_STATS_GROUP(TEXT("NamiCamera"), STATGROUP_NamiCamera, STATCAT_Advanced);

// ============================================================================
// 循环统计（帧时间追踪）
// ============================================================================

/** 每帧在 GetCameraView 中花费的总时间 */
DECLARE_CYCLE_STAT_EXTERN(TEXT("GetCameraView"), STAT_NamiCamera_GetCameraView, STATGROUP_NamiCamera, NAMICAMERA_API);

/** 处理相机模式堆栈所花费的时间 */
DECLARE_CYCLE_STAT_EXTERN(TEXT("Mode Stack"), STAT_NamiCamera_ModeStack, STATGROUP_NamiCamera, NAMICAMERA_API);

/** 计算相机调整所花费的时间 */
DECLARE_CYCLE_STAT_EXTERN(TEXT("Camera Adjust"), STAT_NamiCamera_CameraAdjust, STATGROUP_NamiCamera, NAMICAMERA_API);

/** 应用全局特性所花费的时间 */
DECLARE_CYCLE_STAT_EXTERN(TEXT("Global Features"), STAT_NamiCamera_GlobalFeatures, STATGROUP_NamiCamera, NAMICAMERA_API);

/** 相机平滑处理所花费的时间 */
DECLARE_CYCLE_STAT_EXTERN(TEXT("Smoothing"), STAT_NamiCamera_Smoothing, STATGROUP_NamiCamera, NAMICAMERA_API);

// ============================================================================
// 使用说明
// ============================================================================

/*
 * 如何使用这些统计项：
 *
 * 1. 游戏内控制台：
 *    - 输入：stat NamiCamera
 *    - 这将在屏幕上显示所有 NamiCamera 性能指标
 *
 * 2. Unreal Insights：
 *    - 这些统计数据会自动在 Unreal Insights 中追踪
 *    - 使用 Insights 可以进行详细的逐帧分析
 *
 * 3. 添加新的统计项：
 *    a) 在此头文件中声明：DECLARE_CYCLE_STAT_EXTERN(TEXT("名称"), STAT_名称, STATGROUP_NamiCamera, NAMICAMERA_API);
 *    b) 在 NamiCameraStats.cpp 中定义：DEFINE_STAT(STAT_名称);
 *    c) 在代码中使用：SCOPE_CYCLE_COUNTER(STAT_名称);
 *
 * 示例：
 *    void MyFunction()
 *    {
 *        SCOPE_CYCLE_COUNTER(STAT_NamiCamera_ModeStack);
 *        // 你的代码...
 *    }
 */
