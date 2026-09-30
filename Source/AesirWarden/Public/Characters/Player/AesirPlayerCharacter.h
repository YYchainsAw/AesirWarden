// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Player/AesirCombatPrototypeCharacter.h"
#include "AbilitySystemInterface.h"
#include "AesirPlayerCharacter.generated.h"

class UAesirCombatComponent;
class UAesirHealthComponent;
class UAesirTargetingComponent;
class UAesirCombatStateComponent;
class UInputAction;
class UAnimMontage;
class UAbilitySystemComponent;
class UAesirAbilitySystemComponent;
class UAesirAttributeSet;
class UAesirPlayerAttributeSet;
class UGameplayEffect;
class UGameplayAbility;

struct FBranchingPointNotifyPayload;
struct FOnAttributeChangeData;
struct FGameplayEventData;
struct FGameplayTag;

UENUM(BlueprintType)
enum class EAesirPlayerGait : uint8
{
	Walk,
	Jog
};

UENUM(BlueprintType)
enum class EAesirEvadeDirection : uint8
{
	Forward,
	ForwardRight,
	Right,
	BackwardRight,
	Backward,
	BackwardLeft,
	Left,
	ForwardLeft
};

enum class EAesirEvadeState : uint8
{
	None,
	Dashing,
	Rolling
};

