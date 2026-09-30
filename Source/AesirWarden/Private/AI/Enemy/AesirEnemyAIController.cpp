// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Enemy/AesirEnemyAIController.h"

#include "TimerManager.h"
#include "Characters/Enemy/AesirEnemyCharacter.h"
#include "Combat/AesirHealthComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

void AAesirEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	NextAttackTime = 0.0;
	bAttackFacingLocked = false;
	AttackFacingTarget.Reset();
	
	UE_LOG(LogTemp, Warning, TEXT("AI OnPossess: %s"),
		*GetNameSafe(InPawn));
	
	GetWorldTimerManager().SetTimer(
	ChaseUpdateTimerHandle,
	this,
	&AAesirEnemyAIController::UpdateChase,
	0.2f,
	true,
	0.1f);
}

void AAesirEnemyAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(ChaseUpdateTimerHandle);
	bAttackFacingLocked = false;
	AttackFacingTarget.Reset();
	ClearFocus(EAIFocusPriority::Gameplay);
	StopMovement();
	
	Super::OnUnPossess();
}

void AAesirEnemyAIController::UpdateChase()
{
	APawn* ControlledPawn = GetPawn();
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	AAesirEnemyCharacter* EnemyCharacter = Cast<AAesirEnemyCharacter>(ControlledPawn);
	
	if (!IsValid(PlayerPawn) || !IsValid(EnemyCharacter))
	{
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}
	
	const UAesirHealthComponent* PlayerHealth =
		PlayerPawn->FindComponentByClass<UAesirHealthComponent>();
	
	const bool bPlayerIsDead =
	IsValid(PlayerHealth) && PlayerHealth->IsDead();

	if (EnemyCharacter->IsDead() || bPlayerIsDead)
	{
		GetWorldTimerManager().ClearTimer(ChaseUpdateTimerHandle);
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}
	
	if (EnemyCharacter->IsStunned())
	{
		StopMovement();
		bAttackFacingLocked = false;
		AttackFacingTarget.Reset();
		ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}

	if (EnemyCharacter->IsDodging())
	{
		StopMovement();
		return;
	}

	if (EnemyCharacter->IsDefending())
	{
		StopMovement();
		return;
	}
	
	if (EnemyCharacter->IsAttacking())
	{
		StopMovement();

		if (!bAttackFacingLocked)
		{
			AActor* FacingTarget = AttackFacingTarget.IsValid()
				? AttackFacingTarget.Get()
				: PlayerPawn;
			SetFocus(FacingTarget, EAIFocusPriority::Gameplay);
		}

		return;
	}

	bAttackFacingLocked = false;
	AttackFacingTarget.Reset();
	SetFocus(PlayerPawn, EAIFocusPriority::Gameplay);

	const float DistanceSquared = FVector::DistSquared2D(
		ControlledPawn->GetActorLocation(),
        PlayerPawn->GetActorLocation());

	if (DistanceSquared <= FMath::Square(AttackRange))
	{
		StopMovement();

		if (UWorld* World = GetWorld())
		{
			const double CurrentTime = World->GetTimeSeconds();

			if (CurrentTime >= NextAttackTime &&
				EnemyCharacter->TryAttack())
			{
				NextAttackTime = CurrentTime + AttackCooldown;
			}
		}
		return;
	}
	
	if (GetMoveStatus() != EPathFollowingStatus::Moving)
	{
		const EPathFollowingRequestResult::Type MoveResult =
			MoveToActor(PlayerPawn, AttackRange, false);

		UE_LOG(LogTemp, Warning, TEXT("MoveToActor result: %d"),
			static_cast<int32>(MoveResult));
	}
}

void AAesirEnemyAIController::BeginAttackFacing(AActor* TargetActor)
{
	bAttackFacingLocked = false;
	AttackFacingTarget = TargetActor;

	if (IsValid(TargetActor))
	{
		SetFocus(TargetActor, EAIFocusPriority::Gameplay);
	}
}

void AAesirEnemyAIController::LockAttackFacing()
{
	bAttackFacingLocked = true;
	ClearFocus(EAIFocusPriority::Gameplay);
}

void AAesirEnemyAIController::EndAttackFacing()
{
	bAttackFacingLocked = false;
	AttackFacingTarget.Reset();
	ClearFocus(EAIFocusPriority::Gameplay);
}
