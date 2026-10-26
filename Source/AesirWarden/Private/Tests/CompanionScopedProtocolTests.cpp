#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Services/Companion/CompanionChatSubsystem.h"
#include "Services/Companion/CompanionScopedProtocol.h"

namespace CompanionScopedTests
{
const FString RequestId = TEXT("11111111-1111-4111-8111-111111111111");
const FString SnapshotId = TEXT("88888888-8888-4888-8888-888888888888");
const FString OtherId = TEXT("99999999-9999-4999-8999-999999999999");

CompanionScoped::FBinding Binding()
{
	CompanionScoped::FBinding Result;
	Result.GameId = TEXT("aesir");
	Result.PlayerId = TEXT("77777777-7777-4777-8777-777777777777");
	Result.SaveId = TEXT("22222222-2222-4222-8222-222222222222");
	Result.CompanionId = TEXT("companion.alice");
	Result.ServiceInstanceId = TEXT("44444444-4444-4444-8444-444444444444");
	Result.RunId = TEXT("33333333-3333-4333-8333-333333333333");
	Result.RunBindingId = TEXT("55555555-5555-4555-8555-555555555555");
	Result.MemoryEpoch = TEXT("66666666-6666-4666-8666-666666666666");
	Result.ControlRevision = 1;
	return Result;
}

TSharedPtr<FJsonObject> AgentResponse()
{
	// Same identity and wire shape as the service's v0.4 exploration golden.
	const FString Json = TEXT(R"json({
  "protocol_version":"0.4",
  "request_id":"11111111-1111-4111-8111-111111111111",
  "service_instance_id":"44444444-4444-4444-8444-444444444444",
  "run_id":"33333333-3333-4333-8333-333333333333",
  "run_binding_id":"55555555-5555-4555-8555-555555555555",
  "memory_epoch":"66666666-6666-4666-8666-666666666666",
  "scope":{"game_id":"aesir","player_id":"77777777-7777-4777-8777-777777777777",
    "save_id":"22222222-2222-4222-8222-222222222222","companion_id":"companion.alice"},
  "session_id":"33333333-3333-4333-8333-333333333333",
  "encounter_id":null,"used_snapshot_id":"88888888-8888-4888-8888-888888888888",
  "timing":{"valid_for_ms":2000},"action":"directive","reply_text":"Following you.",
  "directive":{"protocol_version":"0.4","directive_id":"bbbbbbbb-bbbb-4bbb-8bbb-bbbbbbbbbbbb",
    "agent_id":"companion.alice","domain":"exploration","action_type":"follow",
    "priority":50,"expires":{"type":"immediate"},"source":"player_command",
    "reason_codes":["PLAYER_COMMAND"],"policy_revision":"scope-v04-golden",
    "payload":{"target_id":"party.player"},"presentation":null},
  "observability":{"source":"rule","reason_codes":["PLAYER_COMMAND"],
    "used_snapshot_id":"88888888-8888-4888-8888-888888888888"}
})json");
	TSharedPtr<FJsonObject> Result;
	FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Result);
	return Result;
}

TSharedPtr<FJsonObject> Child(const TSharedPtr<FJsonObject>& Parent, const TCHAR* Key)
{
	return Parent->GetObjectField(Key);
}

