// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/AesirTargetingComponent.h"

#include "EngineUtils.h"
#include "Characters/Enemy/AesirEnemyCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"

UAesirTargetingComponent::UAesirTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UAesirTargetingComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const AActor* OwnerActor = GetOwner();

	if (!IsValid(OwnerActor) ||
		!CurrentTarget.IsValid() ||
		CurrentTarget->IsDead())
	{
		ClearTarget();
		return;
	}

	const float DistanceSquared = FVector::DistSquared(
		OwnerActor->GetActorLocation(),
		CurrentTarget->GetActorLocation());

	if (DistanceSquared > FMath::Square(LockOnRadius))
		ClearTarget();
}

AAesirEnemyCharacter* UAesirTargetingComponent::FindBestTarget(
	float SearchRadius, float MaxViewAngleDegrees) const
{
	const AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();

	if (!IsValid(OwnerActor) ||
		!IsValid(World) ||
		SearchRadius <= 0.0f)
		return nullptr;
	
	const APawn* OwnerPawn = Cast<APawn>(OwnerActor);

	const APlayerController* PlayerController =
		OwnerPawn
			? Cast<APlayerController>(OwnerPawn->GetController())
			: nullptr;

	if (!IsValid(PlayerController))
		return nullptr;

	FVector ViewLocation;
	FRotator ViewRotation;

	PlayerController->GetPlayerViewPoint(
		ViewLocation,
		ViewRotation);

	const FVector ViewForward =
		ViewRotation.Vector().GetSafeNormal();

	const float MinimumViewDot =
		FMath::Cos(
			FMath::DegreesToRadians(
				FMath::Clamp(
					MaxViewAngleDegrees,
					0.0f,
					180.0f)));

	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;

	PlayerController->GetViewportSize(
		ViewportWidth,
		ViewportHeight);

	if (ViewportWidth <= 0 || ViewportHeight <= 0)
	{
		return nullptr;
	}

	const FVector2D ScreenCenter(
		ViewportWidth * 0.5f,
		ViewportHeight * 0.5f);

	const float MaxScreenDistance = ScreenCenter.Size();
	
	AAesirEnemyCharacter* BestTarget = nullptr;
	float BestScore = MAX_flt;
	
	for (TActorIterator<AAesirEnemyCharacter> It(World); It; ++It)
	{
		AAesirEnemyCharacter* Candidate = *It;

		if (!IsValid(Candidate) || Candidate->IsDead())
			continue;

		const float DistanceSquared = FVector::DistSquared(
			OwnerActor->GetActorLocation(),
			Candidate->GetActorLocation());

		if (DistanceSquared > FMath::Square(SearchRadius))
			continue;

		const FVector DirectionToCandidate =
			(Candidate->GetActorLocation() - ViewLocation)
			.GetSafeNormal();

		if (DirectionToCandidate.IsNearlyZero())
			continue;

		const float ViewDot = FVector::DotProduct(
			ViewForward,
			DirectionToCandidate);

		if (ViewDot < MinimumViewDot)
			continue;

		FVector2D ScreenLocation;

		if (!PlayerController->ProjectWorldLocationToScreen(
			Candidate->GetActorLocation(),
			ScreenLocation,
			true))
			continue;

		const bool bIsOnScreen =
			ScreenLocation.X >= 0.0f &&
			ScreenLocation.X <= ViewportWidth &&
			ScreenLocation.Y >= 0.0f &&
			ScreenLocation.Y <= ViewportHeight;

		if (!bIsOnScreen)
			continue;

		const float NormalizedScreenDistance =
			FVector2D::Distance(ScreenLocation, ScreenCenter) /
			MaxScreenDistance;

		const float NormalizedWorldDistance =
			FMath::Sqrt(DistanceSquared) / SearchRadius;
		
		const float CandidateScore =
			NormalizedScreenDistance * ScreenCenterWeight +
			NormalizedWorldDistance * (1.0f - ScreenCenterWeight);

		if (CandidateScore < BestScore)
		{
			BestScore = CandidateScore;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

bool UAesirTargetingComponent::ToggleLockOn()
{
	if (CurrentTarget.IsValid())
	{
		ClearTarget();
		return false;
	}

	AAesirEnemyCharacter* NewTarget = FindBestTarget(
		LockOnRadius,
		LockOnMaxViewAngle);
	
	if (!IsValid(NewTarget))
		return false;
	
	CurrentTarget = NewTarget;
	NewTarget->SetLockOnIndicatorVisible(true);
	SetComponentTickEnabled(true);

	UE_LOG(LogTemp, Log, TEXT("Locked on to: %s"),
		*GetNameSafe(NewTarget));

	return true;
}

void UAesirTargetingComponent::ClearTarget()
{
	if (AAesirEnemyCharacter* PreviousTarget = CurrentTarget.Get())
		PreviousTarget->SetLockOnIndicatorVisible(false);
	
	CurrentTarget.Reset();
	SetComponentTickEnabled(false);

	UE_LOG(LogTemp, Log, TEXT("Lock-on cleared."));
}

bool UAesirTargetingComponent::IsLockedOn() const
{
	return CurrentTarget.IsValid();
}

AActor* UAesirTargetingComponent::GetCurrentTarget() const
{
	return CurrentTarget.Get();
}
