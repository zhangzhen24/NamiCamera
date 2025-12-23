// Copyright Qiu, Inc. All Rights Reserved.

#include "Stat/NamiCameraStats.h"

/**
 * NamiCamera 性能统计实现
 *
 * 本文件定义了在 NamiCameraStats.h 中声明的所有性能统计项。
 * 这些统计项用于追踪和分析相机系统的性能表现。
 */

// ============================================================================
// 统计项定义
// ============================================================================

DEFINE_STAT(STAT_NamiCamera_GetCameraView);
DEFINE_STAT(STAT_NamiCamera_ModeStack);
DEFINE_STAT(STAT_NamiCamera_CameraAdjust);
DEFINE_STAT(STAT_NamiCamera_ModeComponents);
DEFINE_STAT(STAT_NamiCamera_Smoothing);
