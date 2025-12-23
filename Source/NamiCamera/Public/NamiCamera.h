// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

/**
 * NamiCamera 插件主入口头文件
 *
 * 包含此文件即可访问所有 NamiCamera 公共 API。
 * 这为使用此插件的项目提供了便捷的单文件包含方式。
 *
 * 使用方式：
 *   #include "NamiCamera.h"
 *
 * 如需加快编译速度，也可直接包含特定头文件：
 *   #include "Components/NamiCameraComponent.h"
 *   #include "CameraModes/NamiCameraModeBase.h"
 *   等等
 */

// ====================================================================================
// 核心模块
// ====================================================================================

#include "Data/NamiCameraModule.h"
#include "Data/NamiCameraEnums.h"
#include "Data/NamiCameraTags.h"
#include "Data/NamiCameraInputProvider.h"
#include "Logging/LogNamiCamera.h"
#include "Math/NamiCameraMath.h"

// ====================================================================================
// 组件（入口点）
// ====================================================================================

#include "Components/NamiCameraComponent.h"
#include "Components/NamiPlayerCameraManager.h"
#include "Components/NamiSpringArm.h"

// ====================================================================================
// 模式（核心相机逻辑）
// ====================================================================================

// 基类和基础设施
#include "CameraModes/NamiCameraModeBase.h"
#include "CameraModes/NamiCameraModeStack.h"
#include "CameraModes/NamiCameraModeStackEntry.h"
#include "CameraModes/NamiCameraModeHandle.h"

// 数据结构
#include "Data/NamiCameraView.h"
#include "Data/NamiCameraState.h"
#include "Data/NamiCameraStateFlags.h"
#include "Data/NamiBlendConfig.h"
#include "Data/NamiCameraPipelineContext.h"

// 组合式相机模式（策略/计算器模式）
#include "CameraModes/NamiComposableCameraMode.h"
#include "CameraModes/NamiFollowTarget.h"

// 开箱即用的预设
#include "CameraModes/NamiThirdPersonCameraMode.h"
#include "CameraModes/NamiDualFocusCameraMode.h"

// ====================================================================================
// 计算器（Calculator - Mode 内部使用，原 Strategy）
// ====================================================================================

// 基类
#include "Calculators/NamiCameraCalculatorBase.h"
#include "Calculators/Target/NamiCameraTargetCalculator.h"
#include "Calculators/Position/NamiCameraPositionCalculator.h"
#include "Calculators/Rotation/NamiCameraRotationCalculator.h"
#include "Calculators/FOV/NamiCameraFOVCalculator.h"

// 目标计算器
#include "Calculators/Target/NamiSingleTargetCalculator.h"
#include "Calculators/Target/NamiDualFocusTargetCalculator.h"

// 位置计算器
#include "Calculators/Position/NamiOffsetPositionCalculator.h"
#include "Calculators/Position/NamiEllipseOrbitPositionCalculator.h"

// 旋转计算器
#include "Calculators/Rotation/NamiControlRotationCalculator.h"
#include "Calculators/Rotation/NamiLookAtRotationCalculator.h"

// FOV 计算器
#include "Calculators/FOV/NamiStaticFOVCalculator.h"
#include "Calculators/FOV/NamiFramingFOVCalculator.h"

// ====================================================================================
// 模式组件（ModeComponent - CameraMode 级别的功能组件）
// ====================================================================================

// 基类
#include "ModeComponents/NamiCameraModeComponent.h"

// 具体组件
#include "ModeComponents/NamiCameraShakeComponent.h"
#include "ModeComponents/NamiCameraCollisionComponent.h"
#include "ModeComponents/NamiCameraDynamicFOVComponent.h"
#include "ModeComponents/NamiCameraLockOnComponent.h"
#include "ModeComponents/NamiTargetVisibilityComponent.h"
#include "ModeComponents/NamiCameraSpringArmComponent.h"
#include "ModeComponents/NamiCameraEffectComponent.h"

// ====================================================================================
// 相机调整器（Adjust - 临时效果）
// ====================================================================================

#include "CameraAdjust/NamiCameraAdjust.h"
#include "CameraAdjust/NamiCameraAdjustParams.h"
#include "CameraAdjust/NamiCameraAdjustCurveBinding.h"
#include "CameraAdjust/NamiAnimNotifyCameraAdjust.h"

// ====================================================================================
// 动画集成
// ====================================================================================

#include "Animation/AnimNotifyState_CameraAdjust.h"

// 配置
#include "DevelopSetting/NamiCameraSettings.h"
