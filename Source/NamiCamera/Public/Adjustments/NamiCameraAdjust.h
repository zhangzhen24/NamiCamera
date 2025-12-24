// Copyright Qiu, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NamiCameraAdjustParams.h"
#include "NamiCameraAdjustCurveBinding.h"
#include "NamiCameraAdjust.generated.h"

class UNamiCameraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCameraAdjustInputInterrupted);

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class NAMICAMERA_API UNamiCameraAdjust : public UObject
{
	GENERATED_BODY()

public:
	UNamiCameraAdjust();

	virtual void Initialize(UNamiCameraComponent* InOwnerComponent);

	UFUNCTION(BlueprintNativeEvent, Category = "Camera Adjust|Lifecycle")
	void OnActivate();

	UFUNCTION(BlueprintNativeEvent, Category = "Camera Adjust|Lifecycle")
	void Tick(float DeltaTime);

	UFUNCTION(BlueprintNativeEvent, Category = "Camera Adjust|Lifecycle")
	void OnDeactivate();

	UFUNCTION(BlueprintNativeEvent, Category = "Camera Adjust")
	FNamiCameraAdjustParams CalculateAdjustParams(float DeltaTime);

	FNamiCameraAdjustParams GetWeightedAdjustParams(float DeltaTime);

	void RequestDeactivate(bool bForceImmediate = false);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending", meta = (ClampMin = "0.0"))
	float BlendInTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending", meta = (ClampMin = "0.0"))
	float BlendOutTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending")
	ENamiCameraBlendType BlendType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending",
		meta = (EditCondition = "BlendType == ENamiCameraBlendType::CustomCurve"))
	UCurveFloat* BlendCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blending")
	ENamiCameraAdjustBlendMode BlendMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override Mode",
		meta = (EditCondition = "BlendMode == ENamiCameraAdjustBlendMode::Override"))
	FRotator ArmRotationTarget = FRotator::ZeroRotator;

	UFUNCTION(BlueprintPure, Category = "Camera Adjust|Override")
	FRotator GetCachedWorldArmRotationTarget() const { return CachedWorldArmRotationTarget; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Control")
	bool bAllowPlayerInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input Control",
		meta = (EditCondition = "!bAllowPlayerInput", ClampMin = "0.1"))
	float InputInterruptThreshold = 1.0f;

	UPROPERTY(BlueprintAssignable, Category = "Camera Adjust|Events")
	FOnCameraAdjustInputInterrupted OnInputInterrupted;

	void TriggerInputInterrupt();

	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	bool IsInputInterrupted() const { return bInputInterrupted; }

	bool IsBlendOutSynced() const { return bBlendOutSynced; }

	void MarkBlendOutSynced() { bBlendOutSynced = true; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Priority")
	int32 Priority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve Driven")
	FNamiCameraAdjustCurveConfig CurveConfig;

	UFUNCTION(BlueprintCallable, Category = "Camera Adjust|Curve")
	void SetCustomInput(float Value);

	UFUNCTION(BlueprintPure, Category = "Camera Adjust|Curve")
	float GetCustomInput() const { return CustomInputValue; }

	UFUNCTION(BlueprintCallable, Category = "Camera Adjust")
	void SetStaticParams(const FNamiCameraAdjustParams& InParams);

	UFUNCTION(BlueprintPure, Category = "Camera Adjust")
	const FNamiCameraAdjustParams& GetStaticParams() const { return StaticParams; }

	UFUNCTION(BlueprintPure, Category = "Camera Adjust")
	bool IsUsingStaticParams() const { return bUseStaticParams; }

	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	float GetCurrentBlendWeight() const { return CurrentBlendWeight; }

	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	ENamiCameraAdjustState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	bool IsActive() const;

	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	bool IsBlendingIn() const { return State == ENamiCameraAdjustState::BlendingIn; }

	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	bool IsBlendingOut() const { return State == ENamiCameraAdjustState::BlendingOut; }

	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	bool IsFullyActive() const { return State == ENamiCameraAdjustState::Active; }

	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	bool IsFullyInactive() const { return State == ENamiCameraAdjustState::Inactive; }

	UFUNCTION(BlueprintPure, Category = "Camera Adjust|State")
	float GetActiveTime() const { return ActiveTime; }

	UFUNCTION(BlueprintPure, Category = "Camera Adjust")
	UNamiCameraComponent* GetOwnerComponent() const;

protected:
	float CurrentBlendWeight;
	float BlendTimer;
	ENamiCameraAdjustState State;
	float ActiveTime;
	float CustomInputValue;
	bool bInputInterrupted;
	bool bBlendOutSynced = false;
	bool bUseStaticParams = false;
	FNamiCameraAdjustParams StaticParams;

	UPROPERTY()
	TWeakObjectPtr<UNamiCameraComponent> OwnerComponent;

	FRotator CachedWorldArmRotationTarget;

	void CacheArmRotationTarget();
	void UpdateBlending(float DeltaTime);
	float CalculateBlendAlpha(float LinearAlpha) const;
	float GetInputSourceValue(ENamiCameraAdjustInputSource Source) const;
	float EvaluateCurveBinding(const FNamiCameraAdjustCurveBinding& Binding) const;
	void ApplyCurveDrivenParams(FNamiCameraAdjustParams& OutParams) const;

	virtual void OnActivate_Implementation();
	virtual void Tick_Implementation(float DeltaTime);
	virtual void OnDeactivate_Implementation();
	virtual FNamiCameraAdjustParams CalculateAdjustParams_Implementation(float DeltaTime);
};
