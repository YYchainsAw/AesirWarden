// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Player/AesirPlayerCharacter.h"

#include "Combat/AesirCombatComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "EnhancedInputComponent.h"
#include "TimerManager.h"
#include "GameplayEffect.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Characters/Enemy/AesirEnemyCharacter.h"
#include "Combat/AesirCombatStateComponent.h"
#include "Combat/AesirHealthComponent.h"
#include "Combat/AesirTargetingComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Abilities/AesirAbilitySystemComponent.h"
#include "Abilities/AesirAttributeSet.h"
#include "Abilities/AesirPlayerAttributeSet.h"
#include "Tags/AesirGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FName InvulnerabilityWindowName(
		TEXT("InvulnerabilityWindow"));
}

AAesirPlayerCharacter::AAesirPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	
	AbilitySystemComponent =
		CreateDefaultSubobject<UAesirAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	CoreAttributeSet = 
		CreateDefaultSubobject<UAesirAttributeSet>(TEXT("CoreAttributeSet"));

	PlayerAttributeSet = 
		CreateDefaultSubobject<UAesirPlayerAttributeSet>(TEXT("PlayerAttributeSet"));
	
	CombatComponent = 
		CreateDefaultSubobject<UAesirCombatComponent>(TEXT("CombatComponent"));
	
	HealthComponent =
		CreateDefaultSubobject<UAesirHealthComponent>(TEXT("HealthComponent"));

	TargetingComponent =
		CreateDefaultSubobject<UAesirTargetingComponent>(TEXT("TargetingComponent"));

	CombatStateComponent =
		CreateDefaultSubobject<UAesirCombatStateComponent>(TEXT("CombatStateComponent"));
}

UAbilitySystemComponent*
AAesirPlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAesirPlayerCharacter::InitializeDefaultAttributes()
{
	if (!AbilitySystemComponent || !DefaultAttributesEffect)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Player GAS attributes were not initialized: "
				 "DefaultAttributesEffect is missing."));
		return;
	}

	FGameplayEffectContextHandle EffectContext =
		AbilitySystemComponent->MakeEffectContext();

	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle EffectSpec =
		AbilitySystemComponent->MakeOutgoingSpec(
			DefaultAttributesEffect,
			1.0f,
			EffectContext);

	if (!EffectSpec.Data.IsValid())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Failed to create player initialization effect spec."));
		return;
	}

	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(
		*EffectSpec.Data.Get());
}

void AAesirPlayerCharacter::GrantStartupAbilities()
{
	if (!HasAuthority() || !AbilitySystemComponent)
		return;

	for (const TSubclassOf<UGameplayAbility>& AbilityClass :
		StartupAbilities)
	{
		if (!AbilityClass)
			continue;

		if (AbilitySystemComponent->
			FindAbilitySpecFromClass(AbilityClass))
			continue;

		AbilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(
				AbilityClass,
				1,
				INDEX_NONE,
				this));
	}
}

UAesirAbilitySystemComponent*
AAesirPlayerCharacter::GetAesirAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAesirPlayerCharacter::DoLook(float Yaw, float Pitch)
{
	const bool bLockedOn =
		TargetingComponent &&
		TargetingComponent->IsLockedOn();

	if (bLockedOn)
	{
		// 锁定时水平角由目标控制，只保留手动俯仰。
		Super::DoLook(0.0f, Pitch * LockOnPitchInputScale);
		return;
	}

	Super::DoLook(Yaw, Pitch);
}

void AAesirPlayerCharacter::DoMove(float Right, float Forward)
{
	DesiredMoveWorldDirection = FVector::ZeroVector;

	if (const AController* OwnerController = GetController())
	{
		const FRotator CameraYaw(
			0.0f,
			OwnerController->GetControlRotation().Yaw,
			0.0f);
		const FRotationMatrix CameraBasis(CameraYaw);
		DesiredMoveWorldDirection = (
			CameraBasis.GetUnitAxis(EAxis::X) * Forward +
			CameraBasis.GetUnitAxis(EAxis::Y) * Right).GetSafeNormal();
	}

	Super::DoMove(Right, Forward);
	UpdateFacingMode();
}

float AAesirPlayerCharacter::GetLocomotionDirectionAngle() const
{
	FVector MovementDirection = GetVelocity();
	MovementDirection.Z = 0.0f;

	if (!MovementDirection.Normalize())
		return 0.0f;
	
	FVector BasisForward = GetActorForwardVector();
	FVector BasisRight = GetActorRightVector();

	TryGetLockOnMovementBasis(
		BasisForward,
		BasisRight);
	
	const float ForwardAmount =
		FVector::DotProduct(MovementDirection, BasisForward);

	const float RightAmount =
		FVector::DotProduct(MovementDirection, BasisRight);

	return FMath::RadiansToDegrees(
		FMath::Atan2(RightAmount, ForwardAmount));
}

