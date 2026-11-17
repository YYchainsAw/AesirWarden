// Copyright (c) 2026 YYchainsAw. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "HAL/PlatformProcess.h"
#include "HttpFwd.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AesirBossProcessSubsystem.generated.h"

UENUM(BlueprintType)
enum class EAesirBossProcessState : uint8
{
	Disabled,
	Stopped,
	Starting,
	Ready,
	Failed
};

/** Owns one local inference process across level reloads. */
UCLASS()
class AESIRWARDEN_API UAesirBossProcessSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Aesir|Boss Service")
	void Start();

	UFUNCTION(BlueprintCallable, Category = "Aesir|Boss Service")
	void Stop();

	UFUNCTION(BlueprintPure, Category = "Aesir|Boss Service")
	EAesirBossProcessState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Aesir|Boss Service")
	FString GetFailureReason() const { return FailureReason; }

	UFUNCTION(BlueprintPure, Category = "Aesir|Boss Service")
	FString GetServiceBaseUrl() const;

	bool IsEnabled() const { return bEnabled; }
	bool IsReady() const { return State == EAesirBossProcessState::Ready; }
	bool MatchesModel(const FString& InModelId, const FString& InReleaseId, const FString& InModelSha256) const;

private:
	bool Tick(float DeltaSeconds);
	void LaunchProcess();
	void PollReadiness();
	void HandleReadiness(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSucceeded, uint64 ExpectedGeneration);
	void Fail(const FString& Reason);
	void ReleaseProcess();
	void DrainOutput();

	EAesirBossProcessState State = EAesirBossProcessState::Disabled;
	bool bEnabled = false;
	bool bDeinitializing = false;
	FString ExecutablePath;
	FString ExecutableSha256;
	FString ManifestSha256;
	FString ModelId;
	FString ReleaseId;
	FString ModelSha256;
	FString SimulationRevision;
	FString RewardRevision;
	FString FailureReason;
	FString ProcessLogPath;
	int32 Port = 8012;
	float StartupTimeoutSeconds = 120.0f;
	float HealthTimeoutSeconds = 2.0f;
	double StartedAt = 0.0;
	uint64 Generation = 0;
	uint32 OwnedProcessId = 0;
	FProcHandle OwnedProcess;
	void* OutputRead = nullptr;
	void* OutputWrite = nullptr;
	FHttpRequestPtr HealthRequest;
	FTSTicker::FDelegateHandle TickerHandle;
};
