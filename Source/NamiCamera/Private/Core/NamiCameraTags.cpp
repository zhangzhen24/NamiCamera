// Copyright Qiu, Inc. All Rights Reserved.
#include "Core/NamiCameraTags.h"

// ==================== ModeComponent 生命周期管理 ====================

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Camera_ModeComponent_ManualCleanup, "Camera.ModeComponent.ManualCleanup",
	"标记需要手动清理的 ModeComponent，这些组件的 EndBehavior 为 Stay，不会自动结束");

