#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/Boss/AesirBossDecisionTypes.h"
#include "AI/Boss/AesirBossTelemetryTypes.h"
#include "AesirBossRewardComponent.generated.h"

class APawn;
class UAbilitySystemComponent;
class UAesirBossTelemetryComponent;
struct FGameplayEventData;
struct FOnAttributeChangeData;

USTRUCT(BlueprintType)
struct AESIRWARDEN_API FAesirBossRewardSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Reward",
		meta = (ClampMin = "0.0"))
	float DamageDealtRewardPerPoint = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Reward",
		meta = (ClampMin = "0.0"))
	float DamageReceivedPenaltyPerPoint = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Reward",
		meta = (ClampMin = "0.0"))
	float TargetDefeatedReward = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Reward",
		meta = (ClampMax = "0.0"))
	float BossDefeatedPenalty = -10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Reward",
		meta = (ClampMin = "0.0"))
	float DefendedDamageRewardPerPoint = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Reward",
		meta = (ClampMax = "0.0"))
	float RejectedActionPenalty = -0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Reward",
		meta = (ClampMax = "0.0"))
	float RepeatedActionPenalty = -0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Reward",
		meta = (ClampMax = "0.0"))
	float DecisionStepPenalty = -0.005f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		Category = "Aesir|AI|Boss|Reward",
		meta = (ClampMin = "1", ClampMax = "20"))
	int32 RepetitionGraceCount = 2;
};

UCLASS(ClassGroup = (Aesir), meta = (BlueprintSpawnableComponent))
class AESIRWARDEN_API UAesirBossRewardComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	UAesirBossRewardComponent();

	void InitializeForAvatar(
		APawn* InAvatar,
		UAesirBossTelemetryComponent* InTelemetryComponent);

	void BeginEpisode();
	void SetTarget(AActor* InTarget);
	void Shutdown();

	void HandleActionOutcome(
		const FAesirBossActionOutcome& Outcome);

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss|Reward")
	FAesirBossRewardSettings GetRewardSettings() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	static UAbilitySystemComponent* ResolveAbilitySystem(AActor* Actor);
	void UnbindBossHealth();
	void UnbindTargetHealth();
	void HandleBossHealthChanged(const FOnAttributeChangeData& Data);
	void HandleTargetHealthChanged(const FOnAttributeChangeData& Data);
	void HandleSuccessfulDefend(const FGameplayEventData* Payload);
	void RecordReward(
		float Reward,
		EAesirBossRewardReason Reason,
		float SourceMagnitude = 0.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI|Boss|Reward")
	FAesirBossRewardSettings RewardSettings;

	UPROPERTY(Transient)
	TObjectPtr<APawn> Avatar;

	UPROPERTY(Transient)
	TObjectPtr<UAesirBossTelemetryComponent> TelemetryComponent;

	TWeakObjectPtr<AActor> Target;
	TWeakObjectPtr<UAbilitySystemComponent> BossAbilitySystem;
	TWeakObjectPtr<UAbilitySystemComponent> TargetAbilitySystem;
	FDelegateHandle BossHealthChangedHandle;
	FDelegateHandle TargetHealthChangedHandle;
	FDelegateHandle SuccessfulDefendHandle;
	EAesirBossAction LastAcceptedAction = EAesirBossAction::LightAttack;
	int32 ConsecutiveAcceptedActionCount = 0;
	bool bHasLastAcceptedAction = false;
};
