#include "Combat/AesirCombatComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Abilities/AesirAttributeSet.h"
#include "Abilities/AesirPlayerAttributeSet.h"
#include "Tags/AesirGameplayTags.h"
#include "Combat/AesirPoiseComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Combat/AesirCombatStateComponent.h"
#include "Components/MeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	const FName ComboWindowName(TEXT("ComboWindow"));
	const FName AttackWindowName(TEXT("AttackWindow"));

	const TCHAR* GetGuardInteractionName(
		EAesirGuardInteraction GuardInteraction)
	{
		switch (GuardInteraction)
		{
		case EAesirGuardInteraction::Blockable:
			return TEXT("Blockable");

		case EAesirGuardInteraction::PerfectGuardRequired:
			return TEXT("PerfectGuardRequired");

		case EAesirGuardInteraction::Unblockable:
			return TEXT("Unblockable");

		default:
			return TEXT("Unknown");
		}
	}

	const TCHAR* GetPerfectGuardReactionName(
		EAesirPerfectGuardReaction Reaction)
	{
		switch (Reaction)
		{
		case EAesirPerfectGuardReaction::StaggerAttacker:
			return TEXT("StaggerAttacker");

		case EAesirPerfectGuardReaction::ContinueCombo:
			return TEXT("ContinueCombo");

		default:
			return TEXT("Unknown");
		}
	}
}

UAesirCombatComponent::UAesirCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; 
}

void UAesirCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UAnimInstance* AnimInstance = GetOwnerAnimInstance())
	{
		AnimInstance->OnPlayMontageNotifyBegin.AddUniqueDynamic(
			this, &UAesirCombatComponent::HandleMontageNotifyBegin);
		AnimInstance->OnPlayMontageNotifyEnd.AddUniqueDynamic(
			this, &UAesirCombatComponent::HandleMontageNotifyEnd);
		AnimInstance->OnMontageEnded.AddUniqueDynamic(
			this, &UAesirCombatComponent::HandleMontageEnded);
	}
	
	CacheWeaponMesh();
}

void UAesirCombatComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!bAttackTraceActive || !IsValid(WeaponMesh)) 
		return;
	
	const FVector CurrentTraceStart =
	   WeaponMesh->GetSocketLocation(TraceStartSocketName);

	const FVector CurrentTraceEnd =
		WeaponMesh->GetSocketLocation(TraceEndSocketName);
	
	if (UWorld* World = GetWorld())
	{
		DrawDebugLine(
			World,
			PreviousTraceStart,
			CurrentTraceStart,
			FColor::Red,
			false,
			0.5f,
			0,
			2.0f);

		DrawDebugLine(
			World,
			PreviousTraceEnd,
			CurrentTraceEnd,
			FColor::Green,
			false,
			0.5f,
			0,
			2.0f);

		DrawDebugLine(
			World,
			CurrentTraceStart,
			CurrentTraceEnd,
			FColor::Yellow,
			false,
			0.5f,
			0,
			1.5f);
	}
	PerformAttackTrace(CurrentTraceStart, CurrentTraceEnd);
	
	PreviousTraceStart = CurrentTraceStart;
	PreviousTraceEnd = CurrentTraceEnd;
}

