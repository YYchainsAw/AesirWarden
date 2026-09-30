// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AesirCombatStateComponent.generated.h"

UENUM(BlueprintType)
enum class EAesirCombatState : uint8
{
	Idle,
	Combat,
	Attacking,
	Evading,
	HitReact,
	Stunned,
	Dead,
	AttackRecoiled
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnAesirCombatStateChanged,
	EAesirCombatState, PreviousState,
	EAesirCombatState, NewState);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AESIRWARDEN_API UAesirCombatStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAesirCombatStateComponent();

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Combat State")
	FOnAesirCombatStateChanged OnCombatStateChanged;
	
	UFUNCTION(BlueprintPure, Category = "Aesir|Combat State")
	EAesirCombatState GetCombatState() const
	{
		return CurrentState;
	}

	UFUNCTION(BlueprintPure, Category = "Aesir|Combat State")
	bool IsInCombatState(EAesirCombatState State) const
	{
		return CurrentState == State;
	}

	UFUNCTION(BlueprintCallable, Category = "Aesir|Combat State")
	bool SetCombatState(EAesirCombatState NewState);
private:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
	   Category = "Aesir|Combat State",
	   meta = (AllowPrivateAccess = "true"))
	EAesirCombatState CurrentState = EAesirCombatState::Idle;
};
