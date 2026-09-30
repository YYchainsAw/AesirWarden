#include "Abilities/Boss/AesirBossDodgeAbility.h"

#include "AbilitySystemComponent.h"
#include "AI/Boss/AesirBossActionComponent.h"
#include "AI/Boss/AesirBossAIController.h"
#include "Tags/AesirGameplayTags.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	const FName BossDodgeInvulnerabilityWindowName(
		TEXT("InvulnerabilityWindow"));
}

UAesirBossDodgeAbility::UAesirBossDodgeAbility()
{
	NetExecutionPolicy =
		EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ActivationOwnedTags.AddTag(
		AesirGameplayTags::State_Dodging);
}

void UAesirBossDodgeAbility::ActivateAbility(
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
		(!IsValid(DodgeMontage) &&
		 !IsValid(LeftDodgeMontage) &&
		 !IsValid(RightDodgeMontage)) ||
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

	AnimInstance->OnPlayMontageNotifyBegin.AddUniqueDynamic(
		this,
		&UAesirBossDodgeAbility::HandleMontageNotifyBegin);
	AnimInstance->OnPlayMontageNotifyEnd.AddUniqueDynamic(
		this,
		&UAesirBossDodgeAbility::HandleMontageNotifyEnd);
	AnimInstance->OnMontageEnded.AddUniqueDynamic(
		this,
		&UAesirBossDodgeAbility::HandleMontageEnded);

	AvatarCharacter->GetCharacterMovement()->StopMovementImmediately();
	AvatarCharacter->SetAnimRootMotionTranslationScale(
		RootMotionTranslationScale);

	if (ActiveController.IsValid())
	{
		ActiveController->StopMovement();

		const UAesirBossActionComponent* ActionComponent =
			ActiveController->GetBossActionComponent();
		AActor* TargetActor = ActionComponent
			? ActionComponent->GetCurrentActionTarget()
			: nullptr;
		if (IsValid(TargetActor))
		{
			FVector ToTarget =
				TargetActor->GetActorLocation() -
				AvatarCharacter->GetActorLocation();
			ToTarget.Z = 0.0f;

			if (ToTarget.Normalize())
			{
				AvatarCharacter->SetActorRotation(
					ToTarget.Rotation());
			}
		}

		ActiveController->LockActionFacing();
	}

	ActiveDodgeMontage = SelectDodgeMontage();
	if (!IsValid(ActiveDodgeMontage) ||
		AnimInstance->Montage_Play(
			ActiveDodgeMontage,
			PlayRate) <= 0.0f)
	{
		EndAbility(
			Handle,
			ActorInfo,
			ActivationInfo,
			true,
			true);
	}
}

void UAesirBossDodgeAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UAnimInstance* AnimInstance = ActiveAnimInstance.Get();
	ACharacter* AvatarCharacter = ActiveCharacter.Get();

	UnbindFromAnimInstance();
	SetInvulnerability(false);

	if (IsValid(AvatarCharacter))
	{
		AvatarCharacter->SetAnimRootMotionTranslationScale(1.0f);
	}

	if (ActiveController.IsValid())
	{
		ActiveController->EndActionFacing();
	}

	if (bWasCancelled && IsValid(AnimInstance) &&
		IsValid(ActiveDodgeMontage) &&
		AnimInstance->Montage_IsPlaying(ActiveDodgeMontage))
	{
		AnimInstance->Montage_Stop(
			CancelBlendOutTime,
			ActiveDodgeMontage);
	}

	ActiveCharacter = nullptr;
	ActiveAbilitySystem = nullptr;
	ActiveAnimInstance = nullptr;
	ActiveDodgeMontage = nullptr;
	ActiveController.Reset();

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UAesirBossDodgeAbility::HandleMontageNotifyBegin(
	FName NotifyName,
	const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (IsDodgeNotify(NotifyName, BranchingPointPayload))
	{
		SetInvulnerability(true);
	}
}

void UAesirBossDodgeAbility::HandleMontageNotifyEnd(
	FName NotifyName,
	const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (IsDodgeNotify(NotifyName, BranchingPointPayload))
	{
		SetInvulnerability(false);
	}
}

