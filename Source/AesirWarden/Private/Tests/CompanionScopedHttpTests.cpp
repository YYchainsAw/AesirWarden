#if WITH_DEV_AUTOMATION_TESTS

#include "Services/Companion/CompanionChatSubsystem.h"
#include "Services/Companion/CompanionScopedProtocol.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
// This save must be explicitly authorized by a disposable rule/mock test service.
constexpr TCHAR ContractSaveId[] = TEXT("27190000-0019-4019-8019-000000000019");
constexpr double HttpDeadlineSeconds = 10.0;

enum class EContractStep : uint8
{
	DiscoverAlice,
	Bind,
	AliceFollow,
	AliceRepeat,
	AliceWait,
	AliceChat,
	DiscoverBruno,
	BrunoWrongWake,
	BrunoFollow,
	End,
	Done
};

bool IsLoopbackUrl(const FString& Url)
{
	for (const TCHAR* Prefix : { TEXT("http://127.0.0.1:"), TEXT("http://localhost:"), TEXT("http://[::1]:") })
	{
		if (!Url.StartsWith(Prefix, ESearchCase::IgnoreCase))
		{
			continue;
		}
		const FString Port = Url.Mid(FCString::Strlen(Prefix));
		if (Port.IsEmpty() || Port.Len() > 5)
		{
			return false;
		}
		for (TCHAR Character : Port)
		{
			if (Character < TEXT('0') || Character > TEXT('9'))
			{
				return false;
			}
		}
		const int32 PortNumber = FCString::Atoi(*Port);
		return PortNumber > 0 && PortNumber <= 65535;
	}
	return false;
}

FString NewContractId()
{
	return FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
}

struct FContractState
{
	FAutomationTestBase* Test = nullptr;
	FString BaseUrl;
	EContractStep Step = EContractStep::DiscoverAlice;
	CompanionScoped::FBinding Alice;
	CompanionScoped::FBinding Bruno;
	FString RunId = NewContractId();
	FString RequestId;
	FString SnapshotId;
	FString FollowRequestId;
	FString FollowSnapshotId;
	FString FollowDirectiveId;
	TSharedPtr<FJsonObject> FollowBody;
	double FollowSentAt = 0.0;
	double FirstSentAt = 0.0;
	double HttpStartedAt = 0.0;
	FHttpRequestPtr HttpRequest;
	FHttpResponsePtr HttpResponse;
	bool bRequestComplete = false;
	bool bTransportSucceeded = false;
	bool bRunBound = false;
	bool bHadFailure = false;

	void Fail(const FString& Message)
	{
		bHadFailure = true;
		Test->AddError(FString::Printf(TEXT("Scoped HTTP step %d: %s"), static_cast<int32>(Step), *Message));
		// Local callbacks are retired before cleanup. Do not retry unknown operations.
		if (HttpRequest.IsValid())
		{
			HttpRequest->OnProcessRequestComplete().Unbind();
			HttpRequest->CancelRequest();
			HttpRequest.Reset();
		}
		HttpResponse.Reset();
		bRequestComplete = false;
		Step = bRunBound && Step != EContractStep::End ? EContractStep::End : EContractStep::Done;
	}
};

class FCompanionHttpContractCommand final : public IAutomationLatentCommand
{
public:
	explicit FCompanionHttpContractCommand(const TSharedRef<FContractState>& InState)
		: State(InState)
	{
	}

