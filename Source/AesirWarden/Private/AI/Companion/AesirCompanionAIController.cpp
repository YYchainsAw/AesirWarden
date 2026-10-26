#include "AI/Companion/AesirCompanionAIController.h"

#include "AI/Companion/TacticalOrderComponent.h"
#include "AesirWarden.h"
#include "BrainComponent.h"
#include "Characters/Companion/AesirCompanionCharacter.h"
#include "Characters/Player/AesirPlayerCharacter.h"
#include "Combat/AesirHealthComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "Framework/AesirCombatPrototypeGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Services/Companion/CompanionChatSubsystem.h"

namespace
{
	constexpr float FollowAcceptanceRadius = 140.0f;
	constexpr int32 MaxRememberedDirectiveIds = 256;
}

void AAesirCompanionAIController::OnPossess(APawn* InPawn)
{
	ReleaseBindings();
	ResetExplorationCommand(false);
	Super::OnPossess(InPawn);

	for (TActorIterator<AAesirLevelFlowDirector> It(GetWorld()); It; ++It)
	{
		LevelFlowDirector = *It;
		const EAesirLevelFlowState State = It->GetFlowState();
		bWaitingForLevelStart = State == EAesirLevelFlowState::Uninitialized ||
			State == EAesirLevelFlowState::MainMenu || State == EAesirLevelFlowState::OpeningCinematic;
		It->OnFlowStateChanged.AddUniqueDynamic(this, &AAesirCompanionAIController::HandleFlowStateChanged);
		break;
	}
	if (IsValid(InPawn))
	{
		CompanionHealth = InPawn->FindComponentByClass<UAesirHealthComponent>();
		if (UAesirHealthComponent* Health = CompanionHealth.Get())
		{
			Health->OnDeath.AddUniqueDynamic(this, &AAesirCompanionAIController::HandleCompanionDeath);
		}
	}
	if (UGameInstance* Instance = GetGameInstance())
	{
		if (UCompanionChatSubsystem* Chat = Instance->GetSubsystem<UCompanionChatSubsystem>())
		{
			Chat->OnDirectiveReceived.AddUniqueDynamic(this, &AAesirCompanionAIController::HandleAgentDirective);
			Chat->OnConversationReset.AddUniqueDynamic(this, &AAesirCompanionAIController::HandleConversationReset);
		}
	}
	StartDefaultBehavior();
}

bool AAesirCompanionAIController::IsExplorationAllowed() const
{
	if (bWaitingForLevelStart || !IsValid(GetPawn()))
		return false;
	if (const AAesirCombatPrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<AAesirCombatPrototypeGameMode>();
		Mode && Mode->GetMatchResult() != EAesirMatchResult::None)
		return false;
	if (const UAesirHealthComponent* Health = CompanionHealth.Get(); Health && Health->HasBegunPlay() && Health->IsDead())
		return false;
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (const AAesirPlayerCharacter* AesirPlayer = Cast<AAesirPlayerCharacter>(Player);
			AesirPlayer && AesirPlayer->HasActorBegunPlay() && AesirPlayer->GetGASHealth() <= 0.0f)
			return false;
		const UAesirHealthComponent* Health = Player->FindComponentByClass<UAesirHealthComponent>();
		if (Health && Health->HasBegunPlay() && Health->IsDead())
			return false;
	}
	const AAesirLevelFlowDirector* Flow = LevelFlowDirector.Get();
	return !Flow || Flow->GetFlowState() == EAesirLevelFlowState::Exploration;
}

void AAesirCompanionAIController::StartDefaultBehavior()
{
	if (bWaitingForLevelStart)
		return;
	if (const AAesirCombatPrototypeGameMode* Mode = GetWorld()->GetAuthGameMode<AAesirCombatPrototypeGameMode>();
		Mode && Mode->GetMatchResult() != EAesirMatchResult::None)
		return;
	const AAesirLevelFlowDirector* Flow = LevelFlowDirector.Get();
	const bool bGameplayPhase = !Flow || Flow->GetFlowState() == EAesirLevelFlowState::Exploration ||
		Flow->GetFlowState() == EAesirLevelFlowState::BossCombat;
	if (!bGameplayPhase || !IsValid(GetPawn()) || !BehaviorTreeAsset)
		return;
	if (const UAesirHealthComponent* Health = CompanionHealth.Get(); Health && Health->HasBegunPlay() && Health->IsDead())
		return;
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (const AAesirPlayerCharacter* AesirPlayer = Cast<AAesirPlayerCharacter>(Player);
			AesirPlayer && AesirPlayer->HasActorBegunPlay() && AesirPlayer->GetGASHealth() <= 0.0f)
			return;
		const UAesirHealthComponent* Health = Player->FindComponentByClass<UAesirHealthComponent>();
		if (Health && Health->HasBegunPlay() && Health->IsDead())
			return;
	}
	UBrainComponent* Brain = GetBrainComponent();
	if (!Brain || !Brain->IsRunning())
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void AAesirCompanionAIController::PauseDefaultBehavior()
{
	UBrainComponent* Brain = GetBrainComponent();
	if (Brain && Brain->IsRunning() && !Brain->IsPaused())
	{
		Brain->PauseLogic(TEXT("Player exploration command"));
		bOwnsExplorationBrainPause = true;
	}
}