void UAesirCombatComponent::PerformAttackTrace(const FVector& CurrentTraceStart, const FVector& CurrentTraceEnd)
{
	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();

	if (!World || !IsValid(OwnerActor) || TraceSampleCount < 2)
	{
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(AesirWeaponTrace),
		false,
		OwnerActor);

	const FCollisionShape TraceShape =
		FCollisionShape::MakeSphere(TraceRadius);
	
	if (CurrentAttackType == EAesirAttackType::None)
		return;

	float AttackDamage = LightAttackDamage;
	float AttackGuardPressure = LightAttackGuardPressure;
	EAesirGuardInteraction AttackGuardInteraction =
		LightAttackGuardInteraction;
	float AttackPoiseDamage = LightAttackPoiseDamage;

	switch (CurrentAttackType)
	{
	case EAesirAttackType::Heavy:
		AttackDamage = HeavyAttackDamage;
		AttackGuardPressure = HeavyAttackGuardPressure;
		AttackGuardInteraction = HeavyAttackGuardInteraction;
		AttackPoiseDamage = HeavyAttackPoiseDamage;
		break;

	case EAesirAttackType::GapCloser:
		AttackDamage = GapCloserDamage;
		AttackGuardPressure = GapCloserGuardPressure;
		AttackGuardInteraction = GapCloserGuardInteraction;
		AttackPoiseDamage = GapCloserPoiseDamage;
		break;

	case EAesirAttackType::UnblockableArea:
		AttackDamage = UnblockableAreaDamage;
		AttackGuardPressure = UnblockableAreaGuardPressure;
		AttackGuardInteraction = UnblockableAreaGuardInteraction;
		AttackPoiseDamage = UnblockableAreaPoiseDamage;
		break;

	default:
		break;
	}
	
	for (int32 SampleIndex = 0;
		 SampleIndex < TraceSampleCount;
		 ++SampleIndex)
	{
		const float Alpha =
			static_cast<float>(SampleIndex) /
			static_cast<float>(TraceSampleCount - 1);

		const FVector PreviousSample = FMath::Lerp(
			PreviousTraceStart, PreviousTraceEnd, Alpha);

		const FVector CurrentSample = FMath::Lerp(
			CurrentTraceStart, CurrentTraceEnd, Alpha);

		TArray<FHitResult> HitResults;

		World->SweepMultiByObjectType(
			HitResults,
			PreviousSample,
			CurrentSample,
			FQuat::Identity,
			ObjectQueryParams,
			TraceShape,
			QueryParams);

		DrawDebugSphere(
			World, CurrentSample, TraceRadius, 12,
			FColor::Cyan, false, 0.5f);

		for (const FHitResult& HitResult : HitResults)
		{
			AActor* HitActor = HitResult.GetActor();

			if (!IsValid(HitActor))
			{
				continue;
			}

			const TWeakObjectPtr<AActor> HitActorKey(HitActor);

			if (HitActorsThisAttack.Contains(HitActorKey))
			{
				continue;
			}

			HitActorsThisAttack.Add(HitActorKey);
			
			AController* InstigatorController = nullptr;

			if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
				InstigatorController = OwnerPawn->GetController();
			
			const FVector HitFromDirection =
				(HitResult.ImpactPoint - OwnerActor->GetActorLocation())
				.GetSafeNormal();
			
			bool bHitNegated = false;

			if (!TryApplyGameplayDamage(
				HitActor,
				AttackDamage,
				AttackGuardPressure,
				AttackGuardInteraction,
				bHitNegated))
			{
				UGameplayStatics::ApplyPointDamage(
					HitActor,
					AttackDamage,
					HitFromDirection,
					HitResult,
					InstigatorController,
					OwnerActor,
					nullptr);
			}
			
			if (!bHitNegated)
			{
				if (UAesirPoiseComponent* HitPoiseComponent =
					HitActor->FindComponentByClass<UAesirPoiseComponent>())
				{
					HitPoiseComponent->ApplyPoiseDamage(
						AttackPoiseDamage);
				}
			}

			/*UE_LOG(
				LogTemp,
				Log,
				TEXT("Weapon trace hit: %s"),
				*HitActor->GetName());*/

			DrawDebugPoint(
				World, HitResult.ImpactPoint, 20.0f,
				FColor::Red, false, 1.0f);
		}
	}
}

