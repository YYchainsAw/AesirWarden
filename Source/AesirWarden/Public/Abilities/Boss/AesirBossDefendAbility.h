#pragma once

#include "CoreMinimal.h"
#include "Abilities/AesirGameplayAbility.h"
#include "AesirBossDefendAbility.generated.h"

class AAesirBossAIController;
class ACharacter;
class UAbilitySystemComponent;
class UAnimInstance;
class UAnimMontage;

/**
 * Timed Boss defense. Normal attacks are blocked while the ability is active.
 */
UCLASS(Abstract, Blueprintable)
class AESIRWARDEN_API UAesirBossDefendAbility
	: public UAesirGameplayAbility
{
	GENERATED_BODY()

public:
	UAesirBossDefendAbility();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Defend")
	TObjectPtr<UAnimMontage> DefendMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Defend",
		meta = (ClampMin = "0.1"))
	float DefendDuration = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Defend",
		meta = (ClampMin = "0.1"))
	float PlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Defend",
		meta = (ClampMin = "0.0"))
	float BlendOutTime = 0.15f;

private:
	UFUNCTION()
	void HandleDefendDurationElapsed();

	UFUNCTION()
	void HandleMontageEnded(
		UAnimMontage* Montage,
		bool bInterrupted);

	void SetDefendState(bool bEnabled);
	void UnbindFromAnimInstance();

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> ActiveCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> ActiveAbilitySystem;

	UPROPERTY(Transient)
	TObjectPtr<UAnimInstance> ActiveAnimInstance;

	TWeakObjectPtr<AAesirBossAIController> ActiveController;
	FTimerHandle DefendTimerHandle;
	bool bDefendStateGranted = false;
};