float AAesirPlayerCharacter::GetDesiredMoveDirectionAngle() const
{
	if (DesiredMoveWorldDirection.IsNearlyZero())
		return 0.0f;

	FVector BasisForward = GetActorForwardVector();
	FVector BasisRight = GetActorRightVector();
	TryGetLockOnMovementBasis(BasisForward, BasisRight);

	return FMath::RadiansToDegrees(FMath::Atan2(
		FVector::DotProduct(DesiredMoveWorldDirection, BasisRight),
		FVector::DotProduct(DesiredMoveWorldDirection, BasisForward)));
}

void AAesirPlayerCharacter::SetDesiredGait(EAesirPlayerGait NewGait)
{
	DesiredGait = NewGait;
	ApplyDesiredGait();
}

void AAesirPlayerCharacter::HandleToggleGaitInput()
{
	SetDesiredGait(DesiredGait == EAesirPlayerGait::Jog
		? EAesirPlayerGait::Walk
		: EAesirPlayerGait::Jog);
}

void AAesirPlayerCharacter::HandleMoveInputReleased()
{
	DesiredMoveWorldDirection = FVector::ZeroVector;
	UpdateFacingMode();
}

void AAesirPlayerCharacter::ApplyDesiredGait()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = DesiredGait == EAesirPlayerGait::Walk
			? WalkMaxSpeed
			: JogMaxSpeed;
	}
}

void AAesirPlayerCharacter::UpdateFacingMode()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
		return;

	const bool bLockedOn = TargetingComponent && TargetingComponent->IsLockedOn();
	const bool bAttacking = CombatComponent && CombatComponent->IsAttacking();
	const bool bCameraFacing = !DesiredMoveWorldDirection.IsNearlyZero() &&
		!bLockedOn && !bAttacking &&
		AttackFacingAssistTimeRemaining <= 0.0f &&
		EvadeState == EAesirEvadeState::None &&
		!bIsHitStunned && !IsGASDead();

	Movement->bOrientRotationToMovement = false;
	Movement->bUseControllerDesiredRotation = bCameraFacing;
}

float AAesirPlayerCharacter::GetGASHealth() const
{
	return CoreAttributeSet
		? CoreAttributeSet->GetHealth()
		: 0.0f;
}

float AAesirPlayerCharacter::GetGASMaxHealth() const
{
	return CoreAttributeSet
		? CoreAttributeSet->GetMaxHealth()
		: 0.0f;
}

float AAesirPlayerCharacter::GetGASGuardPressure() const
{
	return PlayerAttributeSet
		? PlayerAttributeSet->GetGuardPressure()
		: 0.0f;
}

float AAesirPlayerCharacter::GetGASMaxGuardPressure() const
{
	return PlayerAttributeSet
		? PlayerAttributeSet->GetMaxGuardPressure()
		: 0.0f;
}

bool AAesirPlayerCharacter::IsGASDead() const
{
	return (CoreAttributeSet && CoreAttributeSet->GetHealth() <= 0.0f) ||
		(HealthComponent && HealthComponent->IsDead());
}

bool AAesirPlayerCharacter::HasGuardCounterWindow() const
{
	return bEnableGuardCounter &&
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(
			AesirGameplayTags::State_GuardCounterWindow) &&
		GuardCounterTarget.IsValid();
}

bool AAesirPlayerCharacter::TryConsumeGuardCounter(
	AActor*& OutCounterTarget)
{
	OutCounterTarget = nullptr;

	if (!HasGuardCounterWindow())
	{
		EndGuardCounterWindow();
		return false;
	}

	OutCounterTarget = GuardCounterTarget.Get();
	EndGuardCounterWindow();

	UE_LOG(LogTemp, Display,
		TEXT("Guard counter window consumed: target=%s"),
		*GetNameSafe(OutCounterTarget));
	return true;
}

void AAesirPlayerCharacter::SetDamageInvulnerable(
	bool bInvulnerable)
{
	if (HealthComponent)
		HealthComponent->SetInvulnerable(bInvulnerable);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(
			AesirGameplayTags::State_Invulnerable,
			bInvulnerable ? 1 : 0);
	}
}

void AAesirPlayerCharacter::HandleBlockingTagChanged(
	FGameplayTag Tag,
	int32 NewCount)
{
	GetWorldTimerManager().ClearTimer(
		PerfectGuardWindowTimerHandle);

	if (!AbilitySystemComponent ||
		Tag != AesirGameplayTags::State_Blocking ||
		NewCount <= 0)
	{
		EndPerfectGuardWindow();
		return;
	}

	AbilitySystemComponent->SetLooseGameplayTagCount(
		AesirGameplayTags::State_PerfectGuard,
		1);

	GetWorldTimerManager().SetTimer(
		PerfectGuardWindowTimerHandle,
		this,
		&AAesirPlayerCharacter::EndPerfectGuardWindow,
		PerfectGuardWindowDuration,
		false);

	UE_LOG(LogTemp, Display,
		TEXT("Perfect guard window opened: %.2f seconds."),
		PerfectGuardWindowDuration);
}

