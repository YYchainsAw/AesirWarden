// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Companion/AesirCompanionAIController.h"

#include "Characters/Companion/AesirCompanionCharacter.h"
#include "Services/Companion/CompanionChatSubsystem.h"
#include "AesirWarden.h"
#include "BrainComponent.h"
#include "AI/Companion/TacticalOrderComponent.h"

void AAesirCompanionAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	bWaitingForPlayerCommand = false;

	if (BehaviorTreeAsset)
		RunBehaviorTree(BehaviorTreeAsset);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UCompanionChatSubsystem* Chat =
			GameInstance->GetSubsystem<UCompanionChatSubsystem>())
		{
			Chat->OnDirectiveReceived.AddUniqueDynamic(
				this, &AAesirCompanionAIController::HandleAgentDirective);
		}
	}
}

void AAesirCompanionAIController::OnUnPossess()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UCompanionChatSubsystem* Chat =
			GameInstance->GetSubsystem<UCompanionChatSubsystem>())
		{
			Chat->OnDirectiveReceived.RemoveDynamic(
				this, &AAesirCompanionAIController::HandleAgentDirective);
		}
	}
	bWaitingForPlayerCommand = false;
	Super::OnUnPossess();
}

void AAesirCompanionAIController::HandleAgentDirective(
	const FCompanionAgentDirective& Directive)
{
	const AAesirCompanionCharacter* Companion =
		Cast<AAesirCompanionCharacter>(GetPawn());
	if (!IsValid(Companion) ||
		Directive.AgentId != Companion->GetCompanionId().ToString() ||
		Directive.Source != TEXT("player_command") ||
		Directive.Domain != TEXT("exploration") ||
		(Directive.ActionType != TEXT("follow") &&
			Directive.ActionType != TEXT("wait")))
	{
		return;
	}
	const UTacticalOrderComponent* TacticalOrders =
		Companion->FindComponentByClass<UTacticalOrderComponent>();
	if (IsValid(TacticalOrders) && TacticalOrders->IsEncounterActive())
	{
		UE_LOG(LogAesirWarden, Warning,
			TEXT("Companion directive ignored: encounter started, id=%s action=%s"),
			*Directive.DirectiveId, *Directive.ActionType);
		return;
	}

	UBrainComponent* Brain = GetBrainComponent();
	if (!BehaviorTreeAsset || !IsValid(Brain))
	{
		UE_LOG(LogAesirWarden, Warning,
			TEXT("Companion directive ignored: no Behavior Tree, id=%s action=%s"),
			*Directive.DirectiveId, *Directive.ActionType);
		return;
	}

	if (Directive.ActionType == TEXT("wait"))
	{
		if (!bWaitingForPlayerCommand)
		{
			Brain->PauseLogic(TEXT("Player command: wait"));
			StopMovement();
			bWaitingForPlayerCommand = true;
		}
	}
	else if (bWaitingForPlayerCommand)
	{
		Brain->ResumeLogic(TEXT("Player command: follow"));
		bWaitingForPlayerCommand = false;
	}

	UE_LOG(LogAesirWarden, Display,
		TEXT("Companion directive applied: id=%s agent=%s action=%s waiting=%d"),
		*Directive.DirectiveId, *Directive.AgentId,
		*Directive.ActionType, bWaitingForPlayerCommand ? 1 : 0);
}
