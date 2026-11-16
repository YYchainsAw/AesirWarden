// Copyright (c) 2026 YYchainsAw. All rights reserved.

#include "Framework/AesirCombatPrototypeGameMode.h"
#include "AesirWarden.h"
#include "AI/Boss/AesirBossAIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Characters/Enemy/AesirEnemyCharacter.h"
#include "Combat/AesirCombatComponent.h"
#include "Combat/AesirHealthComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Framework/AesirCombatPrototypePlayerController.h"
#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"

AAesirCombatPrototypeGameMode::AAesirCombatPrototypeGameMode() {}

void AAesirCombatPrototypeGameMode::BeginPlay()
{
	Super::BeginPlay();
	RegisterMatchPlayer(UGameplayStatics::GetPlayerPawn(this, 0));
	DiscoverTaggedBoss();
	GetWorldTimerManager().SetTimer(BossEncounterTimer, this,
		&AAesirCombatPrototypeGameMode::MonitorBossEncounter, 0.2f, true);
}

bool AAesirCombatPrototypeGameMode::RegisterMatchPlayer(APawn* Player)
{
	if (MatchResult != EAesirMatchResult::None || bResultResolutionQueued || !IsValid(Player) || Player->GetWorld() != GetWorld()) return false;
	auto* Health = Player->FindComponentByClass<UAesirHealthComponent>();
	if (!IsValid(Health)) return false;
	if (MatchPlayer.Get() == Player && PlayerHealth.Get() == Health) return true;
	if (auto* Old = PlayerHealth.Get()) Old->OnDeath.RemoveDynamic(this, &ThisClass::HandlePlayerDeath);
	MatchPlayer = Player;
	PlayerHealth = Health;
	Health->OnDeath.AddUniqueDynamic(this, &ThisClass::HandlePlayerDeath);
	return true;
}

bool AAesirCombatPrototypeGameMode::RegisterMatchBoss(AActor* Boss)
{
	if (MatchResult != EAesirMatchResult::None || bResultResolutionQueued ||
		!IsValid(Cast<AAesirEnemyCharacter>(Boss)) || Boss->GetWorld() != GetWorld()) return false;
	auto* Health = Boss->FindComponentByClass<UAesirHealthComponent>();
	if (!IsValid(Health) || Health->IsDead()) return false;
	if (MatchBoss.Get() == Boss && BossHealth.Get() == Health) return true;
	if (MatchBoss.IsValid()) return false;
	MatchBoss = Boss;
	BossHealth = Health;
	Health->OnDeath.AddUniqueDynamic(this, &ThisClass::HandleEnemyDeath);
	LastBossHealth = Health->GetCurrentHealth();
	RemainingEnemies = 1; // Retained for existing Blueprint diagnostics, never an enemy counter.
	UE_LOG(LogAesirWarden, Display, TEXT("Match objective registered: boss=%s"), *Boss->GetPathName());
	return true;
}

bool AAesirCombatPrototypeGameMode::UnregisterMatchBoss(AActor* Boss)
{
	if (MatchResult != EAesirMatchResult::None || bResultResolutionQueued || !Boss || MatchBoss.Get() != Boss) return false;
	if (auto* Health = BossHealth.Get()) Health->OnDeath.RemoveDynamic(this, &ThisClass::HandleEnemyDeath);
	MatchBoss.Reset(); BossHealth.Reset(); RemainingEnemies = 0;
	return true;
}

void AAesirCombatPrototypeGameMode::DiscoverTaggedBoss()
{
	if (MatchBoss.IsValid() || MatchResult != EAesirMatchResult::None) return;
	AAesirEnemyCharacter* Candidate = nullptr;
	for (TActorIterator<AAesirEnemyCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("Aesir.MatchBoss"))) continue;
		if (Candidate) return; // Ambiguous authoring must not select an arbitrary actor.
		Candidate = *It;
	}
	if (Candidate) RegisterMatchBoss(Candidate);
}