void AAesirPlayerCharacter::EndPerfectGuardWindow()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(
			AesirGameplayTags::State_PerfectGuard,
			0);
	}
}

void AAesirPlayerCharacter::HandlePerfectGuardEvent(
	const FGameplayEventData* Payload)
{
	if (!bEnableGuardCounter)
	{
		EndGuardCounterWindow();
		return;
	}

	AActor* Attacker = Payload
		? const_cast<AActor*>(Payload->Instigator.Get())
		: nullptr;

	if (!AbilitySystemComponent || !IsValid(Attacker))
		return;

	GuardCounterTarget = Attacker;
	AbilitySystemComponent->SetLooseGameplayTagCount(
		AesirGameplayTags::State_GuardCounterWindow,
		1);

	GetWorldTimerManager().ClearTimer(
		GuardCounterWindowTimerHandle);
	GetWorldTimerManager().SetTimer(
		GuardCounterWindowTimerHandle,
		this,
		&AAesirPlayerCharacter::EndGuardCounterWindow,
		GuardCounterWindowDuration,
		false);

	UE_LOG(LogTemp, Display,
		TEXT("Guard counter window opened: target=%s duration=%.2f"),
		*GetNameSafe(Attacker),
		GuardCounterWindowDuration);
}

void AAesirPlayerCharacter::EndGuardCounterWindow()
{
	GetWorldTimerManager().ClearTimer(
		GuardCounterWindowTimerHandle);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(
			AesirGameplayTags::State_GuardCounterWindow,
			0);
	}

	GuardCounterTarget.Reset();
}

void AAesirPlayerCharacter::HandleGASHealthChanged(
	const FOnAttributeChangeData& Data)
{
	const bool bHealthDecreased = Data.NewValue < Data.OldValue;
	const bool bFatalChange = bHealthDecreased && Data.NewValue <= 0.0f;

	if (bFatalChange)
	{
		if (bGASDeathHandled)
			return;

		bGASDeathHandled = true;

		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->AddLooseGameplayTag(
				AesirGameplayTags::State_Dead);
			AbilitySystemComponent->CancelAllAbilities();
		}

	}

	if (HealthComponent && CoreAttributeSet)
	{
		HealthComponent->SynchronizeHealthFromGAS(
			Data.NewValue,
			CoreAttributeSet->GetMaxHealth());
	}

	if (!bHealthDecreased)
		return;

	if (bFatalChange)
	{
		if (!HealthComponent)
			HandlePlayerDeath(nullptr, nullptr);
		return;
	}

	if (!HealthComponent)
		HandleHitReceived(GetActorLocation(), nullptr, false);
}

void AAesirPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyDesiredGait();
	InitializeCameraOcclusion();
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		AbilitySystemComponent->RegisterGameplayTagEvent(
			AesirGameplayTags::State_Blocking,
			EGameplayTagEventType::NewOrRemoved).
			AddUObject(
				this,
				&AAesirPlayerCharacter::HandleBlockingTagChanged);

		if (bEnableGuardCounter)
		{
			AbilitySystemComponent->GenericGameplayEventCallbacks.
				FindOrAdd(
					AesirGameplayTags::Event_Player_Guard_Perfect).
				AddUObject(
					this,
					&AAesirPlayerCharacter::HandlePerfectGuardEvent);
		}
		else
		{
			EndGuardCounterWindow();
		}

		AbilitySystemComponent->
			GetGameplayAttributeValueChangeDelegate(
				UAesirAttributeSet::GetHealthAttribute()).
			AddUObject(
				this,
				&AAesirPlayerCharacter::HandleGASHealthChanged);

		InitializeDefaultAttributes();

		if (HealthComponent && CoreAttributeSet)
		{
			HealthComponent->SynchronizeHealthFromGAS(
				CoreAttributeSet->GetHealth(),
				CoreAttributeSet->GetMaxHealth());
		}

		GrantStartupAbilities();

		UE_LOG(
			LogTemp,
			Display,
			TEXT(
				"Player GAS initialized: Health=%.1f/%.1f "
				"GuardPressure=%.1f/%.1f"),
			CoreAttributeSet
				? CoreAttributeSet->GetHealth()
				: -1.0f,
			CoreAttributeSet
				? CoreAttributeSet->GetMaxHealth()
				: -1.0f,
			PlayerAttributeSet
				? PlayerAttributeSet->GetGuardPressure()
				: -1.0f,
			PlayerAttributeSet
				? PlayerAttributeSet->GetMaxGuardPressure()
				: -1.0f);
	}
	
	if (HealthComponent)
	{
		HealthComponent->OnHitReceived.AddUniqueDynamic(
			this,
			&AAesirPlayerCharacter::HandleHitReceived);
		
		HealthComponent->OnDeath.AddUniqueDynamic(
			this,
			&AAesirPlayerCharacter::HandlePlayerDeath);
	}
	
	if (CombatComponent)
	{
		CombatComponent->OnAttackStateChanged.AddUniqueDynamic(
			this,
			&AAesirPlayerCharacter::HandleAttackStateChanged);
	}
	
	if (UAnimInstance* AnimInstance =
		GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->OnPlayMontageNotifyBegin.AddUniqueDynamic(
			this,
			&AAesirPlayerCharacter::HandleEvadeNotifyBegin);

		AnimInstance->OnPlayMontageNotifyEnd.AddUniqueDynamic(
			this,
			&AAesirPlayerCharacter::HandleEvadeNotifyEnd);
		
		AnimInstance->OnMontageEnded.AddUniqueDynamic(
			this,
			&AAesirPlayerCharacter::HandleEvadeMontageEnded);
	}
	UpdateFacingMode();
}

