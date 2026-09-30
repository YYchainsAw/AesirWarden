#include "AI/Boss/AesirBossObservationComponent.h"

#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AI/Boss/AesirBossActionComponent.h"
#include "Abilities/AesirAttributeSet.h"
#include "Abilities/AesirPlayerAttributeSet.h"
#include "Characters/Enemy/AesirEnemyCharacter.h"
#include "Tags/AesirGameplayTags.h"
#include "Combat/AesirCombatComponent.h"
#include "Combat/AesirPoiseComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

UAesirBossObservationComponent::UAesirBossObservationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAesirBossObservationComponent::InitializeForAvatar(
	APawn* InAvatar)
{
	Avatar = InAvatar;
	LastObservation = FAesirBossObservation();
	NextSequence = 1;
	RecentTargetAttackSamples.Reset();
	RecentTargetBlockSamples.Reset();
	RecentTargetDodgeSamples.Reset();
	PreviousDistanceToTarget = 0.0f;
	bHasPreviousDistance = false;
}

FAesirBossObservation
UAesirBossObservationComponent::CaptureObservation(AActor* Target)
{
	FAesirBossObservation Observation;
	Observation.Sequence = NextSequence++;

	if (const UWorld* World = GetWorld())
		Observation.WorldTimeSeconds = World->GetTimeSeconds();

	APawn* ControlledAvatar = Avatar.Get();
	if (!IsValid(ControlledAvatar) || !IsValid(Target))
	{
		LastObservation = Observation;
		return LastObservation;
	}

	Observation.bValid = true;
	TryReadHealth(
		ControlledAvatar,
		Observation.BossHealth,
		Observation.BossMaxHealth);
	TryReadHealth(
		Target,
		Observation.TargetHealth,
		Observation.TargetMaxHealth);

	Observation.BossHealthRatio = Observation.BossMaxHealth > 0.0f
		? FMath::Clamp(
			Observation.BossHealth / Observation.BossMaxHealth,
			0.0f,
			1.0f)
		: 0.0f;
	Observation.TargetHealthRatio = Observation.TargetMaxHealth > 0.0f
		? FMath::Clamp(
			Observation.TargetHealth / Observation.TargetMaxHealth,
			0.0f,
			1.0f)
		: 0.0f;

	const FVector ToTarget =
		Target->GetActorLocation() - ControlledAvatar->GetActorLocation();
	Observation.DistanceToTarget = ToTarget.Size();
	if (bHasPreviousDistance)
	{
		const float DistanceDelta =
			Observation.DistanceToTarget - PreviousDistanceToTarget;
		Observation.DistanceTrend = FMath::Clamp(
			0.5f + DistanceDelta /
				(2.0f * FMath::Max(DistanceTrendScale, 1.0f)),
			0.0f,
			1.0f);
	}
	PreviousDistanceToTarget = Observation.DistanceToTarget;
	bHasPreviousDistance = true;
	Observation.NormalizedDistance = FMath::Clamp(
		Observation.DistanceToTarget /
		FMath::Max(MaxRelevantDistance, 1.0f),
		0.0f,
		1.0f);

	const FVector FlatDirection =
		FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal();
	const FVector FlatForward = FVector(
		ControlledAvatar->GetActorForwardVector().X,
		ControlledAvatar->GetActorForwardVector().Y,
		0.0f).GetSafeNormal();
	const float FacingDot = FlatDirection.IsNearlyZero()
		? 1.0f
		: FVector::DotProduct(FlatForward, FlatDirection);
	Observation.FacingAlignment = FMath::Clamp(
		(FacingDot + 1.0f) * 0.5f,
		0.0f,
		1.0f);

	if (AAIController* AIController =
		Cast<AAIController>(GetOwner()))
	{
		Observation.bHasLineOfSight =
			AIController->LineOfSightTo(Target);
	}

	if (const AAesirEnemyCharacter* EnemyAvatar =
		Cast<AAesirEnemyCharacter>(ControlledAvatar))
	{
		Observation.bBossStunned = EnemyAvatar->IsStunned();
		Observation.bBossAttacking = EnemyAvatar->IsAttacking();
	}

	if (const UAesirPoiseComponent* PoiseComponent =
		ControlledAvatar->FindComponentByClass<UAesirPoiseComponent>())
	{
		Observation.BossPoiseRatio = PoiseComponent->GetMaxPoise() > 0.0f
			? FMath::Clamp(
				PoiseComponent->GetCurrentPoise() /
					PoiseComponent->GetMaxPoise(),
				0.0f,
				1.0f)
			: 0.0f;
	}

	if (const UAbilitySystemComponent* TargetAbilitySystem =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
	{
		Observation.bTargetBlocking =
			TargetAbilitySystem->HasMatchingGameplayTag(
				AesirGameplayTags::State_Blocking);
		Observation.bTargetDodging =
			TargetAbilitySystem->HasMatchingGameplayTag(
				AesirGameplayTags::State_Dodging);

		if (const UAesirPlayerAttributeSet* PlayerAttributes =
			TargetAbilitySystem->GetSet<UAesirPlayerAttributeSet>())
		{
			Observation.TargetGuardPressureRatio =
				PlayerAttributes->GetMaxGuardPressure() > 0.0f
					? FMath::Clamp(
						PlayerAttributes->GetGuardPressure() /
							PlayerAttributes->GetMaxGuardPressure(),
						0.0f,
						1.0f)
					: 0.0f;
		}
	}

	if (const UAesirCombatComponent* TargetCombatComponent =
		Target->FindComponentByClass<UAesirCombatComponent>())
	{
		Observation.bTargetAttacking =
			TargetCombatComponent->IsAttacking();
	}

	Observation.bTargetDead =
		Observation.TargetMaxHealth > 0.0f &&
		Observation.TargetHealth <= 0.0f;

	PushRecentSample(
		RecentTargetAttackSamples,
		Observation.bTargetAttacking);
	PushRecentSample(
		RecentTargetBlockSamples,
		Observation.bTargetBlocking);
	PushRecentSample(
		RecentTargetDodgeSamples,
		Observation.bTargetDodging);
	Observation.RecentTargetAttackRate =
		CalculateRecentRate(RecentTargetAttackSamples);
	Observation.RecentTargetBlockRate =
		CalculateRecentRate(RecentTargetBlockSamples);
	Observation.RecentTargetDodgeRate =
		CalculateRecentRate(RecentTargetDodgeSamples);

	if (const UAesirBossActionComponent* ActionComponent =
		GetOwner()->FindComponentByClass<UAesirBossActionComponent>())
	{
		Observation.bLightAttackAvailable =
			ActionComponent->CanExecuteAction(
				EAesirBossAction::LightAttack,
				Target);
		Observation.bHeavyAttackAvailable =
			ActionComponent->CanExecuteAction(
				EAesirBossAction::HeavyAttack,
				Target);
		Observation.bDefendAvailable =
			ActionComponent->CanExecuteAction(
				EAesirBossAction::Defend,
				Target);
		Observation.bDodgeAvailable =
			ActionComponent->CanExecuteAction(
				EAesirBossAction::Dodge,
				Target);
		Observation.bPursueAvailable =
			ActionComponent->CanExecuteAction(
				EAesirBossAction::Pursue,
				Target);
		Observation.bDisengageAvailable =
			ActionComponent->CanExecuteAction(
				EAesirBossAction::Disengage,
				Target);
		Observation.bUseAbilityAvailable =
			ActionComponent->CanExecuteAction(
				EAesirBossAction::UseAbility,
				Target);
		Observation.bGapCloserSkillAvailable =
			ActionComponent->CanExecuteAction(
				EAesirBossAction::GapCloserSkill,
				Target);
		Observation.bUnblockableAreaSkillAvailable =
			ActionComponent->CanExecuteAction(
				EAesirBossAction::UnblockableAreaSkill,
				Target);
	}

	LastObservation = Observation;
	return LastObservation;
}

FAesirBossObservation
UAesirBossObservationComponent::GetLastObservation() const
{
	return LastObservation;
}

TArray<float>
UAesirBossObservationComponent::GetLastFeatureVector() const
{
	return LastObservation.ToFeatureVector();
}

bool UAesirBossObservationComponent::TryReadHealth(
	AActor* Actor,
	float& OutHealth,
	float& OutMaxHealth)
{
	OutHealth = 0.0f;
	OutMaxHealth = 0.0f;

	const UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(
			Actor);
	const UAesirAttributeSet* Attributes = AbilitySystem
		? AbilitySystem->GetSet<UAesirAttributeSet>()
		: nullptr;

	if (!Attributes)
		return false;

	OutHealth = Attributes->GetHealth();
	OutMaxHealth = Attributes->GetMaxHealth();
	return true;
}

void UAesirBossObservationComponent::PushRecentSample(
	TArray<uint8>& Samples,
	bool bValue)
{
	Samples.Add(bValue ? 1 : 0);
	const int32 MaxSamples = FMath::Max(RecentBehaviorWindow, 1);
	if (Samples.Num() > MaxSamples)
	{
		Samples.RemoveAt(0, Samples.Num() - MaxSamples, EAllowShrinking::No);
	}
}

float UAesirBossObservationComponent::CalculateRecentRate(
	const TArray<uint8>& Samples)
{
	if (Samples.IsEmpty())
		return 0.0f;

	int32 ActiveSampleCount = 0;
	for (const uint8 Sample : Samples)
		ActiveSampleCount += Sample != 0 ? 1 : 0;

	return static_cast<float>(ActiveSampleCount) /
		static_cast<float>(Samples.Num());
}