bool UAesirCombatComponent::TryApplyGameplayDamage(
	AActor* TargetActor,
	float Damage,
	float GuardPressure,
	EAesirGuardInteraction GuardInteraction,
	bool& bOutHitNegated)
{
	bOutHitNegated = false;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::
			GetAbilitySystemComponent(TargetActor);

	if (!TargetASC)
		return false;

	if (TargetASC->HasMatchingGameplayTag(
		AesirGameplayTags::State_Invulnerable))
	{
		bOutHitNegated = true;
		UE_LOG(LogTemp, Display,
			TEXT("GAS damage ignored: target=%s state.invulnerable"),
			*GetNameSafe(TargetActor));
		return true;
	}

	const UAesirAttributeSet* CoreAttributes =
		TargetASC->GetSet<UAesirAttributeSet>();

	const UAesirPlayerAttributeSet* PlayerAttributes =
		TargetASC->GetSet<UAesirPlayerAttributeSet>();

	const bool bIsBlocking = TargetASC->HasMatchingGameplayTag(
		AesirGameplayTags::State_Blocking);

	const bool bIsPerfectGuard =
		bIsBlocking &&
		TargetASC->HasMatchingGameplayTag(
			AesirGameplayTags::State_PerfectGuard);

	const bool bDefendedWithoutGuardPressure =
		bIsBlocking &&
		!PlayerAttributes &&
		GuardInteraction == EAesirGuardInteraction::Blockable;

	if (bDefendedWithoutGuardPressure)
	{
		FGameplayEventData Payload;
		Payload.EventTag =
			AesirGameplayTags::Event_Combat_Defend_Blocked;
		Payload.Instigator = GetOwner();
		Payload.Target = TargetActor;
		Payload.EventMagnitude = FMath::Max(Damage, 0.0f);

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			TargetActor,
			AesirGameplayTags::Event_Combat_Defend_Blocked,
			Payload);

		bOutHitNegated = true;
		UE_LOG(LogTemp, Display,
			TEXT("Attack defended: target=%s damage=%.1f"),
			*GetNameSafe(TargetActor),
			Payload.EventMagnitude);
		return true;
	}

	if (bIsPerfectGuard &&
		GuardInteraction != EAesirGuardInteraction::Unblockable)
	{
		FGameplayEventData Payload;
		Payload.EventTag =
			AesirGameplayTags::Event_Player_Guard_Perfect;
		Payload.Instigator = GetOwner();
		Payload.Target = TargetActor;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			TargetActor,
			AesirGameplayTags::Event_Player_Guard_Perfect,
			Payload);

		const EAesirPerfectGuardReaction Reaction =
			GetCurrentPerfectGuardReaction();
		const float RecoilDuration =
			GetCurrentPerfectGuardRecoilDuration();

		if (Reaction ==
			EAesirPerfectGuardReaction::StaggerAttacker)
		{
			ApplyCounterStagger(RecoilDuration);
		}

		bOutHitNegated = true;

		UE_LOG(LogTemp, Display,
			TEXT("Perfect guard: target=%s rule=%s reaction=%s"),
			*GetNameSafe(TargetActor),
			GetGuardInteractionName(GuardInteraction),
			GetPerfectGuardReactionName(Reaction));
		return true;
	}

	float ResolvedGuardPressure = FMath::Max(
		GuardPressure,
		0.0f);

	if (bIsBlocking &&
		GuardInteraction != EAesirGuardInteraction::Blockable &&
		PlayerAttributes)
	{
		ResolvedGuardPressure = FMath::Max(
			ResolvedGuardPressure,
			PlayerAttributes->GetMaxGuardPressure());

		UE_LOG(LogTemp, Display,
			TEXT("Guard break requested: target=%s rule=%s"),
			*GetNameSafe(TargetActor),
			GetGuardInteractionName(GuardInteraction));
	}

	if (!GameplayDamageEffect)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s has GAS target but no GameplayDamageEffect."),
			*GetNameSafe(GetOwner()));
		return true;
	}

	UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::
			GetAbilitySystemComponent(GetOwner());

	UAbilitySystemComponent* SpecOwnerASC = SourceASC
		? SourceASC
		: TargetASC;

	FGameplayEffectContextHandle Context =
		SpecOwnerASC->MakeEffectContext();

	Context.AddInstigator(GetOwner(), GetOwner());
	Context.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle Spec =
		SpecOwnerASC->MakeOutgoingSpec(
			GameplayDamageEffect, 1.0f, Context);

	if (!Spec.Data.IsValid())
		return true;

	Spec.Data->SetSetByCallerMagnitude(
		AesirGameplayTags::Data_Damage,
		FMath::Max(Damage, 0.0f));

	Spec.Data->SetSetByCallerMagnitude(
		AesirGameplayTags::Data_GuardPressure,
		ResolvedGuardPressure);

	const float HealthBefore = CoreAttributes
		? CoreAttributes->GetHealth()
		: 0.0f;

	const float GuardPressureBefore = PlayerAttributes
		? PlayerAttributes->GetGuardPressure()
		: 0.0f;

	if (SourceASC)
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(
			*Spec.Data,
			TargetASC);
	}
	else
	{
		TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
	}

	if (CoreAttributes || PlayerAttributes)
	{
		const float HealthAfter = CoreAttributes
			? CoreAttributes->GetHealth()
			: 0.0f;

		const float GuardPressureAfter = PlayerAttributes
			? PlayerAttributes->GetGuardPressure()
			: 0.0f;

		const bool bBlockedWithoutBreaking =
			bIsBlocking &&
			GuardInteraction == EAesirGuardInteraction::Blockable &&
			PlayerAttributes &&
			GuardPressureAfter > GuardPressureBefore;

		if (bBlockedWithoutBreaking)
		{
			FGameplayEventData Payload;
			Payload.EventTag =
				AesirGameplayTags::Event_Player_Guard_Blocked;
			Payload.Instigator = GetOwner();
			Payload.Target = TargetActor;

			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
				TargetActor,
				AesirGameplayTags::Event_Player_Guard_Blocked,
				Payload);
		}

		UE_LOG(LogTemp, Display,
			TEXT("GAS hit: target=%s Health=%.1f->%.1f "
				"GuardPressure=%.1f->%.1f "
				"InputDamage=%.1f InputGuardPressure=%.1f "
				"AppliedGuardPressure=%.1f GuardRule=%s"),
			*GetNameSafe(TargetActor),
			HealthBefore,
			HealthAfter,
			GuardPressureBefore,
			GuardPressureAfter,
			FMath::Max(Damage, 0.0f),
			FMath::Max(GuardPressure, 0.0f),
			ResolvedGuardPressure,
			GetGuardInteractionName(GuardInteraction));
	}

	return true;
}

