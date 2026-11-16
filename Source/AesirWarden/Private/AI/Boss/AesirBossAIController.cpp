// Copyright © 2026 YYchainsAw. All Rights Reserved.


#include "AI/Boss/AesirBossAIController.h"

#include "AesirWarden.h"
#include "AI/Boss/AesirBossActionComponent.h"
#include "AI/Boss/AesirBossObservationComponent.h"
#include "AI/Boss/AesirBossPolicyClientComponent.h"
#include "AI/Boss/AesirBossRewardComponent.h"
#include "AI/Boss/AesirBossTelemetryComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BrainComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/AesirCombatPrototypeGameMode.h"
#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"

AAesirBossAIController::AAesirBossAIController()
{
	BossActionComponent =
		CreateDefaultSubobject<UAesirBossActionComponent>(
			TEXT("BossActionComponent"));
	BossObservationComponent =
		CreateDefaultSubobject<UAesirBossObservationComponent>(
			TEXT("BossObservationComponent"));
	BossTelemetryComponent =
		CreateDefaultSubobject<UAesirBossTelemetryComponent>(
			TEXT("BossTelemetryComponent"));
	BossRewardComponent =
		CreateDefaultSubobject<UAesirBossRewardComponent>(
			TEXT("BossRewardComponent"));
	BossPolicyClientComponent =
		CreateDefaultSubobject<UAesirBossPolicyClientComponent>(
			TEXT("BossPolicyClientComponent"));
}

UAesirBossActionComponent*
AAesirBossAIController::GetBossActionComponent() const
{
	return BossActionComponent;
}

UAesirBossObservationComponent*
AAesirBossAIController::GetBossObservationComponent() const
{
	return BossObservationComponent;
}

UAesirBossTelemetryComponent*
AAesirBossAIController::GetBossTelemetryComponent() const
{
	return BossTelemetryComponent;
}

UAesirBossRewardComponent*
AAesirBossAIController::GetBossRewardComponent() const
{
	return BossRewardComponent;
}

UAesirBossPolicyClientComponent*
AAesirBossAIController::GetBossPolicyClientComponent() const
{
	return BossPolicyClientComponent;
}

FAesirBossActionOutcome AAesirBossAIController::ExecuteBossDecision(
	EAesirBossAction Action,
	AActor* Target,
	float PolicyEvaluationMilliseconds)
{
	FAesirBossObservation Observation;
	if (IsValid(BossObservationComponent))
	{
		Observation =
			BossObservationComponent->CaptureObservation(Target);
	}
	return ExecuteBossDecisionInternal(
		Action,
		Target,
		PolicyEvaluationMilliseconds,
		Observation);
}

FAesirBossActionOutcome AAesirBossAIController::ExecuteBossPolicyDecision(
	EAesirBossAction Action,
	AActor* Target,
	float PolicyEvaluationMilliseconds,
	const FAesirBossObservation& Observation)
{
	return ExecuteBossDecisionInternal(
		Action,
		Target,
		PolicyEvaluationMilliseconds,
		Observation);
}

FAesirBossActionOutcome AAesirBossAIController::ExecuteBossDecisionInternal(
	EAesirBossAction Action,
	AActor* Target,
	float PolicyEvaluationMilliseconds,
	const FAesirBossObservation& Observation)
{
	if (bWaitingForLevelStart || bMatchCombatEnded)
	{
		FAesirBossActionOutcome Outcome;
		Outcome.Action = Action;
		Outcome.Result = EAesirBossActionResult::Busy;
		Outcome.Target = Target;
		return Outcome;
	}
	const float WorldTimeSeconds = GetWorld()
		? GetWorld()->GetTimeSeconds()
		: 0.0f;

	if (ShouldSuppressRepeatedDecision(
		Action,
		Target,
		WorldTimeSeconds))
	{
		FAesirBossActionOutcome SuppressedOutcome;
		SuppressedOutcome.Action = Action;
		SuppressedOutcome.Result = LastDecisionResult;
		SuppressedOutcome.Target = Target;
		SuppressedOutcome.WorldTimeSeconds =
			LastDecisionWorldTimeSeconds;
		return SuppressedOutcome;
	}

	if (IsValid(BossRewardComponent))
		BossRewardComponent->SetTarget(Target);

	const double ActionRequestStart = FPlatformTime::Seconds();
	FAesirBossActionOutcome Outcome;
	if (IsValid(BossActionComponent))
	{
		Outcome = BossActionComponent->TryExecuteAction(
			Action,
			Target);
	}
	else
	{
		Outcome.Action = Action;
		Outcome.Result =
			EAesirBossActionResult::MissingAbilitySystem;
		Outcome.Target = Target;
	}
	const float ActionRequestMilliseconds = static_cast<float>(
		(FPlatformTime::Seconds() - ActionRequestStart) * 1000.0);

	RememberDecisionRequest(Outcome);

	if (IsValid(BossTelemetryComponent))
	{
		if (!BossTelemetryComponent->IsEpisodeActive())
			StartBossEpisode(ActivePolicySource, FString());

		FAesirBossDecisionRecord Record;
		Record.PolicySource = ActivePolicySource;
		Record.Observation = Observation;
		Record.Outcome = Outcome;
		Record.PolicyEvaluationMilliseconds =
			FMath::Max(PolicyEvaluationMilliseconds, 0.0f);
		Record.ActionRequestMilliseconds =
			ActionRequestMilliseconds;
		BossTelemetryComponent->RecordDecision(Record);

		if (IsValid(BossRewardComponent))
			BossRewardComponent->HandleActionOutcome(Outcome);
	}

	return Outcome;
}