void AAesirPlayerCharacter::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	for (const TPair<TWeakObjectPtr<AActor>, float>& Entry :
		 CameraFadeAmounts)
	{
		if (AActor* Actor = Entry.Key.Get())
			SetActorCameraFade(Actor, 1.0f);
	}

	CameraOccludingActors.Reset();
	CameraFadeAmounts.Reset();
	Super::EndPlay(EndPlayReason);
}

void AAesirPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateCameraFades(DeltaSeconds);
	UpdateFacingMode();
	UpdateAttackFacing(DeltaSeconds);

	if (CombatStateComponent &&
		(CombatStateComponent->IsInCombatState(EAesirCombatState::Idle) ||
		 CombatStateComponent->IsInCombatState(EAesirCombatState::Combat)))
	{
		RestoreDefaultCombatState();
	}
	
	const bool bShouldAssistCamera =
		TargetingComponent &&
		TargetingComponent->IsLockedOn() &&
		!IsGASDead();

	if (!bShouldAssistCamera)
		return;
	
	AActor* Target = TargetingComponent->GetCurrentTarget();

	if (!IsValid(Target))
		return;
	
	FVector DirectionToTarget =
		Target->GetActorLocation() - GetActorLocation();

	DirectionToTarget.Z = 0.0f;

	if (!DirectionToTarget.IsNearlyZero())
	{
		const FRotator DesiredFacing =
			DirectionToTarget.Rotation();

		const FRotator NewFacing = FMath::RInterpTo(
			GetActorRotation(),
			DesiredFacing,
			DeltaSeconds,
			LockOnFacingRotationSpeed);

		SetActorRotation(
			FRotator(0.0f, NewFacing.Yaw, 0.0f));
	}
	
	AController* OwnerController = GetController();

	if (!IsValid(OwnerController) || !GetFollowCamera())
		return;

	FVector TargetCenter;
	FVector TargetExtent;
	Target->GetActorBounds(
		true,
		TargetCenter,
		TargetExtent);

	const FVector TargetPoint = TargetCenter;

	const FVector CameraLocation =
		GetFollowCamera()->GetComponentLocation();

	FRotator DesiredCameraRotation =
		(TargetPoint - CameraLocation).Rotation();

	DesiredCameraRotation.Pitch = FMath::Clamp(
		OwnerController->GetControlRotation().Pitch,
		LockOnMinPitch,
		LockOnMaxPitch);

	DesiredCameraRotation.Roll = 0.0f;

	const FRotator NewCameraRotation = FMath::RInterpTo(
		OwnerController->GetControlRotation(),
		DesiredCameraRotation,
		DeltaSeconds,
		LockOnCameraRotationSpeed);

	OwnerController->SetControlRotation(NewCameraRotation);
}

void AAesirPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(
				MoveAction,
				ETriggerEvent::Completed,
				this,
				&AAesirPlayerCharacter::HandleMoveInputReleased);
			EnhancedInputComponent->BindAction(
				MoveAction,
				ETriggerEvent::Canceled,
				this,
				&AAesirPlayerCharacter::HandleMoveInputReleased);
		}

		if (ToggleGaitAction)
		{
			EnhancedInputComponent->BindAction(
				ToggleGaitAction,
				ETriggerEvent::Started,
				this,
				&AAesirPlayerCharacter::HandleToggleGaitInput);
		}

		if (LightAttackAction)
		{
			EnhancedInputComponent->BindAction(
				LightAttackAction,
				ETriggerEvent::Started,
				this,
				&AAesirPlayerCharacter::HandleLightAttackInput);
		}
		
		if (HeavyAttackAction)
		{
			EnhancedInputComponent->BindAction(
				HeavyAttackAction,
				ETriggerEvent::Started,
				this,
				&AAesirPlayerCharacter::HandleHeavyAttackInput);
		}
		
		if (LockOnAction)
		{
			EnhancedInputComponent->BindAction(
				LockOnAction,
				ETriggerEvent::Started,
				this,
				&AAesirPlayerCharacter::HandleLockOnInput);
		}
		
		if (EvadeAction)
		{
			EnhancedInputComponent->BindAction(
				EvadeAction,
				ETriggerEvent::Started,
				this,
				&AAesirPlayerCharacter::HandleEvadeInput);
		}
	}
}

