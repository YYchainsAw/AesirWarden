// Copyright (c) 2026 YYchainsAw. All rights reserved. Third-party assets belong to their respective owners.


#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "AesirWarden.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

AAesirLevelFlowDirector::AAesirLevelFlowDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AAesirLevelFlowDirector::BeginPlay()
{
	Super::BeginPlay();

	// The local single-player level must have one camera/input owner.
	for (TActorIterator<AAesirLevelFlowDirector> It(GetWorld()); It; ++It)
	{
		if (*It != this && It->bOwnsLevelFlow)
		{
			UE_LOG(LogAesirWarden, Error, TEXT("Level flow: duplicate director %s; %s already owns the flow."),
				*GetName(), *It->GetName());
			return;
		}
	}
	bOwnsLevelFlow = true;

	if (!IsValid(MenuCamera) || !MainMenuWidgetClass)
	{
		UE_LOG(LogAesirWarden, Error, TEXT("Level flow: configure MenuCamera and MainMenuWidgetClass on %s."), *GetName());
		return;
	}

	InitializationStartedAt = GetWorld()->GetTimeSeconds();
	// Initialize on a later frame, after GameMode's spawn/possession and Blueprint BeginPlay work.
	GetWorldTimerManager().SetTimer(InitializationTimerHandle, this,
		&AAesirLevelFlowDirector::TryInitializeFlow, 0.1f, true);
}

void AAesirLevelFlowDirector::TryInitializeFlow()
{
	if (!RefreshPlayerReferences())
	{
		if (GetWorld()->GetTimeSeconds() - InitializationStartedAt >= InitializationTimeout)
		{
			GetWorldTimerManager().ClearTimer(InitializationTimerHandle);
			UE_LOG(LogAesirWarden, Error, TEXT("Level flow: initialization timed out waiting for local player 0 and its pawn."));
		}
		return;
	}

	GetWorldTimerManager().ClearTimer(InitializationTimerHandle);
	MainMenuRef = CreateWidget<UUserWidget>(PlayerControllerRef.Get(), MainMenuWidgetClass);
	if (!IsValid(MainMenuRef))
	{
		UE_LOG(LogAesirWarden, Error, TEXT("Level flow: failed to create main menu widget."));
		return;
	}

	bInitialized = true;
	OnMainMenuCreated(MainMenuRef.Get());
	if (!IsActorBeingDestroyed())
	{
		EnterMainMenu();
	}
}

bool AAesirLevelFlowDirector::RefreshPlayerReferences()
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	if (!IsValid(Controller) || !Controller->IsLocalController() || !IsValid(Controller->GetPawn()))
	{
		return false;
	}
	PlayerControllerRef = Controller;
	PlayerPawnRef = Controller->GetPawn();
	return true;
}

bool AAesirLevelFlowDirector::EnterMainMenu()
{
	// Returning from combat will require a full level reset, rather than reusing stale combat state.
	if (!bInitialized || !bOwnsLevelFlow || CurrentState != EAesirLevelFlowState::Uninitialized ||
		!RefreshPlayerReferences() || !IsValid(MenuCamera) || !IsValid(MainMenuRef))
	{
		return false;
	}

	AcquireInputLocks();
	PlayerControllerRef->SetViewTargetWithBlend(MenuCamera.Get(), 0.0f);
	MainMenuRef->AddToViewport(MainMenuZOrder);
	MainMenuRef->SetIsFocusable(true);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MainMenuRef->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerControllerRef->SetInputMode(InputMode);
	PlayerControllerRef->FlushPressedKeys();
	PlayerControllerRef->SetShowMouseCursor(true);
	SetFlowState(EAesirLevelFlowState::MainMenu);
	return true;
}

