#pragma once

#include "CoreMinimal.h"

class FJsonObject;
struct FCompanionChatReply;
struct FCompanionAgentDirective;

/** Pure v0.4 scoped JSON helpers. No HTTP or gameplay side effects. */
namespace CompanionScoped
{

struct FBinding
{
	FString GameId;
	FString PlayerId;
	FString SaveId;
	FString CompanionId;
	FString ServiceInstanceId;
	FString RunId;
	FString RunBindingId;
	FString MemoryEpoch;
	FString EncounterId;
	int64 ControlRevision = 0;
	int64 EncounterRevision = 0;
};

bool ParseContext(
	const TSharedPtr<FJsonObject>& Root,
	const FString& ExpectedSaveId,
	const FString& ExpectedCompanionId,
	const FString& ExpectedGameId,
	FBinding& OutBinding,
	FString& Error);

/** Preserve the previous companion/epoch while validating the save-scoped control result. */
bool ParseBoundControl(
	const TSharedPtr<FJsonObject>& Root,
	const FBinding& PreviousBinding,
	const FString& ExpectedRequestId,
	const FString& ExpectedRunId,
	FBinding& OutBinding,
	FString& Error);

/** Validate an exploration agent result against the captured binding and first send clock. */
bool ValidateAgentResponse(
	const TSharedPtr<FJsonObject>& Root,
	const FBinding& Binding,
	const FString& RequestId,
	const FString& SnapshotId,
	double FirstSentAt,
	double Now,
	FCompanionChatReply& OutReply,
	FCompanionAgentDirective& OutDirective,
	bool& bOutHasDirective,
	FString& Error);

/** NPC envelope only: callers add the endpoint-specific text/world/timing fields. */
TSharedRef<FJsonObject> BuildScopedBody(const FBinding& Binding, const FString& RequestId);
TSharedRef<FJsonObject> BuildBindBody(
	const FBinding& Binding, const FString& RequestId, const FString& NewRunId);

}
