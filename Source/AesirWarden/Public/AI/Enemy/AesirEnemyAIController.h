// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AesirEnemyAIController.generated.h"

/**
 * 
 */
UCLASS()
class AESIRWARDEN_API AAesirEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	void BeginAttackFacing(AActor* TargetActor);
	void LockAttackFacing();
	void EndAttackFacing();
	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	
	virtual void OnUnPossess() override;
	
private:
	void UpdateChase();
	
	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI")
	float AttackRange = 200.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI",
	meta = (ClampMin = "0.0"))
	float AttackCooldown = 2.0f;

	double NextAttackTime = 0.0;
	bool bAttackFacingLocked = false;
	TWeakObjectPtr<AActor> AttackFacingTarget;
	
	FTimerHandle ChaseUpdateTimerHandle;
};