void AAesirBossAIController::ActivateBehaviorTreeFallback(
	const FString& Reason)
{
	if (bWaitingForLevelStart || bMatchCombatEnded)
		return;
	if (ActivePolicySource == EAesirBossPolicySource::Fallback)
		return;

	UE_LOG(
		LogAesirWarden,
		Warning,
		TEXT("Boss RL policy unavailable; activating BT fallback: %s"),
		*Reason);

	if (IsValid(BossPolicyClientComponent))
		BossPolicyClientComponent->Stop();
	if (IsValid(BossTelemetryComponent) &&
		BossTelemetryComponent->IsEpisodeActive())
	{
		BossTelemetryComponent->EndEpisode(
			EAesirBossEpisodeResult::Aborted);
	}

	ActivePolicySource = EAesirBossPolicySource::Fallback;
	StartBossEpisode(ActivePolicySource, FString());
	if (IsValid(BossTelemetryComponent))
	{
		BossTelemetryComponent->SetPolicyIdentifier(
			BehaviorTreePolicyIdentifier);
	}
	if (IsValid(BossBehaviorTree))
		RunBehaviorTree(BossBehaviorTree);
}

void AAesirBossAIController::StopBossForMatchResult(EAesirBossEpisodeResult Result)
{
	if (bMatchCombatEnded) return;
	bMatchCombatEnded = true;
	ClearLevelStartBinding();
	StopPossessedBossPolicy(Result);
}

void AAesirBossAIController::StopPossessedBossPolicy(EAesirBossEpisodeResult Result)
{
	if (IsValid(BossPolicyClientComponent)) BossPolicyClientComponent->Stop();
	StopMovement();
	if (GetBrainComponent()) GetBrainComponent()->StopLogic(TEXT("Boss encounter ended"));
	EndBossEpisode(Result);
	if (IsValid(BossRewardComponent)) BossRewardComponent->Shutdown();
	ResetDecisionDebounce();
}

bool AAesirBossAIController::ShouldSuppressRepeatedDecision(
	EAesirBossAction Action,
	AActor* Target,
	float WorldTimeSeconds) const
{
	return bHasLastDecisionRequest &&
		RepeatedDecisionDebounceSeconds > 0.0f &&
		LastDecisionAction == Action &&
		LastDecisionTarget.Get() == Target &&
		WorldTimeSeconds - LastDecisionWorldTimeSeconds <
			RepeatedDecisionDebounceSeconds;
}

void AAesirBossAIController::RememberDecisionRequest(
	const FAesirBossActionOutcome& Outcome)
{
	bHasLastDecisionRequest = true;
	LastDecisionAction = Outcome.Action;
	LastDecisionResult = Outcome.Result;
	LastDecisionTarget = Outcome.Target;
	LastDecisionWorldTimeSeconds = Outcome.WorldTimeSeconds;
}

void AAesirBossAIController::ResetDecisionDebounce()
{
	bHasLastDecisionRequest = false;
	LastDecisionTarget.Reset();
	LastDecisionResult = EAesirBossActionResult::InvalidAvatar;
	LastDecisionWorldTimeSeconds = 0.0f;
}

void AAesirBossAIController::BeginActionFacing(AActor* TargetActor)
{
	if (IsValid(TargetActor))
		SetFocus(TargetActor, EAIFocusPriority::Gameplay);
}

void AAesirBossAIController::LockActionFacing()
{
	ClearFocus(EAIFocusPriority::Gameplay);
}

void AAesirBossAIController::EndActionFacing()
{
	ClearFocus(EAIFocusPriority::Gameplay);
}

FString AAesirBossAIController::StartBossEpisode(
	EAesirBossPolicySource PolicySource,
	const FString& EpisodeId)
{
	if (bWaitingForLevelStart || bMatchCombatEnded || !IsValid(BossTelemetryComponent))
		return FString();

	const FString StartedEpisodeId =
		BossTelemetryComponent->StartEpisode(
			PolicySource,
			EpisodeId);

	if (IsValid(BossRewardComponent))
		BossRewardComponent->BeginEpisode();

	return StartedEpisodeId;
}

