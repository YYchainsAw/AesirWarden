#include "Abilities/Boss/AesirBossAttackAbility.h"

#include "AI/Boss/AesirBossActionComponent.h"
#include "AI/Boss/AesirBossAIController.h"
#include "Tags/AesirGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Combat/AesirCombatComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "MotionWarpingComponent.h"

namespace
{
	EAesirAttackType ResolveAttackType(EAesirBossAction Action)
	{
		switch (Action)
		{
		case EAesirBossAction::LightAttack:
			return EAesirAttackType::Light;

		case EAesirBossAction::HeavyAttack:
			return EAesirAttackType::Heavy;

		case EAesirBossAction::GapCloserSkill:
			return EAesirAttackType::GapCloser;

		case EAesirBossAction::UnblockableAreaSkill:
			return EAesirAttackType::UnblockableArea;

		default:
			return EAesirAttackType::None;
		}
	}
}

UAesirBossAttackAbility::UAesirBossAttackAbility()
{
	NetExecutionPolicy =
		EGameplayAbilityNetExecutionPolicy::ServerOnly;

	ActivationBlockedTags.AddTag(
		AesirGameplayTags::Cooldown_Boss_GlobalAttack);
}

bool UAesirBossAttackAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(
		Handle,
		ActorInfo,
		SourceTags,
		TargetTags,
		OptionalRelevantTags))
	{
		return false;
	}

	const AActor* AvatarActor = ActorInfo
		? ActorInfo->AvatarActor.Get()
		: nullptr;
	const UAesirCombatComponent* CombatComponent = AvatarActor
		? AvatarActor->FindComponentByClass<UAesirCombatComponent>()
		: nullptr;

	return IsValid(CombatComponent) &&
		CombatComponent->CanStartAttack(
			ResolveAttackType(AttackAction));
}

