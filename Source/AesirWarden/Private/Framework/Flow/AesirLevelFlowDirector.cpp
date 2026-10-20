// Copyright (c) 2026 YYchainsAw. All rights reserved. Third-party assets belong to their respective owners.


#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "AesirWarden.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraActor.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "UI/AesirMainMenuWidget.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

AAesirLevelFlowDirector::AAesirLevelFlowDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	MainMenuWidgetClass = UAesirMainMenuWidget::StaticClass();
	StartupStreamingSource = CreateDefaultSubobject<UWorldPartitionStreamingSourceComponent>(TEXT("StartupStreamingSource"));
	StartupStreamingSource->Priority = EStreamingSourcePriority::High;
#if WITH_EDITOR
	SetIsSpatiallyLoaded(false);
#endif
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
			StartupStreamingSource->DisableStreamingSource();
			return;
		}
	}
	bOwnsLevelFlow = true;

	if (!IsValid(MenuCamera) || !MainMenuWidgetClass)
	{
		UE_LOG(LogAesirWarden, Error, TEXT("Level flow: configure MenuCamera and MainMenuWidgetClass on %s."), *GetName());
		return;
	}
	if (IsValid(OpeningSequenceActor) && OpeningSequenceActor->PlaybackSettings.bAutoPlay)
	{
		UE_LOG(LogAesirWarden, Error, TEXT("Level flow: disable Auto Play on the opening Sequence actor before starting the level."));
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
	if (UAesirMainMenuWidget* Menu = Cast<UAesirMainMenuWidget>(MainMenuRef))
	{
		Menu->OnStartRequested.AddUniqueDynamic(this, &AAesirLevelFlowDirector::HandleStartRequested);
		Menu->OnQuitRequested.AddUniqueDynamic(this, &AAesirLevelFlowDirector::HandleQuitRequested);
	}
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

	PresentMainMenu();
	return true;
}

void AAesirLevelFlowDirector::PresentMainMenu()
{
	bMenuFocusInitialized = false;
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
	PrepareMainMenuPresentation();
	SetFlowState(EAesirLevelFlowState::MainMenu);
	RefreshMenuStreamingState();
	GetWorldTimerManager().SetTimer(MenuStreamingTimerHandle, this,
		&AAesirLevelFlowDirector::RefreshMenuStreamingState, 0.1f, true);
}

bool AAesirLevelFlowDirector::IsStartupStreamingReady() const
{
	if (!GetWorld() || !GetWorld()->GetWorldPartition())
	{
		return true;
	}
	const UWorldPartitionSubsystem* Partition = GetWorld()->GetSubsystem<UWorldPartitionSubsystem>();
	return Partition && StartupStreamingSource->IsStreamingCompleted() && Partition->IsStreamingCompleted();
}

void AAesirLevelFlowDirector::RefreshMenuStreamingState()
{
	if (CurrentState == EAesirLevelFlowState::MainMenu || CurrentState == EAesirLevelFlowState::OpeningCinematic)
	{
		HideGameplayHUD();
	}
	if (CurrentState != EAesirLevelFlowState::MainMenu)
	{
		return;
	}
	const bool bWasReady = bStartupStreamingReady;
	bStartupStreamingReady = IsStartupStreamingReady();
	if (UAesirMainMenuWidget* Menu = Cast<UAesirMainMenuWidget>(MainMenuRef))
	{
		Menu->SetStartAvailability(bStartupStreamingReady,
			bStartupStreamingReady ? FText::GetEmpty() : NSLOCTEXT("AesirMenu", "PreparingStart", "正在准备营火与前路…"));
		if (bStartupStreamingReady != bWasReady || !bMenuFocusInitialized)
		{
			Menu->FocusDefaultAction();
			bMenuFocusInitialized = true;
		}
	}
}

