#pragma once

#include "CoreMinimal.h"
#include "CombatContextTypes.generated.h"

UENUM(BlueprintType)
enum class EAbilityAvailability : uint8
{
	Ready,
	Cooldown,
	Unavailable,
	Blocked
};

USTRUCT(BlueprintType)
struct FCombatContextPlayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id = TEXT("player.main");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HpPercent = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDowned = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DistanceToBossMeters = 0.0f;
};

USTRUCT(BlueprintType)
struct FCombatContextCompanion
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id = TEXT("companion.alice");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HpPercent = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MpPercent = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CurrentBehavior;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, EAbilityAvailability> AbilityStates;
};

USTRUCT(BlueprintType)
struct FCombatContextBoss
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id = TEXT("encounter.primary_hostile");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HpPercent = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StunPercent = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> StateTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasStunnedRemainingSeconds = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StunnedRemainingSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Phase = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsEnraged = false;
};

USTRUCT(BlueprintType)
struct FCombatContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString EncounterId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SnapshotId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CapturedAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCombatContextPlayer Player;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCombatContextCompanion Companion;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCombatContextBoss Boss;
};