void AAesirCompanionAIController::HandleAgentDirective(const FCompanionAgentDirective& Directive)
{
	const AAesirCompanionCharacter* Companion = Cast<AAesirCompanionCharacter>(GetPawn());
	UGameInstance* Instance = GetGameInstance();
	const UCompanionChatSubsystem* Chat = Instance ? Instance->GetSubsystem<UCompanionChatSubsystem>() : nullptr;
	if (!Companion || !Chat || !IsExplorationAllowed() ||
		Directive.DirectiveId.IsEmpty() || Directive.AgentId != Companion->GetCompanionId().ToString() ||
		Directive.AgentId != Chat->GetSelectedCompanionId() ||
		Directive.Source != TEXT("player_command") || Directive.Domain != TEXT("exploration") ||
		(Directive.ActionType != TEXT("follow") && Directive.ActionType != TEXT("wait")) ||
		!Chat->IsDirectiveCurrent(Directive))
	{
		return;
	}
	if (AppliedDirectiveIds.Contains(Directive.DirectiveId))
	{
		UE_LOG(LogAesirWarden, Display, TEXT("Companion directive duplicate ignored: request=%s directive=%s agent=%s"),
			*Directive.RequestId, *Directive.DirectiveId, *Directive.AgentId);
		return;
	}
	const UTacticalOrderComponent* Orders = Companion->FindComponentByClass<UTacticalOrderComponent>();
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const UAesirHealthComponent* PlayerHealth = Player ? Player->FindComponentByClass<UAesirHealthComponent>() : nullptr;
	if ((Orders && Orders->IsEncounterActive()) || !IsValid(Player) || (PlayerHealth && PlayerHealth->IsDead()))
	{
		return;
	}
	if (Directive.ActionType == TEXT("follow") && Directive.TargetId != TEXT("party.player"))
	{
		return;
	}

	const bool bAlreadyCommanded = bWaitingForPlayerCommand || bFollowingPlayerCommand;
	PauseDefaultBehavior();
	if (Directive.ActionType == TEXT("follow"))
	{
		const EPathFollowingRequestResult::Type Result = MoveToActor(Player, FollowAcceptanceRadius);
		if (Result == EPathFollowingRequestResult::Failed)
		{
			if (!bAlreadyCommanded)
				ResetExplorationCommand(true);
			UE_LOG(LogAesirWarden, Warning, TEXT("Companion follow failed: request=%s directive=%s agent=%s reason=no_navigation_path"),
				*Directive.RequestId, *Directive.DirectiveId, *Directive.AgentId);
			return;
		}
		bFollowingPlayerCommand = true;
		bWaitingForPlayerCommand = false;
	}
	else
	{
		StopMovement();
		Companion->GetCharacterMovement()->StopMovementImmediately();
		bWaitingForPlayerCommand = true;
		bFollowingPlayerCommand = false;
	}
	CommandPlayer = Player;
	ActiveExplorationDirectiveId = Directive.DirectiveId;
	AppliedDirectiveIds.Add(Directive.DirectiveId);
	if (AppliedDirectiveIds.Num() > MaxRememberedDirectiveIds)
		AppliedDirectiveIds.RemoveAt(0, AppliedDirectiveIds.Num() - MaxRememberedDirectiveIds);
	// valid_for_ms governs accepting the response, not the duration of the resulting follow/wait state.
	GetWorldTimerManager().SetTimer(ExplorationCommandTimer, this,
		&AAesirCompanionAIController::UpdateExplorationCommand, 0.35f, true);
	UE_LOG(LogAesirWarden, Display, TEXT("Companion directive applied: request=%s directive=%s agent=%s action=%s waiting=%d following=%d"),
		*Directive.RequestId, *Directive.DirectiveId, *Directive.AgentId, *Directive.ActionType,
		bWaitingForPlayerCommand ? 1 : 0, bFollowingPlayerCommand ? 1 : 0);
}

