#pragma once

#include "CoreMinimal.h"
#include "Abilities/AesirGameplayAbility.h"
#include "AI/Boss/AesirBossDecisionTypes.h"
#include "AesirBossAttackAbility.generated.h"

class UAesirCombatComponent;
class AAesirBossAIController;
class UGameplayEffect;
class UMotionWarpingComponent;

/**
 * GAS lifecycle wrapper for the existing montage and weapon-trace attack path.
 * Create Blueprint children for each attack action, then configure
 * AttackAction, costs, cooldowns, and activation tags on those children.
 */
UCLASS(Abstract, Blueprintable)
class AESIRWARDEN_API UAesirBossAttackAbility
	: public UAesirGameplayAbility
{
	GENERATED_BODY()

public:
	UAesirBossAttackAbility();

protected:
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

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
		Category = "Aesir|AI|Boss|Attack")
	EAesirBossAction AttackAction = EAesirBossAction::LightAttack;

	/** Applied after a committed attack completes to create a readable pause. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Attack")
	TSubclassOf<UGameplayEffect> PostAttackRecoveryEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Attack|Gap Closer")
	bool bUseGapCloserMotionWarping = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Attack|Gap Closer")
	FName GapCloserWarpTargetName = TEXT("BossGapCloserTarget");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Attack|Gap Closer",
		meta = (ClampMin = "0.0"))
	float GapCloserStoppingDistance = 170.0f;

private:
	UFUNCTION()
	void HandleAttackStateChanged(bool bIsAttacking);

	UFUNCTION()
	void HandleAttackWindowStarted();

	void BeginTargetFacing(AActor* AvatarActor);
	void ConfigureGapCloserMotionWarpTarget(AActor* AvatarActor);
	void ClearGapCloserMotionWarpTarget();
	void EndTargetFacing();
	void UnbindFromCombatComponent();

	UPROPERTY(Transient)
	TObjectPtr<UAesirCombatComponent> ActiveCombatComponent;

	TWeakObjectPtr<AAesirBossAIController> FacingController;
	TWeakObjectPtr<UMotionWarpingComponent> ActiveMotionWarpingComponent;
};