void UAesirCombatComponent::TryLightAttack()
{
	if (CurrentComboIndex == INDEX_NONE)
	{
		PlayAttack(EAesirAttackType::Light, 0);
		return;
	}

	if (CurrentAttackType != EAesirAttackType::Light)
		return;

	bAttackInputBuffered = true;

	if (bComboWindowOpen)
		AdvanceCombo();
}

void UAesirCombatComponent::TryHeavyAttack()
{
	if (CurrentComboIndex == INDEX_NONE)
	{
		PlayAttack(EAesirAttackType::Heavy, 0);
		return;
	}

	if (CurrentAttackType != EAesirAttackType::Heavy)
		return;

	bAttackInputBuffered = true;

	if (bComboWindowOpen)
		AdvanceCombo();
}

bool UAesirCombatComponent::IsAttacking() const
{
	return CurrentAttackType != EAesirAttackType::None &&
	   CurrentComboIndex != INDEX_NONE;
}

bool UAesirCombatComponent::IsDamageWindowActive() const
{
	return bAttackTraceActive;
}

void UAesirCombatComponent::CancelAttack()
{
	if (UAnimInstance* AnimInstance = GetOwnerAnimInstance())
	{
		if (UAnimMontage* CurrentMontage = GetCurrentMontage())
		{
			AnimInstance->Montage_Stop(0.1f, CurrentMontage);
		}
	}

	ResetCombo();
}

void UAesirCombatComponent::TryGapCloserSkill()
{
	if (CurrentComboIndex == INDEX_NONE)
		PlayAttack(EAesirAttackType::GapCloser, 0);
}

void UAesirCombatComponent::TryUnblockableAreaSkill()
{
	if (CurrentComboIndex == INDEX_NONE)
		PlayAttack(EAesirAttackType::UnblockableArea, 0);
}

bool UAesirCombatComponent::CanStartAttack(
	EAesirAttackType AttackType) const
{
	const TArray<TObjectPtr<UAnimMontage>>* AttackMontages =
		GetAttackMontages(AttackType);

	return AttackType != EAesirAttackType::None &&
		!IsAttacking() &&
		GetOwnerAnimInstance() &&
		AttackMontages &&
		AttackMontages->IsValidIndex(0) &&
		IsValid((*AttackMontages)[0]);
}

