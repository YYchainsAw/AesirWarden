// Copyright © 2026 YYchainsAw. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/Boss/AesirBossDecisionTypes.h"
#include "AI/Boss/AesirBossTelemetryTypes.h"
#include "AesirBossAIController.generated.h"

class UBehaviorTree;
class UAesirBossActionComponent;
class UAesirBossObservationComponent;
class UAesirBossPolicyClientComponent;
class UAesirBossRewardComponent;
class UAesirBossTelemetryComponent;

UCLASS()
class AESIRWARDEN_API AAesirBossAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAesirBossAIController();

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss")
	UAesirBossActionComponent* GetBossActionComponent() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss")
	UAesirBossObservationComponent*
	GetBossObservationComponent() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss")
	UAesirBossTelemetryComponent* GetBossTelemetryComponent() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss")
	UAesirBossRewardComponent* GetBossRewardComponent() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss")
	UAesirBossPolicyClientComponent* GetBossPolicyClientComponent() const;

	UFUNCTION(BlueprintCallable, Category = "Aesir|AI|Boss")
	FAesirBossActionOutcome ExecuteBossDecision(
		EAesirBossAction Action,
		AActor* Target,
		float PolicyEvaluationMilliseconds = 0.0f);

	FAesirBossActionOutcome ExecuteBossPolicyDecision(
		EAesirBossAction Action,
		AActor* Target,
		float PolicyEvaluationMilliseconds,
		const FAesirBossObservation& Observation);

	void BeginActionFacing(AActor* TargetActor);
	void LockActionFacing();
	void EndActionFacing();

	UFUNCTION(BlueprintCallable, Category = "Aesir|AI|Boss|Telemetry")
	FString StartBossEpisode(
		EAesirBossPolicySource PolicySource,
		const FString& EpisodeId);

	UFUNCTION(BlueprintCallable, Category = "Aesir|AI|Boss|Telemetry")
	FAesirBossEpisodeSummary EndBossEpisode(
		EAesirBossEpisodeResult Result);

	void ActivateBehaviorTreeFallback(const FString& Reason);
	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	bool ShouldSuppressRepeatedDecision(
		EAesirBossAction Action,
		AActor* Target,
		float WorldTimeSeconds) const;
	void RememberDecisionRequest(
		const FAesirBossActionOutcome& Outcome);
	void ResetDecisionDebounce();
	FAesirBossActionOutcome ExecuteBossDecisionInternal(
		EAesirBossAction Action,
		AActor* Target,
		float PolicyEvaluationMilliseconds,
		const FAesirBossObservation& Observation);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "Aesir|AI|Boss",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAesirBossActionComponent> BossActionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "Aesir|AI|Boss",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAesirBossObservationComponent>
		BossObservationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "Aesir|AI|Boss",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAesirBossTelemetryComponent> BossTelemetryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "Aesir|AI|Boss",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAesirBossRewardComponent> BossRewardComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "Aesir|AI|Boss",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAesirBossPolicyClientComponent> BossPolicyClientComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI|Boss|Policy")
	bool bUseReinforcementLearningPolicy = false;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI|Boss|Policy")
	FString BehaviorTreePolicyIdentifier = TEXT("bt_baseline_v1");

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI|Boss")
	EAesirBossPolicySource DefaultPolicySource =
		EAesirBossPolicySource::BehaviorTree;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI")
	TObjectPtr<UBehaviorTree> BossBehaviorTree;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI|Boss|Decision",
		meta = (ClampMin = "0.0"))
	float RepeatedDecisionDebounceSeconds = 0.25f;

	TWeakObjectPtr<AActor> LastDecisionTarget;
	EAesirBossAction LastDecisionAction = EAesirBossAction::Pursue;
	EAesirBossActionResult LastDecisionResult =
		EAesirBossActionResult::InvalidAvatar;
	EAesirBossPolicySource ActivePolicySource =
		EAesirBossPolicySource::BehaviorTree;
	float LastDecisionWorldTimeSeconds = 0.0f;
	bool bHasLastDecisionRequest = false;
};