bool AAesirLevelFlowDirector::RequestStartGame()
{
	if (!bInitialized || CurrentState != EAesirLevelFlowState::MainMenu || !RefreshPlayerReferences())
	{
		return false;
	}

	MainMenuRef->RemoveFromParent();
	PlayerControllerRef->SetShowMouseCursor(false);
	// Keep UI-only routing throughout the opening: movement locks alone do not block GAS attack inputs.
	PlayerControllerRef->SetInputMode(FInputModeUIOnly());
	PlayerControllerRef->FlushPressedKeys();
	SetFlowState(EAesirLevelFlowState::OpeningCinematic);
	if (CurrentState != EAesirLevelFlowState::OpeningCinematic || IsActorBeingDestroyed())
	{
		return true;
	}

	if (IsValid(OpeningSequenceActor))
	{
		ActiveOpeningPlayer = OpeningSequenceActor->GetSequencePlayer();
		if (IsValid(ActiveOpeningPlayer) && ActiveOpeningPlayer->IsValid())
		{
			// This director owns input locks; the sequence must neither loop nor pause at its end.
			FMovieSceneSequencePlaybackSettings Settings = OpeningSequenceActor->PlaybackSettings;
			Settings.bAutoPlay = false;
			Settings.LoopCount.Value = 0;
			Settings.bPauseAtEnd = false;
			Settings.bDisableMovementInput = false;
			Settings.bDisableLookAtInput = false;
			Settings.bRandomStartTime = false;
			Settings.StartTime = 0.0f;
			Settings.PlayRate = 1.0f;
			ActiveOpeningPlayer->SetPlaybackSettings(Settings);
			ActiveOpeningPlayer->OnFinished.AddUniqueDynamic(this, &AAesirLevelFlowDirector::HandleOpeningSequenceFinished);
			ActiveOpeningPlayer->OnStop.AddUniqueDynamic(this, &AAesirLevelFlowDirector::HandleOpeningSequenceFinished);
			ActiveOpeningPlayer->Play();
			return true;
		}
		UE_LOG(LogAesirWarden, Warning, TEXT("Level flow: opening sequence has no initialized player; using camera blend."));
		ActiveOpeningPlayer = nullptr;
	}

	BeginGameplayCameraHandoff();
	return true;
}

void AAesirLevelFlowDirector::HandleOpeningSequenceFinished()
{
	BeginGameplayCameraHandoff();
}

void AAesirLevelFlowDirector::BeginGameplayCameraHandoff()
{
	if (CurrentState != EAesirLevelFlowState::OpeningCinematic || bCameraHandoffStarted)
	{
		return;
	}
	if (!RefreshPlayerReferences())
	{
		UE_LOG(LogAesirWarden, Error, TEXT("Level flow: cannot hand off opening camera without a possessed player pawn."));
		return;
	}

	bCameraHandoffStarted = true;
	UnbindOpeningSequence();
	const float BlendTime = FMath::Max(0.0f, OpeningBlendTime);
	PlayerControllerRef->SetViewTargetWithBlend(PlayerPawnRef.Get(), BlendTime, VTBlend_Cubic, 0.0f, true);
	if (BlendTime <= 0.0f)
	{
		CompleteGameplayCameraHandoff();
	}
	else
	{
		GetWorldTimerManager().SetTimer(CameraHandoffTimerHandle, this,
			&AAesirLevelFlowDirector::CompleteGameplayCameraHandoff, BlendTime, false);
	}
}

void AAesirLevelFlowDirector::CompleteGameplayCameraHandoff()
{
	EnterExploration();
}