void UAesirBossAttackAbility::ActivateAbility(
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

	AActor* AvatarActor = ActorInfo
		? ActorInfo->AvatarActor.Get()
		: nullptr;
	ActiveCombatComponent = AvatarActor
		? AvatarActor->FindComponentByClass<UAesirCombatComponent>()
		: nullptr;

	if (!IsValid(ActiveCombatComponent) ||
		ResolveAttackType(AttackAction) == EAesirAttackType::None ||
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

	ActiveCombatComponent->OnAttackStateChanged.AddUniqueDynamic(
		this,
		&UAesirBossAttackAbility::HandleAttackStateChanged);
	ActiveCombatComponent->OnAttackWindowStarted.AddUniqueDynamic(
		this,
		&UAesirBossAttackAbility::HandleAttackWindowStarted);

	BeginTargetFacing(AvatarActor);
	ConfigureGapCloserMotionWarpTarget(AvatarActor);

	switch (AttackAction)
	{
	case EAesirBossAction::HeavyAttack:
		ActiveCombatComponent->TryHeavyAttack();
		break;

	case EAesirBossAction::GapCloserSkill:
		ActiveCombatComponent->TryGapCloserSkill();
		break;

	case EAesirBossAction::UnblockableAreaSkill:
		ActiveCombatComponent->TryUnblockableAreaSkill();
		break;

	default:
		ActiveCombatComponent->TryLightAttack();
		break;
	}

	if (!ActiveCombatComponent->IsAttacking())
	{
		EndAbility(
			Handle,
			ActorInfo,
			ActivationInfo,
			true,
			true);
	}
}

void UAesirBossAttackAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UAesirCombatComponent* CombatComponent =
		ActiveCombatComponent.Get();
	if (!bWasCancelled && PostAttackRecoveryEffectClass && ActorInfo)
	{
		if (UAbilitySystemComponent* AbilitySystem =
			ActorInfo->AbilitySystemComponent.Get())
		{
			FGameplayEffectContextHandle Context =
				AbilitySystem->MakeEffectContext();
			Context.AddSourceObject(this);
			const FGameplayEffectSpecHandle Spec =
				AbilitySystem->MakeOutgoingSpec(
					PostAttackRecoveryEffectClass,
					GetAbilityLevel(),
					Context);
			if (Spec.IsValid())
			{
				AbilitySystem->ApplyGameplayEffectSpecToSelf(
					*Spec.Data.Get());
			}
		}
	}

	ClearGapCloserMotionWarpTarget();
	EndTargetFacing();
	UnbindFromCombatComponent();

	if (bWasCancelled && IsValid(CombatComponent) &&
		CombatComponent->IsAttacking())
	{
		CombatComponent->CancelAttack();
	}

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UAesirBossAttackAbility::HandleAttackStateChanged(
	bool bIsAttacking)
{
	if (!bIsAttacking && IsActive())
		K2_EndAbility();
}

void UAesirBossAttackAbility::HandleAttackWindowStarted()
{
	if (FacingController.IsValid())
	{
		FacingController->LockActionFacing();
	}
}

void UAesirBossAttackAbility::BeginTargetFacing(AActor* AvatarActor)
{
	APawn* AvatarPawn = Cast<APawn>(AvatarActor);
	AAesirBossAIController* AIController = AvatarPawn
		? Cast<AAesirBossAIController>(AvatarPawn->GetController())
		: nullptr;
	UAesirBossActionComponent* ActionComponent = AIController
		? AIController->FindComponentByClass<UAesirBossActionComponent>()
		: nullptr;
	AActor* TargetActor = ActionComponent
		? ActionComponent->GetCurrentActionTarget()
		: nullptr;

	if (!IsValid(AIController) || !IsValid(TargetActor))
	{
		return;
	}

	AIController->StopMovement();
	AIController->BeginActionFacing(TargetActor);
	FacingController = AIController;
}

void UAesirBossAttackAbility::EndTargetFacing()
{
	if (FacingController.IsValid())
	{
		FacingController->EndActionFacing();
	}

	FacingController.Reset();
}

void UAesirBossAttackAbility::ConfigureGapCloserMotionWarpTarget(
	AActor* AvatarActor)
{
	if (AttackAction != EAesirBossAction::GapCloserSkill ||
		!bUseGapCloserMotionWarping ||
		!IsValid(AvatarActor) ||
		!FacingController.IsValid())
	{
		return;
	}

	const UAesirBossActionComponent* ActionComponent =
		FacingController->GetBossActionComponent();
	AActor* TargetActor = ActionComponent
		? ActionComponent->GetCurrentActionTarget()
		: nullptr;
	UMotionWarpingComponent* MotionWarping =
		AvatarActor->FindComponentByClass<UMotionWarpingComponent>();
	if (!IsValid(TargetActor) || !IsValid(MotionWarping))
	{
		return;
	}

	FVector FromTargetToBoss =
		AvatarActor->GetActorLocation() - TargetActor->GetActorLocation();
	FromTargetToBoss.Z = 0.0f;
	if (!FromTargetToBoss.Normalize())
	{
		FromTargetToBoss = -TargetActor->GetActorForwardVector();
		FromTargetToBoss.Z = 0.0f;
		FromTargetToBoss.Normalize();
	}

	FVector WarpLocation = TargetActor->GetActorLocation() +
		FromTargetToBoss * GapCloserStoppingDistance;
	WarpLocation.Z = AvatarActor->GetActorLocation().Z;

	FVector FacingDirection = TargetActor->GetActorLocation() - WarpLocation;
	FacingDirection.Z = 0.0f;
	const FRotator WarpRotation = FacingDirection.IsNearlyZero()
		? AvatarActor->GetActorRotation()
		: FacingDirection.Rotation();

	MotionWarping->AddOrUpdateWarpTargetFromTransform(
		GapCloserWarpTargetName,
		FTransform(WarpRotation, WarpLocation));
	ActiveMotionWarpingComponent = MotionWarping;
}

void UAesirBossAttackAbility::ClearGapCloserMotionWarpTarget()
{
	if (ActiveMotionWarpingComponent.IsValid())
	{
		ActiveMotionWarpingComponent->RemoveWarpTarget(
			GapCloserWarpTargetName);
	}

	ActiveMotionWarpingComponent.Reset();
}

void UAesirBossAttackAbility::UnbindFromCombatComponent()
{
	if (IsValid(ActiveCombatComponent))
	{
		ActiveCombatComponent->OnAttackStateChanged.RemoveDynamic(
			this,
			&UAesirBossAttackAbility::HandleAttackStateChanged);
		ActiveCombatComponent->OnAttackWindowStarted.RemoveDynamic(
			this,
			&UAesirBossAttackAbility::HandleAttackWindowStarted);
	}

	ActiveCombatComponent = nullptr;
}
