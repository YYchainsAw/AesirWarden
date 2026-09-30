#pragma once

#include "CoreMinimal.h"
#include "AesirBossDecisionTypes.generated.h"

UENUM(BlueprintType)
enum class EAesirBossAction : uint8
{
	LightAttack = 0,
	HeavyAttack = 1,
	Defend = 2,
	Dodge = 3,
	Pursue = 4,
	Disengage = 5,
	UseAbility = 6,
	GapCloserSkill = 7,
	UnblockableAreaSkill = 8
};

UENUM(BlueprintType)
enum class EAesirBossActionResult : uint8
{
	Accepted,
	InvalidAvatar,
	InvalidTarget,
	NotAuthority,
	Dead,
	Stunned,
	Busy,
	MissingAbilitySystem,
	NotConfigured,
	AbilityNotGranted,
	BlockedByGAS
};

USTRUCT(BlueprintType)
struct AESIRWARDEN_API FAesirBossActionOutcome
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss")
	EAesirBossAction Action = EAesirBossAction::LightAttack;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss")
	EAesirBossActionResult Result =
		EAesirBossActionResult::InvalidAvatar;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss")
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Aesir|AI|Boss")
	float WorldTimeSeconds = 0.0f;

	bool WasAccepted() const
	{
		return Result == EAesirBossActionResult::Accepted;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnAesirBossActionResolved,
	FAesirBossActionOutcome,
	Outcome);
