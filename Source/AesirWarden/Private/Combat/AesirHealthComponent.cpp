// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/AesirHealthComponent.h"

#include "GameFramework/Actor.h"

UAesirHealthComponent::UAesirHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
}

void UAesirHealthComponent::SetInvulnerable(bool bNewInvulnerable)
{
	bInvulnerable = bNewInvulnerable;
}

void UAesirHealthComponent::SynchronizeHealthFromGAS(
	float NewCurrentHealth,
	float NewMaxHealth,
	AActor* DamageCauser)
{
	const float PreviousHealth = CurrentHealth;
	const float PreviousMaxHealth = MaxHealth;
	const bool bWasDead = IsDead();

	MaxHealth = FMath::Max(NewMaxHealth, 1.0f);
	CurrentHealth = FMath::Clamp(
		NewCurrentHealth,
		0.0f,
		MaxHealth);

	const float HealthDelta = CurrentHealth - PreviousHealth;

	if (!FMath::IsNearlyZero(HealthDelta) ||
		!FMath::IsNearlyEqual(PreviousMaxHealth, MaxHealth))
	{
		OnHealthChanged.Broadcast(
			CurrentHealth,
			MaxHealth,
			HealthDelta);
	}

	if (HealthDelta < 0.0f)
	{
		OnHitReceived.Broadcast(
			GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector,
			DamageCauser,
			IsDead());
	}

	if (!bWasDead && IsDead())
	{
		OnDeath.Broadcast(nullptr, DamageCauser);
	}
}

void UAesirHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	
	MaxHealth = FMath::Max(MaxHealth, 1.0f);
	CurrentHealth = MaxHealth;
	
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->OnTakeAnyDamage.
			AddUniqueDynamic(this, &UAesirHealthComponent::HandleTakeAnyDamage);
		OwnerActor->OnTakePointDamage.
			AddUniqueDynamic(this, &UAesirHealthComponent::HandleTakePointDamage);
	}
		
}

void UAesirHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	if (DamagedActor != GetOwner() ||
		Damage <= 0.0f ||
		IsDead()) 
		return;

	if (bInvulnerable)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[EvadeTest] %s ignored %.1f damage from %s during invulnerability."),
			*GetNameSafe(DamagedActor),
			Damage,
			*GetNameSafe(DamageCauser));
		return;
	}
	
	const float PreviousHealth = CurrentHealth;
	
	CurrentHealth = FMath::Clamp(
		CurrentHealth - Damage,
		0.0f,
		MaxHealth);
	
	const float HealthDelta = CurrentHealth - PreviousHealth;

	OnHealthChanged.Broadcast(
		CurrentHealth,
		MaxHealth,
		HealthDelta);
	
	UE_LOG(LogTemp, Log, TEXT("%s health: %.1f / %.1f"),
	*GetNameSafe(DamagedActor), CurrentHealth, MaxHealth);
	
	if (IsDead())
	{
		OnDeath.Broadcast(InstigatedBy, DamageCauser);
		UE_LOG(LogTemp, Log, TEXT("%s is dead!"), *GetNameSafe(DamagedActor));
	}
		
}

void UAesirHealthComponent::HandleTakePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy,
	FVector HitLocation, UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection,
	const UDamageType* DamageType, AActor* DamageCauser)
{
	if (DamagedActor != GetOwner() ||
		Damage <= 0.0f ||
		bInvulnerable ||
		IsDead())
		return;

	const bool bFatalHit = Damage >= CurrentHealth;

	OnHitReceived.Broadcast(
		HitLocation,
		DamageCauser,
		bFatalHit);
}