void AAesirPlayerCharacter::InitializeCameraOcclusion()
{
	if (!bEnableCameraOcclusionFade ||
		CameraFadeableActorTag.IsNone())
	{
		return;
	}

	TArray<AActor*> FadeableActors;
	UGameplayStatics::GetAllActorsWithTag(
		this,
		CameraFadeableActorTag,
		FadeableActors);

	for (AActor* Actor : FadeableActors)
		SetActorCameraCollisionIgnored(Actor);
}

void AAesirPlayerCharacter::RefreshCameraOccluders()
{
	CameraOccludingActors.Reset();

	const APlayerController* PlayerController =
		Cast<APlayerController>(GetController());
	const APlayerCameraManager* CameraManager =
		PlayerController
			? PlayerController->PlayerCameraManager
			: nullptr;

	if (!CameraManager || !GetWorld())
		return;

	const FVector TraceStart = CameraManager->GetCameraLocation();
	const FVector TraceEnd =
		GetActorLocation() + CameraOcclusionTargetOffset;

	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(AesirCameraOcclusion),
		false,
		this);

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(
		Hits,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ObjectQuery,
		FCollisionShape::MakeSphere(CameraOcclusionTraceRadius),
		QueryParams);

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!IsCameraFadeableActor(HitActor))
			continue;

		CameraOccludingActors.Add(HitActor);

		if (HitActor->ActorHasTag(CameraFadeableActorTag))
			SetActorCameraCollisionIgnored(HitActor);
	}
}

void AAesirPlayerCharacter::UpdateCameraFades(float DeltaSeconds)
{
	if (!bEnableCameraOcclusionFade)
		return;

	CameraOcclusionTraceTimeRemaining -= DeltaSeconds;
	if (CameraOcclusionTraceTimeRemaining <= 0.0f)
	{
		RefreshCameraOccluders();
		CameraOcclusionTraceTimeRemaining =
			CameraOcclusionTraceInterval;
	}

	for (const TWeakObjectPtr<AActor>& Actor :
		 CameraOccludingActors)
	{
		if (Actor.IsValid() && !CameraFadeAmounts.Contains(Actor))
			CameraFadeAmounts.Add(Actor, 1.0f);
	}

	for (auto It = CameraFadeAmounts.CreateIterator(); It; ++It)
	{
		AActor* Actor = It.Key().Get();
		if (!IsValid(Actor))
		{
			It.RemoveCurrent();
			continue;
		}

		const bool bIsOccluding =
			CameraOccludingActors.Contains(It.Key());
		const float TargetFade = bIsOccluding
			? CameraOccludedFadeAmount
			: 1.0f;

		It.Value() = FMath::FInterpTo(
			It.Value(),
			TargetFade,
			DeltaSeconds,
			CameraFadeInterpSpeed);

		SetActorCameraFade(Actor, It.Value());

		if (!bIsOccluding && FMath::IsNearlyEqual(It.Value(), 1.0f, 0.01f))
		{
			SetActorCameraFade(Actor, 1.0f);
			It.RemoveCurrent();
		}
	}
}

void AAesirPlayerCharacter::SetActorCameraFade(
	AActor* Actor,
	float FadeAmount) const
{
	if (!IsValid(Actor) || CameraFadeMaterialParameter.IsNone())
		return;

	TInlineComponentArray<UMeshComponent*> MeshComponents;
	Actor->GetComponents(MeshComponents);

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (IsValid(MeshComponent))
		{
			MeshComponent->SetScalarParameterValueOnMaterials(
				CameraFadeMaterialParameter,
				FadeAmount);
		}
	}
}

void AAesirPlayerCharacter::SetActorCameraCollisionIgnored(
	AActor* Actor) const
{
	if (!IsValid(Actor))
		return;

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
	Actor->GetComponents(PrimitiveComponents);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (IsValid(PrimitiveComponent))
		{
			PrimitiveComponent->SetCollisionResponseToChannel(
				ECC_Camera,
				ECR_Ignore);
		}
	}
}

