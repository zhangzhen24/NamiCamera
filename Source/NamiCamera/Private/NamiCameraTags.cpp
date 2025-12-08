// Copyright Epic Games, Inc. All Rights Reserved.

#include "NamiCameraTags.h"

// ==================== Feature 生命周期管理 ====================

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Camera_Feature_ManualCleanup, "Camera.Feature.ManualCleanup", 
	"标记需要手动清理的 Feature，这些 Feature 的 EndBehavior 为 Stay，不会自动结束");

// ==================== Feature 来源标识 ====================

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Camera_Feature_Source_ANS_CameraAdjust, "Camera.Feature.Source.ANS.CameraAdjust",
	"标识由 AnimNotifyState_CameraAdjust 创建的 Feature");