void UAesirCombatComponent::ApplyCounterStagger(float Duration)
{
	const float ResolvedDuration = FMath::Max(Duration, 0.0f);

	if (ResolvedDuration <= 0.0f)
		return;

	CancelAttack();

	if (UAbilitySystemComponent* OwnerASC =
		UAbilitySystemBlueprintLibrary::
			GetAbilitySystemComponent(GetOwner()))
	{
		OwnerASC->SetLooseGameplayTagCount(
			AesirGameplayTags::State_AttackRecoiled,
			1);
	}

	if (UAesirCombatStateComponent* StateComponent =
		GetOwner()->FindComponentByClass<UAesirCombatStateComponent>())
	{
		StateComponent->SetCombatState(
			EAesirCombatState::AttackRecoiled);
	}

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		OwnerCharacter->GetCharacterMovement()->
			StopMovementImmediately();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(
			AttackRecoilTimerHandle);
		World->GetTimerManager().SetTimer(
			AttackRecoilTimerHandle,
			this,
			&UAesirCombatComponent::EndAttackRecoil,
			ResolvedDuration,
			false);
	}

	UE_LOG(LogTemp, Display,
		TEXT("Attack recoil started: actor=%s duration=%.2f"),
		*GetNameSafe(GetOwner()),
		ResolvedDuration);
}

bool UAesirCombatComponent::ApplyGuardCounterHit(
	AActor* TargetActor,
	float Damage,
	float StaggerDuration)
{
	if (!IsValid(TargetActor) || TargetActor == GetOwner())
		return false;

	UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::
			GetAbilitySystemComponent(GetOwner());
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::
			GetAbilitySystemComponent(TargetActor);

	if (!SourceASC || !TargetASC || !GameplayDamageEffect)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Guard counter hit failed: source=%s target=%s "
				"sourceASC=%s targetASC=%s damageEffect=%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(TargetActor),
			SourceASC ? TEXT("valid") : TEXT("none"),
			TargetASC ? TEXT("valid") : TEXT("none"),
			*GetNameSafe(GameplayDamageEffect.Get()));
		return false;
	}

	FGameplayEffectContextHandle Context =
		SourceASC->MakeEffectContext();
	Context.AddInstigator(GetOwner(), GetOwner());
	Context.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(
		GameplayDamageEffect,
		1.0f,
		Context);

	if (!Spec.Data.IsValid())
		return false;

	Spec.Data->SetSetByCallerMagnitude(
		AesirGameplayTags::Data_Damage,
		FMath::Max(Damage, 0.0f));
	Spec.Data->SetSetByCallerMagnitude(
		AesirGameplayTags::Data_GuardPressure,
		0.0f);

	SourceASC->ApplyGameplayEffectSpecToTarget(
		*Spec.Data,
		TargetASC);

	const UAesirAttributeSet* TargetAttributes =
		TargetASC->GetSet<UAesirAttributeSet>();
	const bool bTargetAlive = !TargetAttributes ||
		TargetAttributes->GetHealth() > 0.0f;

	if (bTargetAlive)
	{
		if (UAesirCombatComponent* TargetCombat =
			TargetActor->FindComponentByClass<
				UAesirCombatComponent>())
		{
			TargetCombat->ApplyCounterStagger(
				StaggerDuration);
		}
	}

	UE_LOG(LogTemp, Display,
		TEXT("Guard counter hit: source=%s target=%s "
			"damage=%.1f stagger=%.2f"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(TargetActor),
		FMath::Max(Damage, 0.0f),
		FMath::Max(StaggerDuration, 0.0f));
	return true;
}

void UAesirCombatComponent::EndAttackRecoil()
{
	if (UAbilitySystemComponent* OwnerASC =
		UAbilitySystemBlueprintLibrary::
			GetAbilitySystemComponent(GetOwner()))
	{
		OwnerASC->SetLooseGameplayTagCount(
			AesirGameplayTags::State_AttackRecoiled,
			0);
	}

	if (UAesirCombatStateComponent* StateComponent =
		GetOwner()->FindComponentByClass<UAesirCombatStateComponent>())
	{
		if (StateComponent->IsInCombatState(
			EAesirCombatState::AttackRecoiled))
		{
			StateComponent->SetCombatState(
				EAesirCombatState::Combat);
		}
	}
}

