#pragma once

#include "CoreMinimal.h"
#include "TacticalOrderTypes.generated.h"

UENUM(BlueprintType)
enum class ETacticalOrderIntent : uint8
{
	Unknown,
	ConditionalCast,
	HoldAbility,
	PrioritizeAttack,
	FollowKeepDistance,
	Retreat
};

UENUM(BlueprintType)
enum class ETacticalWhenType : uint8
{
	None,
	StateEntered
};

UENUM(BlueprintType)
enum class ETacticalThenType : uint8
{
	Unknown,
	CastAbility,
	HoldAbility,
	SetPriority,
	Follow,
	Retreat
};

UENUM(BlueprintType)
enum class ETacticalTargetType : uint8
{
	None,
	Selector,
	Reference
};

UENUM(BlueprintType)
enum class ETacticalExpiresType : uint8
{
	Unknown,
	EncounterEnd
};

USTRUCT(BlueprintType)
struct FTacticalTarget
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	ETacticalTargetType Type = ETacticalTargetType::None;

	// 例如 party.player
	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	FString Selector;

	// 例如 when.subject
	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	FString Reference;
};

USTRUCT(BlueprintType)
struct FTacticalWhen
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	ETacticalWhenType Type = ETacticalWhenType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	FString Subject;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	FString Tag;
};

USTRUCT(BlueprintType)
struct FTacticalThen
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	ETacticalThenType Type = ETacticalThenType::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	FString AbilityId;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	FTacticalTarget Target;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	bool bActive = true;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	FString Mode;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	bool bKeepDistance = true;
};

USTRUCT(BlueprintType)
struct FTacticalExpires
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	ETacticalExpiresType Type = ETacticalExpiresType::Unknown;
};

USTRUCT(BlueprintType)
struct FTacticalOrder
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	FString OrderId;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	FString AgentId;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	ETacticalOrderIntent Intent = ETacticalOrderIntent::Unknown;

	// false 对应 JSON 中的 "when": null
	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	bool bHasWhen = false;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	FTacticalWhen When;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	FTacticalThen Then;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	int32 Priority = 50;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|Tactical Order")
	FTacticalExpires Expires;
};