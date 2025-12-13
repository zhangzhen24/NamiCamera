# Nami Camera System

A powerful, modular camera system for Unreal Engine featuring mode stacking, feature-based extensions, and runtime adjustments.

## Features

- **Mode Stack System**: Stack multiple camera modes with automatic blending
- **Feature-Based Architecture**: Extend camera behavior through composable features
- **Runtime Adjustments**: Dynamic camera parameter modifications with blend control
- **Animation Integration**: Timeline-driven camera adjustments via Animation Notifies
- **Pipeline Processing**: Structured camera calculation pipeline
- **Debug Visualization**: Built-in debug drawing and logging

## Quick Start

### 1. Basic Setup

1. Add `UNamiCameraComponent` to your Pawn/Character
2. Set `ANamiPlayerCameraManager` as your PlayerCameraManagerClass
3. Configure a default camera mode (e.g., `UNamiThirdPersonCamera`)

**Example C++ Setup:**

```cpp
// In your PlayerController
AMyPlayerController::AMyPlayerController()
{
    PlayerCameraManagerClass = ANamiPlayerCameraManager::StaticClass();
}

// In your Character
AMyCharacter::AMyCharacter()
{
    // Create camera component
    CameraComponent = CreateDefaultSubobject<UNamiCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(RootComponent);

    // Set default camera mode
    CameraComponent->DefaultCameraMode = UNamiThirdPersonCamera::StaticClass();
}
```

### 2. Using Camera Modes

Camera modes define how the camera behaves. You can push and pop modes at runtime:

```cpp
// Push a new camera mode (e.g., aiming mode)
FNamiCameraModeHandle Handle = CameraComponent->PushCameraMode(UMyAimingMode::StaticClass());

// Pop the mode when done
CameraComponent->PopCameraMode(Handle);
```

### 3. Adding Features

Features extend camera modes with additional functionality:

```cpp
// Add a camera shake feature
UNamiCameraShakeFeature* ShakeFeature = NewObject<UNamiCameraShakeFeature>(CameraMode);
CameraMode->AddFeature(ShakeFeature);

// Or add globally to affect all modes
CameraComponent->AddGlobalFeature(ShakeFeature);
```

### 4. Runtime Adjustments

Adjustments allow you to modify camera parameters at runtime with automatic blending:

```cpp
// Push an adjustment (e.g., for cinematic effect)
UNamiCameraAdjust* Adjust = CameraComponent->PushCameraAdjust(UMyCinematicAdjust::StaticClass());

// It will blend in, stay active, and blend out based on your settings
// Remove manually if needed
CameraComponent->PopCameraAdjust(Adjust);
```

## Architecture

### System Layers

The camera system is organized in clear architectural layers:

```
┌─────────────────────────────────────────┐
│         Components (Entry Point)        │  ← NamiCameraComponent, PlayerCameraManager
├─────────────────────────────────────────┤
│         Mode Stack (Core Logic)         │  ← Mode calculation & blending
├─────────────────────────────────────────┤
│    Features (Modular Extensions)        │  ← Effects, shake, debug
├─────────────────────────────────────────┤
│   Adjustments (Runtime Modifications)   │  ← Dynamic parameter changes
└─────────────────────────────────────────┘
```

### Camera Pipeline

Each frame, the camera view is calculated through these stages:

1. **Pre-Process**: Initialize context, validate component
2. **Mode Stack**: Calculate and blend all active modes
3. **Global Features**: Apply global effects (shake, procedural effects)
4. **Adjustments**: Apply runtime parameter modifications
5. **Controller Sync**: Update PlayerController rotation
6. **Smoothing**: Blend from current to target view
7. **Post-Process**: Debug drawing, logging

## Folder Structure

```
Source/NamiCamera/
├── Core/                   # Foundation (module, enums, logging, math)
│   ├── Logging/           # Log categories and macros
│   └── Math/              # Camera-specific math utilities
├── Components/            # Camera components (entry point)
├── Modes/                 # Mode system
│   ├── Base/             # Base classes and infrastructure
│   ├── Data/             # Data structures (View, State, Context)
│   ├── Templates/        # Abstract templates for inheritance
│   └── Presets/          # Ready-to-use implementations
├── Features/              # Feature system
│   └── Effects/          # Effect features (shake, etc.)
└── Adjustments/           # Adjustment system
    ├── Base/             # Core adjustment classes
    ├── Animation/        # Animation notify integration
    └── Config/           # Configuration settings
```

