#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "HttpFwd.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Services/Companion/CompanionScopedProtocol.h"
#include "CompanionChatSubsystem.generated.h"

class FJsonObject;
class AAesirCompanionCharacter;

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
	FString RequestId;

	UPROPERTY(BlueprintReadOnly)
	FString RunId;

	UPROPERTY(BlueprintReadOnly)
	int64 ContextGeneration = 0;

	// A local monotonic deadline, never a new TTL starting when the response arrives.
	double ValidUntilSeconds = 0.0;

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCompanionConversationReset);

UCLASS()
class AESIRWARDEN_API UCompanionChatSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Commands and ordinary text share the authenticated v0.4 agent entry point.
	UFUNCTION(BlueprintCallable, Category = "Aesir|Companion Chat")
	void SendChatMessage(const FString& Text);

	UFUNCTION(BlueprintPure, Category = "Aesir|Companion Chat")
	FString GetSelectedCompanionId() const { return SelectedCompanionId; }

	UFUNCTION(BlueprintCallable, Category = "Aesir|Companion Chat")
	void SetSelectedCompanionId(const FString& CompanionId);

	/** Call when loading a save/account. Tokens come from the process environment, never assets. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Companion Chat")
	bool ConfigureSession(const FString& BaseUrl, const FString& GameId, const FString& SaveId);

	/** Invalidate locally first; best-effort cancellation/end never blocks travel or exit. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Companion Chat")
	void ResetConversation();

	/** Explicit retry keeps the same logical ID, snapshot and first-send deadline. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Companion Chat")
	bool RetryLastMessage();

	bool IsDirectiveCurrent(const FCompanionAgentDirective& Directive) const;

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Companion Chat")
	FOnCompanionChatReply OnReplyReceived;

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Companion Chat")
	FOnCompanionChatError OnRequestFailed;

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Companion Agent")
	FOnCompanionAgentDirective OnDirectiveReceived;

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Companion Chat")
	FOnCompanionConversationReset OnConversationReset;

private:
	friend class FCompanionExplorationExecutionTest;

	friend class FCompanionChatLifecycleTest;
	struct FPendingStep
	{
		FString RequestId;
		FString Text;
		FString SnapshotId;
		TSharedPtr<FJsonObject> WorldContext;
		CompanionScoped::FBinding Binding;
		double CapturedAt = 0.0;
		double FirstSentAt = 0.0;
		int64 Generation = 0;
		bool bPlayerCommand = false;
	};

	bool TickHeartbeat(float DeltaTime);
	void PumpQueue();
	void DiscoverContext();
	void BindRun(const CompanionScoped::FBinding& Discovered);
	void SendAgentStep(TSharedPtr<FPendingStep> Step);
	void CompleteStep(TSharedPtr<FPendingStep> Step);
	bool RefreshLocalContext();
	bool IsExplorationActive() const;
	void InvalidateSession(bool bEndRun, bool bPreserveRun = false);
	void FailSession(const FString& Message);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	FHttpRequestPtr MakeRequest(const FString& Path, const TSharedPtr<FJsonObject>& Body = nullptr);
	void SendCancellation(const TSharedPtr<FPendingStep>& Step);
	void SendRunEnd();
	bool ReadResponse(FHttpResponsePtr Response, bool bSucceeded, TSharedPtr<FJsonObject>& Root, FString& Error) const;
	bool BuildWorldContext(
		TSharedRef<FJsonObject>& OutContext,
		FString& OutCompanionId,
		FString& OutSnapshotId) const;
	void HandleAgentStepResponse(
		FHttpRequestPtr Request,
		FHttpResponsePtr Response,
		bool bSucceeded,
		TSharedPtr<FPendingStep> Step);
	void ReportFailure(bool bPlayerCommand, const FString& Message);

	FString ServiceBaseUrl = TEXT("http://127.0.0.1:8011");
	FString ExpectedGameId = TEXT("aesir");
	FString CurrentSaveId;
	FString SelectedCompanionId = TEXT("companion.alice");
	FString BearerToken;
	FString DesiredRunId;
	CompanionScoped::FBinding Binding;
	TArray<TSharedPtr<FPendingStep>> PendingSteps;
	TSharedPtr<FPendingStep> ActiveStep;
	TSharedPtr<FPendingStep> UncertainStep;
	FHttpRequestPtr ActiveHttpRequest;
	TSet<FString> AppliedDirectiveIds;
	TWeakObjectPtr<UWorld> SessionWorld;
	TWeakObjectPtr<APawn> SessionPlayer;
	TWeakObjectPtr<AAesirCompanionCharacter> SessionCompanion;
	FTSTicker::FDelegateHandle HeartbeatTickerHandle;
	FDelegateHandle WorldCleanupHandle;
	double NextHeartbeatAt = 0.0;
	int64 ContextGeneration = 0;
	bool bContextActive = false;
	bool bBindingReady = false;
	bool bConnecting = false;
	bool bReconnectRequired = false;
	bool bRequireBearer = false;
	bool bInvalidating = false;
	bool bHeartbeatFailureLogged = false;
};