void UAesirBossDodgeAbility::HandleMontageEnded(
	UAnimMontage* Montage,
	bool bInterrupted)
{
	if (Montage != ActiveDodgeMontage || !IsActive())
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

bool UAesirBossDodgeAbility::IsDodgeNotify(
	FName NotifyName,
	const FBranchingPointNotifyPayload& BranchingPointPayload) const
{
	return NotifyName == BossDodgeInvulnerabilityWindowName &&
		Cast<UAnimMontage>(BranchingPointPayload.SequenceAsset) ==
		ActiveDodgeMontage;
}

UAnimMontage* UAesirBossDodgeAbility::SelectDodgeMontage()
{
	switch (DirectionPolicy)
	{
	case EAesirBossDodgeDirectionPolicy::Left:
		return IsValid(LeftDodgeMontage)
			? LeftDodgeMontage.Get()
			: DodgeMontage.Get();

	case EAesirBossDodgeDirectionPolicy::Right:
		return IsValid(RightDodgeMontage)
			? RightDodgeMontage.Get()
			: DodgeMontage.Get();

	case EAesirBossDodgeDirectionPolicy::AlternateLateral:
	{
		UAnimMontage* PreferredMontage = bPreferLeftDodge
			? LeftDodgeMontage.Get()
			: RightDodgeMontage.Get();
		UAnimMontage* OtherMontage = bPreferLeftDodge
			? RightDodgeMontage.Get()
			: LeftDodgeMontage.Get();
		bPreferLeftDodge = !bPreferLeftDodge;

		if (IsValid(PreferredMontage))
		{
			return PreferredMontage;
		}

		return IsValid(OtherMontage)
			? OtherMontage
			: DodgeMontage.Get();
	}

	case EAesirBossDodgeDirectionPolicy::SafestLateral:
	{
		const FVector RightDirection =
			ActiveCharacter->GetActorRightVector();
		const float LeftClearance = MeasureDirectionClearance(
			-RightDirection);
		const float RightClearance = MeasureDirectionClearance(
			RightDirection);
		const bool bChooseLeft = FMath::IsNearlyEqual(
			LeftClearance,
			RightClearance)
			? bPreferLeftDodge
			: LeftClearance > RightClearance;
		bPreferLeftDodge = !bChooseLeft;

		UAnimMontage* ChosenMontage = bChooseLeft
			? LeftDodgeMontage.Get()
			: RightDodgeMontage.Get();
		UAnimMontage* OtherMontage = bChooseLeft
			? RightDodgeMontage.Get()
			: LeftDodgeMontage.Get();

		if (IsValid(ChosenMontage))
		{
			return ChosenMontage;
		}

		return IsValid(OtherMontage)
			? OtherMontage
			: DodgeMontage.Get();
	}

	case EAesirBossDodgeDirectionPolicy::SafestRetreat:
	{
		const FVector ForwardDirection =
			ActiveCharacter->GetActorForwardVector();
		const FVector RightDirection =
			ActiveCharacter->GetActorRightVector();
		const FVector BackDirection = -ForwardDirection;
		const FVector BackLeftDirection =
			(BackDirection - RightDirection).GetSafeNormal();
		const FVector BackRightDirection =
			(BackDirection + RightDirection).GetSafeNormal();

		const float BackClearance = MeasureDirectionClearance(
			BackDirection);
		const float BackLeftClearance = MeasureDirectionClearance(
			BackLeftDirection);
		const float BackRightClearance = MeasureDirectionClearance(
			BackRightDirection);

		if (BackClearance >= BackLeftClearance &&
			BackClearance >= BackRightClearance &&
			IsValid(DodgeMontage))
		{
			return DodgeMontage.Get();
		}

		const bool bChooseLeft = BackLeftClearance >= BackRightClearance;
		UAnimMontage* ChosenMontage = bChooseLeft
			? LeftDodgeMontage.Get()
			: RightDodgeMontage.Get();
		UAnimMontage* OtherMontage = bChooseLeft
			? RightDodgeMontage.Get()
			: LeftDodgeMontage.Get();

		if (IsValid(ChosenMontage))
		{
			return ChosenMontage;
		}

		return IsValid(OtherMontage)
			? OtherMontage
			: DodgeMontage.Get();
	}

	case EAesirBossDodgeDirectionPolicy::Backward:
	default:
		return DodgeMontage.Get();
	}
}

float UAesirBossDodgeAbility::MeasureDirectionClearance(
	const FVector& Direction) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World) ||
		!IsValid(ActiveCharacter) ||
		ClearanceProbeDistance <= 0.0f)
	{
		return 0.0f;
	}

	const FVector Start = ActiveCharacter->GetActorLocation();
	const FVector SafeDirection = Direction.GetSafeNormal2D();
	const FVector End = Start +
		SafeDirection * ClearanceProbeDistance;

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(BossDodgeClearance),
		false,
		ActiveCharacter.Get());
	if (ActiveController.IsValid())
	{
		if (const UAesirBossActionComponent* ActionComponent =
			ActiveController->GetBossActionComponent())
		{
			QueryParams.AddIgnoredActor(
				ActionComponent->GetCurrentActionTarget());
		}
	}

	FHitResult Hit;
	const bool bBlocked = World->SweepSingleByChannel(
		Hit,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(ClearanceProbeRadius),
		QueryParams);

	return bBlocked ? Hit.Distance : ClearanceProbeDistance;
}

void UAesirBossDodgeAbility::SetInvulnerability(bool bEnabled)
{
	if (!IsValid(ActiveAbilitySystem) ||
		bInvulnerabilityGranted == bEnabled)
	{
		return;
	}

	if (bEnabled)
	{
		ActiveAbilitySystem->AddLooseGameplayTag(
			AesirGameplayTags::State_Invulnerable);
	}
	else
	{
		ActiveAbilitySystem->RemoveLooseGameplayTag(
			AesirGameplayTags::State_Invulnerable);
	}

	bInvulnerabilityGranted = bEnabled;
}

void UAesirBossDodgeAbility::UnbindFromAnimInstance()
{
	if (!IsValid(ActiveAnimInstance))
	{
		return;
	}

	ActiveAnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(
		this,
		&UAesirBossDodgeAbility::HandleMontageNotifyBegin);
	ActiveAnimInstance->OnPlayMontageNotifyEnd.RemoveDynamic(
		this,
		&UAesirBossDodgeAbility::HandleMontageNotifyEnd);
	ActiveAnimInstance->OnMontageEnded.RemoveDynamic(
		this,
		&UAesirBossDodgeAbility::HandleMontageEnded);
}
