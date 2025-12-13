# Architecture Overview

This document describes the design philosophy, system architecture, and key concepts of the Nami Camera System.

## Design Philosophy

NamiCamera is built on three core principles:

### 1. Composition over Inheritance
Extend behavior through features and adjustments rather than deep inheritance hierarchies. This makes the system more flexible and maintainable.

### 2. Stackable Systems
Multiple modes, features, and adjustments can coexist and blend together, allowing for rich, layered camera behavior.

### 3. Pipeline Processing
Camera view calculation follows a clear, debuggable pipeline with well-defined stages.

## System Layers

The camera system is organized into four architectural layers:

```
┌──────────────────────────────────────────────┐
│  1. Core Layer                               │  Foundation types and utilities
│     - Module, Enums, Tags                    │
│     - Logging, Math                          │
└──────────────────────────────────────────────┘
                    ↓
┌──────────────────────────────────────────────┐
│  2. Component Layer                          │  Entry points to the system
│     - NamiCameraComponent                    │
│     - NamiPlayerCameraManager                │
│     - NamiSpringArm                          │
└──────────────────────────────────────────────┘
                    ↓
┌──────────────────────────────────────────────┐
│  3. Mode Layer                               │  Core camera calculation logic
│     - Base classes (ModeBase, Stack)         │
│     - Data structures (View, State)          │
│     - Templates (FollowCameraMode)           │
│     - Presets (ThirdPerson, Shoulder)        │
└──────────────────────────────────────────────┘
                    ↓
┌──────────────────────────────────────────────┐
│  4. Features & Adjustments Layer            │  Extensions and modifications
│     - Features (shake, effects, debug)       │
│     - Adjustments (runtime modifications)    │
└──────────────────────────────────────────────┘
```

### Layer 1: Core

**Purpose**: Foundation types and utilities used throughout the system.

**Key Components:**
- `NamiCameraModule` - Plugin initialization and lifecycle management
- `NamiCameraEnums` - Shared enumeration types (blend modes, states)
- `NamiCameraTags` - Gameplay tag definitions for camera classification
- `LogNamiCamera` - Logging categories and macros
- `NamiCameraMath` - Camera-specific math utilities (rotation, interpolation)

### Layer 2: Components

**Purpose**: Entry points that connect the camera system to Unreal Engine.

**Key Components:**
- `UNamiCameraComponent` - Main camera component that manages modes, features, and the pipeline
- `ANamiPlayerCameraManager` - Integration with Unreal's PlayerCameraManager system
- `UNamiSpringArm` - Utility for spring arm-based camera collision

### Layer 3: Modes

**Purpose**: Core camera calculation logic.

