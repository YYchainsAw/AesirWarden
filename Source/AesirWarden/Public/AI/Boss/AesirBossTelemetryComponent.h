#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/Boss/AesirBossTelemetryTypes.h"
#include "AesirBossTelemetryComponent.generated.h"

UCLASS(ClassGroup = (Aesir), meta = (BlueprintSpawnableComponent))
class AESIRWARDEN_API UAesirBossTelemetryComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	UAesirBossTelemetryComponent();

	UFUNCTION(BlueprintCallable, Category = "Aesir|AI|Boss|Telemetry")
	FString StartEpisode(
		EAesirBossPolicySource PolicySource,
		const FString& RequestedEpisodeId);

	UFUNCTION(BlueprintCallable, Category = "Aesir|AI|Boss|Telemetry")
	void RecordDecision(const FAesirBossDecisionRecord& Record);

	UFUNCTION(BlueprintCallable, Category = "Aesir|AI|Boss|Telemetry")
	void SetPolicyIdentifier(const FString& PolicyIdentifier);

	UFUNCTION(BlueprintCallable, Category = "Aesir|AI|Boss|Telemetry")
	bool AddRewardToLatestDecision(
		float RewardDelta,
		EAesirBossRewardReason Reason,
		float SourceMagnitude = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Aesir|AI|Boss|Telemetry")
	FAesirBossEpisodeSummary EndEpisode(
		EAesirBossEpisodeResult Result);

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss|Telemetry")
	bool IsEpisodeActive() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss|Telemetry")
	TArray<FAesirBossDecisionRecord> GetDecisionRecords() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss|Telemetry")
	TArray<FAesirBossRewardEvent> GetRewardEvents() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss|Telemetry")
	FAesirBossEpisodeSummary GetEpisodeSummary() const;

private:
	bool ExportEpisodeToDisk() const;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI|Boss|Telemetry",
		meta = (ClampMin = "100", ClampMax = "100000"))
	int32 MaxDecisionRecords = 10000;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI|Boss|Telemetry")
	bool bExportCompletedEpisodes = true;

	UPROPERTY(Transient)
	TArray<FAesirBossDecisionRecord> DecisionRecords;

	UPROPERTY(Transient)
	TArray<FAesirBossRewardEvent> RewardEvents;

	UPROPERTY(Transient)
	FAesirBossEpisodeSummary EpisodeSummary;

	float EpisodeStartTimeSeconds = 0.0f;
	bool bEpisodeActive = false;
};
