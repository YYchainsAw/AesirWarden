// Copyright © 2026 YYchainsAw. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HttpFwd.h"
#include "TimerManager.h"
#include "AI/Boss/AesirBossObservationTypes.h"
#include "AesirBossPolicyClientComponent.generated.h"

class AAesirBossAIController;
class UAesirBossObservationComponent;

UCLASS(ClassGroup = (Aesir), meta = (BlueprintSpawnableComponent))
class AESIRWARDEN_API UAesirBossPolicyClientComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	UAesirBossPolicyClientComponent();

	void Initialize(
		AAesirBossAIController* InController,
		UAesirBossObservationComponent* InObservationComponent);
	void Start(AActor* InitialTarget = nullptr);
	void Stop();

	UFUNCTION(BlueprintPure, Category = "Aesir|AI|Boss|Policy")
	bool IsRunning() const;

private:
	void RequestDecision();
	void HandleDecisionResponse(
		FHttpRequestPtr Request,
		FHttpResponsePtr Response,
		bool bSucceeded,
		FString ExpectedRequestId,
		int64 ExpectedSequence,
		double RequestStartedSeconds);
	AActor* ResolveTarget() const;
	void RegisterFailure(const FString& Reason);
	void ResetFailures();

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI|Boss|Policy")
	FString ServiceBaseUrl = TEXT("http://127.0.0.1:8012");

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI|Boss|Policy",
		meta = (ClampMin = "0.05"))
	float DecisionIntervalSeconds = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI|Boss|Policy",
		meta = (ClampMin = "0.05"))
	float RequestTimeoutSeconds = 0.20f;

	UPROPERTY(EditDefaultsOnly, Category = "Aesir|AI|Boss|Policy",
		meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxConsecutiveFailures = 3;

	UPROPERTY(Transient)
	TObjectPtr<AAesirBossAIController> Controller;

	UPROPERTY(Transient)
	TObjectPtr<UAesirBossObservationComponent> ObservationComponent;

	UPROPERTY(Transient)
	TObjectPtr<AActor> TargetActor;

	FHttpRequestPtr ActiveRequest;
	FAesirBossObservation PendingObservation;
	FTimerHandle DecisionTimerHandle;
	int32 ConsecutiveFailures = 0;
	bool bRunning = false;
};
