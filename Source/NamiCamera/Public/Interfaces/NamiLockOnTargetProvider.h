// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NamiLockOnTargetProvider.generated.h"

UINTERFACE(MinimalAPI, Blueprintable, meta = (CannotImplementInterfaceInBlueprint))
class UNamiLockOnTargetProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * Lock-on target provider interface
 *
 * Decouples lock-on camera mode from specific lock-on component implementations.
 * Project-layer lock-on components should implement this interface.
 *
 * Usage:
 * 1. Implement this interface in your lock-on component
 * 2. Pass the component to NamiLockOnCameraMode via SetLockOnProvider()
 * 3. Camera mode will query target info through interface methods
 */
class NAMICAMERA_API INamiLockOnTargetProvider
{
	GENERATED_BODY()

public:
	/**
	 * Check if there is a valid locked target
	 * @return true if a target is currently locked
	 */
	virtual bool HasLockedTarget() const = 0;

	/**
	 * Get the locked target's position
	 * This is where the target physically is
	 * @return Target location in world space
	 */
	virtual FVector GetLockedLocation() const = 0;

	/**
	 * Get the locked target's focus position
	 * This is where the camera should look at (may differ from physical location)
	 * @return Focus location in world space
	 */
	virtual FVector GetLockedFocusLocation() const = 0;

	/**
	 * Get the locked target actor (optional)
	 * @return The locked target actor, or nullptr if not available
	 */
	virtual AActor* GetLockedTargetActor() const { return nullptr; }

	/**
	 * Get distance to the locked target (optional, for optimization)
	 * @return Distance to target, or 0 if not calculated
	 */
	virtual float GetDistanceToTarget() const { return 0.f; }
};