void AAesirLevelFlowDirector::HideGameplayHUD()
{
	for (const TSubclassOf<UUserWidget>& HUDClass : GameplayHUDWidgetClasses)
	{
		if (!HUDClass)
		{
			continue;
		}
		TArray<UUserWidget*> Widgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, HUDClass, true);
		for (UUserWidget* Widget : Widgets)
		{
			if (IsValid(Widget) && Widget != MainMenuRef)
			{
				if (!HiddenGameplayHUD.Contains(Widget))
				{
					HiddenGameplayHUD.Add(Widget, Widget->GetVisibility());
				}
				Widget->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void AAesirLevelFlowDirector::RestoreGameplayHUD()
{
	for (const auto& Entry : HiddenGameplayHUD)
	{
		if (UUserWidget* Widget = Entry.Key.Get())
		{
			Widget->SetVisibility(Entry.Value);
		}
	}
	HiddenGameplayHUD.Empty();
}

void AAesirLevelFlowDirector::HandleStartRequested()
{
	RequestStartGame();
}

void AAesirLevelFlowDirector::HandleQuitRequested()
{
	RequestQuitGame();
}

bool AAesirLevelFlowDirector::RequestQuitGame()
{
	if (CurrentState != EAesirLevelFlowState::MainMenu || !IsValid(PlayerControllerRef))
	{
		return false;
	}
	UKismetSystemLibrary::QuitGame(this, PlayerControllerRef.Get(), EQuitPreference::Quit, false);
	return true;
}

bool AAesirLevelFlowDirector::RequestStartGame()
{
	if (!bInitialized || CurrentState != EAesirLevelFlowState::MainMenu || !RefreshPlayerReferences())
	{
		return false;
	}

	RefreshMenuStreamingState();
	if (!bStartupStreamingReady)
	{
		return false;
	}
	MainMenuRef->RemoveFromParent();
	PlayerControllerRef->SetShowMouseCursor(false);
	// Keep UI-only routing throughout the opening: movement locks alone do not block GAS attack inputs.
	PlayerControllerRef->SetInputMode(FInputModeUIOnly());
	PlayerControllerRef->FlushPressedKeys();
	// Set the gate before notifying Blueprint listeners, which may call completion/skip synchronously.
	bOpeningPresentationPending = true;
	SetFlowState(EAesirLevelFlowState::OpeningCinematic);
	if (CurrentState != EAesirLevelFlowState::OpeningCinematic || bCameraHandoffStarted || IsActorBeingDestroyed())
	{
		return true;
	}

	if (!BeginOpeningPresentation())
	{
		CompleteOpeningPresentation();
	}
	return true;
}

void AAesirLevelFlowDirector::PrepareMainMenuPresentation()
{
}

bool AAesirLevelFlowDirector::BeginOpeningPresentation()
{
	return false;
}

void AAesirLevelFlowDirector::FinishOpeningPresentation(bool bSkipped)
{
}

void AAesirLevelFlowDirector::CompleteOpeningPresentation()
{
	if (CurrentState != EAesirLevelFlowState::OpeningCinematic || !bOpeningPresentationPending || IsActorBeingDestroyed())
	{
		return;
	}
	bOpeningPresentationPending = false;
	FinishOpeningPresentation(false);
	PlayOpeningSequenceOrHandoff();
}

void AAesirLevelFlowDirector::PlayOpeningSequenceOrHandoff()
{
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
			// Changing playback settings alone does not rewind the existing sequence player.
			ActiveOpeningPlayer->SetPlaybackPosition(FMovieSceneSequencePlaybackParams(
				ActiveOpeningPlayer->GetStartTime().Time, EUpdatePositionMethod::Jump));
			if (CurrentState != EAesirLevelFlowState::OpeningCinematic || bCameraHandoffStarted || IsActorBeingDestroyed())
			{
				return;
			}
			ActiveOpeningPlayer->OnFinished.AddUniqueDynamic(this, &AAesirLevelFlowDirector::HandleOpeningSequenceFinished);
			ActiveOpeningPlayer->OnStop.AddUniqueDynamic(this, &AAesirLevelFlowDirector::HandleOpeningSequenceFinished);
			ActiveOpeningPlayer->Play();
			return;
		}
		UE_LOG(LogAesirWarden, Warning, TEXT("Level flow: opening sequence has no initialized player; using camera blend."));
		ActiveOpeningPlayer = nullptr;
	}

	BeginGameplayCameraHandoff();
}

void AAesirLevelFlowDirector::HandleOpeningSequenceFinished()
{
	BeginGameplayCameraHandoff();
}

void AAesirLevelFlowDirector::BeginGameplayCameraHandoff()
{
	if (CurrentState != EAesirLevelFlowState::OpeningCinematic || bCameraHandoffStarted || bOpeningPresentationPending)
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
	if (CurrentState != EAesirLevelFlowState::OpeningCinematic)
	{
		return;
	}
	if (IsStartupStreamingReady())
	{
		EnterExploration();
		return;
	}
	if (!bWaitingForGameplayStreaming)
	{
		bWaitingForGameplayStreaming = true;
		GameplayStreamingStartedAt = GetWorld()->GetTimeSeconds();
		GetWorldTimerManager().SetTimer(CameraHandoffTimerHandle, this,
			&AAesirLevelFlowDirector::CompleteGameplayCameraHandoff, 0.1f, true);
	}
	else if (GetWorld()->GetTimeSeconds() - GameplayStreamingStartedAt >= GameplayStreamingTimeout)
	{
		GetWorldTimerManager().ClearTimer(CameraHandoffTimerHandle);
		bWaitingForGameplayStreaming = false;
		bCameraHandoffStarted = false;
		UE_LOG(LogAesirWarden, Error, TEXT("Level flow: gameplay streaming timed out; returning to the menu with input locked."));
		PresentMainMenu();
	}
}

bool AAesirLevelFlowDirector::EnterExploration()
{
	if (!bInitialized || CurrentState != EAesirLevelFlowState::OpeningCinematic || bOpeningPresentationPending ||
		!RefreshPlayerReferences() || !IsStartupStreamingReady())
	{
		return false;
	}

	GetWorldTimerManager().ClearTimer(CameraHandoffTimerHandle);
	GetWorldTimerManager().ClearTimer(MenuStreamingTimerHandle);
	RestoreGameplayHUD();
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
	StartupStreamingSource->DisableStreamingSource();
	bWaitingForGameplayStreaming = false;
	SetFlowState(EAesirLevelFlowState::Exploration);
	return true;
}

bool AAesirLevelFlowDirector::SkipOpeningCinematic()
{
	if (CurrentState != EAesirLevelFlowState::OpeningCinematic || bCameraHandoffStarted)
	{
		return false;
	}
	bOpeningPresentationPending = false;
	FinishOpeningPresentation(true);
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
	// A listener may have synchronously requested another transition or destroyed this actor.
	if (CurrentState == NewState && !IsActorBeingDestroyed())
	{
		OnPhaseChanged(PreviousState, NewState);
	}
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
	bOpeningPresentationPending = false;
	FinishOpeningPresentation(true);
	GetWorldTimerManager().ClearTimer(InitializationTimerHandle);
	GetWorldTimerManager().ClearTimer(CameraHandoffTimerHandle);
	GetWorldTimerManager().ClearTimer(MenuStreamingTimerHandle);
	RestoreGameplayHUD();
	UnbindOpeningSequence();
	if (IsValid(ActiveOpeningPlayer))
	{
		ActiveOpeningPlayer->Stop();
	}
	if (IsValid(MainMenuRef))
	{
		if (UAesirMainMenuWidget* Menu = Cast<UAesirMainMenuWidget>(MainMenuRef))
		{
			Menu->OnStartRequested.RemoveDynamic(this, &AAesirLevelFlowDirector::HandleStartRequested);
			Menu->OnQuitRequested.RemoveDynamic(this, &AAesirLevelFlowDirector::HandleQuitRequested);
		}
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

