// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AesirPoiseComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnAesirPoiseChanged,
	float, CurrentPoise,
	float, MaxPoise,
	float, PoiseDelta);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(
	FOnAesirPoiseBroken);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AESIRWARDEN_API UAesirPoiseComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAesirPoiseComponent();
	
protected:
	virtual void BeginPlay() override;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Aesir|Poise")
	bool ApplyPoiseDamage(float PoiseDamage);

	UFUNCTION(BlueprintCallable, Category = "Aesir|Poise")
	void ResetPoise();

	UFUNCTION(BlueprintPure, Category = "Aesir|Poise")
	bool IsBroken() const
	{
		return bIsBroken;
	}

	UFUNCTION(BlueprintPure, Category = "Aesir|Poise")
	float GetCurrentPoise() const { return CurrentPoise; }

	UFUNCTION(BlueprintPure, Category = "Aesir|Poise")
	float GetMaxPoise() const { return MaxPoise; }

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Poise")
	FOnAesirPoiseChanged OnPoiseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Poise")
	FOnAesirPoiseBroken OnPoiseBroken;
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly,
	Category = "Aesir|Poise",
	meta = (AllowPrivateAccess = "true"))
	float MaxPoise = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
		Category = "Aesir|Poise",
		meta = (AllowPrivateAccess = "true"))
	float CurrentPoise = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
		Category = "Aesir|Poise",
		meta = (AllowPrivateAccess = "true"))
	bool bIsBroken = false;
		
};