void AAesirCombatPrototypeGameMode::MonitorBossEncounter()
{
	if (MatchResult != EAesirMatchResult::None) return;
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Player && MatchPlayer.Get() != Player) RegisterMatchPlayer(Player);
	DiscoverTaggedBoss();
	AAesirLevelFlowDirector* Flow = nullptr;
	for (TActorIterator<AAesirLevelFlowDirector> It(GetWorld()); It; ++It) { Flow = *It; break; }
	if (!Flow) return;
	AActor* Boss = MatchBoss.Get();
	const auto* TargetHealth = BossHealth.Get();
	if (!Boss || !TargetHealth || TargetHealth->IsDead()) return;
	const bool bBossDamaged = TargetHealth->GetCurrentHealth() < LastBossHealth;
	LastBossHealth = TargetHealth->GetCurrentHealth();
	// Only damage to the objective starts the compatibility handoff. Damage
	// from an unrelated nearby enemy must not start the Boss encounter.
	if (Flow->GetFlowState() == EAesirLevelFlowState::Exploration && bBossDamaged)
		Flow->BeginBossCombat(Boss);
}

void AAesirCombatPrototypeGameMode::HandlePlayerDeath(AController*, AActor*)
{
	if (PlayerHealth.IsValid() && PlayerHealth->IsDead()) QueueMatchResult(true);
}

void AAesirCombatPrototypeGameMode::HandleEnemyDeath(AController*, AActor*)
{
	if (BossHealth.IsValid() && BossHealth->IsDead()) QueueMatchResult(false);
}

void AAesirCombatPrototypeGameMode::QueueMatchResult(bool bPlayerDied)
{
	if (MatchResult != EAesirMatchResult::None || (bResultResolutionQueued && GFrameCounter != PendingResultFrame)) return;
	if (bPlayerDied) bPendingPlayerDeath = true; else bPendingBossDeath = true;
	if (bResultResolutionQueued) return;
	bResultResolutionQueued = true;
	PendingResultFrame = GFrameCounter;
	ResultResolutionTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::ResolvePendingMatchResult);
}

void AAesirCombatPrototypeGameMode::ResolvePendingMatchResult()
{
	if (!bResultResolutionQueued || MatchResult != EAesirMatchResult::None) return;
	// TimerManager can still tick later in the death frame. Wait until all
	// callbacks from that frame have arrived, regardless of their order.
	if (GFrameCounter <= PendingResultFrame)
	{
		ResultResolutionTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::ResolvePendingMatchResult);
		return;
	}
	bResultResolutionQueued = false;
	if (bPendingPlayerDeath)
	{
		FinishCombat(EAesirMatchResult::Defeat);
	}
	else if (bPendingBossDeath)
	{
		FinishCombat(EAesirMatchResult::Victory);
	}
}

void AAesirCombatPrototypeGameMode::StopMatchCombat()
{
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* Pawn = *It;
		if (auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn)) ASC->CancelAllAbilities();
		if (auto* Combat = Pawn->FindComponentByClass<UAesirCombatComponent>()) Combat->CancelAttack();
		if (auto* Health = Pawn->FindComponentByClass<UAesirHealthComponent>()) Health->SetInvulnerable(true);
		if (auto* Character = Cast<ACharacter>(Pawn))
		{
			Character->GetCharacterMovement()->StopMovementImmediately();
			Character->GetCharacterMovement()->DisableMovement();
		}
		if (auto* Boss = Cast<AAesirBossAIController>(Pawn->GetController()))
		{
			const auto Result = Pawn == MatchBoss.Get()
				? (MatchResult == EAesirMatchResult::Victory ? EAesirBossEpisodeResult::BossDefeat : EAesirBossEpisodeResult::BossVictory)
				: EAesirBossEpisodeResult::Aborted;
			Boss->StopBossForMatchResult(Result);
		}
		else if (auto* AI = Cast<AAIController>(Pawn->GetController()))
		{
			AI->StopMovement();
			if (AI->GetBrainComponent()) AI->GetBrainComponent()->StopLogic(TEXT("Match ended"));
		}
	}
}

