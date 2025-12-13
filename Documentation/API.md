# API Reference

Quick reference for the most commonly used classes and methods in the Nami Camera System.

## Core Classes

### UNamiCameraComponent

Main camera component that manages modes, features, and adjustments.

**Key Methods:**

```cpp
// Mode Management
FNamiCameraModeHandle PushCameraMode(TSubclassOf<UNamiCameraModeBase> ModeClass, int32 Priority = 0);
bool PopCameraMode(FNamiCameraModeHandle& ModeHandle);
UNamiCameraModeBase* GetActiveCameraMode() const;

// Global Feature Management
void AddGlobalFeature(UNamiCameraFeature* Feature);
bool RemoveGlobalFeature(UNamiCameraFeature* Feature);
UNamiCameraFeature* FindGlobalFeatureByName(FName FeatureName) const;

// Adjustment Management
UNamiCameraAdjust* PushCameraAdjust(TSubclassOf<UNamiCameraAdjust> AdjustClass);
bool PopCameraAdjust(UNamiCameraAdjust* AdjustInstance, bool bForceImmediate = false);

// Debug
void DumpCameraModeStack(bool bPrintToScreen = true, bool bPrintToLog = true);
```

### UNamiCameraModeBase

Base class for all camera modes.

**Key Methods:**

```cpp
// Lifecycle (override in subclasses)
virtual void Initialize(UNamiCameraComponent* InCameraComponent);
virtual void Activate();
virtual void Deactivate();
virtual void Tick(float DeltaTime);

// Core calculation (override in subclasses)
virtual FNamiCameraView CalculateView(float DeltaTime);
virtual FVector CalculatePivotLocation(float DeltaTime);

// Feature Management
void AddFeature(UNamiCameraFeature* Feature);
void RemoveFeature(UNamiCameraFeature* Feature);
template<typename T> T* GetFeature() const;

// Properties
float GetBlendWeight() const;
bool IsActive() const;
```

### UNamiFollowCameraMode

Template for camera modes that follow a target.

**Key Properties:**

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite)
FNamiFollowTarget FollowTarget;  // Configure what/how to follow

UPROPERTY(EditAnywhere, BlueprintReadWrite)
float ArmLength = 300.0f;  // Distance from pivot to camera

UPROPERTY(EditAnywhere, BlueprintReadWrite)
FRotator ArmRotation = FRotator(-20, 0, 0);  // Camera angle
```

**Key Methods:**

```cpp
// Override this to define what the camera follows
virtual FVector CalculatePivotLocation_Implementation(float DeltaTime) override;
```

## Features

### UNamiCameraFeature

Base class for all camera features.

**Key Methods:**

```cpp
// Lifecycle
virtual void Initialize(UNamiCameraModeBase* InOwningMode);
virtual void Activate();
virtual void Deactivate();
virtual void Tick(float DeltaTime);

// View modification
virtual void ApplyToViewWithContext(
    FNamiCameraView& InOutView,
    float DeltaTime,
    const FNamiCameraPipelineContext& Context);
```

### UNamiCameraShakeFeature

Feature that triggers camera shakes.

**Key Properties:**

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite)
TSubclassOf<UCameraShakeBase> ShakeClass;  // The shake to play

UPROPERTY(EditAnywhere, BlueprintReadWrite)
float ShakeScale = 1.0f;  // Intensity multiplier
```

**Key Methods:**

```cpp
void PlayShake();
void StopShake(bool bImmediately = false);
```

## Adjustments

### UNamiCameraAdjust

Base class for camera parameter adjustments.

**Key Methods:**

```cpp
// Override this to provide adjustment parameters
virtual FNamiCameraAdjustParams CalculateAdjustParams(
    float DeltaTime,
    const FRotator& CurrentArmRotation,
    const FNamiCameraView& CurrentView);
```

**Key Properties:**

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite)
float BlendInTime = 0.5f;  // Time to blend in

UPROPERTY(EditAnywhere, BlueprintReadWrite)
float BlendOutTime = 0.5f;  // Time to blend out

