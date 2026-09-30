// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AesirCombatComponent.generated.h"

struct FBranchingPointNotifyPayload;
class UAnimMontage;
class UAnimInstance;
class UMeshComponent;
class AActor;
class UGameplayEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FAesirAttackStateChangedSignature,
	bool,
	bIsAttacking);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(
	FAesirAttackWindowStartedSignature);

enum class EAesirAttackType : uint8
{
	None,
	Light,
	Heavy,
	GapCloser,
	UnblockableArea
};

UENUM(BlueprintType)
enum class EAesirGuardInteraction : uint8
{
	Blockable UMETA(DisplayName = "Blockable"),
	PerfectGuardRequired UMETA(DisplayName = "Perfect Guard Required"),
	Unblockable UMETA(DisplayName = "Unblockable")
};

UENUM(BlueprintType)
enum class EAesirPerfectGuardReaction : uint8
{
	StaggerAttacker UMETA(DisplayName = "Stagger Attacker"),
	ContinueCombo UMETA(DisplayName = "Continue Combo")
};

UCLASS(ClassGroup=(Aesir), meta=(BlueprintSpawnableComponent))
class AESIRWARDEN_API UAesirCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAesirCombatComponent();

	void TryLightAttack();
	void TryHeavyAttack();
	void TryGapCloserSkill();
	void TryUnblockableAreaSkill();
	bool CanStartAttack(EAesirAttackType AttackType) const;
	
	bool IsAttacking() const;
	bool IsDamageWindowActive() const;

	void CancelAttack();

	UFUNCTION(BlueprintCallable, Category = "Aesir|Combat|Guard Counter")
	void ApplyCounterStagger(float Duration = 0.8f);

	UFUNCTION(BlueprintCallable, Category = "Aesir|Combat|Guard Counter")
	bool ApplyGuardCounterHit(
		AActor* TargetActor,
		float Damage = 20.0f,
		float StaggerDuration = 0.8f);
	
	UPROPERTY(BlueprintAssignable, Category="Aesir|Combat")
	FAesirAttackStateChangedSignature OnAttackStateChanged;

	UPROPERTY(BlueprintAssignable, Category="Aesir|Combat")
	FAesirAttackWindowStartedSignature OnAttackWindowStarted;

