// Copyright Epic Games, Inc. All Rights Reserved.


#include "Framework/AesirCombatPrototypePlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "AesirWarden.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "StructUtils/UserDefinedStruct.h"
#include "Blueprint/WidgetTree.h"
#include "Combat/AesirHealthComponent.h"
#include "Characters/Player/AesirPlayerCharacter.h"
#include "Components/EditableTextBox.h"
#include "EngineUtils.h"
#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "Framework/AesirCombatPrototypeGameMode.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/AesirCompanionChatWidget.h"
#include "Services/Companion/CompanionChatSubsystem.h"

namespace
{
	/** UI-only routing bypasses the gameplay toggle binding, including for existing Blueprint panels. */
	class FCompanionChatInputProcessor final : public IInputProcessor
	{
	public:
		explicit FCompanionChatInputProcessor(AAesirCombatPrototypePlayerController* InController)
			: Controller(InController) {}
		virtual void Tick(float, FSlateApplication&, TSharedRef<ICursor>) override {}
		virtual bool HandleKeyDownEvent(FSlateApplication&, const FKeyEvent& Event) override
		{
			if (Event.GetKey() == EKeys::Escape && Controller.IsValid() && Controller->IsCompanionChatOpen())
			{
				Controller->CloseCompanionChat();
				return true;
			}
			return false;
		}
	private:
		TWeakObjectPtr<AAesirCombatPrototypePlayerController> Controller;
	};
}

void AAesirCombatPrototypePlayerController::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<AAesirLevelFlowDirector> It(GetWorld()); It; ++It)
	{
		ChatFlowDirector = *It;
		It->OnFlowStateChanged.AddUniqueDynamic(this, &AAesirCombatPrototypePlayerController::HandleFlowStateChanged);
		break;
	}
	if (UGameInstance* Instance = GetGameInstance())
	{
		if (UCompanionChatSubsystem* Chat = Instance->GetSubsystem<UCompanionChatSubsystem>())
		{
			Chat->OnConversationReset.AddUniqueDynamic(this, &AAesirCombatPrototypePlayerController::HandleConversationReset);
		}
	}

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogAesirWarden, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AAesirCombatPrototypePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (IsLocalPlayerController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
			
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			if (ToggleCompanionChatAction)
			{
				EnhancedInputComponent->BindAction(
					ToggleCompanionChatAction,
					ETriggerEvent::Started,
					this,
					&AAesirCombatPrototypePlayerController::
						ToggleCompanionChat);
			}
		}
		if (!ToggleCompanionChatAction && InputComponent)
		{
			InputComponent->BindKey(EKeys::T, IE_Pressed, this,
				&AAesirCombatPrototypePlayerController::ToggleCompanionChat);
		}
	}
}

