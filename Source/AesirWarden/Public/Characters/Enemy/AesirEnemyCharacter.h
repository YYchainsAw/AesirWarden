// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "AesirEnemyCharacter.generated.h"

class UAesirHealthComponent;
class UAesirCombatComponent;
class UAesirCombatStateComponent;
class UAesirPoiseComponent;
class UAesirAbilitySystemComponent;
class UAesirAttributeSet;
class UAbilitySystemComponent;
class UGameplayEffect;
class UMotionWarpingComponent;
class AController;
struct FOnAttributeChangeData;

UCLASS()
class AESIRWARDEN_API AAesirEnemyCharacter
	: public ACharacter,
	  public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAesirEnemyCharacter();

	virtual UAbilitySystemComponent*
	GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Aesir|Abilities")
	UAesirAbilitySystemComponent*
	GetAesirAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|Attributes")
	float GetGASHealth() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|Attributes")
	float GetGASMaxHealth() const;
	
	bool TryAttack();
	bool IsAttacking() const;
	bool IsDodging() const;
	bool IsDefending() const;
	bool IsExecutingMovementAction() const;
	bool IsDead() const;
	bool IsStunned() const;

	UFUNCTION(BlueprintImplementableEvent, Category="Aesir|Targeting")
	void SetLockOnIndicatorVisible(bool bVisible);
	
protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void HandlePoiseBroken();

	UFUNCTION()
	void HandleDeath(
		AController* InstigatedBy,
		AActor* DamageCauser);
	
	void EndStun();
	void InitializeDefaultAttributes();
	void HandleGASHealthChanged(const FOnAttributeChangeData& Data);

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|Poise",
		meta = (ClampMin = "0.1"))
	float StunDuration = 1.5f;

	FTimerHandle StunTimerHandle;
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "Aesir|Abilities",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAesirAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "Aesir|Attributes",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAesirAttributeSet> CoreAttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|Abilities|Initialization")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	bool bGASDeathHandled = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
	Category="Aesir|Health",
	meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAesirHealthComponent> HealthComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
	Category="Aesir|Combat",
	meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAesirCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "Aesir|Motion Warping",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
	Category = "Aesir|Combat State",
	meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAesirCombatStateComponent> CombatStateComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
    Category = "Aesir|Poise",
    meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAesirPoiseComponent> PoiseComponent;
};