UCLASS()
class AESIRWARDEN_API AAesirPlayerCharacter : public AAesirCombatPrototypeCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	AAesirPlayerCharacter();
	
	virtual UAbilitySystemComponent*
	GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Aesir|Abilities")
	UAesirAbilitySystemComponent* GetAesirAbilitySystemComponent() const;
	
	virtual void DoLook(float Yaw, float Pitch) override;
	
	virtual void DoMove(float Right, float Forward) override;
	
	UFUNCTION(BlueprintPure, Category = "Aesir|Movement")
	float GetLocomotionDirectionAngle() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|Movement")
	float GetDesiredMoveDirectionAngle() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|Movement")
	EAesirPlayerGait GetDesiredGait() const { return DesiredGait; }

	UFUNCTION(BlueprintCallable, Category = "Aesir|Movement")
	void SetDesiredGait(EAesirPlayerGait NewGait);
	
	UFUNCTION(BlueprintPure, Category = "Aesir|Attributes")
	float GetGASHealth() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|Attributes")
	float GetGASMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|Attributes")
	float GetGASGuardPressure() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|Attributes")
	float GetGASMaxGuardPressure() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|Abilities|Guard Counter")
	bool HasGuardCounterWindow() const;

	UFUNCTION(BlueprintCallable, Category = "Aesir|Abilities|Guard Counter")
	bool TryConsumeGuardCounter(AActor*& OutCounterTarget);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void Tick(float DeltaSeconds) override;
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void HandleToggleGaitInput();
	void HandleMoveInputReleased();
	void ApplyDesiredGait();
	void UpdateFacingMode();
	void HandleLightAttackInput();
	void HandleHeavyAttackInput();
	AActor* ResolveAttackAssistTarget() const;
	void UpdateAttackFacing(float DeltaSeconds);
	void HandleLockOnInput();
	void HandleEvadeInput();
	EAesirEvadeDirection DetermineEvadeDirection() const;
	bool TryPlayEvadeMontage(
		const TMap<EAesirEvadeDirection, TObjectPtr<UAnimMontage>>& MontageMap,
		EAesirEvadeDirection Direction,
		EAesirEvadeState NewState);
	
	bool TryGetLockOnMovementBasis(
		FVector& OutForwardDirection,
		FVector& OutRightDirection) const;
	
	void InitializeDefaultAttributes();
	void GrantStartupAbilities();
	void InitializeCameraOcclusion();
	void RefreshCameraOccluders();
	void UpdateCameraFades(float DeltaSeconds);
	void SetActorCameraFade(AActor* Actor, float FadeAmount) const;
	void SetActorCameraCollisionIgnored(AActor* Actor) const;
	bool IsCameraFadeableActor(const AActor* Actor) const;
	bool IsGASDead() const;
	void SetDamageInvulnerable(bool bInvulnerable);
	void HandleGASHealthChanged(const FOnAttributeChangeData& Data);
	void HandleBlockingTagChanged(FGameplayTag Tag, int32 NewCount);
	void EndPerfectGuardWindow();
	void HandlePerfectGuardEvent(const FGameplayEventData* Payload);
	void EndGuardCounterWindow();

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|Abilities|Initialization")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|Abilities|Initialization")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	bool bGASDeathHandled = false;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|Abilities|Guard",
		meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float PerfectGuardWindowDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|Abilities|Guard Counter",
		meta = (AllowPrivateAccess = "true"))
	bool bEnableGuardCounter = false;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|Abilities|Guard Counter",
		meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float GuardCounterWindowDuration = 0.75f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aesir|Abilities",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAesirAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aesir|Attributes",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAesirAttributeSet> CoreAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aesir|Attributes",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAesirPlayerAttributeSet> PlayerAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Aesir|Combat",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAesirCombatComponent> CombatComponent;
	
	UPROPERTY(EditDefaultsOnly, Category = "Aesir|Combat",
		meta = (ClampMin = "0.05"))
	float HitStunDuration = 0.8f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Aesir|Health",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAesirHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Aesir|CombatState",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAesirCombatStateComponent> CombatStateComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aesir|Input",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> LightAttackAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aesir|Input",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> HeavyAttackAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aesir|Input",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> EvadeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aesir|Input",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<UInputAction> ToggleGaitAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aesir|Movement",
		meta=(AllowPrivateAccess="true"))
	EAesirPlayerGait DesiredGait = EAesirPlayerGait::Jog;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Movement",
		meta=(ClampMin="0.0"))
	float WalkMaxSpeed = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Movement",
		meta=(ClampMin="0.0"))
	float JogMaxSpeed = 450.0f;

	FVector DesiredMoveWorldDirection = FVector::ZeroVector;
	
	UPROPERTY(EditDefaultsOnly, Category="Aesir|Evade|Input",
	meta=(ClampMin="0.10", ClampMax="0.50"))
	float RollDoubleTapWindow = 0.5f;
	
	UPROPERTY(EditDefaultsOnly, Category="Aesir|Evade|Animation")
	TMap<EAesirEvadeDirection, TObjectPtr<UAnimMontage>> DashMontages;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Evade|Animation")
	TMap<EAesirEvadeDirection, TObjectPtr<UAnimMontage>> RollMontages;
	
	UPROPERTY(EditDefaultsOnly, Category="Aesir|Evade|Movement",
	meta=(ClampMin="0.0", ClampMax="2.0"))
	float DashRootMotionTranslationScale = 0.7f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Evade|Movement",
		meta=(ClampMin="0.0", ClampMax="2.0"))
	float RollRootMotionTranslationScale = 1.5f;
	
	UPROPERTY(EditDefaultsOnly, Category="Aesir|Evade|Animation",
	meta=(ClampMin="0.10", ClampMax="3.0"))
	float DashPlayRate = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Evade|Animation",
		meta=(ClampMin="0.10", ClampMax="3.0"))
	float RollPlayRate = 2.0f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Aesir|Targeting",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAesirTargetingComponent> TargetingComponent;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Targeting",
	meta=(ClampMin="0.0"))
	float LockOnCameraRotationSpeed = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Aesir|Targeting",
		meta=(AllowPrivateAccess="true"))
	
	TObjectPtr<UInputAction> LockOnAction;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Targeting",
		meta=(ClampMin="0.0"))
	float LockOnFacingRotationSpeed = 12.0f;
	
	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Target Assist",
	meta=(ClampMin="0.0"))
	float AttackAssistRadius = 800.0f;
	
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> AttackAssistTarget;

	float AttackFacingAssistTimeRemaining = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Target Assist",
		meta=(ClampMin="0.0"))
	float AttackFacingAssistDuration = 0.18f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Target Assist",
		meta=(ClampMin="0.0"))
	float AttackFacingRotationSpeed = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Combat|Target Assist",
		meta=(ClampMin="0.0", ClampMax="180.0"))
	float AttackAssistMaxViewAngle = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Camera",
	meta=(ClampMin="0.0", ClampMax="1.0"))
	float LockOnPitchInputScale = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Camera")
	float LockOnMinPitch = -30.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Camera")
	float LockOnMaxPitch = 15.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Camera|Occlusion")
	bool bEnableCameraOcclusionFade = false;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Camera|Occlusion")
	bool bFadeOtherPawns = true;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Camera|Occlusion")
	FName CameraFadeableActorTag = TEXT("CameraFadeable");

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Camera|Occlusion")
	FName CameraFadeMaterialParameter = TEXT("CameraFade");

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Camera|Occlusion",
		meta=(ClampMin="0.0", ClampMax="1.0"))
	float CameraOccludedFadeAmount = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Camera|Occlusion",
		meta=(ClampMin="0.0"))
	float CameraFadeInterpSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Camera|Occlusion",
		meta=(ClampMin="0.0"))
	float CameraOcclusionTraceRadius = 18.0f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Camera|Occlusion",
		meta=(ClampMin="0.01"))
	float CameraOcclusionTraceInterval = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Camera|Occlusion")
	FVector CameraOcclusionTargetOffset = FVector(0.0f, 0.0f, 70.0f);
	
	UPROPERTY(EditDefaultsOnly, Category="Aesir|Death")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Death",
		meta=(ClampMin="0.1", ClampMax="1.0"))
	float DeathMontagePlayRate = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, Category="Aesir|Death",
	meta=(ClampMin="0.1", ClampMax="1.0"))
	float DeathWorldTimeDilation = 0.55f;

	UFUNCTION()
	void HandleHitReceived(
		FVector HitLocation,
		AActor* DamageCauser,
		bool bFatalHit);
	
	UFUNCTION()
	void HandleEvadeNotifyBegin(
		FName NotifyName,
		const FBranchingPointNotifyPayload& BranchingPointPayload);

	UFUNCTION()
	void HandleEvadeNotifyEnd(
		FName NotifyName,
		const FBranchingPointNotifyPayload& BranchingPointPayload);
	
	UFUNCTION()
	void HandleEvadeMontageEnded(
		UAnimMontage* Montage,
		bool bInterrupted);
	
	UFUNCTION()
	void HandleAttackStateChanged(bool bIsAttacking);
	
	UFUNCTION()
	void HandlePlayerDeath(
		AController* InstigatedBy,
		AActor* DamageCauser);

	void RestoreDefaultCombatState();
	void EndHitStun();
	
	FTimerHandle HitStunTimerHandle;
	FTimerHandle PerfectGuardWindowTimerHandle;
	FTimerHandle GuardCounterWindowTimerHandle;
	TWeakObjectPtr<AActor> GuardCounterTarget;
	bool bIsHitStunned = false;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveEvadeMontage;

	EAesirEvadeState EvadeState = EAesirEvadeState::None;

	EAesirEvadeDirection ActiveEvadeDirection =
		EAesirEvadeDirection::Backward;

	double LastEvadeInputTime = -1.0;

	float CameraOcclusionTraceTimeRemaining = 0.0f;
	TSet<TWeakObjectPtr<AActor>> CameraOccludingActors;
	TMap<TWeakObjectPtr<AActor>, float> CameraFadeAmounts;
};
