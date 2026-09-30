#include "Abilities/Boss/AesirBossDefendAbility.h"

#include "AbilitySystemComponent.h"
#include "AI/Boss/AesirBossActionComponent.h"
#include "AI/Boss/AesirBossAIController.h"
#include "Tags/AesirGameplayTags.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

UAesirBossDefendAbility::UAesirBossDefendAbility()
{
	NetExecutionPolicy =
		EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UAesirBossDefendAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		TriggerEventData);

	ACharacter* AvatarCharacter = ActorInfo
		? Cast<ACharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;
	UAnimInstance* AnimInstance = AvatarCharacter &&
		AvatarCharacter->GetMesh()
		? AvatarCharacter->GetMesh()->GetAnimInstance()
		: nullptr;
	UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponentFromActorInfo();

	if (!IsValid(AvatarCharacter) ||
		!IsValid(AnimInstance) ||
		!IsValid(AbilitySystem) ||
		!IsValid(DefendMontage) ||
		!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(
			Handle,
			ActorInfo,
			ActivationInfo,
			true,
			true);
		return;
	}

	ActiveCharacter = AvatarCharacter;
	ActiveAbilitySystem = AbilitySystem;
	ActiveAnimInstance = AnimInstance;
	ActiveController = Cast<AAesirBossAIController>(
		AvatarCharacter->GetController());

	AnimInstance->OnMontageEnded.AddUniqueDynamic(
		this,
		&UAesirBossDefendAbility::HandleMontageEnded);

	SetDefendState(true);
	AvatarCharacter->GetCharacterMovement()->StopMovementImmediately();

	if (ActiveController.IsValid())
	{
		ActiveController->StopMovement();

		UAesirBossActionComponent* ActionComponent =
			ActiveController->FindComponentByClass<
				UAesirBossActionComponent>();
		AActor* TargetActor = ActionComponent
			? ActionComponent->GetCurrentActionTarget()
			: nullptr;

		if (IsValid(TargetActor))
		{
			ActiveController->BeginActionFacing(TargetActor);
		}
		else
		{
			ActiveController->LockActionFacing();
		}
	}

	if (AnimInstance->Montage_Play(DefendMontage, PlayRate) <= 0.0f)
	{
		EndAbility(
			Handle,
			ActorInfo,
			ActivationInfo,
			true,
			true);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DefendTimerHandle,
			this,
			&UAesirBossDefendAbility::HandleDefendDurationElapsed,
			DefendDuration,
			false);
	}
}

void UAesirBossDefendAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UAnimInstance* AnimInstance = ActiveAnimInstance.Get();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DefendTimerHandle);
	}

	UnbindFromAnimInstance();
	SetDefendState(false);

	if (ActiveController.IsValid())
	{
		ActiveController->EndActionFacing();
	}

	if (IsValid(AnimInstance) &&
		IsValid(DefendMontage) &&
		AnimInstance->Montage_IsPlaying(DefendMontage))
	{
		AnimInstance->Montage_Stop(
			BlendOutTime,
			DefendMontage);
	}

	ActiveCharacter = nullptr;
	ActiveAbilitySystem = nullptr;
	ActiveAnimInstance = nullptr;
	ActiveController.Reset();

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UAesirBossDefendAbility::HandleDefendDurationElapsed()
{
	if (IsActive())
	{
		K2_EndAbility();
	}
}

void UAesirBossDefendAbility::HandleMontageEnded(
	UAnimMontage* Montage,
	bool bInterrupted)
{
	if (Montage != DefendMontage || !IsActive())
	{
		return;
	}

	if (bInterrupted)
	{
		K2_CancelAbility();
	}
	else
	{
		K2_EndAbility();
	}
}

void UAesirBossDefendAbility::SetDefendState(bool bEnabled)
{
	if (!IsValid(ActiveAbilitySystem) ||
		bDefendStateGranted == bEnabled)
	{
		return;
	}

	if (bEnabled)
	{
		ActiveAbilitySystem->AddLooseGameplayTag(
			AesirGameplayTags::State_Blocking);
	}
	else
	{
		ActiveAbilitySystem->RemoveLooseGameplayTag(
			AesirGameplayTags::State_Blocking);
	}

	bDefendStateGranted = bEnabled;
}

void UAesirBossDefendAbility::UnbindFromAnimInstance()
{
	if (IsValid(ActiveAnimInstance))
	{
		ActiveAnimInstance->OnMontageEnded.RemoveDynamic(
			this,
			&UAesirBossDefendAbility::HandleMontageEnded);
	}
}
