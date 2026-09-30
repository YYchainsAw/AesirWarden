// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AesirHealthComponent.generated.h"

class AController;
class UDamageType;
class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FAesirHealthChangedSignature,
	float, CurrentHealth,
	float, MaxHealth,
	float, HealthDelta);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FAesirDeathSignature,
	AController*, InstigatedBy,
	AActor*, DamageCauser);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FAesirHitReceivedSignature,
	FVector, HitLocation,
	AActor*, DamageCauser,
	bool, bFatalHit);

UCLASS( ClassGroup=(Aesir), meta=(BlueprintSpawnableComponent) )
class AESIRWARDEN_API UAesirHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAesirHealthComponent();
	
	UFUNCTION(BlueprintPure, Category="Aesir|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category="Aesir|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category="Aesir|Health")
	bool IsDead() const { return CurrentHealth <= 0.0f; }
	
	UFUNCTION(BlueprintCallable, Category="Aesir|Health")
	void SetInvulnerable(bool bNewInvulnerable);

	UFUNCTION(BlueprintPure, Category="Aesir|Health")
	bool IsInvulnerable() const { return bInvulnerable; }

	void SynchronizeHealthFromGAS(
		float NewCurrentHealth,
		float NewMaxHealth,
		AActor* DamageCauser = nullptr);
	
	UPROPERTY(BlueprintAssignable, Category="Aesir|Health")
	FAesirHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Aesir|Health")
	FAesirDeathSignature OnDeath;
	
	UPROPERTY(BlueprintAssignable, Category="Aesir|Health")
	FAesirHitReceivedSignature OnHitReceived;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category="Aesir|Health",
		meta=(AllowPrivateAccess="true", ClampMin="1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
		Category="Aesir|Health",
		meta=(AllowPrivateAccess="true"))
	float CurrentHealth = 0.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
	Category="Aesir|Health",
	meta=(AllowPrivateAccess="true"))
	bool bInvulnerable = false;

	UFUNCTION()
	void HandleTakeAnyDamage(
		AActor* DamagedActor,
		float Damage,
		const UDamageType* DamageType,
		AController* InstigatedBy,
		AActor* DamageCauser);
	
	UFUNCTION()
	void HandleTakePointDamage(
		AActor* DamagedActor,
		float Damage,
		AController* InstigatedBy,
		FVector HitLocation,
		UPrimitiveComponent* HitComponent,
		FName BoneName,
		FVector ShotFromDirection,
		const UDamageType* DamageType,
		AActor* DamageCauser);
};
