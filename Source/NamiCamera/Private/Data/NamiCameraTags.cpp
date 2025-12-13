// Copyright Epic Games, Inc. All Rights Reserved.
#include "Data/NamiCameraTags.h"

// ==================== Feature 生命周期管理 ====================

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Camera_Feature_ManualCleanup, "Camera.Feature.ManualCleanup",
	"标记需要手动清理的 Feature，这些 Feature 的 EndBehavior 为 Stay，不会自动结束");