	virtual bool Update() override
	{
		if (State->Step == EContractStep::Done)
		{
			return true;
		}
		if (!State->HttpRequest.IsValid())
		{
			StartRequest();
			return State->Step == EContractStep::Done;
		}
		if (!State->bRequestComplete)
		{
			if (FPlatformTime::Seconds() - State->HttpStartedAt >= HttpDeadlineSeconds)
			{
				State->Fail(TEXT("Request exceeded the 10 second deadline; outcome was not retried."));
			}
			return State->Step == EContractStep::Done;
		}

		State->HttpRequest->OnProcessRequestComplete().Unbind();
		State->HttpRequest.Reset();
		if (!State->bTransportSucceeded || !State->HttpResponse.IsValid())
		{
			State->Fail(TEXT("Loopback service did not return a response."));
			return State->Step == EContractStep::Done;
		}
		if (State->HttpResponse->GetResponseCode() != 200)
		{
			// Do not include server bodies, tokens, or request text in test diagnostics.
			State->Fail(FString::Printf(TEXT("Unexpected HTTP status %d."), State->HttpResponse->GetResponseCode()));
			return State->Step == EContractStep::Done;
		}
		TSharedPtr<FJsonObject> Root;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(State->HttpResponse->GetContentAsString()), Root) || !Root.IsValid())
		{
			State->Fail(TEXT("Response was not a JSON object."));
			return State->Step == EContractStep::Done;
		}
		State->HttpResponse.Reset();
		State->bRequestComplete = false;
		ValidateResponse(Root);
		return State->Step == EContractStep::Done;
	}

