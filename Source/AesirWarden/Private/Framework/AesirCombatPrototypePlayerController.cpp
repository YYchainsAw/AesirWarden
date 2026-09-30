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

void AAesirCombatPrototypePlayerController::BeginPlay()
{
	Super::BeginPlay();

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
	if (!IsValid(CompanionChatWidget))
	{
		if (!CompanionChatWidgetClass)
		{
			UE_LOG(
				LogAesirWarden,
				Warning,
				TEXT("CompanionChatWidgetClass is not configured."));
			return;
		}
		
		CompanionChatWidget = CreateWidget<UUserWidget>(this, CompanionChatWidgetClass);
		
		if (!IsValid(CompanionChatWidget)) 
			return;
		
		CompanionChatWidget->AddToViewport(10);
		SetCompanionChatOpen(true);
		return;
	}
	
	SetCompanionChatOpen(!CompanionChatWidget->IsVisible());
}

void AAesirCombatPrototypePlayerController::SetCompanionChatOpen(bool bOpen)
{
	if (!IsValid(CompanionChatWidget))
		return;

	CompanionChatWidget->SetVisibility(bOpen
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);

	if (bOpen)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);

		SetInputMode(InputMode);
		bShowMouseCursor = true;
		return;
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
}


void AAesirCombatPrototypePlayerController::CloseCompanionChat()
{
	SetCompanionChatOpen(false);
}

