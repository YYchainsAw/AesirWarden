#include "AI/Boss/AesirBossRewardComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AI/Boss/AesirBossTelemetryComponent.h"
#include "Abilities/AesirAttributeSet.h"
#include "Tags/AesirGameplayTags.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/Pawn.h"

UAesirBossRewardComponent::UAesirBossRewardComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAesirBossRewardComponent::InitializeForAvatar(
	APawn* InAvatar,
	UAesirBossTelemetryComponent* InTelemetryComponent)
{
	Shutdown();
	Avatar = InAvatar;
	TelemetryComponent = InTelemetryComponent;
	BeginEpisode();

	BossAbilitySystem = ResolveAbilitySystem(InAvatar);
	if (BossAbilitySystem.IsValid())
	{
		BossHealthChangedHandle = BossAbilitySystem->
			GetGameplayAttributeValueChangeDelegate(
				UAesirAttributeSet::GetHealthAttribute()).
			AddUObject(
				this,
				&UAesirBossRewardComponent::HandleBossHealthChanged);

		SuccessfulDefendHandle = BossAbilitySystem->
			GenericGameplayEventCallbacks.FindOrAdd(
				AesirGameplayTags::Event_Combat_Defend_Blocked).
			AddUObject(
				this,
				&UAesirBossRewardComponent::HandleSuccessfulDefend);
	}
}

void UAesirBossRewardComponent::BeginEpisode()
{
	bHasLastAcceptedAction = false;
	ConsecutiveAcceptedActionCount = 0;
}

void UAesirBossRewardComponent::SetTarget(AActor* InTarget)
{
	if (Target.Get() == InTarget)
		return;

	UnbindTargetHealth();
	Target = InTarget;
	TargetAbilitySystem = ResolveAbilitySystem(InTarget);

	if (TargetAbilitySystem.IsValid())
	{
		TargetHealthChangedHandle = TargetAbilitySystem->
			GetGameplayAttributeValueChangeDelegate(
				UAesirAttributeSet::GetHealthAttribute()).
			AddUObject(
				this,
				&UAesirBossRewardComponent::HandleTargetHealthChanged);
	}
}

void UAesirBossRewardComponent::Shutdown()
{
	UnbindBossHealth();
	UnbindTargetHealth();
	Avatar = nullptr;
	TelemetryComponent = nullptr;
	bHasLastAcceptedAction = false;
	ConsecutiveAcceptedActionCount = 0;
}

void UAesirBossRewardComponent::HandleActionOutcome(
	const FAesirBossActionOutcome& Outcome)
{
	RecordReward(
		RewardSettings.DecisionStepPenalty,
		EAesirBossRewardReason::DecisionStep);

	if (!Outcome.WasAccepted())
	{
		RecordReward(
			RewardSettings.RejectedActionPenalty,
			EAesirBossRewardReason::RejectedAction);
		return;
	}

	if (bHasLastAcceptedAction &&
		LastAcceptedAction == Outcome.Action)
	{
		++ConsecutiveAcceptedActionCount;
	}
	else
	{
		LastAcceptedAction = Outcome.Action;
		ConsecutiveAcceptedActionCount = 1;
		bHasLastAcceptedAction = true;
	}

	if (ConsecutiveAcceptedActionCount >
		RewardSettings.RepetitionGraceCount)
	{
		RecordReward(
			RewardSettings.RepeatedActionPenalty,
			EAesirBossRewardReason::RepeatedAction,
			static_cast<float>(ConsecutiveAcceptedActionCount));
	}
}

FAesirBossRewardSettings
UAesirBossRewardComponent::GetRewardSettings() const
{
	return RewardSettings;
}

void UAesirBossRewardComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	Shutdown();
	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent*
UAesirBossRewardComponent::ResolveAbilitySystem(AActor* Actor)
{
	return IsValid(Actor)
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor)
		: nullptr;
}

void UAesirBossRewardComponent::UnbindBossHealth()
{
	if (BossAbilitySystem.IsValid() &&
		SuccessfulDefendHandle.IsValid())
	{
		if (FGameplayEventMulticastDelegate* Delegate =
			BossAbilitySystem->GenericGameplayEventCallbacks.Find(
				AesirGameplayTags::Event_Combat_Defend_Blocked))
		{
			Delegate->Remove(SuccessfulDefendHandle);
		}
	}

	SuccessfulDefendHandle.Reset();

	if (BossAbilitySystem.IsValid() &&
		BossHealthChangedHandle.IsValid())
	{
		BossAbilitySystem->GetGameplayAttributeValueChangeDelegate(
			UAesirAttributeSet::GetHealthAttribute()).Remove(
				BossHealthChangedHandle);
	}

	BossHealthChangedHandle.Reset();
	BossAbilitySystem.Reset();
}

void UAesirBossRewardComponent::HandleSuccessfulDefend(
	const FGameplayEventData* Payload)
{
	if (!Payload)
	{
		return;
	}

	const float DefendedDamage = FMath::Max(
		Payload->EventMagnitude,
		0.0f);
	RecordReward(
		DefendedDamage *
			RewardSettings.DefendedDamageRewardPerPoint,
		EAesirBossRewardReason::SuccessfulDefend,
		DefendedDamage);
}

void UAesirBossRewardComponent::UnbindTargetHealth()
{
	if (TargetAbilitySystem.IsValid() &&
		TargetHealthChangedHandle.IsValid())
	{
		TargetAbilitySystem->GetGameplayAttributeValueChangeDelegate(
			UAesirAttributeSet::GetHealthAttribute()).Remove(
				TargetHealthChangedHandle);
	}

	TargetHealthChangedHandle.Reset();
	TargetAbilitySystem.Reset();
	Target.Reset();
}

void UAesirBossRewardComponent::HandleBossHealthChanged(
	const FOnAttributeChangeData& Data)
{
	if (Data.NewValue >= Data.OldValue)
		return;

	const float DamageReceived = Data.OldValue - Data.NewValue;
	RecordReward(
		-DamageReceived *
			RewardSettings.DamageReceivedPenaltyPerPoint,
		EAesirBossRewardReason::DamageReceived,
		DamageReceived);

	if (Data.OldValue > 0.0f && Data.NewValue <= 0.0f)
	{
		RecordReward(
			RewardSettings.BossDefeatedPenalty,
			EAesirBossRewardReason::BossDefeated);

		if (IsValid(TelemetryComponent))
		{
			TelemetryComponent->EndEpisode(
				EAesirBossEpisodeResult::BossDefeat);
		}
	}
}

void UAesirBossRewardComponent::HandleTargetHealthChanged(
	const FOnAttributeChangeData& Data)
{
	if (Data.NewValue >= Data.OldValue)
		return;

	const float DamageDealt = Data.OldValue - Data.NewValue;
	RecordReward(
		DamageDealt * RewardSettings.DamageDealtRewardPerPoint,
		EAesirBossRewardReason::DamageDealt,
		DamageDealt);

	if (Data.OldValue > 0.0f && Data.NewValue <= 0.0f)
	{
		RecordReward(
			RewardSettings.TargetDefeatedReward,
			EAesirBossRewardReason::TargetDefeated);

		if (IsValid(TelemetryComponent))
		{
			TelemetryComponent->EndEpisode(
				EAesirBossEpisodeResult::BossVictory);
		}
	}
}

void UAesirBossRewardComponent::RecordReward(
	float Reward,
	EAesirBossRewardReason Reason,
	float SourceMagnitude)
{
	if (IsValid(TelemetryComponent))
	{
		TelemetryComponent->AddRewardToLatestDecision(
			Reward,
			Reason,
			SourceMagnitude);
	}
}
