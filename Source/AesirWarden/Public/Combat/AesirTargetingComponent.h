// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AesirTargetingComponent.generated.h"

class AAesirEnemyCharacter;

UCLASS(ClassGroup=(Aesir), meta=(BlueprintSpawnableComponent))
class AESIRWARDEN_API UAesirTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAesirTargetingComponent();
	
	bool ToggleLockOn();

	UFUNCTION(BlueprintPure, Category = "Aesir|Targeting")
	bool IsLockedOn() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|Targeting")
	AActor* GetCurrentTarget() const;
	
	AAesirEnemyCharacter* FindBestTarget(
		float SearchRadius,
		float MaxViewAngleDegrees) const;

	void ClearTarget();

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Aesir|Targeting",
	meta = (ClampMin = "100.0"))
	float LockOnRadius = 1500.0f;
	
	UPROPERTY(EditDefaultsOnly, Category="Aesir|Targeting",
	meta=(ClampMin="0.0", ClampMax="1.0"))
	float ScreenCenterWeight = 0.85f;

	UPROPERTY(EditDefaultsOnly, Category="Aesir|Targeting",
		meta=(ClampMin="0.0", ClampMax="180.0"))
	float LockOnMaxViewAngle = 70.0f;

	UPROPERTY(Transient)
	TWeakObjectPtr<AAesirEnemyCharacter> CurrentTarget;
};
