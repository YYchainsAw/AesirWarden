// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Services/Companion/TacticalOrderTypes.h"
#include "TacticalOrderComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AESIRWARDEN_API UTacticalOrderComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTacticalOrderComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Aesir|Tactical Order")
	bool ValidateOrder(
		FTacticalOrder Order,
		FString& FailureReason) const;
		
	UFUNCTION(BlueprintCallable, Category = "Aesir|Tactical Order")
	void BeginEncounter();

	UFUNCTION(BlueprintCallable, Category = "Aesir|Tactical Order")
	void EndEncounter();

	UFUNCTION(BlueprintCallable, Category = "Aesir|Tactical Order")
	bool TryAcceptOrder(
		FTacticalOrder Order,
		FString& FailureReason);

	UFUNCTION(BlueprintPure, Category = "Aesir|Tactical Order")
	bool HasActiveOrder() const { return bHasActiveOrder; }

	UFUNCTION(BlueprintPure, Category = "Aesir|Tactical Order")
	bool IsEncounterActive() const { return bEncounterActive; }
	
	UFUNCTION(BlueprintCallable, Category = "Aesir|Tactical Order")
	bool ValidateActiveOrderAtTrigger(
		bool bHasEnoughResource,
		bool bAbilityOffCooldown,
		float MaxExecutionDistance,
		FString& FailureReason) const;
	
	UFUNCTION(BlueprintCallable, Category = "Aesir|Tactical Order")
	bool CompleteActiveOrder(
		FString& CompletedOrderId,
		FString& FailureReason);
	
	UFUNCTION(BlueprintCallable, Category = "Aesir|Tactical Order")
	bool CancelActiveOrder(
		const FString& CancelReason,
		FString& CancelledOrderId,
		FString& ResultMessage);
	
private:
	AActor* ResolveTargetSelector(const FString& Selector) const;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Aesir|Tactical Order",
		meta = (AllowPrivateAccess = "true"))
	TArray<FName> SupportedAbilityIds;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
	Category = "Aesir|Tactical Order",
	meta = (AllowPrivateAccess = "true"))
	bool bEncounterActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
		Category = "Aesir|Tactical Order",
		meta = (AllowPrivateAccess = "true"))
	bool bHasActiveOrder = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
		Category = "Aesir|Tactical Order",
		meta = (AllowPrivateAccess = "true"))
	FTacticalOrder ActiveOrder;
};