private:
	TSharedRef<FContractState> State;

	TSharedRef<FJsonObject> AgentBody(const CompanionScoped::FBinding& Binding, const FString& Text)
	{
		State->RequestId = NewContractId();
		State->SnapshotId = NewContractId();
		State->FirstSentAt = FPlatformTime::Seconds();
		const TSharedRef<FJsonObject> Body = CompanionScoped::BuildScopedBody(Binding, State->RequestId);
		Body->SetStringField(TEXT("text"), Text);
		const TSharedRef<FJsonObject> Timing = MakeShared<FJsonObject>();
		Timing->SetNumberField(TEXT("capture_age_ms"), 0);
		Timing->SetNumberField(TEXT("request_age_ms"), 0);
		Timing->SetNumberField(TEXT("max_snapshot_age_ms"), 10000);
		Timing->SetNumberField(TEXT("max_request_age_ms"), 10000);
		Body->SetObjectField(TEXT("timing"), Timing);
		const TSharedRef<FJsonObject> World = MakeShared<FJsonObject>();
		World->SetStringField(TEXT("snapshot_id"), State->SnapshotId);
		World->SetStringField(TEXT("captured_at"), FDateTime::UtcNow().ToIso8601());
		World->SetStringField(TEXT("scene"), TEXT("exploration"));
		const TSharedRef<FJsonObject> Player = MakeShared<FJsonObject>();
		Player->SetStringField(TEXT("id"), TEXT("party.player"));
		Player->SetNumberField(TEXT("hp_percent"), 100);
		Player->SetBoolField(TEXT("is_downed"), false);
		World->SetObjectField(TEXT("player"), Player);
		const TSharedRef<FJsonObject> Companion = MakeShared<FJsonObject>();
		Companion->SetStringField(TEXT("id"), Binding.CompanionId);
		Companion->SetNumberField(TEXT("hp_percent"), 100);
		Companion->SetNumberField(TEXT("mp_percent"), 0);
		Companion->SetStringField(TEXT("current_behavior"), TEXT("idle"));
		Companion->SetBoolField(TEXT("is_casting"), false);
		World->SetObjectField(TEXT("companion"), Companion);
		Body->SetObjectField(TEXT("world_context"), World);
		return Body;
	}

	void StartRequest()
	{
		FString Verb = TEXT("POST");
		FString Path;
		TSharedPtr<FJsonObject> Body;
		switch (State->Step)
		{
		case EContractStep::DiscoverAlice:
		case EContractStep::DiscoverBruno:
			Verb = TEXT("GET");
			Path = FString::Printf(TEXT("/v1/scoped/context?save_id=%s&companion_id=%s"), ContractSaveId,
				State->Step == EContractStep::DiscoverAlice ? TEXT("companion.alice") : TEXT("companion.bruno"));
			break;
		case EContractStep::Bind:
			State->RequestId = NewContractId();
			Body = CompanionScoped::BuildBindBody(State->Alice, State->RequestId, State->RunId);
			Path = TEXT("/v1/scoped/runs/bind");
			break;
		case EContractStep::AliceFollow:
			Body = AgentBody(State->Alice, TEXT("EIRIN follow"));
			State->FollowRequestId = State->RequestId;
			State->FollowSnapshotId = State->SnapshotId;
			State->FollowSentAt = State->FirstSentAt;
			State->FollowBody = Body;
			Path = TEXT("/v1/scoped/agent/step");
			break;
		case EContractStep::AliceRepeat:
		{
			Body = State->FollowBody;
			State->RequestId = State->FollowRequestId;
			State->SnapshotId = State->FollowSnapshotId;
			State->FirstSentAt = State->FollowSentAt;
			const int64 AgeMs = FMath::FloorToInt64((FPlatformTime::Seconds() - State->FirstSentAt) * 1000.0);
			const TSharedRef<FJsonObject> Timing = MakeShared<FJsonObject>();
			Timing->SetNumberField(TEXT("capture_age_ms"), AgeMs);
			Timing->SetNumberField(TEXT("request_age_ms"), AgeMs);
			Timing->SetNumberField(TEXT("max_snapshot_age_ms"), 10000);
			Timing->SetNumberField(TEXT("max_request_age_ms"), 10000);
			Body->SetObjectField(TEXT("timing"), Timing);
			Path = TEXT("/v1/scoped/agent/step");
			break;
		}
		case EContractStep::AliceWait:
			Body = AgentBody(State->Alice, TEXT("EIRIN wait"));
			Path = TEXT("/v1/scoped/agent/step");
			break;
		case EContractStep::AliceChat:
			Body = AgentBody(State->Alice, TEXT("Hello, how are you today?"));
			Path = TEXT("/v1/scoped/agent/step");
			break;
		case EContractStep::BrunoWrongWake:
			Body = AgentBody(State->Bruno, TEXT("EIRIN follow"));
			Path = TEXT("/v1/scoped/agent/step");
			break;
		case EContractStep::BrunoFollow:
			Body = AgentBody(State->Bruno, TEXT("BRUNO follow"));
			Path = TEXT("/v1/scoped/agent/step");
			break;
		case EContractStep::End:
			State->RequestId = NewContractId();
			Body = MakeShared<FJsonObject>();
			Body->SetStringField(TEXT("protocol_version"), TEXT("0.4"));
			Body->SetStringField(TEXT("request_id"), State->RequestId);
			Body->SetStringField(TEXT("save_id"), State->Alice.SaveId);
			Body->SetStringField(TEXT("service_instance_id"), State->Alice.ServiceInstanceId);
			Body->SetStringField(TEXT("run_id"), State->Alice.RunId);
			Body->SetStringField(TEXT("run_binding_id"), State->Alice.RunBindingId);
			Body->SetNumberField(TEXT("expected_control_revision"), State->Alice.ControlRevision);
			Path = TEXT("/v1/scoped/runs/end");
			break;
		default:
			State->Step = EContractStep::Done;
			return;
		}

		FString JsonBody;
		if (Body.IsValid() && !FJsonSerializer::Serialize(Body.ToSharedRef(), TJsonWriterFactory<>::Create(&JsonBody)))
		{
			State->Fail(TEXT("Could not serialize request."));
			return;
		}
		State->HttpRequest = FHttpModule::Get().CreateRequest();
		State->HttpRequest->SetURL(State->BaseUrl + Path);
		State->HttpRequest->SetVerb(Verb);
		State->HttpRequest->SetHeader(TEXT("Accept"), TEXT("application/json"));
		State->HttpRequest->SetTimeout(static_cast<float>(HttpDeadlineSeconds));
		if (Body.IsValid())
		{
			State->HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
			State->HttpRequest->SetContentAsString(JsonBody);
		}
		State->HttpStartedAt = FPlatformTime::Seconds();
		State->bRequestComplete = false;
		const TWeakPtr<FContractState> WeakState = State;
		State->HttpRequest->OnProcessRequestComplete().BindLambda(
			[WeakState](FHttpRequestPtr, FHttpResponsePtr Response, bool bSucceeded)
			{
				if (const TSharedPtr<FContractState> Pinned = WeakState.Pin())
				{
					Pinned->HttpResponse = Response;
					Pinned->bTransportSucceeded = bSucceeded;
					Pinned->bRequestComplete = true;
				}
			});
		if (!State->HttpRequest->ProcessRequest())
		{
			State->Fail(TEXT("Could not start loopback HTTP request."));
		}
	}

	void ValidateResponse(const TSharedPtr<FJsonObject>& Root)
	{
		FString Error;
		switch (State->Step)
		{
		case EContractStep::DiscoverAlice:
			if (!CompanionScoped::ParseContext(Root, ContractSaveId, TEXT("companion.alice"), TEXT("aesir"), State->Alice, Error))
			{
				State->Fail(Error);
				return;
			}
			// Refuse to replace a live run, even on the explicitly designated test save.
			if (!State->Alice.RunId.IsEmpty())
			{
				State->Fail(TEXT("Dedicated contract save already has a live run; use a fresh test service."));
				return;
			}
			break;
		case EContractStep::Bind:
		{
			CompanionScoped::FBinding Bound;
			if (!CompanionScoped::ParseBoundControl(Root, State->Alice, State->RequestId, State->RunId, Bound, Error))
			{
				State->Fail(Error);
				return;
			}
			State->Alice = Bound;
			State->bRunBound = true;
			break;
		}
		case EContractStep::DiscoverBruno:
			if (!CompanionScoped::ParseContext(Root, ContractSaveId, TEXT("companion.bruno"), TEXT("aesir"), State->Bruno, Error))
			{
				State->Fail(Error);
				return;
			}
			if (State->Bruno.RunId != State->Alice.RunId || State->Bruno.RunBindingId != State->Alice.RunBindingId ||
				State->Bruno.ServiceInstanceId != State->Alice.ServiceInstanceId || State->Bruno.PlayerId != State->Alice.PlayerId)
			{
				State->Fail(TEXT("Companions did not share the same save/run binding."));
				return;
			}
			break;
		case EContractStep::End:
		{
			// Bind and end have different nullability rules; do not feed end into the bind parser.
			const auto MatchesString = [](const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, const FString& Expected)
			{
				FString Value;
				return Object.IsValid() && Object->TryGetStringField(Field, Value) && Value == Expected;
			};
			const TSharedPtr<FJsonObject>* Scope = nullptr;
			const TSharedPtr<FJsonValue> Run = Root->TryGetField(TEXT("run_id"));
			const TSharedPtr<FJsonValue> RunBinding = Root->TryGetField(TEXT("run_binding_id"));
			const TSharedPtr<FJsonValue> Encounter = Root->TryGetField(TEXT("encounter_id"));
			double ControlRevision = -1.0;
			if (!MatchesString(Root, TEXT("protocol_version"), TEXT("0.4")) ||
				!MatchesString(Root, TEXT("request_id"), State->RequestId) ||
				!MatchesString(Root, TEXT("service_instance_id"), State->Alice.ServiceInstanceId) ||
				!Root->TryGetObjectField(TEXT("scope"), Scope) ||
				!MatchesString(*Scope, TEXT("game_id"), State->Alice.GameId) ||
				!MatchesString(*Scope, TEXT("player_id"), State->Alice.PlayerId) ||
				!MatchesString(*Scope, TEXT("save_id"), State->Alice.SaveId) ||
				!Run.IsValid() || Run->Type != EJson::Null ||
				!RunBinding.IsValid() || RunBinding->Type != EJson::Null ||
				!Encounter.IsValid() || Encounter->Type != EJson::Null ||
				!Root->TryGetNumberField(TEXT("control_revision"), ControlRevision) ||
				ControlRevision != static_cast<double>(State->Alice.ControlRevision + 1))
			{
				State->Fail(TEXT("Run end did not confirm the expected owner, request, instance, revision, and cleared run."));
				return;
			}
			State->bRunBound = false;
			State->Test->AddInfo(State->bHadFailure
				? TEXT("Scoped loopback test run was closed after an earlier test failure.")
				: TEXT("Scoped loopback contract passed: Alice chat/follow/retry/wait, Bruno routing, and run cleanup. This does not verify gameplay movement or GAS."));
			State->Step = EContractStep::Done;
			return;
		}
		default:
		{
			const CompanionScoped::FBinding& Binding = State->Step >= EContractStep::BrunoWrongWake ? State->Bruno : State->Alice;
			FCompanionChatReply Reply;
			FCompanionAgentDirective Directive;
			bool bHasDirective = false;
			if (!CompanionScoped::ValidateAgentResponse(Root, Binding, State->RequestId, State->SnapshotId,
				State->FirstSentAt, FPlatformTime::Seconds(), Reply, Directive, bHasDirective, Error))
			{
				State->Fail(Error);
				return;
			}
			if (State->Step == EContractStep::BrunoWrongWake || State->Step == EContractStep::AliceChat)
			{
				if (bHasDirective || Reply.ReplyText.IsEmpty())
				{
					State->Fail(TEXT("Ordinary dialogue unexpectedly executed an action or returned no reply."));
					return;
				}
			}
			else
			{
				const FString ExpectedAction = State->Step == EContractStep::AliceWait ? TEXT("wait") : TEXT("follow");
				if (!bHasDirective || Directive.AgentId != Binding.CompanionId || Directive.ActionType != ExpectedAction ||
					(Directive.ActionType == TEXT("follow") && Directive.TargetId != TEXT("party.player")))
				{
					State->Fail(TEXT("Expected a role-matched exploration directive referencing the snapshot player."));
					return;
				}
				if (State->Step == EContractStep::AliceFollow)
				{
					State->FollowDirectiveId = Directive.DirectiveId;
				}
				else if (State->Step == EContractStep::AliceRepeat && Directive.DirectiveId != State->FollowDirectiveId)
				{
					State->Fail(TEXT("Same logical request returned a different directive identity."));
					return;
				}
			}
			break;
		}
		}
		State->Step = static_cast<EContractStep>(static_cast<uint8>(State->Step) + 1);
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCompanionScopedHttpContractTest,
	"Aesir.Companion.HttpContract.ExplorationRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompanionScopedHttpContractTest::RunTest(const FString& Parameters)
{
	FString BaseUrl = FPlatformMisc::GetEnvironmentVariable(TEXT("AESIR_NPC_CONTRACT_URL")).TrimStartAndEnd();
	if (BaseUrl.IsEmpty())
	{
		AddInfo(TEXT("Skipped loopback HTTP contract: AESIR_NPC_CONTRACT_URL is unset. Enable only with a disposable rule/mock development service and the dedicated contract save."));
		return true;
	}
	BaseUrl.RemoveFromEnd(TEXT("/"));
	if (!IsLoopbackUrl(BaseUrl))
	{
		AddError(TEXT("AESIR_NPC_CONTRACT_URL must be a plain HTTP loopback origin with an explicit port and no credentials, path, or query."));
		return false;
	}
	const TSharedRef<FContractState> State = MakeShared<FContractState>();
	State->Test = this;
	State->BaseUrl = BaseUrl;
	ADD_LATENT_AUTOMATION_COMMAND(FCompanionHttpContractCommand(State));
	return true;
}

#endif
