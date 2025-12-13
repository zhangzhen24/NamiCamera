# Quick Start Guide

This guide will help you get started with the Nami Camera System quickly.

## Installation

1. Copy the `NamiCamera` plugin folder to your project's `Plugins/` directory
2. Regenerate your project files (right-click .uproject → Generate Visual Studio project files)
3. Build your project
4. Enable the plugin in Edit → Plugins → Search for "Nami Camera"

## Basic Setup

### Step 1: Configure Player Controller

Create or modify your PlayerController class to use the Nami Player Camera Manager:

**C++:**
```cpp
// MyPlayerController.h
UCLASS()
class AMyPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AMyPlayerController();
};

// MyPlayerController.cpp
AMyPlayerController::AMyPlayerController()
{
    // Set the Nami Player Camera Manager
    PlayerCameraManagerClass = ANamiPlayerCameraManager::StaticClass();
}
```

**Blueprint:**
1. Open your PlayerController Blueprint
2. In Class Defaults, set **Player Camera Manager Class** to `NamiPlayerCameraManager`

### Step 2: Add Camera Component to Pawn

Add the `UNamiCameraComponent` to your Character or Pawn:

**C++:**
```cpp
// MyCharacter.h
UCLASS()
class AMyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AMyCharacter();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UNamiCameraComponent* CameraComponent;
};

// MyCharacter.cpp
AMyCharacter::AMyCharacter()
{
    // Create camera component
    CameraComponent = CreateDefaultSubobject<UNamiCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(RootComponent);

    // Set default camera mode
    CameraComponent->DefaultCameraMode = UNamiThirdPersonCamera::StaticClass();
}
```

**Blueprint:**
1. Open your Character Blueprint
2. Add Component → Search for `NamiCameraComponent`
3. In the component details, set **Default Camera Mode** to `NamiThirdPersonCamera`

### Step 3: Test in Editor

1. Place your character in a level
2. Play in Editor (PIE)
3. You should now see the camera following your character using the third-person camera mode!

## Common Use Cases

### Switching Camera Modes

Push a temporary camera mode (like aiming mode):

```cpp
// C++
FNamiCameraModeHandle AimHandle = CameraComponent->PushCameraMode(UMyAimingMode::StaticClass());

// When done aiming
CameraComponent->PopCameraMode(AimHandle);
```

**Blueprint:** Use nodes **Push Camera Mode** and **Pop Camera Mode**

### Creating a Custom Camera Mode

1. Create a new C++ class inheriting from `UNamiFollowCameraMode`:

```cpp
// MyAimingMode.h
#pragma once

#include "Modes/Templates/NamiFollowCameraMode.h"
#include "MyAimingMode.generated.h"

UCLASS()
class UMyAimingMode : public UNamiFollowCameraMode
{
    GENERATED_BODY()

public:
    UMyAimingMode();

protected:
    virtual FVector CalculatePivotLocation_Implementation(float DeltaTime) override;
};

// MyAimingMode.cpp
#include "MyAimingMode.h"
#include "Components/NamiCameraComponent.h"

UMyAimingMode::UMyAimingMode()
{
    // Configure aiming camera settings
    FollowTarget.bUseTargetSocketOffset = true;
    FollowTarget.TargetSocket = "AimSocket";  // Socket on character mesh
    FollowTarget.LocalOffset = FVector(50, 80, 60);  // Over-the-shoulder offset

    DefaultFOV = 70.0f;  // Narrower FOV for aiming

    // Configure blend
    BlendConfig.BlendTime = 0.3f;
    BlendConfig.BlendFunction = ENamiCameraBlendFunction::EaseInOut;
}

FVector UMyAimingMode::CalculatePivotLocation_Implementation(float DeltaTime)
{
    // Use parent's follow target logic
    return Super::CalculatePivotLocation_Implementation(DeltaTime);
}
```

2. You can now use this mode: `CameraComponent->PushCameraMode(UMyAimingMode::StaticClass())`

### Adding Global Features

Add a feature that affects the camera regardless of which mode is active:

```cpp
// C++: Add a camera shake when taking damage
void AMyCharacter::OnTakeDamage()
{
    UNamiCameraShakeFeature* Shake = NewObject<UNamiCameraShakeFeature>(CameraComponent);
    Shake->ShakeClass = UMyDamageShake::StaticClass();
    CameraComponent->AddGlobalFeature(Shake);
}
```

### Using Camera Adjustments

Adjustments provide temporary modifications to camera parameters:

```cpp
// C++: Push an adjustment for a cinematic moment
UNamiCameraAdjust* CinematicAdjust = CameraComponent->PushCameraAdjust(UMyCinematicAdjust::StaticClass());

// The adjustment will automatically blend in, stay active, and blend out
// based on its configuration
```

### Animation-Driven Camera Adjustments

Use Animation Notifies to sync camera with animations:

1. Open your animation in the Animation Editor
2. Add a **Notify State** → `AnimNotifyState_CameraAdjust`
3. Configure the adjustment class and parameters
4. When the animation plays, the camera will automatically adjust!

## Debugging

### Enable Debug Visualization

Call `DumpCameraModeStack()` to see which modes are active:

```cpp
// C++
CameraComponent->DumpCameraModeStack(true, true);  // Print to screen and log
```

**Blueprint:** Use the **Dump Camera Mode Stack** node

### Console Commands

In the game console (~):
- `ShowDebug Camera` - Shows camera debug info

### Logging

Adjust logging verbosity in `DefaultEngine.ini`:
```ini
[Core.Log]
LogNamiCamera=VeryVerbose
```

## Next Steps

- **[Architecture Guide](Architecture.md)** - Understand the system design
- **[API Reference](API.md)** - Detailed API documentation
- **Example Modes** - Check `Source/NamiCamera/Public/Modes/Presets/` for working examples:
  - `NamiThirdPersonCamera.h` - Standard third-person camera
  - `NamiShoulderCamera.h` - Over-the-shoulder camera

## Troubleshooting

### Camera not working
- Verify `ANamiPlayerCameraManager` is set in your PlayerController
- Check that your character has a `UNamiCameraComponent`
- Ensure a default camera mode is set

### Compilation errors
- Regenerate project files
- Clean and rebuild
- Verify all include paths are correct

### Camera stuttering
- Check blend speeds in camera component
- Verify tick dependencies
- Look for competing camera systems

## Tips

1. **Start Simple**: Use the preset modes first before creating custom ones
2. **Use Features**: Extend behavior with features instead of modifying modes
3. **Blend Configuration**: Tune blend times and functions for smooth transitions
4. **Debug Early**: Use debug visualization to understand what's happening
5. **Reference Examples**: The preset modes are fully functional examples

Need more help? Check the full [Architecture documentation](Architecture.md)!