EAesirPerfectGuardReaction
UAesirCombatComponent::GetCurrentPerfectGuardReaction() const
{
	const TArray<EAesirPerfectGuardReaction>* Reactions = nullptr;

	switch (CurrentAttackType)
	{
	case EAesirAttackType::Light:
		Reactions = &LightAttackPerfectGuardReactions;
		break;

	case EAesirAttackType::Heavy:
		Reactions = &HeavyAttackPerfectGuardReactions;
		break;

	case EAesirAttackType::GapCloser:
		Reactions = &GapCloserPerfectGuardReactions;
		break;

	default:
		break;
	}

	return Reactions && Reactions->IsValidIndex(CurrentAttackWindowIndex)
		? (*Reactions)[CurrentAttackWindowIndex]
		: EAesirPerfectGuardReaction::StaggerAttacker;
}

float UAesirCombatComponent::GetCurrentPerfectGuardRecoilDuration() const
{
	switch (CurrentAttackType)
	{
	case EAesirAttackType::Heavy:
		return HeavyAttackPerfectGuardRecoilDuration;

	case EAesirAttackType::GapCloser:
		return GapCloserPerfectGuardRecoilDuration;

	default:
		return LightAttackPerfectGuardRecoilDuration;
	}
}

UAnimInstance* UAesirCombatComponent::GetOwnerAnimInstance() const
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->GetMesh())
	{
		return nullptr;
	}

	return OwnerCharacter->GetMesh()->GetAnimInstance();
}

UAnimMontage* UAesirCombatComponent::GetCurrentMontage() const
{
	const TArray<TObjectPtr<UAnimMontage>>* AttackMontages =
		GetAttackMontages(CurrentAttackType);

	if (!AttackMontages ||
		!AttackMontages->IsValidIndex(CurrentComboIndex))
		return nullptr;

	return (*AttackMontages)[CurrentComboIndex].Get();
}

void UAesirCombatComponent::PlayAttack(EAesirAttackType AttackType, int32 ComboIndex)
{
	const bool bWasAttacking = IsAttacking();
	UAnimInstance* AnimInstance = GetOwnerAnimInstance();

	const TArray<TObjectPtr<UAnimMontage>>* AttackMontages =
		GetAttackMontages(AttackType);

	if (!AnimInstance ||
		!AttackMontages ||
		!AttackMontages->IsValidIndex(ComboIndex) ||
		!(*AttackMontages)[ComboIndex])
	{
		ResetCombo();
		return;
	}

	EndAttackTrace();

	CurrentAttackType = AttackType;
	CurrentComboIndex = ComboIndex;
	if (!bWasAttacking)
	{
		CurrentAttackWindowIndex = INDEX_NONE;
	}
	bAttackInputBuffered = false;
	bComboWindowOpen = false;

	if (AnimInstance->Montage_Play(
		(*AttackMontages)[ComboIndex]) <= 0.0f)
	{
		ResetCombo();
		return;
	}
	
	if (!bWasAttacking)
		OnAttackStateChanged.Broadcast(true);
}

const TArray<TObjectPtr<UAnimMontage>>* UAesirCombatComponent::GetAttackMontages(EAesirAttackType AttackType) const
{
	switch (AttackType)
	{
	case EAesirAttackType::Light:
		return &LightAttackMontages;

	case EAesirAttackType::Heavy:
		return &HeavyAttackMontages;

	case EAesirAttackType::GapCloser:
		return &GapCloserSkillMontages;

	case EAesirAttackType::UnblockableArea:
		return &UnblockableAreaSkillMontages;

	default:
		return nullptr;
	}
}

void UAesirCombatComponent::AdvanceCombo()
{
	const int32 NextComboIndex = CurrentComboIndex + 1;

	const TArray<TObjectPtr<UAnimMontage>>* AttackMontages =
		GetAttackMontages(CurrentAttackType);

	if (!AttackMontages ||
		!AttackMontages->IsValidIndex(NextComboIndex) ||
		!(*AttackMontages)[NextComboIndex])
	{
		bAttackInputBuffered = false;
		bComboWindowOpen = false;
		return;
	}

	PlayAttack(CurrentAttackType, NextComboIndex);
}

