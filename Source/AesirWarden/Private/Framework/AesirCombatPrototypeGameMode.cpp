// Copyright Epic Games, Inc. All Rights Reserved.

#include "Framework/AesirCombatPrototypeGameMode.h"
#include "Characters/Enemy/AesirEnemyCharacter.h"
#include "Combat/AesirHealthComponent.h"
#include "Kismet/GameplayStatics.h"

AAesirCombatPrototypeGameMode::AAesirCombatPrototypeGameMode()
{
}

void AAesirCombatPrototypeGameMode::BeginPlay()
{
	Super::BeginPlay(); //NOTE: 不同 Actor 的 BeginPlay 顺序不应被依赖
	
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (UAesirHealthComponent* PlayerHealth =
			PlayerPawn->FindComponentByClass<UAesirHealthComponent>())
		{
			PlayerHealth->OnDeath.AddUniqueDynamic(
				this,
				&AAesirCombatPrototypeGameMode::HandlePlayerDeath);
		}
	}
	
	TArray<AActor*> FoundEnemies;

	UGameplayStatics::GetAllActorsOfClass(
		this,
		AAesirEnemyCharacter::StaticClass(),
		FoundEnemies);

	RemainingEnemies = 0;

	for (AActor* EnemyActor : FoundEnemies)
	{
		if (!IsValid(EnemyActor))
			continue;

		UAesirHealthComponent* EnemyHealth =
			EnemyActor->FindComponentByClass<UAesirHealthComponent>();

		if (!IsValid(EnemyHealth))
			continue;

		EnemyHealth->OnDeath.AddUniqueDynamic(
			this,
			&AAesirCombatPrototypeGameMode::HandleEnemyDeath);

		++RemainingEnemies;
	}
}

void AAesirCombatPrototypeGameMode::HandlePlayerDeath(AController* InstigatedBy, AActor* DamageCauser)
{
	FinishCombat(EAesirMatchResult::Defeat);
}

void AAesirCombatPrototypeGameMode::HandleEnemyDeath(AController* InstigatedBy, AActor* DamageCauser)
{
	if (MatchResult != EAesirMatchResult::None)
		return;

	RemainingEnemies = FMath::Max(RemainingEnemies - 1, 0);

	if (RemainingEnemies == 0)
		FinishCombat(EAesirMatchResult::Victory);
}

void AAesirCombatPrototypeGameMode::FinishCombat(EAesirMatchResult NewResult)
{
	if (MatchResult != EAesirMatchResult::None ||
		NewResult == EAesirMatchResult::None)
		return;

	MatchResult = NewResult;
	OnCombatEnded(MatchResult);
}