bool Validate(const TSharedPtr<FJsonObject>& Root, double Now = 100.5)
{
	FCompanionChatReply Reply;
	FCompanionAgentDirective Directive;
	bool bHasDirective = false;
	FString Error;
	return CompanionScoped::ValidateAgentResponse(Root, Binding(), RequestId, SnapshotId,
		100.0, Now, Reply, Directive, bHasDirective, Error);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionScopedContextControlTest,
	"Aesir.Companion.ScopedProtocol.ContextAndBind", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompanionScopedContextControlTest::RunTest(const FString& Parameters)
{
	using namespace CompanionScopedTests;
	const CompanionScoped::FBinding Expected = Binding();
	TSharedPtr<FJsonObject> Root = AgentResponse();
	for (const TCHAR* Key : {TEXT("request_id"), TEXT("session_id"), TEXT("used_snapshot_id"),
		TEXT("timing"), TEXT("action"), TEXT("directive"), TEXT("reply_text"), TEXT("observability")})
		Root->RemoveField(Key);
	Root->SetField(TEXT("run_id"), MakeShared<FJsonValueNull>());
	Root->SetField(TEXT("run_binding_id"), MakeShared<FJsonValueNull>());
	Root->SetNumberField(TEXT("control_revision"), 0);
	Root->SetNumberField(TEXT("encounter_revision"), 0);
	CompanionScoped::FBinding Parsed;
	FString Error;
	TestTrue(TEXT("Unbound context parses without a fabricated request identity"),
		CompanionScoped::ParseContext(Root, Expected.SaveId, Expected.CompanionId, Expected.GameId, Parsed, Error));
	TestTrue(TEXT("Unbound context preserves null run/binding"), Parsed.RunId.IsEmpty() && Parsed.RunBindingId.IsEmpty());
	TestFalse(TEXT("Wrong save is not accepted as a context"),
		CompanionScoped::ParseContext(Root, OtherId, Expected.CompanionId, Expected.GameId, Parsed, Error));
	TestFalse(TEXT("Wrong game is not accepted as a context"),
		CompanionScoped::ParseContext(Root, Expected.SaveId, Expected.CompanionId, TEXT("other"), Parsed, Error));
	Root->SetStringField(TEXT("run_id"), Expected.RunId);
	TestFalse(TEXT("Half-present binding is rejected"),
		CompanionScoped::ParseContext(Root, Expected.SaveId, Expected.CompanionId, Expected.GameId, Parsed, Error));

	CompanionScoped::FBinding Unbound = Expected;
	Unbound.RunId.Reset();
	Unbound.RunBindingId.Reset();
	Unbound.ControlRevision = 0;
	const TSharedRef<FJsonObject> Body = CompanionScoped::BuildBindBody(Unbound, RequestId, Expected.RunId);
	TestTrue(TEXT("First bind sends explicit expected_run_id null"), Body->TryGetField(TEXT("expected_run_id"))->Type == EJson::Null);
	TestFalse(TEXT("Bind never sends caller claimed player identity"), Body->HasField(TEXT("player_id")));

	Root = AgentResponse();
	for (const TCHAR* Key : {TEXT("memory_epoch"), TEXT("session_id"), TEXT("used_snapshot_id"),
		TEXT("timing"), TEXT("action"), TEXT("directive"), TEXT("reply_text"), TEXT("observability")})
		Root->RemoveField(Key);
	Child(Root, TEXT("scope"))->RemoveField(TEXT("companion_id"));
	Root->SetNumberField(TEXT("control_revision"), 1);
	Root->SetNumberField(TEXT("encounter_revision"), 0);
	TestTrue(TEXT("CAS bind accepts the save-scoped response"),
		CompanionScoped::ParseBoundControl(Root, Unbound, RequestId, Expected.RunId, Parsed, Error));
	TestEqual(TEXT("Bind retains companion epoch obtained from context"), Parsed.MemoryEpoch, Expected.MemoryEpoch);
	Root->SetNumberField(TEXT("control_revision"), 2);
	TestFalse(TEXT("Unexpected control revision is rejected"),
		CompanionScoped::ParseBoundControl(Root, Unbound, RequestId, Expected.RunId, Parsed, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionScopedIdentityTest,
	"Aesir.Companion.ScopedProtocol.IdentityIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompanionScopedIdentityTest::RunTest(const FString& Parameters)
{
	using namespace CompanionScopedTests;
	TestTrue(TEXT("Service golden identity shape is accepted"), Validate(AgentResponse()));
	for (const TCHAR* Key : {TEXT("request_id"), TEXT("service_instance_id"), TEXT("run_id"),
		TEXT("run_binding_id"), TEXT("memory_epoch"), TEXT("session_id"), TEXT("used_snapshot_id")})
	{
		TSharedPtr<FJsonObject> Root = AgentResponse();
		Root->SetStringField(Key, OtherId);
		TestFalse(FString::Printf(TEXT("Mismatched %s is rejected"), Key), Validate(Root));
	}
	for (const TCHAR* Key : {TEXT("player_id"), TEXT("save_id")})
	{
		TSharedPtr<FJsonObject> Root = AgentResponse();
		Child(Root, TEXT("scope"))->SetStringField(Key, OtherId);
		TestFalse(FString::Printf(TEXT("Forged scope %s is rejected"), Key), Validate(Root));
	}
	TSharedPtr<FJsonObject> Root = AgentResponse();
	Child(Root, TEXT("scope"))->SetStringField(TEXT("companion_id"), TEXT("companion.bruno"));
	TestFalse(TEXT("Other companion cannot supply Alice's result"), Validate(Root));
	Root = AgentResponse();
	Root->SetStringField(TEXT("encounter_id"), OtherId);
	TestFalse(TEXT("Battle result cannot enter exploration"), Validate(Root));
	Root = AgentResponse();
	Child(Root, TEXT("observability"))->SetStringField(TEXT("used_snapshot_id"), OtherId);
	TestFalse(TEXT("Outer and observability snapshots must agree"), Validate(Root));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionScopedTimingTest,
	"Aesir.Companion.ScopedProtocol.MonotonicValidity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompanionScopedTimingTest::RunTest(const FString& Parameters)
{
	using namespace CompanionScopedTests;
	TestTrue(TEXT("Before original deadline remains valid"), Validate(AgentResponse(), 101.999));
	TestFalse(TEXT("Exact original deadline is expired"), Validate(AgentResponse(), 102.0));
	TestFalse(TEXT("Cached response does not restart its validity"), Validate(AgentResponse(), 103.0));
	TestFalse(TEXT("Backwards monotonic clock is rejected"), Validate(AgentResponse(), 99.0));
	TSharedPtr<FJsonObject> Root = AgentResponse();
	Child(Root, TEXT("timing"))->SetNumberField(TEXT("valid_for_ms"), 1999.5);
	TestFalse(TEXT("Fractional millisecond integer field is rejected"), Validate(Root));
	Root = AgentResponse();
	Child(Root, TEXT("timing"))->SetStringField(TEXT("valid_for_ms"), TEXT("2000"));
	TestFalse(TEXT("Coerced string timing is rejected"), Validate(Root));
	FCompanionChatReply Reply;
	FCompanionAgentDirective First;
	FCompanionAgentDirective Replay;
	bool bHasDirective = false;
	FString Error;
	Root = AgentResponse();
	TestTrue(TEXT("First response is valid"), CompanionScoped::ValidateAgentResponse(
		Root, Binding(), RequestId, SnapshotId, 100.0, 100.5, Reply, First, bHasDirective, Error));
	TestTrue(TEXT("Logical replay remains parseable within its existing deadline"), CompanionScoped::ValidateAgentResponse(
		Root, Binding(), RequestId, SnapshotId, 100.0, 101.0, Reply, Replay, bHasDirective, Error));
	TestEqual(TEXT("Replay retains directive identity for execution deduplication"), Replay.DirectiveId, First.DirectiveId);
	TestEqual(TEXT("Replay retains first-send deadline"), Replay.ValidUntilSeconds, First.ValidUntilSeconds);
	Root = AgentResponse();
	TSharedPtr<FJsonObject> Expires = Child(Child(Root, TEXT("directive")), TEXT("expires"));
	Expires->SetStringField(TEXT("type"), TEXT("before_seconds"));
	Expires->SetNumberField(TEXT("remaining_seconds"), 0.1);
	TestTrue(TEXT("Short directive is valid before its own deadline"), CompanionScoped::ValidateAgentResponse(
		Root, Binding(), RequestId, SnapshotId, 100.0, 100.05, Reply, First, bHasDirective, Error));
	TestEqual(TEXT("Short directive tightens the two-second response budget"), First.ValidUntilSeconds, 100.1);
	TestFalse(TEXT("Short directive is expired at the exact action deadline"), Validate(Root, 100.1));
	TestFalse(TEXT("Longer response budget cannot extend an expired short directive"), Validate(Root, 100.5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanionScopedActionsTest,
	"Aesir.Companion.ScopedProtocol.ExplorationActions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompanionScopedActionsTest::RunTest(const FString& Parameters)
{
	using namespace CompanionScopedTests;
	TSharedPtr<FJsonObject> Root = AgentResponse();
	Child(Child(Root, TEXT("directive")), TEXT("payload"))->SetStringField(TEXT("target_id"), TEXT("encounter.primary_hostile"));
	TestFalse(TEXT("Follow cannot reference a hostile or unobserved target"), Validate(Root));
	Root = AgentResponse();
	Child(Child(Root, TEXT("directive")), TEXT("payload"))->SetStringField(TEXT("x"), TEXT("10000"));
	TestFalse(TEXT("Additional movement coordinates cannot be smuggled into follow"), Validate(Root));
	Root = AgentResponse();
	Child(Root, TEXT("directive"))->SetStringField(TEXT("action_type"), TEXT("move_to"));
	TestFalse(TEXT("Unimplemented movement action is rejected"), Validate(Root));
	Root = AgentResponse();
	Child(Root, TEXT("directive"))->SetStringField(TEXT("action_type"), TEXT("wait"));
	TestFalse(TEXT("Wait cannot carry a follow payload"), Validate(Root));
	Child(Root, TEXT("directive"))->SetObjectField(TEXT("payload"), MakeShared<FJsonObject>());
	TestTrue(TEXT("Wait with the service's empty payload is valid"), Validate(Root));
	Root = AgentResponse();
	Child(Root, TEXT("directive"))->SetNumberField(TEXT("priority"), 50.5);
	TestFalse(TEXT("Fractional priority is rejected"), Validate(Root));
	Root = AgentResponse();
	Child(Root, TEXT("directive"))->SetStringField(TEXT("source"), TEXT("autonomy"));
	FCompanionChatReply Reply;
	FCompanionAgentDirective Directive;
	bool bHasDirective = true;
	FString Error;
	TestTrue(TEXT("Valid autonomous reply can still be displayed"), CompanionScoped::ValidateAgentResponse(
		Root, Binding(), RequestId, SnapshotId, 100.0, 100.5, Reply, Directive, bHasDirective, Error));
	TestFalse(TEXT("Autonomy cannot execute in the text-only exploration slice"), bHasDirective);
	TestTrue(TEXT("Suppressed autonomous output has no executable identity"), Directive.DirectiveId.IsEmpty());
	Root = AgentResponse();
	Root->SetStringField(TEXT("action"), TEXT("none"));
	TestFalse(TEXT("None cannot contain an executable directive"), Validate(Root));
	Root->SetField(TEXT("directive"), MakeShared<FJsonValueNull>());
	TestTrue(TEXT("Valid none response does not require an action"), Validate(Root));
	const TSharedRef<FJsonObject> Envelope = CompanionScoped::BuildScopedBody(Binding(), RequestId);
	TestFalse(TEXT("Generic NPC body does not invent endpoint-specific timing"), Envelope->HasField(TEXT("timing")));
	TestFalse(TEXT("Generic NPC body does not invent endpoint-specific snapshot"), Envelope->HasField(TEXT("world_context")));
	TestEqual(TEXT("Default session is the run"), Envelope->GetStringField(TEXT("session_id")), Binding().RunId);
	TestTrue(TEXT("Exploration envelope uses explicit null encounter"), Envelope->TryGetField(TEXT("encounter_id"))->Type == EJson::Null);
	return true;
}

#endif