bool AAesirPlayerCharacter::IsCameraFadeableActor(
	const AActor* Actor) const
{
	if (!IsValid(Actor) || Actor == this)
		return false;

	return (bFadeOtherPawns && Actor->IsA<APawn>()) ||
		Actor->ActorHasTag(CameraFadeableActorTag);
}

void AAesirPlayerCharacter::HandleLightAttackInput()
{
	if (bIsHitStunned ||
		EvadeState != EAesirEvadeState::None ||
		IsGASDead())
		return;
	
	if (CombatComponent)
	{
		AActor* AttackTarget = ResolveAttackAssistTarget();

		AttackAssistTarget = AttackTarget;
		AttackFacingAssistTimeRemaining =
			AttackFacingAssistDuration;
		
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[STT] Attack target: %s"),
			*GetNameSafe(AttackTarget));

		CombatComponent->TryLightAttack();
	}
}

void AAesirPlayerCharacter::HandleHeavyAttackInput()
{
	if (bIsHitStunned ||
		EvadeState != EAesirEvadeState::None ||
		IsGASDead())
		return;
	
	if (CombatComponent)
	{
		AActor* AttackTarget = ResolveAttackAssistTarget();

		AttackAssistTarget = AttackTarget;
		AttackFacingAssistTimeRemaining =
			AttackFacingAssistDuration;
		
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[STT] Attack target: %s"),
			*GetNameSafe(AttackTarget));

		CombatComponent->TryHeavyAttack();
	}
}

AActor* AAesirPlayerCharacter::ResolveAttackAssistTarget() const
{
	if (!TargetingComponent)
		return nullptr;

	AActor* LockedTarget =
		TargetingComponent->GetCurrentTarget();

	if (IsValid(LockedTarget))
		return LockedTarget;

	AAesirEnemyCharacter* AssistTarget =
		TargetingComponent->FindBestTarget(
			AttackAssistRadius,
			AttackAssistMaxViewAngle);

	return AssistTarget;
}

void AAesirPlayerCharacter::UpdateAttackFacing(float DeltaSeconds)
{
	if (AttackFacingAssistTimeRemaining <= 0.0f)
	{
		AttackAssistTarget.Reset();
		return;
	}

	const bool bCannotAssistFacing =
		IsGASDead() ||
		bIsHitStunned ||
		EvadeState != EAesirEvadeState::None ||
		(TargetingComponent &&
			TargetingComponent->IsLockedOn());

	if (bCannotAssistFacing)
	{
		AttackFacingAssistTimeRemaining = 0.0f;
		AttackAssistTarget.Reset();
		return;
	}

	FVector FacingDirection = FVector::ZeroVector;

	if (AttackAssistTarget.IsValid())
	{
		FacingDirection =
			AttackAssistTarget->GetActorLocation() -
			GetActorLocation();
	}
	else if (const AController* OwnerController = GetController())
		FacingDirection =
			OwnerController->GetControlRotation().Vector();

	FacingDirection.Z = 0.0f;

	if (!FacingDirection.Normalize())
	{
		AttackFacingAssistTimeRemaining = 0.0f;
		AttackAssistTarget.Reset();
		return;
	}
	
	const FRotator DesiredFacing =
		FacingDirection.Rotation();

	const FRotator DesiredActorRotation(
		0.0f,
		DesiredFacing.Yaw,
		0.0f);

	const FRotator NewFacing =
		FMath::RInterpConstantTo(
			GetActorRotation(),
			DesiredActorRotation,
			DeltaSeconds,
			AttackFacingRotationSpeed);

	SetActorRotation(NewFacing);

	AttackFacingAssistTimeRemaining =
		FMath::Max(
			0.0f,
			AttackFacingAssistTimeRemaining - DeltaSeconds);

	if (AttackFacingAssistTimeRemaining <= 0.0f)
		AttackAssistTarget.Reset();
}

void AAesirPlayerCharacter::HandleLockOnInput()
{
	if (bIsHitStunned ||
		IsGASDead())
		return;
	
	if (TargetingComponent)
	{
		TargetingComponent->ToggleLockOn();
		UpdateFacingMode();

		if (CombatStateComponent &&
			(CombatStateComponent->IsInCombatState(EAesirCombatState::Idle) ||
			 CombatStateComponent->IsInCombatState(EAesirCombatState::Combat)))
		{
			RestoreDefaultCombatState();
		}
	}
}