void AAesirCombatPrototypeGameMode::FinishCombat(EAesirMatchResult NewResult)
{
	if (MatchResult != EAesirMatchResult::None || NewResult == EAesirMatchResult::None) return;
	MatchResult = NewResult; // Set the terminal guard before cancellation can dispatch callbacks.
	GetWorldTimerManager().ClearTimer(BossEncounterTimer);
	UnbindMatchDeaths();
	StopMatchCombat();
	for (TActorIterator<AAesirLevelFlowDirector> It(GetWorld()); It; ++It) { It->EndBossCombat(false); break; }
	UE_LOG(LogAesirWarden, Display, TEXT("Match ended once: result=%s boss=%s frame=%llu"),
		*UEnum::GetValueAsString(MatchResult), *GetNameSafe(MatchBoss.Get()), GFrameCounter);
	OnCombatEnded(MatchResult); // Existing Blueprint presentation remains the authored owner.
	if (auto* PC = Cast<AAesirCombatPrototypePlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
		PC->PresentCombatResult(MatchResult);
	OnMatchFinished.Broadcast(MatchResult);
}

bool AAesirCombatPrototypeGameMode::RequestReload(bool bRetry)
{
	if (MatchResult == EAesirMatchResult::None || bTravelRequested) return false;
	UWorld* World = GetWorld();
	if (!World || !GEngine) return false;
	const FString Level = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
	if (!FPackageName::IsValidLongPackageName(Level) || !FPackageName::DoesPackageExist(Level))
	{
		UE_LOG(LogAesirWarden, Warning, TEXT("Match reload: current map package %s is unavailable."), *Level);
		return false;
	}
	bTravelRequested = true;
	TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleMatchTravelFailure);
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, FName(*Level), true, bRetry ? TEXT("AesirRetry=1") : TEXT(""));
	return true;
}

void AAesirCombatPrototypeGameMode::HandleMatchTravelFailure(
	UWorld* FailedWorld, ETravelFailure::Type FailureType, const FString& Error)
{
	if (FailedWorld != GetWorld() || !bTravelRequested) return;
	bTravelRequested = false;
	GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	TravelFailureHandle.Reset();
	UE_LOG(LogAesirWarden, Warning, TEXT("Match reload failed (%d): %s"), static_cast<int32>(FailureType), *Error);
}

bool AAesirCombatPrototypeGameMode::RequestRetryMatch() { return RequestReload(true); }
bool AAesirCombatPrototypeGameMode::RequestReturnToMenu() { return RequestReload(false); }
bool AAesirCombatPrototypeGameMode::RequestQuitMatch()
{
	if (MatchResult == EAesirMatchResult::None || bTravelRequested) return false;
	bTravelRequested = true;
	UKismetSystemLibrary::QuitGame(this, UGameplayStatics::GetPlayerController(this, 0), EQuitPreference::Quit, false);
	return true;
}

void AAesirCombatPrototypeGameMode::UnbindMatchDeaths()
{
	if (auto* Health = PlayerHealth.Get(true)) Health->OnDeath.RemoveDynamic(this, &ThisClass::HandlePlayerDeath);
	if (auto* Health = BossHealth.Get(true)) Health->OnDeath.RemoveDynamic(this, &ThisClass::HandleEnemyDeath);
}

void AAesirCombatPrototypeGameMode::ClearMatchBindings()
{
	UnbindMatchDeaths();
	PlayerHealth.Reset(); BossHealth.Reset(); MatchPlayer.Reset(); MatchBoss.Reset();
}

void AAesirCombatPrototypeGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GEngine && TravelFailureHandle.IsValid())
	{
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
		TravelFailureHandle.Reset();
	}
	GetWorldTimerManager().ClearTimer(BossEncounterTimer);
	GetWorldTimerManager().ClearTimer(ResultResolutionTimer);
	ClearMatchBindings();
	OnMatchFinished.Clear();
	Super::EndPlay(EndPlayReason);
}
