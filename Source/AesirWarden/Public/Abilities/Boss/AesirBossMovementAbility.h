#pragma once

#include "CoreMinimal.h"
#include "AITypes.h"
#include "Abilities/AesirGameplayAbility.h"
#include "AI/Boss/AesirBossDecisionTypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "AesirBossMovementAbility.generated.h"

class AAesirBossAIController;
class ACharacter;
class UAbilitySystemComponent;

/**
 * GAS lifecycle for high-level Boss movement executed by Unreal navigation.
 */
UCLASS(Abstract, Blueprintable)
class AESIRWARDEN_API UAesirBossMovementAbility
	: public UAesirGameplayAbility
{
	GENERATED_BODY()

public:
	UAesirBossMovementAbility();

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
		Category = "Aesir|AI|Boss|Movement")
	EAesirBossAction MovementAction = EAesirBossAction::Pursue;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Movement",
		meta = (ClampMin = "0.0"))
	float PursueAcceptanceRadius = 190.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Movement",
		meta = (ClampMin = "50.0"))
	float DisengageDistance = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Movement",
		meta = (ClampMin = "0.0"))
	float DisengageAcceptanceRadius = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Movement|Speed",
		meta = (ClampMin = "0.0"))
	float PursueMoveSpeed = 520.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Movement|Speed",
		meta = (ClampMin = "0.0"))
	float DisengageMoveSpeed = 380.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Movement",
		meta = (ClampMin = "0.1"))
	float MovementTimeout = 4.0f;

private:
	UFUNCTION()
	void HandleMoveCompleted(
		FAIRequestID RequestID,
		EPathFollowingResult::Type Result);

	UFUNCTION()
	void HandleMovementTimeout();

	AActor* ResolveActionTarget() const;
	EPathFollowingRequestResult::Type StartMovement(AActor* TargetActor);
	void ApplyMovementSpeed();
	void RestoreMovementSpeed();
	void SetMovementState(bool bEnabled);
	void UnbindFromController();

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> ActiveCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> ActiveAbilitySystem;

	UPROPERTY(Transient)
	TObjectPtr<AAesirBossAIController> ActiveController;

	FTimerHandle MovementTimeoutHandle;
	float PreviousMaxWalkSpeed = 0.0f;
	bool bMovementStateGranted = false;
	bool bMovementSpeedOverridden = false;
};