void AAesirPlayerCharacter::HandleEvadeInput()
{
	if (bIsHitStunned ||
		(CombatComponent &&
			CombatComponent->IsDamageWindowActive()) ||
		IsGASDead() ||
		GetCharacterMovement()->IsFalling())
		return;

	UWorld* World = GetWorld();

	if (!World || EvadeState == EAesirEvadeState::Rolling)
		return;

	const double CurrentTime = World->GetTimeSeconds();

	const bool bWithinRollWindow =
		LastEvadeInputTime >= 0.0 &&
		CurrentTime - LastEvadeInputTime <= RollDoubleTapWindow;

	if (bWithinRollWindow)
	{
		if (TryPlayEvadeMontage(
			RollMontages,
			ActiveEvadeDirection,
			EAesirEvadeState::Rolling))
			LastEvadeInputTime = -1.0;

		return;
	}

	if (EvadeState != EAesirEvadeState::None)
		return;

	const EAesirEvadeDirection Direction =
		DetermineEvadeDirection();

	if (TryPlayEvadeMontage(
		DashMontages,
		Direction,
		EAesirEvadeState::Dashing))
	{
		ActiveEvadeDirection = Direction;
		LastEvadeInputTime = CurrentTime;
	}
}

EAesirEvadeDirection AAesirPlayerCharacter::DetermineEvadeDirection() const
{
	FVector InputDirection = GetPendingMovementInputVector();

	if (InputDirection.IsNearlyZero())
	{
		InputDirection = GetLastMovementInputVector();
	}

	InputDirection.Z = 0.0f;

	if (InputDirection.IsNearlyZero())
		return EAesirEvadeDirection::Backward;

	FVector BasisForward = GetActorForwardVector();
	FVector BasisRight = GetActorRightVector();

	TryGetLockOnMovementBasis(
		BasisForward,
		BasisRight);

	const FVector NormalizedInput =
		InputDirection.GetSafeNormal();

	const float ForwardAmount =
		FVector::DotProduct(NormalizedInput, BasisForward);

	const float RightAmount =
		FVector::DotProduct(NormalizedInput, BasisRight);

	const float AngleDegrees = FMath::RadiansToDegrees(
		FMath::Atan2(RightAmount, ForwardAmount));

	const int32 DirectionIndex =
		(FMath::RoundToInt(AngleDegrees / 45.0f) + 8) % 8;

	return static_cast<EAesirEvadeDirection>(DirectionIndex);
}

bool AAesirPlayerCharacter::TryPlayEvadeMontage(const TMap<EAesirEvadeDirection, TObjectPtr<UAnimMontage>>& MontageMap,
	EAesirEvadeDirection Direction, EAesirEvadeState NewState)
{
	UAnimInstance* AnimInstance =
		GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;

	const TObjectPtr<UAnimMontage>* FoundMontage =
		MontageMap.Find(Direction);

	if (!AnimInstance ||
		!FoundMontage ||
		!IsValid(FoundMontage->Get()))
		return false;

	UAnimMontage* Montage = FoundMontage->Get();

	ActiveEvadeMontage = Montage;
	EvadeState = NewState;
	
	float RootMotionScale = 1.0f;
	float PlayRate = 1.0f;

	if (NewState == EAesirEvadeState::Dashing)
	{
		RootMotionScale = DashRootMotionTranslationScale;
		PlayRate = DashPlayRate;
	}
	else if (NewState == EAesirEvadeState::Rolling)
	{
		RootMotionScale = RollRootMotionTranslationScale;
		PlayRate = RollPlayRate;
	}

	SetAnimRootMotionTranslationScale(RootMotionScale);

	const float MontageDuration =
		AnimInstance->Montage_Play(Montage, PlayRate);

	if (MontageDuration <= 0.0f)
	{
		SetAnimRootMotionTranslationScale(1.0f);
		ActiveEvadeMontage = nullptr;
		EvadeState = EAesirEvadeState::None;
		return false;
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(
			AesirGameplayTags::State_Dodging,
			1);
	}

	GetCharacterMovement()->StopMovementImmediately();

	if (CombatStateComponent)
	{
		CombatStateComponent->SetCombatState(
			EAesirCombatState::Evading);
	}

	return true;
}

bool AAesirPlayerCharacter::TryGetLockOnMovementBasis(FVector& OutForwardDirection, FVector& OutRightDirection) const
{
	if (!TargetingComponent ||
		!TargetingComponent->IsLockedOn())
		return false;

	const AActor* Target =
		TargetingComponent->GetCurrentTarget();

	if (!IsValid(Target))
		return false;

	FVector ForwardDirection =
		Target->GetActorLocation() - GetActorLocation();

	ForwardDirection.Z = 0.0f;

	if (!ForwardDirection.Normalize())
		return false;

	OutForwardDirection = ForwardDirection;
	OutRightDirection = FVector::CrossProduct(
		FVector::UpVector,
		ForwardDirection).GetSafeNormal();

	return true;
}