bool AAesirLevelFlowDirector::EnterExploration()
{
	if (!bInitialized || CurrentState != EAesirLevelFlowState::OpeningCinematic || !RefreshPlayerReferences())
	{
		return false;
	}

	GetWorldTimerManager().ClearTimer(CameraHandoffTimerHandle);
	UnbindOpeningSequence();
	if (IsValid(ActiveOpeningPlayer) && (ActiveOpeningPlayer->IsPlaying() || ActiveOpeningPlayer->IsPaused()))
	{
		ActiveOpeningPlayer->GoToEndAndStop();
	}
	ActiveOpeningPlayer = nullptr;
	// Explicitly establish the gameplay camera even when a Blueprint calls this entry directly.
	PlayerControllerRef->SetViewTargetWithBlend(PlayerPawnRef.Get(), 0.0f);
	ReleaseInputLocks();
	PlayerControllerRef->SetInputMode(FInputModeGameOnly());
	PlayerControllerRef->FlushPressedKeys();
	PlayerControllerRef->SetShowMouseCursor(false);
	SetFlowState(EAesirLevelFlowState::Exploration);
	return true;
}

bool AAesirLevelFlowDirector::SkipOpeningCinematic()
{
	if (CurrentState != EAesirLevelFlowState::OpeningCinematic || bCameraHandoffStarted)
	{
		return false;
	}
	if (IsValid(ActiveOpeningPlayer))
	{
		ActiveOpeningPlayer->GoToEndAndStop();
	}
	// Stop and natural completion may both arrive; the handoff guard allows one transition.
	BeginGameplayCameraHandoff();
	return true;
}

void AAesirLevelFlowDirector::AcquireInputLocks()
{
	if (!bOwnsInputLocks && IsValid(PlayerControllerRef))
	{
		PlayerControllerRef->SetIgnoreMoveInput(true);
		PlayerControllerRef->SetIgnoreLookInput(true);
		InputLockController = PlayerControllerRef.Get();
		bOwnsInputLocks = true;
	}
}

void AAesirLevelFlowDirector::ReleaseInputLocks()
{
	if (bOwnsInputLocks)
	{
		if (APlayerController* Controller = InputLockController.Get())
		{
			// These APIs use counters. Do not reset locks owned by other systems.
			Controller->SetIgnoreMoveInput(false);
			Controller->SetIgnoreLookInput(false);
		}
		bOwnsInputLocks = false;
		InputLockController.Reset();
	}
}

void AAesirLevelFlowDirector::SetFlowState(EAesirLevelFlowState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}
	const EAesirLevelFlowState PreviousState = CurrentState;
	CurrentState = NewState;
	UE_LOG(LogAesirWarden, Display, TEXT("Level flow: %s -> %s"),
		*UEnum::GetValueAsString(PreviousState), *UEnum::GetValueAsString(NewState));
	OnFlowStateChanged.Broadcast(PreviousState, NewState);
	OnPhaseChanged(PreviousState, NewState);
}

void AAesirLevelFlowDirector::UnbindOpeningSequence()
{
	if (IsValid(ActiveOpeningPlayer))
	{
		ActiveOpeningPlayer->OnFinished.RemoveDynamic(this, &AAesirLevelFlowDirector::HandleOpeningSequenceFinished);
		ActiveOpeningPlayer->OnStop.RemoveDynamic(this, &AAesirLevelFlowDirector::HandleOpeningSequenceFinished);
	}
}

void AAesirLevelFlowDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(InitializationTimerHandle);
	GetWorldTimerManager().ClearTimer(CameraHandoffTimerHandle);
	UnbindOpeningSequence();
	if (IsValid(ActiveOpeningPlayer))
	{
		ActiveOpeningPlayer->Stop();
	}
	if (IsValid(MainMenuRef))
	{
		MainMenuRef->RemoveFromParent();
	}
	ReleaseInputLocks();
	if (EndPlayReason == EEndPlayReason::Destroyed && bInitialized && IsValid(PlayerControllerRef))
	{
		PlayerControllerRef->SetInputMode(FInputModeGameOnly());
		PlayerControllerRef->SetShowMouseCursor(false);
		if (IsValid(PlayerControllerRef->GetPawn()))
		{
			PlayerControllerRef->SetViewTargetWithBlend(PlayerControllerRef->GetPawn(), 0.0f);
		}
	}
	bOwnsLevelFlow = false;
	Super::EndPlay(EndPlayReason);
}

