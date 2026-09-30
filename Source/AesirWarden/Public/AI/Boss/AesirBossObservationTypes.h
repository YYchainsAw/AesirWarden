#pragma once

#include "CoreMinimal.h"
#include "AesirBossObservationTypes.generated.h"

USTRUCT(BlueprintType)
struct AESIRWARDEN_API FAesirBossObservation
{
	GENERATED_BODY()

	static constexpr int32 CurrentSchemaVersion = 4;
	static constexpr int32 FeatureCount = 26;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	int32 SchemaVersion = CurrentSchemaVersion;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	int64 Sequence = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float WorldTimeSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	bool bValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float BossHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float BossMaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float BossHealthRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float TargetHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float TargetMaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float TargetHealthRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float DistanceToTarget = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float NormalizedDistance = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float FacingAlignment = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	bool bHasLineOfSight = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	bool bBossStunned = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	bool bBossAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	bool bTargetBlocking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	bool bTargetAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	bool bTargetDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float BossPoiseRatio = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float TargetGuardPressureRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	bool bTargetDodging = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float RecentTargetAttackRate = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float RecentTargetBlockRate = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float RecentTargetDodgeRate = 0.0f;

	/** 0 means rapidly approaching, 0.5 stable, 1 means moving away. */
	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation")
	float DistanceTrend = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation|Availability")
	bool bLightAttackAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation|Availability")
	bool bHeavyAttackAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation|Availability")
	bool bDefendAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation|Availability")
	bool bDodgeAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation|Availability")
	bool bPursueAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation|Availability")
	bool bDisengageAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation|Availability")
	bool bUseAbilityAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation|Availability")
	bool bGapCloserSkillAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss|Observation|Availability")
	bool bUnblockableAreaSkillAvailable = false;

	/** Stable schema-v4 order used by both training and runtime inference. */
	TArray<float> ToFeatureVector() const;
};