void AAesirCompanionAIController::UpdateExplorationCommand()
{
	APawn* Player = CommandPlayer.Get();
	const AAesirCompanionCharacter* Companion = Cast<AAesirCompanionCharacter>(GetPawn());
	UGameInstance* Instance = GetGameInstance();
	const UCompanionChatSubsystem* Chat = Instance ? Instance->GetSubsystem<UCompanionChatSubsystem>() : nullptr;
	const UAesirHealthComponent* PlayerHealth = Player ? Player->FindComponentByClass<UAesirHealthComponent>() : nullptr;
	const UTacticalOrderComponent* Orders = Companion ? Companion->FindComponentByClass<UTacticalOrderComponent>() : nullptr;
	if (!IsExplorationAllowed() || !Companion || !Chat || !Player ||
		Player != UGameplayStatics::GetPlayerPawn(this, 0) || (PlayerHealth && PlayerHealth->IsDead()) ||
		Chat->GetSelectedCompanionId() != Companion->GetCompanionId().ToString() ||
		(Orders && Orders->IsEncounterActive()))
	{
		ResetExplorationCommand(false);
		return;
	}
	if (bFollowingPlayerCommand && GetMoveStatus() == EPathFollowingStatus::Idle &&
		FVector::DistSquared2D(Companion->GetActorLocation(), Player->GetActorLocation()) >
		FMath::Square(FollowAcceptanceRadius + 60.0f))
	{
		if (MoveToActor(Player, FollowAcceptanceRadius) == EPathFollowingRequestResult::Failed)
		{
			UE_LOG(LogAesirWarden, Warning, TEXT("Companion follow stopped: directive=%s reason=no_navigation_path"),
				*ActiveExplorationDirectiveId);
			ResetExplorationCommand(true);
		}
	}
}

void AAesirCompanionAIController::ResetExplorationCommand(bool bResumeDefaultBehavior)
{
	GetWorldTimerManager().ClearTimer(ExplorationCommandTimer);
	if (bWaitingForPlayerCommand || bFollowingPlayerCommand)
		StopMovement();
	bWaitingForPlayerCommand = false;
	bFollowingPlayerCommand = false;
	CommandPlayer.Reset();
	ActiveExplorationDirectiveId.Reset();
	if (bOwnsExplorationBrainPause)
	{
		if (UBrainComponent* Brain = GetBrainComponent())
		{
			if (bResumeDefaultBehavior)
				Brain->ResumeLogic(TEXT("Player exploration command cleared"));
			else
				Brain->StopLogic(TEXT("Exploration command context ended"));
		}
		bOwnsExplorationBrainPause = false;
	}
	if (bResumeDefaultBehavior)
		StartDefaultBehavior();
}

void AAesirCompanionAIController::HandleConversationReset()
{
	ResetExplorationCommand(IsExplorationAllowed());
	AppliedDirectiveIds.Reset();
}

void AAesirCompanionAIController::HandleFlowStateChanged(EAesirLevelFlowState, EAesirLevelFlowState NewState)
{
	ResetExplorationCommand(false);
	AppliedDirectiveIds.Reset();
	bWaitingForLevelStart = NewState == EAesirLevelFlowState::Uninitialized ||
		NewState == EAesirLevelFlowState::MainMenu || NewState == EAesirLevelFlowState::OpeningCinematic;
	if (NewState == EAesirLevelFlowState::Exploration || NewState == EAesirLevelFlowState::BossCombat)
	{
		// Restarting also clears a pause left behind by a completed exploration command.
		if (UBrainComponent* Brain = GetBrainComponent())
			Brain->StopLogic(TEXT("Level phase changed"));
		StartDefaultBehavior();
	}
	else
	{
		StopMovement();
		if (UBrainComponent* Brain = GetBrainComponent())
			Brain->StopLogic(TEXT("Waiting for gameplay phase"));
	}
}

void AAesirCompanionAIController::HandleCompanionDeath(AController*, AActor*)
{
	ResetExplorationCommand(false);
	StopMovement();
	if (UBrainComponent* Brain = GetBrainComponent())
		Brain->StopLogic(TEXT("Companion died"));
}

void AAesirCompanionAIController::ReleaseBindings()
{
	if (UGameInstance* Instance = GetGameInstance())
	{
		if (UCompanionChatSubsystem* Chat = Instance->GetSubsystem<UCompanionChatSubsystem>())
		{
			Chat->OnDirectiveReceived.RemoveDynamic(this, &AAesirCompanionAIController::HandleAgentDirective);
			Chat->OnConversationReset.RemoveDynamic(this, &AAesirCompanionAIController::HandleConversationReset);
		}
	}
	if (AAesirLevelFlowDirector* Flow = LevelFlowDirector.Get())
		Flow->OnFlowStateChanged.RemoveDynamic(this, &AAesirCompanionAIController::HandleFlowStateChanged);
	if (UAesirHealthComponent* Health = CompanionHealth.Get())
		Health->OnDeath.RemoveDynamic(this, &AAesirCompanionAIController::HandleCompanionDeath);
	LevelFlowDirector.Reset();
	bWaitingForLevelStart = false;
	CompanionHealth.Reset();
	AppliedDirectiveIds.Reset();
}

void AAesirCompanionAIController::OnUnPossess()
{
	ResetExplorationCommand(false);
	ReleaseBindings();
	Super::OnUnPossess();
}

void AAesirCompanionAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetExplorationCommand(false);
	ReleaseBindings();
	Super::EndPlay(EndPlayReason);
}
