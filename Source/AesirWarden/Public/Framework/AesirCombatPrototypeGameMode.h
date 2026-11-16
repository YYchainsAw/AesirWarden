// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/EngineBaseTypes.h"
#include "TimerManager.h"
#include "AesirCombatPrototypeGameMode.generated.h"

UENUM(BlueprintType)
enum class EAesirMatchResult : uint8
{
	None,
	Victory,
	Defeat
};

DECLARE_MULTICAST_DELEGATE_OneParam(FAesirMatchFinished, EAesirMatchResult);

class UAesirHealthComponent;

UCLASS(abstract)
class AAesirCombatPrototypeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AAesirCombatPrototypeGameMode();

	UFUNCTION(BlueprintPure, Category="Aesir|Match")
	EAesirMatchResult GetMatchResult() const { return MatchResult; }

	/** Explicitly select the single objective; ordinary enemy deaths are irrelevant. */
	UFUNCTION(BlueprintCallable, Category="Aesir|Match")
	bool RegisterMatchBoss(AActor* Boss);
	UFUNCTION(BlueprintCallable, Category="Aesir|Match")
	bool UnregisterMatchBoss(AActor* Boss);
	UFUNCTION(BlueprintPure, Category="Aesir|Match")
	AActor* GetMatchBoss() const { return MatchBoss.Get(); }
	bool IsMatchPlayer(const AActor* Actor) const { return Actor && MatchPlayer.Get() == Actor; }
	bool RegisterMatchPlayer(APawn* Player);
	UFUNCTION(BlueprintCallable, Category="Aesir|Match")
	bool RequestRetryMatch();
	UFUNCTION(BlueprintCallable, Category="Aesir|Match")
	bool RequestReturnToMenu();
	UFUNCTION(BlueprintCallable, Category="Aesir|Match")
	bool RequestQuitMatch();
	FAesirMatchFinished OnMatchFinished;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandlePlayerDeath(
		AController* InstigatedBy,
		AActor* DamageCauser);

	UFUNCTION()
	void HandleEnemyDeath(
		AController* InstigatedBy,
		AActor* DamageCauser);

	void FinishCombat(EAesirMatchResult NewResult);

	UFUNCTION(BlueprintImplementableEvent, Category="Aesir|Match")
	void OnCombatEnded(EAesirMatchResult Result);
	
private:
	void QueueMatchResult(bool bPlayerDied);
	void ResolvePendingMatchResult();
	void StopMatchCombat();
	bool RequestReload(bool bRetry);
	void HandleMatchTravelFailure(UWorld* FailedWorld, ETravelFailure::Type FailureType, const FString& Error);
	void DiscoverTaggedBoss();
	void UnbindMatchDeaths();
	void ClearMatchBindings();
	TWeakObjectPtr<AActor> MatchBoss;
	TWeakObjectPtr<UAesirHealthComponent> BossHealth;
	TWeakObjectPtr<APawn> MatchPlayer;
	TWeakObjectPtr<UAesirHealthComponent> PlayerHealth;
	FTimerHandle ResultResolutionTimer;
	FDelegateHandle TravelFailureHandle;
	uint64 PendingResultFrame = 0;
	bool bPendingPlayerDeath = false;
	bool bPendingBossDeath = false;
	bool bResultResolutionQueued = false;
	bool bTravelRequested = false;
	void MonitorBossEncounter();
	FTimerHandle BossEncounterTimer;
	float LastBossHealth = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
	Category="Aesir|Match",
	meta=(AllowPrivateAccess="true"))
	int32 RemainingEnemies = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
		Category="Aesir|Match",
		meta=(AllowPrivateAccess="true"))
	EAesirMatchResult MatchResult = EAesirMatchResult::None;
};