FAesirBossEpisodeSummary AAesirBossAIController::EndBossEpisode(
	EAesirBossEpisodeResult Result)
{
	return IsValid(BossTelemetryComponent)
		? BossTelemetryComponent->EndEpisode(Result)
		: FAesirBossEpisodeSummary();
}

void AAesirBossAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ClearLevelStartBinding();
	bWaitingForLevelStart = false;
	bMatchCombatEnded = false;
	ResetDecisionDebounce();
	ActivePolicySource = bUseReinforcementLearningPolicy
		? EAesirBossPolicySource::ReinforcementLearning
		: DefaultPolicySource;

	if (IsValid(BossActionComponent))
		BossActionComponent->InitializeForAvatar(InPawn);
	if (IsValid(BossObservationComponent))
		BossObservationComponent->InitializeForAvatar(InPawn);

	// Main waits for the objective encounter. Combat test maps without a
	// flow director retain immediate startup.
	for (TActorIterator<AAesirLevelFlowDirector> It(GetWorld()); It; ++It)
	{
		bWaitingForLevelStart = true;
		StartupFlowDirector = *It;
		It->OnFlowStateChanged.AddUniqueDynamic(this, &ThisClass::HandleLevelFlowStateChanged);
		HandleLevelFlowStateChanged(EAesirLevelFlowState::Uninitialized, It->GetFlowState());
		return;
	}
	StartPossessedBossPolicy();
}

void AAesirBossAIController::StartPossessedBossPolicy()
{
	APawn* InPawn = GetPawn();
	if (!IsValid(InPawn) || bWaitingForLevelStart)
		return;

	if (IsValid(BossTelemetryComponent))
	{
		BossTelemetryComponent->StartEpisode(
			ActivePolicySource,
			FString());
		if (!bUseReinforcementLearningPolicy)
		{
			BossTelemetryComponent->SetPolicyIdentifier(
				BehaviorTreePolicyIdentifier);
		}
	}
	if (IsValid(BossRewardComponent))
	{
		BossRewardComponent->InitializeForAvatar(
			InPawn,
			BossTelemetryComponent);
	}

	if (bUseReinforcementLearningPolicy &&
		IsValid(BossPolicyClientComponent))
	{
		BossPolicyClientComponent->Initialize(
			this,
			BossObservationComponent);
		BossPolicyClientComponent->Start(
			UGameplayStatics::GetPlayerPawn(this, 0));
	}
	else if (IsValid(BossBehaviorTree))
	{
		RunBehaviorTree(BossBehaviorTree);
	}
}

void AAesirBossAIController::HandleLevelFlowStateChanged(
	EAesirLevelFlowState PreviousState, EAesirLevelFlowState NewState)
{
	if (bMatchCombatEnded) return;
	if (NewState != EAesirLevelFlowState::BossCombat)
	{
		if (!bWaitingForLevelStart)
		{
			bWaitingForLevelStart = true;
			StopPossessedBossPolicy(EAesirBossEpisodeResult::Aborted);
		}
		return;
	}
	const auto* Mode = GetWorld()->GetAuthGameMode<AAesirCombatPrototypeGameMode>();
	if (!bWaitingForLevelStart || !Mode || Mode->GetMatchBoss() != GetPawn())
	{
		return;
	}
	bWaitingForLevelStart = false;
	ActivePolicySource = bUseReinforcementLearningPolicy
		? EAesirBossPolicySource::ReinforcementLearning : DefaultPolicySource;
	StartPossessedBossPolicy();
}

void AAesirBossAIController::ClearLevelStartBinding()
{
	if (AAesirLevelFlowDirector* Flow = StartupFlowDirector.Get())
	{
		Flow->OnFlowStateChanged.RemoveDynamic(this,
			&AAesirBossAIController::HandleLevelFlowStateChanged);
	}
	StartupFlowDirector.Reset();
}

void AAesirBossAIController::OnUnPossess()
{
	ClearLevelStartBinding();
	bWaitingForLevelStart = false;
	if (IsValid(BossPolicyClientComponent))
		BossPolicyClientComponent->Stop();
	if (IsValid(BossTelemetryComponent) &&
		BossTelemetryComponent->IsEpisodeActive())
	{
		BossTelemetryComponent->EndEpisode(
			EAesirBossEpisodeResult::Aborted);
	}
	if (IsValid(BossRewardComponent))
		BossRewardComponent->Shutdown();
	ResetDecisionDebounce();

	Super::OnUnPossess();
}

void AAesirBossAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearLevelStartBinding();
	StopPossessedBossPolicy(EAesirBossEpisodeResult::Aborted);
	Super::EndPlay(EndPlayReason);
}