UPROPERTY(EditAnywhere, BlueprintReadWrite)
bool bEnableInputInterruption = true;  // Cancel on player input
```

## Data Structures

### FNamiCameraView

The final calculated camera view.

```cpp
struct FNamiCameraView
{
    FVector Location;           // Camera position in world
    FRotator Rotation;          // Camera orientation
    FVector ControlLocation;    // PlayerController location
    FRotator ControlRotation;   // PlayerController rotation
    float FieldOfView;          // FOV in degrees
    float ArmLength;            // Distance from pivot
};
```

### FNamiCameraState

Input state for camera calculation.

```cpp
struct FNamiCameraState
{
    FVector PivotLocation;      // What we're looking at
    FRotator PivotRotation;     // Direction of the target
    FVector TargetVelocity;     // Movement velocity
    // ... and more
};
```

### FNamiBlendConfig

Configuration for blend behavior.

```cpp
struct FNamiBlendConfig
{
    float BlendTime = 0.5f;
    ENamiCameraBlendFunction BlendFunction = EaseInOut;
    UCurveFloat* CustomCurve = nullptr;
};
```

### FNamiCameraAdjustParams

Parameters for camera adjustments.

```cpp
struct FNamiCameraAdjustParams
{
    FRotator ArmRotationOffset = FRotator::ZeroRotator;
    float ArmLengthOffset = 0.0f;
    float FOVOffset = 0.0f;
    FVector PivotLocationOffset = FVector::ZeroVector;

    // Override values (when BlendMode = Override)
    FRotator ArmRotationTarget = FRotator::ZeroRotator;
    float ArmLengthTarget = 300.0f;

    ENamiCameraAdjustBlendMode BlendMode = Additive;
};
```

## Enums

### ENamiCameraBlendFunction

```cpp
enum class ENamiCameraBlendFunction : uint8
{
    Linear,         // Constant speed
    EaseIn,         // Slow start, fast end
    EaseOut,        // Fast start, slow end
    EaseInOut,      // Slow start and end
    Custom          // Use custom curve
};
```

### ENamiCameraAdjustBlendMode

```cpp
enum class ENamiCameraAdjustBlendMode : uint8
{
    Additive,       // Add offset to current value
    Override        // Set to target value
};
```

## Common Patterns

### Pattern: Temporary Camera Mode

```cpp
// Push mode
FNamiCameraModeHandle Handle = CameraComp->PushCameraMode(UMyMode::StaticClass());

// Use it...

// Pop when done
CameraComp->PopCameraMode(Handle);
```

### Pattern: One-Shot Effect Feature

```cpp
UMyEffectFeature* Effect = NewObject<UMyEffectFeature>(CameraMode);
Effect->Duration = 2.0f;  // Auto-remove after 2 seconds
CameraMode->AddFeature(Effect);
```

### Pattern: Animation-Driven Adjustment

1. Create adjustment class:
```cpp
UCLASS()
class UMyCinematicAdjust : public UNamiCameraAdjust
{
    GENERATED_BODY()

    virtual FNamiCameraAdjustParams CalculateAdjustParams_Implementation(...) override
    {
        FNamiCameraAdjustParams Params;
        Params.ArmRotationOffset = FRotator(MyPitch, MyYaw, 0);
        return Params;
    }
};
```

2. Use in Animation Notify State:
   - Add `AnimNotifyState_CameraAdjust` to animation
   - Set `AdjustClass` to `UMyCinematicAdjust`
   - Configure blend times

### Pattern: Context-Aware Mode

```cpp
FVector UMyMode::CalculatePivotLocation_Implementation(float DeltaTime)
{
    APawn* Pawn = GetCameraComponent()->GetOwnerPawn();

    // Different pivot based on context
    if (Pawn->IsInCover())
    {
        return Pawn->GetCoverPeekLocation();
    }
    else
    {
        return Pawn->GetActorLocation();
    }
}
```

## Best Practices

1. **Use Templates**: Start with `UNamiFollowCameraMode` instead of `UNamiCameraModeBase`
2. **Features for Extensions**: Add behavior via features rather than modifying modes
3. **Tune Blend Times**: Smooth transitions are key to good camera feel
4. **Debug Often**: Use `DumpCameraModeStack()` to understand state
5. **Pool Modes**: Modes are automatically pooled, don't worry about creation cost
6. **Input Interruption**: Enable for adjustments that shouldn't override player control
7. **Global vs Mode Features**: Use global features for always-active effects
8. **Adjustment Blend Modes**: Use Additive for offsets, Override for targets

## Troubleshooting Reference

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| Camera not updating | Missing `ANamiPlayerCameraManager` | Set in PlayerController |
| Modes not blending | Blend times set to 0 | Configure `BlendConfig.BlendTime` |
| Jumpy camera | Conflicting adjustments | Check active adjustments with logs |
| Feature not working | Not activated | Verify `Initialize()` and `Activate()` called |
| Adjustment stuck | Input interruption disabled | Enable or manually pop adjustment |

## Further Reading

- [Quick Start](QuickStart.md) - Get up and running quickly
- [Architecture](Architecture.md) - Understand the system design
- [Source Code](../Source/NamiCamera/Public/) - Browse the implementation

For more details, see the inline documentation in the header files!
