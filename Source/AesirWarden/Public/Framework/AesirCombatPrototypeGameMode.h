// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AesirCombatPrototypeGameMode.generated.h"

UENUM(BlueprintType)
enum class EAesirMatchResult : uint8
{
	None,
	Victory,
	Defeat
};

UCLASS(abstract)
class AAesirCombatPrototypeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AAesirCombatPrototypeGameMode();
	
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandlePlayerDeath(
		AController* InstigatedBy,
		AActor* DamageCauser);

	UFUNCTION()
	void HandleEnemyDeath(
		AController* InstigatedBy,
		AActor* DamageCauser);

	void FinishCombat(EAesirMatchResult NewResult);

	UFUNCTION(BlueprintImplementableEvent, Category="Aesir|Match")
	void OnCombatEnded(EAesirMatchResult Result);
	
private:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
	Category="Aesir|Match",
	meta=(AllowPrivateAccess="true"))
	int32 RemainingEnemies = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
		Category="Aesir|Match",
		meta=(AllowPrivateAccess="true"))
	EAesirMatchResult MatchResult = EAesirMatchResult::None;
};



