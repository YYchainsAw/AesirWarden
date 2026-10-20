// Copyright (c) 2026 YYchainsAw. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraTypes.h"
#include "Components/ActorComponent.h"
#include "AesirOpeningCameraComponent.generated.h"

class ACameraActor;
class ACharacter;
class APlayerController;
class UAnimInstance;
class UAnimMontage;
class UCurveFloat;
struct FCollisionQueryParams;

UENUM(BlueprintType)
enum class EAesirOpeningCameraOrbitDirection : uint8
{
	Auto,
	Left,
	Right
};

/** Animation-driven camera route around the player; never moves through the protected player volume. */
UCLASS(ClassGroup = (Aesir), meta = (BlueprintSpawnableComponent))
class AESIRWARDEN_API UAesirOpeningCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAesirOpeningCameraComponent();

	/** Preflight both endpoints and the route before taking over the current view. */
	bool BeginTransition(APlayerController* Controller, ACharacter* Character, UAnimInstance* AnimInstance, UAnimMontage* Montage);
	/** Natural montage completion requests the final pose; an interruption is a failure, not a skip. */
	void NotifyAnimationFinished(bool bInterrupted);
	/** Skip safely evaluates every remaining route segment before handing over. */
	void RequestFinishTransition();
	/** Call after establishing another view target when bRestorePlayerView is false. */
	void CancelTransition(bool bRestorePlayerView = false);

	FSimpleMulticastDelegate OnTransitionFinished;
	FSimpleMulticastDelegate OnTransitionFailed;

	UFUNCTION(BlueprintPure, Category = "Aesir|Opening Camera")
	bool IsTransitionActive() const { return bActive; }
	UFUNCTION(BlueprintPure, Category = "Aesir|Opening Camera")
	bool WasTransitionCompleted() const { return bCompleted; }
	UFUNCTION(BlueprintPure, Category = "Aesir|Opening Camera")
	float GetProgress() const { return Progress; }
	/** Clearance from the camera probe's outside to the conservative player envelope, in cm. */
	UFUNCTION(BlueprintPure, Category = "Aesir|Opening Camera")
	float GetMinimumObservedClearance() const { return MinimumObservedClearance; }
	UFUNCTION(BlueprintPure, Category = "Aesir|Opening Camera")
	float GetEndPositionError() const { return EndPositionError; }
	UFUNCTION(BlueprintPure, Category = "Aesir|Opening Camera")
	float GetEndRotationError() const { return EndRotationError; }
	UFUNCTION(BlueprintPure, Category = "Aesir|Opening Camera")
	float GetEndFOVError() const { return EndFOVError; }
	UFUNCTION(BlueprintPure, Category = "Aesir|Opening Camera")
	FString GetLastFailure() const { return LastFailure; }
	ACameraActor* GetTransitionCamera() const { return TransitionCamera.Get(); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Opening Camera")
	bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Opening Camera", meta = (ClampMin = "100.0", Units = "cm"))
	float MinPlayerClearance = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Opening Camera", meta = (ClampMin = "5.0", ClampMax = "50.0", Units = "cm"))
	float CameraProbeRadius = 12.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Opening Camera")
	EAesirOpeningCameraOrbitDirection OrbitDirection = EAesirOpeningCameraOrbitDirection::Auto;
	/** Preferred extra height at the middle of the route; endpoints remain unchanged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Opening Camera", meta = (ClampMin = "0.0", Units = "cm"))
	float ArcHeight = 60.0f;
	/** Optional monotonic 0..1 curve. Endpoints and forward-only progress are enforced. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Opening Camera")
	TObjectPtr<UCurveFloat> EaseCurve;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool ReadPlayerView(FMinimalViewInfo& OutView) const;
	void UpdatePlayerEnvelope();
	FVector EvaluateLocation(float RouteAlpha) const;
	FMinimalViewInfo EvaluateView(float RouteAlpha) const;
	bool IsPointSafe(const FVector& Location, FString* OutReason = nullptr) const;
	bool IsSegmentSafe(const FVector& From, const FVector& To, FString* OutReason = nullptr) const;
	bool ValidateRoute(float FromAlpha, float ToAlpha, bool bObserve, FString* OutReason = nullptr);
	FCollisionQueryParams MakeQueryParams() const;
	float GetClearance(const FVector& Location) const;
	float GetAnimationProgress();
	float Ease(float InProgress) const;
	void ApplyView(const FMinimalViewInfo& View);
	void Fail(const FString& Reason);
	void DestroyTransitionCamera();

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> TransitionCamera;
	TWeakObjectPtr<APlayerController> PlayerController;
	TWeakObjectPtr<ACharacter> PlayerCharacter;
	TWeakObjectPtr<UAnimInstance> PlayerAnimInstance;
	TWeakObjectPtr<UAnimMontage> StandUpMontage;
	FMinimalViewInfo StartView;
	FMinimalViewInfo EndView;
	FMinimalViewInfo LastView;
	FVector EnvelopeCenter = FVector::ZeroVector;
	float EnvelopeRadius = 0.0f;
	float StartRadius = 0.0f;
	float StartYaw = 0.0f;
	float InitialEndYaw = 0.0f;
	float SweepAngle = 0.0f;
	float SelectedArcHeight = 0.0f;
	float StartZOffset = 0.0f;
	float Progress = 0.0f;
	float RouteAlpha = 0.0f;
	float BlendOutStartProgress = -1.0f;
	float MinimumObservedClearance = TNumericLimits<float>::Max();
	float EndPositionError = 0.0f;
	float EndRotationError = 0.0f;
	float EndFOVError = 0.0f;
	FString LastFailure;
	bool bActive = false;
	bool bCompleted = false;
	bool bAnimationFinished = false;
	bool bSkipRequested = false;
};