void UAesirCombatComponent::HandleMontageNotifyBegin(
	FName NotifyName,
	const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (Cast<UAnimMontage>(BranchingPointPayload.SequenceAsset) !=
		GetCurrentMontage())
	{
		return;
	}

	if (NotifyName == AttackWindowName)
	{
		++CurrentAttackWindowIndex;
		OnAttackWindowStarted.Broadcast();
		BeginAttackTrace();
		return;
	}

	if (NotifyName != ComboWindowName)
	{
		return;
	}

	bComboWindowOpen = true;

	if (bAttackInputBuffered)
	{
		AdvanceCombo();
	}
}

void UAesirCombatComponent::HandleMontageNotifyEnd(
	FName NotifyName,
	const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (Cast<UAnimMontage>(BranchingPointPayload.SequenceAsset) !=
		GetCurrentMontage())
	{
		return;
	}

	if (NotifyName == AttackWindowName)
	{
		EndAttackTrace();
		return;
	}

	if (NotifyName != ComboWindowName)
	{
		return;
	}

	bComboWindowOpen = false;
	bAttackInputBuffered = false;
}

void UAesirCombatComponent::HandleMontageEnded(
	UAnimMontage* Montage,
	bool bInterrupted)
{
	if (Montage == GetCurrentMontage())
	{
		ResetCombo();
	}
}

void UAesirCombatComponent::ResetCombo()
{
	const bool bWasAttacking = IsAttacking();

	EndAttackTrace();
	CurrentAttackType = EAesirAttackType::None;
	CurrentComboIndex = INDEX_NONE;
	CurrentAttackWindowIndex = INDEX_NONE;
	bAttackInputBuffered = false;
	bComboWindowOpen = false;

	if (bWasAttacking)
		OnAttackStateChanged.Broadcast(false);
}

void UAesirCombatComponent::CacheWeaponMesh()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	WeaponMesh = nullptr;

	TArray<UMeshComponent*> MeshComponents;
	OwnerActor->GetComponents<UMeshComponent>(MeshComponents);

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (IsValid(MeshComponent) &&
			MeshComponent->ComponentHasTag(WeaponComponentTag) &&
			MeshComponent->DoesSocketExist(TraceStartSocketName) &&
			MeshComponent->DoesSocketExist(TraceEndSocketName))
		{
			WeaponMesh = MeshComponent;
			UE_LOG(
				LogTemp,
				Log,
				TEXT("Weapon mesh cached: %s (trace sockets: %s, %s)"),
				*WeaponMesh->GetName(),
				*TraceStartSocketName.ToString(),
				*TraceEndSocketName.ToString());
			return;
		}
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("No weapon mesh found with tag %s and trace sockets %s/%s on %s"),
		*WeaponComponentTag.ToString(),
		*TraceStartSocketName.ToString(),
		*TraceEndSocketName.ToString(),
		*OwnerActor->GetName());
}

void UAesirCombatComponent::BeginAttackTrace()
{
	if (bAttackTraceActive || !IsValid(WeaponMesh))
	{
		return;
	}

	if (!WeaponMesh->DoesSocketExist(TraceStartSocketName) ||
		!WeaponMesh->DoesSocketExist(TraceEndSocketName))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Weapon trace sockets are missing."));
		return;
	}

	PreviousTraceStart =
		WeaponMesh->GetSocketLocation(TraceStartSocketName);

	PreviousTraceEnd =
		WeaponMesh->GetSocketLocation(TraceEndSocketName);

	bAttackTraceActive = true;
	SetComponentTickEnabled(true);

	// UE_LOG(LogTemp, Log, TEXT("Attack trace started."));
	HitActorsThisAttack.Reset();
}

void UAesirCombatComponent::EndAttackTrace()
{
	if (!bAttackTraceActive)
	{
		return;
	}

	bAttackTraceActive = false;
	// UE_LOG(LogTemp, Log, TEXT("Attack trace ended."));
	
	SetComponentTickEnabled(false);
	HitActorsThisAttack.Reset();
}