**Structure:**
- **Base/**: Abstract base classes and infrastructure
  - `UNamiCameraModeBase` - Root base class for all modes
  - `FNamiCameraModeStack` - Manages mode blending
  - `FNamiCameraModeHandle` - Handle to active modes

- **Data/**: Data structures passed through the pipeline
  - `FNamiCameraView` - Output view (location, rotation, FOV)
  - `FNamiCameraState` - Input state (targets, velocities, input)
  - `FNamiCameraPipelineContext` - Pipeline processing context
  - `FNamiBlendConfig` - Blend configuration (time, function, curve)

- **Templates/**: Abstract templates for inheritance
  - `UNamiFollowCameraMode` - Follow a target with offset
  - `FNamiFollowTarget` - Target following parameters
  - `UNamiSpringArmCameraMode` - Adds collision detection

- **Presets/**: Concrete ready-to-use implementations
  - `UNamiThirdPersonCamera` - Standard third-person camera
  - `UNamiShoulderCamera` - Over-shoulder camera

### Layer 4: Features & Adjustments

**Purpose**: Modular extensions and runtime modifications.

**Features:**
- `UNamiCameraFeature` - Base class for all features
- `UNamiCameraEffectFeature` - Feature with blend support (duration, interruption)
- `UNamiCameraShakeFeature` - Camera shake integration
- `UNamiCameraDebugInfo` - Debug visualization

**Adjustments:**
- `UNamiCameraAdjust` - Base class for stackable parameter modifiers
- `FNamiCameraAdjustParams` - Parameter structure (rotation offset, arm length, etc.)
- `AnimNotifyState_CameraAdjust` - Animation timeline integration

## Camera Pipeline

The camera view is calculated each frame through a series of pipeline stages:

```
Frame Start
     │
     v
┌────────────────────────────────────────┐
│ 0. Pre-Process                         │
│    - Validate component                │
│    - Initialize FNamiCameraPipeline    │
│      Context                           │
└────────────────────────────────────────┘
     │
     v
┌────────────────────────────────────────┐
│ 1. Mode Stack Processing               │
│    - Update all active modes           │
│    - Calculate individual mode views   │
│    - Blend based on weights            │
│    - Output: BaseView                  │
└────────────────────────────────────────┘
     │
     v
┌────────────────────────────────────────┐
│ 2. Global Features                     │
│    - Apply global effects              │
│    - Camera shake, procedural effects  │
│    - Output: EffectView                │
└────────────────────────────────────────┘
     │
     v
┌────────────────────────────────────────┐
│ 3. Camera Adjustments                  │
│    - Calculate adjustment parameters   │
│    - Apply (additive or override)      │
│    - Handle input interruption         │
│    - Output: AdjustedView              │
└────────────────────────────────────────┘
     │
     v
┌────────────────────────────────────────┐
│ 4. Controller Sync                     │
│    - Sync ControlRotation to          │
│      PlayerController                  │
└────────────────────────────────────────┘
     │
     v
┌────────────────────────────────────────┐
│ 5. Smoothing                           │
│    - Blend from current to target      │
│    - Apply blend speeds                │
│    - Output: SmoothedView              │
└────────────────────────────────────────┘
     │
     v
┌────────────────────────────────────────┐
│ 6. Post-Process                        │
│    - Debug drawing                     │
│    - Logging                           │
│    - Output: Final POV                 │
└────────────────────────────────────────┘
     │
     v
  To Engine
```

### Pipeline Details

**Stage 0: Pre-Process**
- Validates component state
- Initializes `FNamiCameraPipelineContext` with owner pawn, controller, delta time

**Stage 1: Mode Stack**
- Calls `Tick()` on all active modes
- Each mode calculates its view via `CalculateView()`
- Modes are blended based on their weights
- Weight calculation uses `FAlphaBlend` with configurable blend functions

**Stage 2: Global Features**
- Features attached to the component (not a specific mode) are processed
- Each feature can modify the view via `ApplyToViewWithContext()`
- Processed in priority order

**Stage 3: Adjustments**
- All active adjustments calculate their parameters via `CalculateAdjustParams()`
- Parameters are combined (additive offsets are summed, overrides use the last one)
- Input interruption detection: if player provides camera input, adjustments can be canceled
- Applied to the view (rotation offsets, arm length changes, etc.)

**Stage 4: Controller Sync**
- The view's `ControlRotation` is synced to the `PlayerController`
- This ensures the player's aim direction matches the camera

**Stage 5: Smoothing**
- Smoothly interpolates from the current actual view to the target view
- Uses configurable blend speeds for location, rotation, and FOV
- Prevents jarring camera jumps

**Stage 6: Post-Process**
- Debug visualization (draw pivot location, camera location, lines)
- Logging of camera state
- Final view is returned to the engine

## Key Concepts

### Mode Stack

**What is it?**
A stack of camera modes that can be pushed and popped at runtime. Multiple modes can be active simultaneously, and they are blended together based on their weights.

**How it works:**
1. Each mode has a `BlendWeight` (0 to 1)
2. Weight is calculated by `FAlphaBlend` using the mode's `BlendConfig`
3. When a mode is pushed, it blends in from 0 to 1
4. When popped, it blends out from 1 to 0
5. Final view = weighted average of all active modes

**Example:**
```
Stack:
  [ThirdPersonMode] weight: 0.7
  [AimingMode]      weight: 0.3

Final Location = (ThirdPersonView.Location * 0.7) + (AimingView.Location * 0.3)
```

### Features

**What are they?**
Modular components that extend camera behavior without modifying the mode classes.

**Types:**
- **Mode Features**: Attached to a specific camera mode
- **Global Features**: Attached to the component, affect all modes

**Lifecycle:**
1. `Initialize()` - Called once when created
2. `Activate()` - Called when the feature becomes active
3. `Tick()` - Called every frame while active
4. `ApplyToViewWithContext()` - Modify the camera view
5. `Deactivate()` - Called when the feature is removed

**Example Use Cases:**
- Camera shake during damage
- Procedural camera effects (breathing, head bob)
- Debug visualization

### Adjustments

**What are they?**
Temporary modifications to camera parameters with automatic blending.

**Key Properties:**
- **Blend Mode**: `Additive` (offset) or `Override` (target)
- **Blend In/Out Time**: How long to smoothly transition
- **Curve Binding**: Drive parameters from curves (speed, time, custom)
- **Input Interruption**: Automatically cancel when player takes control

**Example:**
```cpp
// Cinematic adjustment: rotate camera 45° to the right
FNamiCameraAdjustParams Params;
Params.ArmRotationOffset = FRotator(0, 45, 0);
Params.BlendMode = ENamiCameraAdjustBlendMode::Additive;
```

**Input Interruption:**
When enabled, if the player moves the mouse/stick to control the camera, the adjustment is automatically canceled and the `ControlRotation` is synced to prevent jumps.

### State Separation

The system maintains clear separation between input and output:

**Input State (`FNamiCameraState`):**
- Pivot location (where we're looking at)
- Pivot rotation
- Target velocity
- Player input
- Changed flags (optimization)

**Output View (`FNamiCameraView`):**
- Camera location (final position in world)
- Camera rotation (final orientation)
- Control location/rotation (for PlayerController sync)
- Field of view
- Arm length (distance from pivot to camera)

**Pipeline Context (`FNamiCameraPipelineContext`):**
- Owner pawn
- Player controller
- Delta time
- Frame number
- Passed between pipeline stages

This separation allows modes to focus on "what to look at" (state) while the system handles "where to place the camera" (view).

## Extension Points

### Creating Custom Modes

**Option 1: Inherit from Template**
```cpp
class UMyMode : public UNamiFollowCameraMode
{
    // Override CalculatePivotLocation() only
};
```

**Option 2: Inherit from Base**
```cpp
class UMyMode : public UNamiCameraModeBase
{
    // Override CalculateView() for full control
};
```

### Creating Custom Features

```cpp
class UMyFeature : public UNamiCameraFeature
{
    // Override ApplyToViewWithContext()
};
```

### Creating Custom Adjustments

```cpp
class UMyAdjust : public UNamiCameraAdjust
{
    // Override CalculateAdjustParams()
};
```

## Thread Safety

- **Single-threaded**: Designed for game thread only
- **No async processing**: All processing happens during `GetCameraView()`
- **Safe for Blueprints**: All public APIs are Blueprint-safe
- **Tick Dependencies**: Camera component ticks after pawn/controller

## Performance Considerations

- **Mode Pooling**: Modes are pooled and reused to avoid allocations
- **Feature Maps**: Lookups optimized with internal hash maps
- **Lazy Rebuilding**: Feature maps rebuilt only when dirty
- **Minimal Allocations**: Most data structures are stack-allocated
- **Early Outs**: Pipeline stages can skip processing when not needed

## Design Patterns Used

1. **Template Method**: Base classes define algorithm structure, subclasses fill in details
2. **Strategy**: Features and adjustments encapsulate algorithms
3. **Chain of Responsibility**: Pipeline stages process view in sequence
4. **Object Pool**: Mode instances are pooled for reuse
5. **Observer**: Delegates notify about mode push/pop events
6. **Facade**: `UNamiCameraComponent` provides simple interface to complex system

## Future Extensibility

The architecture supports future additions:

- **New Feature Types**: Add new subfolders under `Features/` (e.g., `Collision/`, `PostProcess/`)
- **New Adjustment Types**: Add curve types, input sources
- **New Mode Templates**: Create specialized templates for specific use cases
- **Additional Pipeline Stages**: Insert new stages in the pipeline

## Summary

The Nami Camera System provides a flexible, modular architecture for complex camera behavior in Unreal Engine games. By separating concerns into layers (Core → Components → Modes → Features/Adjustments) and using a clear pipeline structure, the system is both powerful and maintainable.

Key takeaways:
- **Modes**: Define core camera behavior
- **Features**: Add modular extensions
- **Adjustments**: Provide runtime modifications
- **Pipeline**: Structured, debuggable processing
- **Composition**: Build complex behavior from simple pieces
