#if WITH_DEV_AUTOMATION_TESTS

#include "Services/Companion/CompanionChatSubsystem.h"

#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCompanionChatLifecycleTest,
	"Aesir.Companion.Lifecycle.CallbacksAndSessionIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompanionChatLifecycleTest::RunTest(const FString& Parameters)
{
	// Deliberately do not Initialize: these tests have no ticker, world, real token, or HTTP.
	UGameInstance* Instance = NewObject<UGameInstance>();
	UCompanionChatSubsystem* Chat = NewObject<UCompanionChatSubsystem>(Instance);
	const FString SaveId = TEXT("27190000-0019-4019-8019-000000000019");
	const FString RunId = TEXT("33333333-3333-4333-8333-333333333333");
	const FString RunBindingId = TEXT("55555555-5555-4555-8555-555555555555");
	const FString ServiceInstanceId = TEXT("44444444-4444-4444-8444-444444444444");
	const FString MemoryEpoch = TEXT("66666666-6666-4666-8666-666666666666");

	Chat->CurrentSaveId = SaveId;
	Chat->DesiredRunId = RunId;
	Chat->Binding.GameId = TEXT("aesir");
	Chat->Binding.SaveId = SaveId;
	Chat->Binding.CompanionId = TEXT("companion.alice");
	Chat->Binding.RunId = RunId;
	Chat->Binding.RunBindingId = RunBindingId;
	Chat->Binding.ServiceInstanceId = ServiceInstanceId;
	Chat->Binding.MemoryEpoch = MemoryEpoch;
	Chat->ContextGeneration = 12;
	Chat->bContextActive = true;
	Chat->bBindingReady = true;
	Chat->NextHeartbeatAt = 12345.0;
	Chat->AppliedDirectiveIds.Add(TEXT("already-applied"));

	auto CurrentStep = MakeShared<UCompanionChatSubsystem::FPendingStep>();
	CurrentStep->RequestId = TEXT("11111111-1111-4111-8111-111111111111");
	CurrentStep->Generation = Chat->ContextGeneration;
	CurrentStep->bPlayerCommand = true;
	// Empty per-step bindings ensure even invalidation cannot send a cancellation.
	Chat->ActiveStep = CurrentStep;
	auto Uncertain = MakeShared<UCompanionChatSubsystem::FPendingStep>();
	Uncertain->Generation = Chat->ContextGeneration;
	Chat->UncertainStep = Uncertain;
	auto Queued = MakeShared<UCompanionChatSubsystem::FPendingStep>();
	Queued->bPlayerCommand = true;
	Chat->PendingSteps.Add(Queued);

	auto StaleStep = MakeShared<UCompanionChatSubsystem::FPendingStep>();
	StaleStep->Generation = Chat->ContextGeneration - 1;
	Chat->HandleAgentStepResponse(nullptr, nullptr, false, StaleStep);
	TestTrue(TEXT("Late previous-generation callback preserves active request"), Chat->ActiveStep == CurrentStep);
	TestTrue(TEXT("Late callback preserves uncertain request"), Chat->UncertainStep == Uncertain);
	TestEqual(TEXT("Late callback preserves queue"), Chat->PendingSteps.Num(), 1);
	TestEqual(TEXT("Late callback preserves generation"), Chat->ContextGeneration, int64(12));
	TestEqual(TEXT("Late callback preserves run"), Chat->Binding.RunId, RunId);
	TestEqual(TEXT("Late callback preserves heartbeat deadline"), Chat->NextHeartbeatAt, 12345.0);
	TestTrue(TEXT("Late callback preserves directive deduplication"), Chat->AppliedDirectiveIds.Contains(TEXT("already-applied")));

	// A cancelled/replaced request can carry the current generation but is still not active.
	StaleStep->Generation = Chat->ContextGeneration;
	Chat->HandleAgentStepResponse(nullptr, nullptr, false, StaleStep);
	TestTrue(TEXT("Inactive callback preserves actual active request"), Chat->ActiveStep == CurrentStep);
	TestTrue(TEXT("Inactive callback preserves uncertain request"), Chat->UncertainStep == Uncertain);
	TestEqual(TEXT("Inactive callback preserves queued message"), Chat->PendingSteps.Num(), 1);
	TestTrue(TEXT("Inactive callback preserves binding readiness"), Chat->bBindingReady);
	TestFalse(TEXT("Stale callbacks never start HTTP"), Chat->ActiveHttpRequest.IsValid());

	Chat->bConnecting = true;
	Chat->InvalidateSession(false);
	TestEqual(TEXT("Invalidation advances callback generation"), Chat->ContextGeneration, int64(13));
	TestFalse(TEXT("Invalidation clears active request"), Chat->ActiveStep.IsValid());
	TestFalse(TEXT("Invalidation clears uncertain request"), Chat->UncertainStep.IsValid());
	TestTrue(TEXT("Invalidation clears queued messages"), Chat->PendingSteps.IsEmpty());
	TestTrue(TEXT("Invalidation clears applied directives"), Chat->AppliedDirectiveIds.IsEmpty());
	TestTrue(TEXT("Invalidation clears binding identity"), Chat->Binding.RunBindingId.IsEmpty());
	TestTrue(TEXT("Invalidation clears desired run"), Chat->DesiredRunId.IsEmpty());
	TestFalse(TEXT("Invalidation clears readiness"), Chat->bBindingReady);
	TestFalse(TEXT("Invalidation clears connecting flag"), Chat->bConnecting);
	TestFalse(TEXT("Invalidation clears local context"), Chat->bContextActive);
	TestFalse(TEXT("Invalidation releases its reentrancy guard"), Chat->bInvalidating);
	TestEqual(TEXT("Invalidation keeps selected save configuration"), Chat->CurrentSaveId, SaveId);
	TestEqual(TEXT("Cancelled request identity remains immutable"), CurrentStep->RequestId,
		FString(TEXT("11111111-1111-4111-8111-111111111111")));
	TestFalse(TEXT("Local-only invalidation starts no HTTP"), Chat->ActiveHttpRequest.IsValid());

	Chat->DesiredRunId = RunId;
	Chat->Binding.RunId = RunId;
	Chat->Binding.RunBindingId = RunBindingId;
	Chat->Binding.ServiceInstanceId = ServiceInstanceId;
	Chat->Binding.MemoryEpoch = MemoryEpoch;
	Chat->Binding.CompanionId = TEXT("companion.alice");
	Chat->Binding.SaveId = SaveId;
	Chat->bBindingReady = true;
	Chat->bContextActive = true;
	Chat->AppliedDirectiveIds.Add(TEXT("alice-old-directive"));
	Chat->SetSelectedCompanionId(TEXT("companion.bruno"));
	TestEqual(TEXT("NPC selection changes"), Chat->GetSelectedCompanionId(), FString(TEXT("companion.bruno")));
	TestEqual(TEXT("NPC switch retires old callbacks"), Chat->ContextGeneration, int64(14));
	TestEqual(TEXT("NPC switch keeps desired save-scoped run"), Chat->DesiredRunId, RunId);
	TestEqual(TEXT("NPC switch preserves known shared run"), Chat->Binding.RunId, RunId);
	TestEqual(TEXT("NPC switch preserves known binding for cleanup"), Chat->Binding.RunBindingId, RunBindingId);
	TestEqual(TEXT("NPC switch preserves old epoch ownership until discovery"), Chat->Binding.CompanionId, FString(TEXT("companion.alice")));
	TestEqual(TEXT("NPC switch does not invent a new NPC epoch"), Chat->Binding.MemoryEpoch, MemoryEpoch);
	TestFalse(TEXT("New NPC requires fresh context discovery"), Chat->bBindingReady);
	TestFalse(TEXT("Old actor context is not reusable after switching"), Chat->bContextActive);
	TestTrue(TEXT("NPC switch clears old applied directives"), Chat->AppliedDirectiveIds.IsEmpty());
	TestFalse(TEXT("Selection alone starts no HTTP"), Chat->ActiveHttpRequest.IsValid());

	Chat->SetSelectedCompanionId(TEXT("companion.bruno"));
	TestEqual(TEXT("Selecting same NPC is idempotent"), Chat->ContextGeneration, int64(14));
	Chat->SetSelectedCompanionId(TEXT("../companion.alice"));
	TestEqual(TEXT("Invalid NPC selection keeps current NPC"), Chat->GetSelectedCompanionId(), FString(TEXT("companion.bruno")));
	TestEqual(TEXT("Invalid NPC selection keeps generation"), Chat->ContextGeneration, int64(14));

	// A synchronous reset listener cannot send, select, configure, or retry midway through invalidation.
	Chat->bInvalidating = true;
	Chat->SetSelectedCompanionId(TEXT("companion.alice"));
	TestFalse(TEXT("Configuration is rejected during invalidation"), Chat->ConfigureSession(TEXT("https://example.test"), TEXT("aesir"), SaveId));
	Chat->SendChatMessage(TEXT("EIRIN follow"));
	TestFalse(TEXT("Retry is rejected during invalidation"), Chat->RetryLastMessage());
	TestFalse(TEXT("Local context cannot refresh during invalidation"), Chat->RefreshLocalContext());
	Chat->InvalidateSession(false);
	TestEqual(TEXT("Reentrant selection is ignored"), Chat->GetSelectedCompanionId(), FString(TEXT("companion.bruno")));
	TestEqual(TEXT("Reentrant invalidation does not recurse"), Chat->ContextGeneration, int64(14));
	TestTrue(TEXT("Reentrant send cannot queue work"), Chat->PendingSteps.IsEmpty());
	TestFalse(TEXT("Reentrant operations start no HTTP"), Chat->ActiveHttpRequest.IsValid());
	Chat->bInvalidating = false;

	// Configuration rejection must not replace a usable identity or inspect process credentials.
	Chat->ServiceBaseUrl = TEXT("http://127.0.0.1:8011");
	Chat->BearerToken = TEXT("synthetic-test-only");
	const FString PreviousUrl = Chat->ServiceBaseUrl;
	TestFalse(TEXT("Configuration rejects malformed save UUID"),
		Chat->ConfigureSession(TEXT("https://example.test"), TEXT("aesir"), TEXT("not-a-uuid")));
	TestFalse(TEXT("Configuration rejects missing game ID"),
		Chat->ConfigureSession(TEXT("https://example.test"), FString(), SaveId));
	TestFalse(TEXT("Configuration rejects unsupported URL scheme"),
		Chat->ConfigureSession(TEXT("ftp://127.0.0.1:8011"), TEXT("aesir"), SaveId));
	TestFalse(TEXT("Configuration rejects embedded URL credentials"),
		Chat->ConfigureSession(TEXT("https://user:synthetic@example.test"), TEXT("aesir"), SaveId));
	TestFalse(TEXT("Configuration rejects URL query credentials"),
		Chat->ConfigureSession(TEXT("https://example.test?token=synthetic"), TEXT("aesir"), SaveId));
	TestFalse(TEXT("Configuration rejects URL fragments"),
		Chat->ConfigureSession(TEXT("https://example.test#synthetic"), TEXT("aesir"), SaveId));
	TestEqual(TEXT("Rejected configuration preserves endpoint"), Chat->ServiceBaseUrl, PreviousUrl);
	TestEqual(TEXT("Rejected configuration preserves save"), Chat->CurrentSaveId, SaveId);
	TestEqual(TEXT("Rejected configuration preserves credentials"), Chat->BearerToken, FString(TEXT("synthetic-test-only")));
	TestEqual(TEXT("Rejected configuration preserves generation"), Chat->ContextGeneration, int64(14));

	// An explicit retry of an expired uncertain operation releases it without minting a request ID.
	Chat->Binding = CompanionScoped::FBinding();
	Chat->DesiredRunId.Empty();
	Chat->bContextActive = false;
	auto Expired = MakeShared<UCompanionChatSubsystem::FPendingStep>();
	Expired->RequestId = TEXT("99999999-9999-4999-8999-999999999999");
	Expired->bPlayerCommand = true;
	Expired->Generation = Chat->ContextGeneration;
	Expired->WorldContext = MakeShared<FJsonObject>();
	Expired->FirstSentAt = FPlatformTime::Seconds() - 30.0;
	Expired->CapturedAt = Expired->FirstSentAt;
	Chat->ActiveStep = Expired;
	Chat->UncertainStep = Expired;
	Chat->SendAgentStep(Chat->ActiveStep);
	TestFalse(TEXT("Expired retry releases active operation"), Chat->ActiveStep.IsValid());
	TestFalse(TEXT("Expired retry releases uncertainty slot"), Chat->UncertainStep.IsValid());
	TestEqual(TEXT("Expired operation retains original logical ID"), Expired->RequestId,
		FString(TEXT("99999999-9999-4999-8999-999999999999")));
	TestFalse(TEXT("Expired operation is never sent"), Chat->ActiveHttpRequest.IsValid());
	TestTrue(TEXT("Expired operation is never requeued under a new ID"), Chat->PendingSteps.IsEmpty());
	return true;
}

#endif
