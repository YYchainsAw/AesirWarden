// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Enemy/AesirEnemyCharacter.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "AI/Enemy/AesirEnemyAIController.h"
#include "Abilities/AesirAbilitySystemComponent.h"
#include "Abilities/AesirAttributeSet.h"
#include "Tags/AesirGameplayTags.h"
#include "Combat/AesirCombatComponent.h"
#include "Combat/AesirCombatStateComponent.h"
#include "Combat/AesirHealthComponent.h"
#include "Combat/AesirPoiseComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"

// Sets default values

AAesirEnemyCharacter::AAesirEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent =
		CreateDefaultSubobject<UAesirAbilitySystemComponent>(
			TEXT("AbilitySystemComponent"));

	CoreAttributeSet =
		CreateDefaultSubobject<UAesirAttributeSet>(
			TEXT("CoreAttributeSet"));

	HealthComponent =
		CreateDefaultSubobject<UAesirHealthComponent>(TEXT("HealthComponent"));
	CombatComponent = 
		CreateDefaultSubobject<UAesirCombatComponent>(TEXT("CombatComponent"));
	MotionWarpingComponent =
		CreateDefaultSubobject<UMotionWarpingComponent>(
			TEXT("MotionWarpingComponent"));
	CombatStateComponent =
		CreateDefaultSubobject<UAesirCombatStateComponent>(TEXT("CombatStateComponent"));
	PoiseComponent =
		CreateDefaultSubobject<UAesirPoiseComponent>(TEXT("PoiseComponent"));
	
	bUseControllerRotationYaw = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 250.0f;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAesirEnemyAIController::StaticClass();
}

UAbilitySystemComponent*
AAesirEnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAesirAbilitySystemComponent*
AAesirEnemyCharacter::GetAesirAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

float AAesirEnemyCharacter::GetGASHealth() const
{
	return CoreAttributeSet
		? CoreAttributeSet->GetHealth()
		: 0.0f;
}

float AAesirEnemyCharacter::GetGASMaxHealth() const
{
	return CoreAttributeSet
		? CoreAttributeSet->GetMaxHealth()
		: 0.0f;
}

void AAesirEnemyCharacter::InitializeDefaultAttributes()
{
	if (!AbilitySystemComponent || !CoreAttributeSet)
		return;

	if (DefaultAttributesEffect)
	{
		FGameplayEffectContextHandle Context =
			AbilitySystemComponent->MakeEffectContext();

		Context.AddSourceObject(this);

		const FGameplayEffectSpecHandle Spec =
			AbilitySystemComponent->MakeOutgoingSpec(
				DefaultAttributesEffect,
				1.0f,
				Context);

		if (Spec.Data.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(
				*Spec.Data.Get());
			return;
		}
	}

	const float LegacyMaxHealth = HealthComponent
		? HealthComponent->GetMaxHealth()
		: CoreAttributeSet->GetMaxHealth();

	CoreAttributeSet->SetMaxHealth(LegacyMaxHealth);
	CoreAttributeSet->SetHealth(LegacyMaxHealth);
}

void AAesirEnemyCharacter::HandleGASHealthChanged(
	const FOnAttributeChangeData& Data)
{
	const bool bHealthDecreased = Data.NewValue < Data.OldValue;
	const bool bFatalChange = bHealthDecreased && Data.NewValue <= 0.0f;

	if (bFatalChange)
	{
		if (bGASDeathHandled)
			return;

		bGASDeathHandled = true;

		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->AddLooseGameplayTag(
				AesirGameplayTags::State_Dead);
			AbilitySystemComponent->CancelAllAbilities();
		}
	}

	if (HealthComponent && CoreAttributeSet)
	{
		HealthComponent->SynchronizeHealthFromGAS(
			Data.NewValue,
			CoreAttributeSet->GetMaxHealth());
	}

	if (bFatalChange && !HealthComponent)
		HandleDeath(nullptr, nullptr);
}

void AAesirEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (IsValid(PrimitiveComponent))
		{
			PrimitiveComponent->SetCollisionResponseToChannel(
				ECC_Camera,
				ECR_Ignore);
		}
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		InitializeDefaultAttributes();

		if (HealthComponent && CoreAttributeSet)
		{
			HealthComponent->SynchronizeHealthFromGAS(
				CoreAttributeSet->GetHealth(),
				CoreAttributeSet->GetMaxHealth());
		}

		AbilitySystemComponent->
			GetGameplayAttributeValueChangeDelegate(
				UAesirAttributeSet::GetHealthAttribute()).
			AddUObject(
				this,
				&AAesirEnemyCharacter::HandleGASHealthChanged);

		UE_LOG(LogTemp, Display,
			TEXT("Enemy GAS initialized: %s Health=%.1f/%.1f"),
			*GetNameSafe(this),
			CoreAttributeSet
				? CoreAttributeSet->GetHealth()
				: -1.0f,
			CoreAttributeSet
				? CoreAttributeSet->GetMaxHealth()
				: -1.0f);
	}

	if (!IsValid(PoiseComponent))
	{
		PoiseComponent =
			FindComponentByClass<UAesirPoiseComponent>();
	}

	if (IsValid(PoiseComponent))
	{
		PoiseComponent->OnPoiseBroken.AddUniqueDynamic(
			this,
			&AAesirEnemyCharacter::HandlePoiseBroken);
	}
	if (IsValid(HealthComponent))
	{
		HealthComponent->OnDeath.AddUniqueDynamic(
			this,
			&AAesirEnemyCharacter::HandleDeath);
	}
}

void AAesirEnemyCharacter::HandlePoiseBroken()
{
	if (IsDead() || !IsValid(CombatStateComponent)) return;
	
	if (!CombatStateComponent->SetCombatState(
		EAesirCombatState::Stunned))
		return;
	
	if (IsValid(CombatComponent))
		CombatComponent->CancelAttack();

	GetCharacterMovement()->StopMovementImmediately();

	GetWorldTimerManager().SetTimer(
		StunTimerHandle,
		this,
		&AAesirEnemyCharacter::EndStun,
		StunDuration,
		false);
}

void AAesirEnemyCharacter::HandleDeath(AController* InstigatedBy, AActor* DamageCauser)
{
	if (!IsValid(CombatStateComponent)) return;

	GetWorldTimerManager().ClearTimer(StunTimerHandle);

	if (IsValid(CombatComponent))
		CombatComponent->CancelAttack();

	GetCharacterMovement()->StopMovementImmediately();
	CombatStateComponent->SetCombatState(EAesirCombatState::Dead);
}

void AAesirEnemyCharacter::EndStun()
{
	if (IsDead()) return;

	if (IsValid(PoiseComponent))
		PoiseComponent->ResetPoise();

	if (IsValid(CombatStateComponent))
		CombatStateComponent->SetCombatState(
			EAesirCombatState::Combat);
}

bool AAesirEnemyCharacter::TryAttack()
{
	if (IsDead() ||
		IsStunned() ||
		!IsValid(CombatComponent) ||
		CombatComponent->IsAttacking())
		return false;

	CombatComponent->TryLightAttack();

	return CombatComponent->IsAttacking();
}

bool AAesirEnemyCharacter::IsAttacking() const
{
	return IsValid(CombatComponent) && CombatComponent->IsAttacking();
}

bool AAesirEnemyCharacter::IsDodging() const
{
	return IsValid(AbilitySystemComponent) &&
		AbilitySystemComponent->HasMatchingGameplayTag(
			AesirGameplayTags::State_Dodging);
}

bool AAesirEnemyCharacter::IsDefending() const
{
	return IsValid(AbilitySystemComponent) &&
		AbilitySystemComponent->HasMatchingGameplayTag(
			AesirGameplayTags::State_Blocking);
}

bool AAesirEnemyCharacter::IsExecutingMovementAction() const
{
	if (!IsValid(AbilitySystemComponent))
		return false;

	return AbilitySystemComponent->HasMatchingGameplayTag(
			AesirGameplayTags::State_Pursuing) ||
		AbilitySystemComponent->HasMatchingGameplayTag(
			AesirGameplayTags::State_Disengaging);
}

bool AAesirEnemyCharacter::IsDead() const
{
	return (CoreAttributeSet && CoreAttributeSet->GetHealth() <= 0.0f) ||
		(IsValid(HealthComponent) && HealthComponent->IsDead());
}

bool AAesirEnemyCharacter::IsStunned() const
{
	return IsValid(CombatStateComponent) &&
		(CombatStateComponent->IsInCombatState(
			EAesirCombatState::Stunned) ||
		 CombatStateComponent->IsInCombatState(
			EAesirCombatState::AttackRecoiled));
}
