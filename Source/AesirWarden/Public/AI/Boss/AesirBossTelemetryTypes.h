#pragma once

#include "CoreMinimal.h"
#include "AI/Boss/AesirBossDecisionTypes.h"
#include "AI/Boss/AesirBossObservationTypes.h"
#include "AesirBossTelemetryTypes.generated.h"

UENUM(BlueprintType)
enum class EAesirBossPolicySource : uint8
{
	BehaviorTree,
	ReinforcementLearning,
	Fallback
};

UENUM(BlueprintType)
enum class EAesirBossEpisodeResult : uint8
{
	InProgress,
	BossVictory,
	BossDefeat,
	Timeout,
	Aborted
};

UENUM(BlueprintType)
enum class EAesirBossRewardReason : uint8
{
	DecisionStep,
	DamageDealt,
	DamageReceived,
	TargetDefeated,
	BossDefeated,
	RejectedAction,
	RepeatedAction,
	SuccessfulDefend
};

USTRUCT(BlueprintType)
struct AESIRWARDEN_API FAesirBossRewardEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	int64 DecisionIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	EAesirBossRewardReason Reason =
		EAesirBossRewardReason::DecisionStep;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	float SourceMagnitude = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	float Reward = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	float WorldTimeSeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct AESIRWARDEN_API FAesirBossDecisionRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	int64 DecisionIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	EAesirBossPolicySource PolicySource =
		EAesirBossPolicySource::BehaviorTree;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	FAesirBossObservation Observation;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	FAesirBossActionOutcome Outcome;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	float PolicyEvaluationMilliseconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	float ActionRequestMilliseconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	float Reward = 0.0f;
};

USTRUCT(BlueprintType)
struct AESIRWARDEN_API FAesirBossEpisodeSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	FString EpisodeId;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	EAesirBossPolicySource PolicySource =
		EAesirBossPolicySource::BehaviorTree;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	FString PolicyIdentifier;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	EAesirBossEpisodeResult Result =
		EAesirBossEpisodeResult::InProgress;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	float DurationSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	int32 DecisionCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	int32 AcceptedActionCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	int32 RejectedActionCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	float TotalReward = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	float DamageDealt = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	float DamageReceived = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	int32 RejectedRewardCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Telemetry")
	int32 RepeatedActionPenaltyCount = 0;
};
