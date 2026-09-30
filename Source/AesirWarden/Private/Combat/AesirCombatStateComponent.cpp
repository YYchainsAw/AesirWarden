// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/AesirCombatStateComponent.h"

UAesirCombatStateComponent::UAesirCombatStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UAesirCombatStateComponent::SetCombatState(EAesirCombatState NewState)
{
	if (NewState == CurrentState) return false;
	
	if (CurrentState == EAesirCombatState::Dead) return false;
	
	const EAesirCombatState PreviousState = CurrentState;
	CurrentState = NewState;
	
	OnCombatStateChanged.Broadcast(PreviousState, CurrentState);
	
	return true;
}

