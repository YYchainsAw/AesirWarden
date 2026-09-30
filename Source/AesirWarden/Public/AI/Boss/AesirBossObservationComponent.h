#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/Boss/AesirBossObservationTypes.h"
#include "AesirBossObservationComponent.generated.h"

class APawn;

UCLASS(ClassGroup = (Aesir), meta = (BlueprintSpawnableComponent))
class AESIRWARDEN_API UAesirBossObservationComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	UAesirBossObservationComponent();

	void InitializeForAvatar(APawn* InAvatar);

	UFUNCTION(BlueprintCallable, Category = "Aesir|AI|Boss|Observation")
	FAesirBossObservation CaptureObservation(AActor* Target);

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss|Observation")
	FAesirBossObservation GetLastObservation() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss|Observation")
	TArray<float> GetLastFeatureVector() const;

private:
	static bool TryReadHealth(
		AActor* Actor,
		float& OutHealth,
		float& OutMaxHealth);
	void PushRecentSample(TArray<uint8>& Samples, bool bValue);
	static float CalculateRecentRate(const TArray<uint8>& Samples);

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI|Boss|Observation",
		meta = (ClampMin = "100.0"))
	float MaxRelevantDistance = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI|Boss|Observation",
		meta = (ClampMin = "1", ClampMax = "32"))
	int32 RecentBehaviorWindow = 8;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI|Boss|Observation",
		meta = (ClampMin = "1.0"))
	float DistanceTrendScale = 200.0f;

	UPROPERTY(Transient)
	TObjectPtr<APawn> Avatar;

	UPROPERTY(Transient)
	FAesirBossObservation LastObservation;

	TArray<uint8> RecentTargetAttackSamples;
	TArray<uint8> RecentTargetBlockSamples;
	TArray<uint8> RecentTargetDodgeSamples;
	float PreviousDistanceToTarget = 0.0f;
	bool bHasPreviousDistance = false;

	int64 NextSequence = 1;
};