void AAesirPlayerCharacter::HandleHitReceived(FVector HitLocation, AActor* DamageCauser, bool bFatalHit)
{
	if (bFatalHit)
		return;
	
	if (CombatComponent)
		CombatComponent->CancelAttack();

	if (ActiveEvadeMontage)
	{
		if (UAnimInstance* AnimInstance =
			GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->Montage_Stop(0.1f, ActiveEvadeMontage);
		}
	}

	if (bFatalHit)
	{
		GetWorldTimerManager().ClearTimer(HitStunTimerHandle);
		bIsHitStunned = false;

		SetDamageInvulnerable(false);

		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
		StopJumping();

		if (CombatStateComponent)
		{
			CombatStateComponent->SetCombatState(
				EAesirCombatState::Dead);
		}

		return;
	}
	
	bIsHitStunned = true;

	if (CombatStateComponent)
	{
		CombatStateComponent->SetCombatState(
			EAesirCombatState::HitReact);
	}
	
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	StopJumping();
	
	GetWorldTimerManager().SetTimer(
		HitStunTimerHandle,	
		this, 
		&AAesirPlayerCharacter::EndHitStun, 
		HitStunDuration, 
		false);
}

void AAesirPlayerCharacter::HandleEvadeNotifyBegin(FName NotifyName,
	const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (NotifyName != InvulnerabilityWindowName ||
	   Cast<UAnimMontage>(BranchingPointPayload.SequenceAsset) !=
		   ActiveEvadeMontage)
		return;

	SetDamageInvulnerable(true);
}

void AAesirPlayerCharacter::HandleEvadeNotifyEnd(FName NotifyName,
	const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (NotifyName != InvulnerabilityWindowName ||
	   Cast<UAnimMontage>(BranchingPointPayload.SequenceAsset) !=
		   ActiveEvadeMontage)
		return;

	SetDamageInvulnerable(false);
}

void AAesirPlayerCharacter::HandleEvadeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActiveEvadeMontage)
		return;
	
	SetDamageInvulnerable(false);
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(
			AesirGameplayTags::State_Dodging,
			0);
	}
	
	SetAnimRootMotionTranslationScale(1.0f);
	ActiveEvadeMontage = nullptr;
	EvadeState = EAesirEvadeState::None;

	if (CombatStateComponent &&
		CombatStateComponent->IsInCombatState(EAesirCombatState::Evading))
	{
		RestoreDefaultCombatState();
	}

	if (bInterrupted)
		LastEvadeInputTime = -1.0;
}

void AAesirPlayerCharacter::HandleAttackStateChanged(bool bIsAttacking)
{
	if (!CombatStateComponent)
		return;

	if (bIsAttacking)
	{
		CombatStateComponent->SetCombatState(
			EAesirCombatState::Attacking);
		return;
	}

	if (CombatStateComponent->IsInCombatState(
		EAesirCombatState::Attacking))
	{
		RestoreDefaultCombatState();
	}
}

void AAesirPlayerCharacter::HandlePlayerDeath(AController* InstigatedBy, AActor* DamageCauser)
{
	GetWorldTimerManager().ClearTimer(HitStunTimerHandle);
	GetWorldTimerManager().ClearTimer(
		PerfectGuardWindowTimerHandle);
	GetWorldTimerManager().ClearTimer(
		GuardCounterWindowTimerHandle);
	bIsHitStunned = false;

	if (CombatComponent)
		CombatComponent->CancelAttack();

	SetDamageInvulnerable(false);
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(
			AesirGameplayTags::State_Dodging,
			0);
	}
	EndPerfectGuardWindow();
	EndGuardCounterWindow();

	if (TargetingComponent)
		TargetingComponent->ClearTarget();

	AttackAssistTarget.Reset();
	AttackFacingAssistTimeRemaining = 0.0f;
	LastEvadeInputTime = -1.0;

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	StopJumping();

	if (CombatStateComponent)
		CombatStateComponent->SetCombatState(
			EAesirCombatState::Dead);
	
	SetAnimRootMotionTranslationScale(1.0f);
	ActiveEvadeMontage = nullptr;
	EvadeState = EAesirEvadeState::None;

	UGameplayStatics::SetGlobalTimeDilation(
		this,
		DeathWorldTimeDilation);
		
	if (DeathMontage)
	{
		if (UAnimInstance* AnimInstance =
			GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->Montage_Play(
				DeathMontage,
				DeathMontagePlayRate);
		}
	}
}

void AAesirPlayerCharacter::RestoreDefaultCombatState()
{
	if (!CombatStateComponent ||
		IsGASDead())
	{
		return;
	}

	const bool bLockedOn =
		TargetingComponent && TargetingComponent->IsLockedOn();

	CombatStateComponent->SetCombatState(
		bLockedOn
			? EAesirCombatState::Combat
			: EAesirCombatState::Idle);
}

void AAesirPlayerCharacter::EndHitStun()
{
	bIsHitStunned = false;

	if (!IsGASDead())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);

		if (CombatStateComponent &&
			CombatStateComponent->IsInCombatState(EAesirCombatState::HitReact))
		{
			RestoreDefaultCombatState();
		}
	}
}
