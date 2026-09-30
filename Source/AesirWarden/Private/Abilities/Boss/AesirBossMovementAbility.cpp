#include "Abilities/Boss/AesirBossMovementAbility.h"

#include "AbilitySystemComponent.h"
#include "AI/Boss/AesirBossActionComponent.h"
#include "AI/Boss/AesirBossAIController.h"
#include "Tags/AesirGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

UAesirBossMovementAbility::UAesirBossMovementAbility()
{
	NetExecutionPolicy =
		EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UAesirBossMovementAbility::ActivateAbility(
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
	AAesirBossAIController* BossController = AvatarCharacter
		? Cast<AAesirBossAIController>(AvatarCharacter->GetController())
		: nullptr;
	UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponentFromActorInfo();
	const bool bSupportedAction =
		MovementAction == EAesirBossAction::Pursue ||
		MovementAction == EAesirBossAction::Disengage;

	if (!IsValid(AvatarCharacter) ||
		!IsValid(BossController) ||
		!IsValid(AbilitySystem) ||
		!bSupportedAction)
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
	ActiveController = BossController;

	AActor* TargetActor = ResolveActionTarget();
	if (!IsValid(TargetActor) ||
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

	BossController->StopMovement();
	BossController->BeginActionFacing(TargetActor);
	BossController->ReceiveMoveCompleted.AddUniqueDynamic(
		this,
		&UAesirBossMovementAbility::HandleMoveCompleted);
	ApplyMovementSpeed();
	SetMovementState(true);

	const EPathFollowingRequestResult::Type MoveResult =
		StartMovement(TargetActor);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		K2_CancelAbility();
		return;
	}

	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		K2_EndAbility();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			MovementTimeoutHandle,
			this,
			&UAesirBossMovementAbility::HandleMovementTimeout,
			MovementTimeout,
			false);
	}
}

void UAesirBossMovementAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MovementTimeoutHandle);
	}

	UnbindFromController();

	if (IsValid(ActiveController))
	{
		ActiveController->StopMovement();
		ActiveController->EndActionFacing();
	}

	SetMovementState(false);
	RestoreMovementSpeed();

	ActiveCharacter = nullptr;
	ActiveAbilitySystem = nullptr;
	ActiveController = nullptr;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UAesirBossMovementAbility::HandleMoveCompleted(
	FAIRequestID RequestID,
	EPathFollowingResult::Type Result)
{
	if (!IsActive())
	{
		return;
	}

	if (Result == EPathFollowingResult::Success)
	{
		K2_EndAbility();
	}
	else
	{
		K2_CancelAbility();
	}
}

void UAesirBossMovementAbility::HandleMovementTimeout()
{
	if (IsActive())
	{
		K2_CancelAbility();
	}
}

AActor* UAesirBossMovementAbility::ResolveActionTarget() const
{
	const UAesirBossActionComponent* ActionComponent =
		IsValid(ActiveController)
		? ActiveController->GetBossActionComponent()
		: nullptr;

	return ActionComponent
		? ActionComponent->GetCurrentActionTarget()
		: nullptr;
}

EPathFollowingRequestResult::Type
UAesirBossMovementAbility::StartMovement(AActor* TargetActor)
{
	if (!IsValid(ActiveController) ||
		!IsValid(ActiveCharacter) ||
		!IsValid(TargetActor))
	{
		return EPathFollowingRequestResult::Failed;
	}

	if (MovementAction == EAesirBossAction::Pursue)
	{
		return ActiveController->MoveToActor(
			TargetActor,
			PursueAcceptanceRadius,
			false,
			true,
			true,
			nullptr,
			true);
	}

	FVector AwayDirection =
		ActiveCharacter->GetActorLocation() -
		TargetActor->GetActorLocation();
	AwayDirection.Z = 0.0f;

	if (!AwayDirection.Normalize())
	{
		AwayDirection =
			-ActiveCharacter->GetActorForwardVector();
	}

	const FVector Destination =
		ActiveCharacter->GetActorLocation() +
		AwayDirection * DisengageDistance;

	return ActiveController->MoveToLocation(
		Destination,
		DisengageAcceptanceRadius,
		false,
		true,
		true,
		true,
		nullptr,
		true);
}

void UAesirBossMovementAbility::ApplyMovementSpeed()
{
	if (!IsValid(ActiveCharacter))
	{
		return;
	}

	UCharacterMovementComponent* Movement =
		ActiveCharacter->GetCharacterMovement();
	if (!IsValid(Movement))
	{
		return;
	}

	PreviousMaxWalkSpeed = Movement->MaxWalkSpeed;

	switch (MovementAction)
	{
	case EAesirBossAction::Pursue:
		Movement->MaxWalkSpeed = PursueMoveSpeed;
		break;

	case EAesirBossAction::Disengage:
		Movement->MaxWalkSpeed = DisengageMoveSpeed;
		break;

	default:
		return;
	}

	bMovementSpeedOverridden = true;
}

void UAesirBossMovementAbility::RestoreMovementSpeed()
{
	if (!bMovementSpeedOverridden)
	{
		return;
	}

	if (IsValid(ActiveCharacter))
	{
		if (UCharacterMovementComponent* Movement =
			ActiveCharacter->GetCharacterMovement())
		{
			Movement->MaxWalkSpeed = PreviousMaxWalkSpeed;
		}
	}

	bMovementSpeedOverridden = false;
}

void UAesirBossMovementAbility::SetMovementState(bool bEnabled)
{
	if (!IsValid(ActiveAbilitySystem) ||
		bMovementStateGranted == bEnabled)
	{
		return;
	}

	const FGameplayTag StateTag =
		MovementAction == EAesirBossAction::Pursue
		? AesirGameplayTags::State_Pursuing.GetTag()
		: AesirGameplayTags::State_Disengaging.GetTag();

	if (bEnabled)
	{
		ActiveAbilitySystem->AddLooseGameplayTag(StateTag);
	}
	else
	{
		ActiveAbilitySystem->RemoveLooseGameplayTag(StateTag);
	}

	bMovementStateGranted = bEnabled;
}

void UAesirBossMovementAbility::UnbindFromController()
{
	if (IsValid(ActiveController))
	{
		ActiveController->ReceiveMoveCompleted.RemoveDynamic(
			this,
			&UAesirBossMovementAbility::HandleMoveCompleted);
	}
}
