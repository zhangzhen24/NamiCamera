// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

/**
 * Main include header for NamiCamera plugin
 *
 * Include this file to get access to all public NamiCamera APIs.
 * This provides a convenient single-include option for projects using the plugin.
 *
 * Usage:
 *   #include "NamiCamera.h"
 *
 * Alternatively, include specific headers directly for faster compilation:
 *   #include "Components/NamiCameraComponent.h"
 *   #include "CameraModes/NamiCameraModeBase.h"
 *   etc.
 */

// ====================================================================================
// Core Module
// ====================================================================================

#include "Data/NamiCameraModule.h"
#include "Data/NamiCameraEnums.h"
#include "Data/NamiCameraTags.h"
#include "Data/NamiCameraInputProvider.h"
#include "Logging/LogNamiCamera.h"
#include "Math/NamiCameraMath.h"

// ====================================================================================
// Components (Entry Point)
// ====================================================================================

#include "Components/NamiCameraComponent.h"
#include "Components/NamiPlayerCameraManager.h"
#include "Components/NamiSpringArm.h"

// ====================================================================================
// Modes (Core Camera Logic)
// ====================================================================================

// Base classes and infrastructure
#include "CameraModes/NamiCameraModeBase.h"
#include "CameraModes/NamiCameraModeStack.h"
#include "CameraModes/NamiCameraModeStackEntry.h"
#include "CameraModes/NamiCameraModeHandle.h"

// Data structures
#include "Data/NamiCameraView.h"
#include "Data/NamiCameraState.h"
#include "Data/NamiCameraStateFlags.h"
#include "Data/NamiBlendConfig.h"
#include "Data/NamiCameraPipelineContext.h"

// Templates for custom modes
#include "CameraModes/NamiFollowCameraMode.h"
#include "CameraModes/NamiFollowTarget.h"
#include "CameraModes/NamiSpringArmCameraMode.h"

// Ready-to-use presets
#include "CameraModes/NamiThirdPersonCamera.h"
#include "CameraModes/NamiShoulderCamera.h"

// ====================================================================================
// Features (Modular Extensions)
// ====================================================================================

#include "CameraFeatures/NamiCameraFeature.h"
#include "CameraFeatures/NamiCameraDebugInfo.h"
#include "CameraFeatures/Effects/NamiCameraEffectFeature.h"
#include "CameraFeatures/Effects/NamiCameraShakeFeature.h"

// ====================================================================================
// Adjustments (Runtime Modifications)
// ====================================================================================

// Base adjustment system
#include "CameraAdjust/NamiCameraAdjust.h"
#include "CameraAdjust/NamiCameraAdjustParams.h"
#include "CameraAdjust/NamiCameraAdjustCurveBinding.h"

// Animation integration
#include "Animation/AnimNotifyState_CameraAdjust.h"
#include "Animation/NamiAnimNotifyCameraAdjust.h"

// Configuration
#include "DevelopSetting/NamiCameraSettings.h"
