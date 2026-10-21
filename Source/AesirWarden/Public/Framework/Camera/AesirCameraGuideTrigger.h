// Copyright (c) 2026 YYchainsAw. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AesirCameraGuideTrigger.generated.h"

class AAesirPlayerCharacter;
class APawn;
class APlayerController;
class UArrowComponent;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class EAesirCameraGuideAimMode : uint8
{
	ActorTarget,
	FixedDirection
};

UENUM(BlueprintType)
enum class EAesirCameraGuideStage : uint8
{
	Inactive,
	Turning,
	Holding,
	Returning
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAesirCameraGuideEvent);

/** Place in a level, resize TriggerBox, and select the target to reveal with the player's own camera. */
UCLASS(Blueprintable)
class AESIRWARDEN_API AAesirCameraGuideTrigger : public AActor
{
	GENERATED_BODY()

public:
	AAesirCameraGuideTrigger();
	virtual void Tick(float DeltaSeconds) override;

	/** The same validation as entering the trigger; failed attempts do not consume Trigger Once. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Camera Guide")
	bool TryStartGuide(APawn* PlayerPawn);
	UFUNCTION(BlueprintCallable, Category = "Aesir|Camera Guide")
	void CancelGuide();
	/** Cancel the current guide and allow a Trigger Once instance to be used again. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Camera Guide")
	void ResetTrigger();
	UFUNCTION(BlueprintPure, Category = "Aesir|Camera Guide")
	bool IsGuiding() const { return Stage != EAesirCameraGuideStage::Inactive; }
	UFUNCTION(BlueprintPure, Category = "Aesir|Camera Guide")
	bool HasTriggered() const { return bHasTriggered; }
	UFUNCTION(BlueprintPure, Category = "Aesir|Camera Guide")
	EAesirCameraGuideStage GetStage() const { return Stage; }
	UFUNCTION(BlueprintPure, Category = "Aesir|Camera Guide")
	float GetProgress() const { return Progress; }
	UFUNCTION(BlueprintPure, Category = "Aesir|Camera Guide")
	FString GetLastFailure() const { return LastFailure; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Guide|Components")
	TObjectPtr<USceneComponent> GuideRoot;
	/** Resize this box in the level to choose where the reveal is triggered. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Guide|Components")
	TObjectPtr<UBoxComponent> TriggerBox;
	/** Fixed Direction mode uses this arrow's world rotation. It is hidden in game. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Guide|Components")
	TObjectPtr<UArrowComponent> DirectionArrow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Guide")
	bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Guide|Target")
	EAesirCameraGuideAimMode AimMode = EAesirCameraGuideAimMode::ActorTarget;
	/** A placed TargetPoint or other loaded actor. Soft reference avoids forcing its World Partition cell to load. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Camera Guide|Target",
		meta = (EditCondition = "AimMode == EAesirCameraGuideAimMode::ActorTarget"))
	TSoftObjectPtr<AActor> TargetActor;
	/** World-space offset from Target Actor's origin; useful for aiming above a mesh pivot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Guide|Target", meta = (Units = "cm",
		EditCondition = "AimMode == EAesirCameraGuideAimMode::ActorTarget"))
	FVector TargetOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Guide|Timing", meta = (ClampMin = "0.0", Units = "s"))
	float TurnDuration = 1.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Guide|Timing", meta = (ClampMin = "0.0", Units = "s"))
	float HoldDuration = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Guide|Timing")
	bool bReturnToOriginalView = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Guide|Timing", meta = (ClampMin = "0.0", Units = "s",
		EditCondition = "bReturnToOriginalView"))
	float ReturnDuration = 1.0f;
	/** A successful start consumes this instance even if the guide is later cancelled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Guide|Behavior")
	bool bTriggerOnce = true;
	/** Otherwise only mouse/right-stick look is locked; walking and gameplay input remain available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Guide|Behavior")
	bool bLockMovement = false;

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Camera Guide")
	FAesirCameraGuideEvent OnGuideStarted;
	UPROPERTY(BlueprintAssignable, Category = "Aesir|Camera Guide")
	FAesirCameraGuideEvent OnGuideFinished;
	UPROPERTY(BlueprintAssignable, Category = "Aesir|Camera Guide")
	FAesirCameraGuideEvent OnGuideCancelled;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	void TryStartForOverlappingPlayer();
	bool IsExplorationAllowed() const;
	bool GetDesiredRotation(const AAesirPlayerCharacter* Player, const APlayerController* Controller,
		FRotator& OutRotation) const;
	bool IsRuntimeValid() const;
	FRotator ClampCameraRotation(const FRotator& Rotation, const APlayerController* Controller) const;
	void CompleteGuide(bool bCancelled, const FString& Failure = FString());

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Camera Guide|Runtime", meta = (AllowPrivateAccess = "true"))
	EAesirCameraGuideStage Stage = EAesirCameraGuideStage::Inactive;
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Camera Guide|Runtime", meta = (AllowPrivateAccess = "true"))
	bool bHasTriggered = false;
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Camera Guide|Runtime", meta = (AllowPrivateAccess = "true"))
	float Progress = 0.0f;
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Camera Guide|Runtime", meta = (AllowPrivateAccess = "true"))
	FString LastFailure;

	TWeakObjectPtr<AAesirPlayerCharacter> GuidedPlayer;
	TWeakObjectPtr<APlayerController> GuidedController;
	FRotator OriginalRotation = FRotator::ZeroRotator;
	FRotator ReturnStartRotation = FRotator::ZeroRotator;
	float PhaseElapsed = 0.0f;
	float TotalElapsed = 0.0f;
	float TurnSeconds = 0.0f;
	float HoldSeconds = 0.0f;
	float ReturnSeconds = 0.0f;
	float LastPlayerHealth = 0.0f;
	bool bOwnsLookLock = false;
	bool bOwnsMoveLock = false;
	bool bShouldReturn = false;
	bool bEndingPlay = false;
};
