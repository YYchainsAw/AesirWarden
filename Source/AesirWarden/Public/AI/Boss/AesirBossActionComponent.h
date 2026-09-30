#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/Boss/AesirBossDecisionTypes.h"
#include "AesirBossActionComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class APawn;

UCLASS(ClassGroup = (Aesir), meta = (BlueprintSpawnableComponent))
class AESIRWARDEN_API UAesirBossActionComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	UAesirBossActionComponent();

	void InitializeForAvatar(APawn* InAvatar);

	UFUNCTION(BlueprintCallable, Category = "Aesir|AI|Boss")
	FAesirBossActionOutcome TryExecuteAction(
		EAesirBossAction Action,
		AActor* Target);

	/** Performs the same legality checks as execution without activating an ability. */
	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss")
	bool CanExecuteAction(
		EAesirBossAction Action,
		AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss")
	bool IsActionActive(EAesirBossAction Action) const;

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss")
	AActor* GetCurrentActionTarget() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss")
	APawn* GetAvatar() const;

	UPROPERTY(BlueprintAssignable, Category = "Aesir|AI|Boss")
	FOnAesirBossActionResolved OnActionResolved;

private:
	UAbilitySystemComponent* ResolveAbilitySystem() const;
	bool ActionRequiresTarget(EAesirBossAction Action) const;
	EAesirBossActionResult ValidateAction(
		EAesirBossAction Action,
		AActor* Target) const;
	void GrantConfiguredAbilities();
	FAesirBossActionOutcome MakeOutcome(
		EAesirBossAction Action,
		EAesirBossActionResult Result,
		AActor* Target) const;
	void PublishOutcome(const FAesirBossActionOutcome& Outcome);

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI|Boss|Actions")
	TMap<EAesirBossAction, TSubclassOf<UGameplayAbility>> ActionAbilities;

	UPROPERTY(Transient)
	TObjectPtr<APawn> Avatar;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentActionTarget;
};
