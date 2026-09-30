#pragma once

#include "CoreMinimal.h"
#include "Abilities/AesirGameplayAbility.h"
#include "AesirBossDodgeAbility.generated.h"

UENUM(BlueprintType)
enum class EAesirBossDodgeDirectionPolicy : uint8
{
	Backward,
	Left,
	Right,
	AlternateLateral,
	SafestLateral,
	SafestRetreat
};

struct FBranchingPointNotifyPayload;
class AAesirBossAIController;
class ACharacter;
class UAbilitySystemComponent;
class UAnimInstance;
class UAnimMontage;

/**
 * GAS lifecycle for a committed Boss dodge driven by a Root Motion Montage.
 */
UCLASS(Abstract, Blueprintable)
class AESIRWARDEN_API UAesirBossDodgeAbility
	: public UAesirGameplayAbility
{
	GENERATED_BODY()

public:
	UAesirBossDodgeAbility();

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
		Category = "Aesir|AI|Boss|Dodge")
	TObjectPtr<UAnimMontage> DodgeMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Dodge")
	TObjectPtr<UAnimMontage> LeftDodgeMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Dodge")
	TObjectPtr<UAnimMontage> RightDodgeMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Dodge")
	EAesirBossDodgeDirectionPolicy DirectionPolicy =
		EAesirBossDodgeDirectionPolicy::Backward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Dodge",
		meta = (ClampMin = "0.1"))
	float PlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Dodge",
		meta = (ClampMin = "0.0"))
	float RootMotionTranslationScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Dodge|Clearance",
		meta = (ClampMin = "0.0"))
	float ClearanceProbeDistance = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Dodge|Clearance",
		meta = (ClampMin = "0.0"))
	float ClearanceProbeRadius = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Dodge",
		meta = (ClampMin = "0.0"))
	float CancelBlendOutTime = 0.1f;

private:
	UFUNCTION()
	void HandleMontageNotifyBegin(
		FName NotifyName,
		const FBranchingPointNotifyPayload& BranchingPointPayload);

	UFUNCTION()
	void HandleMontageNotifyEnd(
		FName NotifyName,
		const FBranchingPointNotifyPayload& BranchingPointPayload);

	UFUNCTION()
	void HandleMontageEnded(
		UAnimMontage* Montage,
		bool bInterrupted);

	bool IsDodgeNotify(
		FName NotifyName,
		const FBranchingPointNotifyPayload& BranchingPointPayload) const;
	UAnimMontage* SelectDodgeMontage();
	float MeasureDirectionClearance(const FVector& Direction) const;
	void SetInvulnerability(bool bEnabled);
	void UnbindFromAnimInstance();

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> ActiveCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> ActiveAbilitySystem;

	UPROPERTY(Transient)
	TObjectPtr<UAnimInstance> ActiveAnimInstance;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveDodgeMontage;

	TWeakObjectPtr<AAesirBossAIController> ActiveController;
	bool bInvulnerabilityGranted = false;
	bool bPreferLeftDodge = true;
};
