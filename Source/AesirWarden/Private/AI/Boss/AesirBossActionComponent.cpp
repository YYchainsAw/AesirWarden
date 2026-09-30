#include "AI/Boss/AesirBossActionComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Characters/Enemy/AesirEnemyCharacter.h"
#include "Engine/World.h"
#include "GameplayAbilitySpec.h"

UAesirBossActionComponent::UAesirBossActionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAesirBossActionComponent::InitializeForAvatar(APawn* InAvatar)
{
	Avatar = InAvatar;
	CurrentActionTarget = nullptr;

	if (UAbilitySystemComponent* AbilitySystem = ResolveAbilitySystem())
	{
		AbilitySystem->InitAbilityActorInfo(InAvatar, InAvatar);
	}

	GrantConfiguredAbilities();
}

FAesirBossActionOutcome UAesirBossActionComponent::TryExecuteAction(
	EAesirBossAction Action,
	AActor* Target)
{
	auto Resolve = [this, Action, Target](
		EAesirBossActionResult Result)
	{
		const FAesirBossActionOutcome Outcome =
			MakeOutcome(Action, Result, Target);
		PublishOutcome(Outcome);
		return Outcome;
	};

	const EAesirBossActionResult ValidationResult =
		ValidateAction(Action, Target);
	if (ValidationResult != EAesirBossActionResult::Accepted)
		return Resolve(ValidationResult);

	UAbilitySystemComponent* AbilitySystem = ResolveAbilitySystem();
	const TSubclassOf<UGameplayAbility>* AbilityClass =
		ActionAbilities.Find(Action);

	CurrentActionTarget = Target;

	if (!AbilitySystem->TryActivateAbilityByClass(*AbilityClass))
	{
		CurrentActionTarget = nullptr;
		return Resolve(EAesirBossActionResult::BlockedByGAS);
	}

	return Resolve(EAesirBossActionResult::Accepted);
}

bool UAesirBossActionComponent::CanExecuteAction(
	EAesirBossAction Action,
	AActor* Target) const
{
	return ValidateAction(Action, Target) ==
		EAesirBossActionResult::Accepted;
}

bool UAesirBossActionComponent::IsActionActive(
	EAesirBossAction Action) const
{
	const TSubclassOf<UGameplayAbility>* AbilityClass =
		ActionAbilities.Find(Action);
	UAbilitySystemComponent* AbilitySystem = ResolveAbilitySystem();

	if (!AbilityClass || !AbilityClass->Get() || !AbilitySystem)
		return false;

	const FGameplayAbilitySpec* AbilitySpec =
		AbilitySystem->FindAbilitySpecFromClass(*AbilityClass);

	return AbilitySpec && AbilitySpec->IsActive();
}

AActor* UAesirBossActionComponent::GetCurrentActionTarget() const
{
	return CurrentActionTarget;
}

APawn* UAesirBossActionComponent::GetAvatar() const
{
	return Avatar;
}

UAbilitySystemComponent*
UAesirBossActionComponent::ResolveAbilitySystem() const
{
	const IAbilitySystemInterface* AbilitySystemOwner =
		Cast<IAbilitySystemInterface>(GetAvatar());

	return AbilitySystemOwner
		? AbilitySystemOwner->GetAbilitySystemComponent()
		: nullptr;
}

bool UAesirBossActionComponent::ActionRequiresTarget(
	EAesirBossAction Action) const
{
	return Action == EAesirBossAction::LightAttack ||
		Action == EAesirBossAction::HeavyAttack ||
		Action == EAesirBossAction::Pursue ||
		Action == EAesirBossAction::Disengage ||
		Action == EAesirBossAction::UseAbility ||
		Action == EAesirBossAction::GapCloserSkill ||
		Action == EAesirBossAction::UnblockableAreaSkill;
}

