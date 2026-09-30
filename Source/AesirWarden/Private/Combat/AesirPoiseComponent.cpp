// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/AesirPoiseComponent.h"

// Sets default values for this component's properties
UAesirPoiseComponent::UAesirPoiseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAesirPoiseComponent::BeginPlay()
{
	Super::BeginPlay();
	
	MaxPoise = FMath::Max(MaxPoise,0.0f);
	CurrentPoise = MaxPoise;
	bIsBroken = false;
}

bool UAesirPoiseComponent::ApplyPoiseDamage(float PoiseDamage)
{
	if (PoiseDamage <= 0.0f || bIsBroken)
		return false;

	const float PreviousPoise = CurrentPoise;

	CurrentPoise = FMath::Clamp(
		CurrentPoise - PoiseDamage,
		0.0f,
		MaxPoise);

	OnPoiseChanged.Broadcast(
		CurrentPoise,
		MaxPoise,
		CurrentPoise - PreviousPoise);

	if (CurrentPoise > 0.0f)
		return false;

	bIsBroken = true;
	OnPoiseBroken.Broadcast();

	return true;
}

void UAesirPoiseComponent::ResetPoise()
{
	const float PreviousPoise = CurrentPoise;

	CurrentPoise = MaxPoise;
	bIsBroken = false;

	OnPoiseChanged.Broadcast(
		CurrentPoise,
		MaxPoise,
		CurrentPoise - PreviousPoise);
}