bool AAesirCombatPrototypePlayerController::ShouldUseTouchControls() const
{
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

bool AAesirCombatPrototypePlayerController::IsCompanionChatOpen() const
{
	return IsValid(CompanionChatWidget) && CompanionChatWidget->IsVisible();
}

void AAesirCombatPrototypePlayerController::ToggleCompanionChat()
{
	if (!IsCompanionChatOpen() && !CanOpenCompanionChat())
	{
		return;
	}
	if (!IsValid(CompanionChatWidget))
	{
		TSubclassOf<UUserWidget> WidgetClass = CompanionChatWidgetClass;
		if (!WidgetClass)
			WidgetClass = UAesirCompanionChatWidget::StaticClass();
		CompanionChatWidget = CreateWidget<UUserWidget>(this, WidgetClass);
		
		if (!IsValid(CompanionChatWidget)) 
			return;
		
		CompanionChatWidget->AddToViewport(10);
		SetCompanionChatOpen(true);
		return;
	}
	
	SetCompanionChatOpen(!CompanionChatWidget->IsVisible());
}

void AAesirCombatPrototypePlayerController::SetCompanionChatOpen(bool bOpen, bool bRestoreGameplay)
{
	if (bOpen && !IsValid(CompanionChatWidget))
		return;
	if (bOpen && !CanOpenCompanionChat())
		return;
	const bool bWasOpen = IsCompanionChatOpen();

	if (IsValid(CompanionChatWidget))
	{
		CompanionChatWidget->SetVisibility(bOpen
			? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (bOpen)
	{
		if (!bOwnsChatInputLocks)
		{
			SetIgnoreMoveInput(true);
			SetIgnoreLookInput(true);
			bOwnsChatInputLocks = true;
		}
		// UI-only routing also prevents attack/GAS actions while typing.
		CompanionChatWidget->SetIsFocusable(true);
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(CompanionChatWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		FlushPressedKeys();
		bShowMouseCursor = true;
		if (!ChatInputProcessor.IsValid() && FSlateApplication::IsInitialized())
		{
			ChatInputProcessor = MakeShared<FCompanionChatInputProcessor>(this);
			FSlateApplication::Get().RegisterInputPreProcessor(ChatInputProcessor, 0);
		}
		if (UAesirCompanionChatWidget* NativeChat = Cast<UAesirCompanionChatWidget>(CompanionChatWidget))
		{
			NativeChat->ActivateChat();
		}
		else if (CompanionChatWidget->WidgetTree)
		{
			TArray<UWidget*> Controls;
			CompanionChatWidget->WidgetTree->GetAllWidgets(Controls);
			for (UWidget* Control : Controls)
			{
				if (UEditableTextBox* Input = Cast<UEditableTextBox>(Control))
				{
					Input->SetUserFocus(this);
					break;
				}
			}
		}
		return;
	}
	if (ChatInputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(ChatInputProcessor);
	}
	ChatInputProcessor.Reset();
	if (UAesirCompanionChatWidget* NativeChat = Cast<UAesirCompanionChatWidget>(CompanionChatWidget))
	{
		NativeChat->DeactivateChat();
	}
	if (bOwnsChatInputLocks)
	{
		// Ignore-input APIs use counters; preserve locks held by cinematics and menus.
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);
		bOwnsChatInputLocks = false;
	}
	if (bWasOpen)
	{
		FlushPressedKeys();
		if (bRestoreGameplay && CanOpenCompanionChat())
		{
			SetInputMode(FInputModeGameOnly());
			bShowMouseCursor = false;
		}
	}
}


void AAesirCombatPrototypePlayerController::CloseCompanionChat()
{
	SetCompanionChatOpen(false);
}

bool AAesirCombatPrototypePlayerController::CanOpenCompanionChat() const
{
	if (!IsLocalPlayerController() || !IsValid(GetPawn()))
	{
		return false;
	}
	if (const AAesirCombatPrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<AAesirCombatPrototypeGameMode>();
		Mode && Mode->GetMatchResult() != EAesirMatchResult::None)
	{
		return false;
	}
	if (const AAesirPlayerCharacter* PlayerCharacter = Cast<AAesirPlayerCharacter>(GetPawn());
		PlayerCharacter && PlayerCharacter->GetGASHealth() <= 0.0f)
	{
		return false;
	}
	if (const AAesirLevelFlowDirector* Flow = ChatFlowDirector.Get())
	{
		if (Flow->GetFlowState() != EAesirLevelFlowState::Exploration)
			return false;
	}
	const UAesirHealthComponent* Health = GetPawn()->FindComponentByClass<UAesirHealthComponent>();
	return !Health || !Health->IsDead();
}

void AAesirCombatPrototypePlayerController::HandleFlowStateChanged(
	EAesirLevelFlowState, EAesirLevelFlowState NewState)
{
	if (NewState != EAesirLevelFlowState::Exploration)
	{
		// The next phase owns routing and cursor state; closing chat must not overwrite them.
		SetCompanionChatOpen(false, false);
	}
}

void AAesirCombatPrototypePlayerController::OnPossess(APawn* InPawn)
{
	SetCompanionChatOpen(false, false);
	UnbindPawnHealth();
	Super::OnPossess(InPawn);
	if (InPawn)
	{
		ChatPawnHealth = InPawn->FindComponentByClass<UAesirHealthComponent>();
		if (UAesirHealthComponent* Health = ChatPawnHealth.Get())
		{
			Health->OnDeath.AddUniqueDynamic(this, &AAesirCombatPrototypePlayerController::HandlePlayerDeath);
		}
	}
}

void AAesirCombatPrototypePlayerController::UnbindPawnHealth()
{
	if (UAesirHealthComponent* Health = ChatPawnHealth.Get())
	{
		Health->OnDeath.RemoveDynamic(this, &AAesirCombatPrototypePlayerController::HandlePlayerDeath);
	}
	ChatPawnHealth.Reset();
}

void AAesirCombatPrototypePlayerController::OnUnPossess()
{
	SetCompanionChatOpen(false, false);
	UnbindPawnHealth();
	Super::OnUnPossess();
}

void AAesirCombatPrototypePlayerController::HandlePlayerDeath(AController*, AActor*)
{
	SetCompanionChatOpen(false, false);
}

void AAesirCombatPrototypePlayerController::HandleConversationReset()
{
	// Session polling also discovers GAS death and a terminal match in maps without a phase transition.
	if (!CanOpenCompanionChat())
	{
		SetCompanionChatOpen(false, false);
	}
}

void AAesirCombatPrototypePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetCompanionChatOpen(false, false);
	UnbindPawnHealth();
	if (UGameInstance* Instance = GetGameInstance())
	{
		if (UCompanionChatSubsystem* Chat = Instance->GetSubsystem<UCompanionChatSubsystem>())
		{
			Chat->OnConversationReset.RemoveDynamic(this, &AAesirCombatPrototypePlayerController::HandleConversationReset);
		}
	}
	if (AAesirLevelFlowDirector* Flow = ChatFlowDirector.Get())
	{
		Flow->OnFlowStateChanged.RemoveDynamic(this, &AAesirCombatPrototypePlayerController::HandleFlowStateChanged);
	}
	ChatFlowDirector.Reset();
	if (CompanionChatWidget)
	{
		CompanionChatWidget->RemoveFromParent();
		CompanionChatWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