## Documentation

- **[Quick Start](Documentation/QuickStart.md)** - Detailed getting started guide
- **[Architecture](Documentation/Architecture.md)** - System design and concepts
- **[API Reference](Documentation/API.md)** - Key classes and methods

## Examples

See the `Modes/Presets/` folder for production-ready camera mode examples:

- **NamiThirdPersonCamera** - Standard third-person camera with configurable offset
- **NamiShoulderCamera** - Over-the-shoulder camera for aiming
- **NamiSpringArmCameraMode** - Template with spring arm collision detection

## Creating Custom Content

### Custom Camera Mode

1. Inherit from `UNamiFollowCameraMode` (easiest) or `UNamiCameraModeBase`
2. Override `CalculatePivotLocation()` to define what the camera looks at
3. Configure blend settings in the class defaults

```cpp
UCLASS()
class UMyCustomMode : public UNamiFollowCameraMode
{
    GENERATED_BODY()

protected:
    virtual FVector CalculatePivotLocation_Implementation(float DeltaTime) override
    {
        // Return where the camera should focus
        return GetCameraComponent()->GetOwnerPawn()->GetActorLocation();
    }
};
```

### Custom Feature

1. Inherit from `UNamiCameraFeature`
2. Override `ApplyToViewWithContext()` to modify the camera view

```cpp
UCLASS()
class UMyCustomFeature : public UNamiCameraFeature
{
    GENERATED_BODY()

protected:
    virtual void ApplyToViewWithContext_Implementation(
        FNamiCameraView& InOutView,
        float DeltaTime,
        const FNamiCameraPipelineContext& Context) override
    {
        // Modify InOutView here
        InOutView.Location += FVector(0, 0, 100); // Example: offset up
    }
};
```

### Custom Adjustment

1. Inherit from `UNamiCameraAdjust`
2. Override `CalculateAdjustParams()` to provide parameter modifications

```cpp
UCLASS()
class UMyAdjustment : public UNamiCameraAdjust
{
    GENERATED_BODY()

protected:
    virtual FNamiCameraAdjustParams CalculateAdjustParams_Implementation(
        float DeltaTime,
        const FRotator& CurrentArmRotation,
        const FNamiCameraView& CurrentView) override
    {
        FNamiCameraAdjustParams Params;
        Params.ArmRotationOffset = FRotator(0, 45, 0); // Rotate 45 degrees
        Params.BlendMode = ENamiCameraAdjustBlendMode::Additive;
        return Params;
    }
};
```

## Key Concepts

### Mode Stack

- Multiple modes can be active simultaneously
- Each mode has a blend weight (0-1) based on its blend configuration
- Final view is a weighted average of all active modes
- Modes can be pushed/popped at any time

### Features

- Modular extensions that modify camera views
- Can be attached to specific modes or globally to the component
- Processed in priority order
- Support lifecycle (Initialize, Activate, Deactivate, Tick)

### Adjustments

- Provide runtime parameter modifications (rotation offset, arm length, etc.)
- Support smooth blend in/out with configurable durations
- Two blend modes: **Additive** (offset) and **Override** (target)
- Can be driven by curves (speed-based, time-based, custom parameters)
- Animation notify integration for timeline control

### State Separation

Clear separation between input and output:
- **Input State** (`FNamiCameraState`): What we're looking at (targets, velocities, player input)
- **Output View** (`FNamiCameraView`): Where the camera is (location, rotation, FOV)
- **Pipeline Context**: Data passed between pipeline stages

## Performance Notes

- The system is designed for game thread only (no async/parallel processing)
- All processing happens during `GetCameraView()` each frame
- Feature and adjustment lookups are optimized with internal maps
- Lightweight mode switching with object pooling

## Version

- **Version**: 1.0
- **Author**: 秋 (Qiu)
- **Engine Compatibility**: Unreal Engine 5.x

## License

Copyright Epic Games, Inc. All Rights Reserved.
