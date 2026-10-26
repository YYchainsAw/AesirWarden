// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Flow/AesirLevelFlowTypes.h"
#include "AesirCombatPrototypePlayerController.generated.h"


class UInputMappingContext;
class UUserWidget;
class UInputAction;
class AAesirLevelFlowDirector;
class UAesirHealthComponent;
class IInputProcessor;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AAesirCombatPrototypePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	bool IsCompanionChatOpen() const;

	UFUNCTION(BlueprintCallable, Category = "UI|Companion Chat")
	void CloseCompanionChat();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI|Companion Chat")
	TObjectPtr<UInputAction> ToggleCompanionChatAction;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Companion Chat")
	TSubclassOf<UUserWidget> CompanionChatWidgetClass;
	
	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

private:
	void ToggleCompanionChat();

	void SetCompanionChatOpen(bool bOpen, bool bRestoreGameplay = true);
	bool CanOpenCompanionChat() const;
	void UnbindPawnHealth();

	UFUNCTION()
	void HandleFlowStateChanged(EAesirLevelFlowState PreviousState, EAesirLevelFlowState NewState);

	UFUNCTION()
	void HandlePlayerDeath(AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void HandleConversationReset();

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CompanionChatWidget;

	TWeakObjectPtr<AAesirLevelFlowDirector> ChatFlowDirector;
	TWeakObjectPtr<UAesirHealthComponent> ChatPawnHealth;
	bool bOwnsChatInputLocks = false;
	TSharedPtr<IInputProcessor> ChatInputProcessor;
};
