// Copyright (c) 2026 YYchainsAw. All rights reserved. Third-party assets belong to their respective owners.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Framework/Flow/AesirLevelFlowTypes.h"
#include "TimerManager.h"
#include "AesirLevelFlowDirector.generated.h"

class ACameraActor;
class ALevelSequenceActor;
class APawn;
class APlayerController;
class ULevelSequencePlayer;
class UUserWidget;

/** One placed actor owns the first-level menu, opening and gameplay handoff. */
UCLASS(Blueprintable)
class AESIRWARDEN_API AAesirLevelFlowDirector : public AActor
{
	GENERATED_BODY()

public:
	AAesirLevelFlowDirector();

	/** Enter the startup menu once initialization has completed. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Level Flow")
	bool EnterMainMenu();

	/** Called by the menu's Start button. Repeated requests are rejected. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Level Flow")
	bool RequestStartGame();

	/** Complete the opening and restore gameplay control. Safe against duplicate callbacks. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Level Flow")
	bool EnterExploration();

	/** Evaluate the sequence's end pose before using the same camera handoff as natural completion. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Level Flow")
	bool SkipOpeningCinematic();

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Level Flow")
	FAesirLevelFlowStateChanged OnFlowStateChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Bind the menu's Blueprint dispatcher here, before the widget is added to the viewport. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Aesir|Level Flow")
	void OnMainMenuCreated(UUserWidget* MenuWidget);

	/** Use this for authored presentation, e.g. hiding/showing the existing combat HUD. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Aesir|Level Flow")
	void OnPhaseChanged(EAesirLevelFlowState PreviousState, EAesirLevelFlowState NewState);

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Aesir|Level Flow|Scene")
	TObjectPtr<ACameraActor> MenuCamera;

	/** Optional placed Sequence actor. Keep its Auto Play disabled in the level. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Aesir|Level Flow|Scene")
	TObjectPtr<ALevelSequenceActor> OpeningSequenceActor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aesir|Level Flow|UI")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aesir|Level Flow|UI")
	int32 MainMenuZOrder = 100;

	/** Camera blend duration after the opening, or directly from the menu when no sequence is set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aesir|Level Flow|Camera", meta = (ClampMin = "0.0", Units = "s"))
	float OpeningBlendTime = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|Level Flow|Initialization", meta = (ClampMin = "0.1", Units = "s"))
	float InitializationTimeout = 10.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Aesir|Level Flow|Runtime")
	EAesirLevelFlowState CurrentState = EAesirLevelFlowState::Uninitialized;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Aesir|Level Flow|Runtime")
	TObjectPtr<APlayerController> PlayerControllerRef;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Aesir|Level Flow|Runtime")
	TObjectPtr<APawn> PlayerPawnRef;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Aesir|Level Flow|Runtime")
	TObjectPtr<UUserWidget> MainMenuRef;

private:
	void TryInitializeFlow();
	bool RefreshPlayerReferences();
	void SetFlowState(EAesirLevelFlowState NewState);
	void AcquireInputLocks();
	void ReleaseInputLocks();
	void BeginGameplayCameraHandoff();
	void CompleteGameplayCameraHandoff();
	void UnbindOpeningSequence();

	UFUNCTION()
	void HandleOpeningSequenceFinished();

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> ActiveOpeningPlayer;

	// Release locks on the same controller that received them, even if possession changes.
	TWeakObjectPtr<APlayerController> InputLockController;
	FTimerHandle InitializationTimerHandle;
	FTimerHandle CameraHandoffTimerHandle;
	double InitializationStartedAt = 0.0;
	bool bInitialized = false;
	bool bOwnsLevelFlow = false;
	bool bOwnsInputLocks = false;
	bool bCameraHandoffStarted = false;
};