EAesirBossActionResult UAesirBossActionComponent::ValidateAction(
	EAesirBossAction Action,
	AActor* Target) const
{
	APawn* ControlledAvatar = GetAvatar();
	if (!IsValid(ControlledAvatar))
		return EAesirBossActionResult::InvalidAvatar;

	if (!ControlledAvatar->HasAuthority())
		return EAesirBossActionResult::NotAuthority;

	if (ActionRequiresTarget(Action) && !IsValid(Target))
		return EAesirBossActionResult::InvalidTarget;

	if (const AAesirEnemyCharacter* EnemyAvatar =
		Cast<AAesirEnemyCharacter>(ControlledAvatar))
	{
		if (EnemyAvatar->IsDead())
			return EAesirBossActionResult::Dead;

		if (EnemyAvatar->IsStunned())
			return EAesirBossActionResult::Stunned;

		if (EnemyAvatar->IsAttacking() ||
			EnemyAvatar->IsDodging() ||
			EnemyAvatar->IsDefending() ||
			EnemyAvatar->IsExecutingMovementAction())
		{
			return EAesirBossActionResult::Busy;
		}
	}

	UAbilitySystemComponent* AbilitySystem = ResolveAbilitySystem();
	if (!IsValid(AbilitySystem))
		return EAesirBossActionResult::MissingAbilitySystem;

	const TSubclassOf<UGameplayAbility>* AbilityClass =
		ActionAbilities.Find(Action);
	if (!AbilityClass || !AbilityClass->Get())
		return EAesirBossActionResult::NotConfigured;

	const FGameplayAbilitySpec* AbilitySpec =
		AbilitySystem->FindAbilitySpecFromClass(*AbilityClass);
	if (!AbilitySpec)
		return EAesirBossActionResult::AbilityNotGranted;

	if (!AbilitySpec->Ability ||
		!AbilitySystem->AbilityActorInfo.IsValid() ||
		!AbilitySpec->Ability->CanActivateAbility(
			AbilitySpec->Handle,
			AbilitySystem->AbilityActorInfo.Get()))
	{
		return EAesirBossActionResult::BlockedByGAS;
	}

	return EAesirBossActionResult::Accepted;
}

void UAesirBossActionComponent::GrantConfiguredAbilities()
{
	APawn* ControlledAvatar = GetAvatar();
	UAbilitySystemComponent* AbilitySystem = ResolveAbilitySystem();

	if (!IsValid(ControlledAvatar) ||
		!ControlledAvatar->HasAuthority() ||
		!IsValid(AbilitySystem))
	{
		return;
	}

	for (const TPair<EAesirBossAction,
		TSubclassOf<UGameplayAbility>>& Pair : ActionAbilities)
	{
		if (!Pair.Value ||
			AbilitySystem->FindAbilitySpecFromClass(Pair.Value))
		{
			continue;
		}

		AbilitySystem->GiveAbility(
			FGameplayAbilitySpec(
				Pair.Value,
				1,
				INDEX_NONE,
				ControlledAvatar));
	}
}

FAesirBossActionOutcome UAesirBossActionComponent::MakeOutcome(
	EAesirBossAction Action,
	EAesirBossActionResult Result,
	AActor* Target) const
{
	FAesirBossActionOutcome Outcome;
	Outcome.Action = Action;
	Outcome.Result = Result;
	Outcome.Target = Target;

	if (const UWorld* World = GetWorld())
		Outcome.WorldTimeSeconds = World->GetTimeSeconds();

	return Outcome;
}

void UAesirBossActionComponent::PublishOutcome(
	const FAesirBossActionOutcome& Outcome)
{
	/*UE_LOG(
		LogTemp,
		Display,
		TEXT("Boss action: action=%s result=%s target=%s time=%.3f"),
		*StaticEnum<EAesirBossAction>()->GetNameStringByValue(
			static_cast<int64>(Outcome.Action)),
		*StaticEnum<EAesirBossActionResult>()->GetNameStringByValue(
			static_cast<int64>(Outcome.Result)),
		*GetNameSafe(Outcome.Target),
		Outcome.WorldTimeSeconds);*/

	OnActionResolved.Broadcast(Outcome);
}