protected:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UAnimInstance* GetOwnerAnimInstance() const;
	UAnimMontage* GetCurrentMontage() const;
	void PlayAttack(EAesirAttackType AttackType, int32 ComboIndex);
	const TArray<TObjectPtr<UAnimMontage>>* GetAttackMontages(
		EAesirAttackType AttackType) const;
	void AdvanceCombo();
	void ResetCombo();
	void CacheWeaponMesh();
	void BeginAttackTrace();
	void EndAttackTrace();
	void EndAttackRecoil();
	EAesirPerfectGuardReaction GetCurrentPerfectGuardReaction() const;
	float GetCurrentPerfectGuardRecoilDuration() const;
	void PerformAttackTrace(
		const FVector& CurrentTraceStart,
		const FVector& CurrentTraceEnd);
	
	bool TryApplyGameplayDamage(
		AActor* TargetActor,
		float Damage,
		float GuardPressure,
		EAesirGuardInteraction GuardInteraction,
		bool& bOutHitNegated);

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Damage")
	TSubclassOf<UGameplayEffect> GameplayDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Guard",
		meta=(ClampMin="0.0"))
	float LightAttackGuardPressure = 40.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Guard",
		meta=(ClampMin="0.0"))
	float HeavyAttackGuardPressure = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Guard")
	EAesirGuardInteraction LightAttackGuardInteraction =
		EAesirGuardInteraction::Blockable;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Guard")
	EAesirGuardInteraction HeavyAttackGuardInteraction =
		EAesirGuardInteraction::PerfectGuardRequired;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Guard",
		meta=(ClampMin="0.0"))
	float GapCloserGuardPressure = 70.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Guard")
	EAesirGuardInteraction GapCloserGuardInteraction =
		EAesirGuardInteraction::PerfectGuardRequired;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Guard",
		meta=(ClampMin="0.0"))
	float UnblockableAreaGuardPressure = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Guard")
	EAesirGuardInteraction UnblockableAreaGuardInteraction =
		EAesirGuardInteraction::Unblockable;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Perfect Guard")
	TArray<EAesirPerfectGuardReaction>
		LightAttackPerfectGuardReactions;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Perfect Guard")
	TArray<EAesirPerfectGuardReaction>
		HeavyAttackPerfectGuardReactions;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Perfect Guard")
	TArray<EAesirPerfectGuardReaction>
		GapCloserPerfectGuardReactions;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Perfect Guard",
		meta=(ClampMin="0.0"))
	float LightAttackPerfectGuardRecoilDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Perfect Guard",
		meta=(ClampMin="0.0"))
	float HeavyAttackPerfectGuardRecoilDuration = 0.7f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Perfect Guard",
		meta=(ClampMin="0.0"))
	float GapCloserPerfectGuardRecoilDuration = 0.8f;

	UFUNCTION()
	void HandleMontageNotifyBegin(
		FName NotifyName,
		const FBranchingPointNotifyPayload& BranchingPointPayload);

	UFUNCTION()
	void HandleMontageNotifyEnd(
		FName NotifyName,
		const FBranchingPointNotifyPayload& BranchingPointPayload);

	UFUNCTION()
	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category="Aesir|Combat|Animation",
		meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<UAnimMontage>> LightAttackMontages;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category="Aesir|Combat|Animation",
		meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<UAnimMontage>> HeavyAttackMontages;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category="Aesir|Combat|Animation",
		meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<UAnimMontage>> GapCloserSkillMontages;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category="Aesir|Combat|Animation",
		meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<UAnimMontage>> UnblockableAreaSkillMontages;

	UPROPERTY(Transient)
	TObjectPtr<UMeshComponent> WeaponMesh;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Damage",
	meta=(ClampMin="0.0"))
	float LightAttackDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|Combat|Poise",
	meta = (ClampMin = "0.0"))
	float LightAttackPoiseDamage = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Damage",
	meta=(ClampMin="0.0"))
	float HeavyAttackDamage = 15.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|Combat|Poise",
	meta = (ClampMin = "0.0"))
	float HeavyAttackPoiseDamage = 45.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Damage",
		meta=(ClampMin="0.0"))
	float GapCloserDamage = 18.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Poise",
		meta=(ClampMin="0.0"))
	float GapCloserPoiseDamage = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Damage",
		meta=(ClampMin="0.0"))
	float UnblockableAreaDamage = 22.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Poise",
		meta=(ClampMin="0.0"))
	float UnblockableAreaPoiseDamage = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|Combat|Weapon")
	FName WeaponComponentTag = TEXT("Weapon");

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|Combat|Weapon")
	FName TraceStartSocketName = TEXT("TraceStart");

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|Combat|Weapon")
	FName TraceEndSocketName = TEXT("TraceEnd");

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Weapon Trace",
	meta=(ClampMin="1.0"))
	float TraceRadius = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Weapon Trace",
		meta=(ClampMin="2", ClampMax="8"))
	int32 TraceSampleCount = 4;

	TSet<TWeakObjectPtr<AActor>> HitActorsThisAttack;

	FVector PreviousTraceStart = FVector::ZeroVector;
	FVector PreviousTraceEnd = FVector::ZeroVector;
	EAesirAttackType CurrentAttackType = EAesirAttackType::None;
	int32 CurrentComboIndex = INDEX_NONE;
	int32 CurrentAttackWindowIndex = INDEX_NONE;
	bool bAttackInputBuffered = false;
	bool bComboWindowOpen = false;
	bool bAttackTraceActive = false;
	FTimerHandle AttackRecoilTimerHandle;
};
