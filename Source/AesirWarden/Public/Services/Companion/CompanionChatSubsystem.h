#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "HttpFwd.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CompanionChatSubsystem.generated.h"

class FJsonObject;

USTRUCT(BlueprintType)
struct FCompanionChatReply
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString ProtocolVersion;

	UPROPERTY(BlueprintReadOnly)
	FString CompanionId;

	UPROPERTY(BlueprintReadOnly)
	FString ReplyText;

	UPROPERTY(BlueprintReadOnly)
	FString EmotionId;

	UPROPERTY(BlueprintReadOnly)
	FString GestureId;

	UPROPERTY(BlueprintReadOnly)
	FString FacialExpressionId;

	UPROPERTY(BlueprintReadOnly)
	bool bInterruptible = true;

	UPROPERTY(BlueprintReadOnly)
	FString Source;
};

USTRUCT(BlueprintType)
struct FCompanionAgentDirective
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString DirectiveId;

	UPROPERTY(BlueprintReadOnly)
	FString AgentId;

	UPROPERTY(BlueprintReadOnly)
	FString Domain;

	UPROPERTY(BlueprintReadOnly)
	FString ActionType;

	UPROPERTY(BlueprintReadOnly)
	FString TargetId;

	UPROPERTY(BlueprintReadOnly)
	FString Source;

	UPROPERTY(BlueprintReadOnly)
	FString PolicyRevision;

	UPROPERTY(BlueprintReadOnly)
	int32 Priority = 0;

	UPROPERTY(BlueprintReadOnly)
	FString ExpiresType;

	UPROPERTY(BlueprintReadOnly)
	float RemainingSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	TMap<FString, FString> Payload;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnCompanionChatReply,
	const FCompanionChatReply&, Reply);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnCompanionChatError,
	const FString&, Error);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnCompanionAgentDirective,
	const FCompanionAgentDirective&, Directive);

UCLASS()
class AESIRWARDEN_API UCompanionChatSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Existing chat widget input now uses the v0.3 agent command path.
	UFUNCTION(BlueprintCallable, Category = "Aesir|Companion Chat")
	void SendChatMessage(const FString& Text);

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Companion Chat")
	FOnCompanionChatReply OnReplyReceived;

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Companion Chat")
	FOnCompanionChatError OnRequestFailed;

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Companion Agent")
	FOnCompanionAgentDirective OnDirectiveReceived;

private:
	bool TickHeartbeat(float DeltaTime);
	void SendAgentStep(const FString& Text, bool bPlayerCommand);
	bool BuildWorldContext(
		TSharedRef<FJsonObject>& OutContext,
		TSet<FString>& OutAllowedTargetIds,
		FString& OutCompanionId,
		FString& OutSnapshotId,
		FString& OutScene,
		bool& bOutEncounterActive) const;
	void HandleAgentStepResponse(
		FHttpRequestPtr Request,
		FHttpResponsePtr Response,
		bool bSucceeded,
		FString ExpectedRequestId,
		FString ExpectedSnapshotId,
		FString ExpectedCompanionId,
		FString ExpectedScene,
		TSet<FString> AllowedTargetIds,
		bool bPlayerCommand,
		bool bEncounterActive,
		uint64 RequestSequence,
		double SentAtSeconds);
	void ReportFailure(bool bPlayerCommand, const FString& Message);

	FString ServiceBaseUrl = TEXT("http://127.0.0.1:8011");
	FTSTicker::FDelegateHandle HeartbeatTickerHandle;
	bool bHeartbeatInFlight = false;
	bool bHeartbeatFailureLogged = false;
	uint64 NextRequestSequence = 0;
	uint64 LatestPlayerCommandSequence = 0;
};
