#include "Services/Companion/CommandServiceSubsystem.h"

#include "AesirWarden.h"
#include "Dom/JsonObject.h"
#include "Http.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	void AppendUtf8(TArray<uint8>& Destination, const FString& Text)
	{
		const FTCHARToUTF8 Utf8(*Text);

		Destination.Append(
			reinterpret_cast<const uint8*>(Utf8.Get()),
			Utf8.Length());
	}
}

void UCommandServiceSubsystem::CheckHealth()
{
	const TSharedRef<IHttpRequest> Request =
		FHttpModule::Get().CreateRequest();

	Request->SetURL(ServiceBaseUrl + TEXT("/health"));
	Request->SetVerb(TEXT("GET"));
	Request->SetTimeout(3.0f);
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));

	Request->OnProcessRequestComplete().BindUObject(
		this,
		&UCommandServiceSubsystem::HandleHealthResponse);

	if (!Request->ProcessRequest())
	{
		const FString Message = TEXT("无法启动 AI 服务健康检查。");
		UE_LOG(LogAesirWarden, Warning, TEXT("%s"), *Message);
		OnHealthChecked.Broadcast(false, Message);
	}
}

void UCommandServiceSubsystem::SendRawCommandForTest(const FString& JsonBody)
{
	const TSharedRef<IHttpRequest> Request =
		FHttpModule::Get().CreateRequest();

	Request->SetURL(ServiceBaseUrl + TEXT("/v1/commands/parse"));
	Request->SetVerb(TEXT("POST"));
	Request->SetTimeout(3.0f);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonBody);

	const FString TestRequestId = TEXT("invalid-json-test");

	Request->OnProcessRequestComplete().BindUObject(
		this,
		&UCommandServiceSubsystem::HandleParseCommandResponse,
		TestRequestId);

	if (!Request->ProcessRequest())
	{
		UE_LOG(LogAesirWarden, Warning,
			TEXT("无法启动非法 JSON 测试请求。"));
	}
}

void UCommandServiceSubsystem::ParseVoiceCommand(
	const TArray<uint8>& WavData)
{
	if (WavData.Num() <= 44)
	{
		const FString Error = TEXT("录音数据为空或 WAV 不完整。");
		UE_LOG(LogAesirWarden, Warning, TEXT("%s"), *Error);
		OnCommandParsed.Broadcast(
			false, false, Error, FTacticalOrder{});
		return;
	}

	const FString RequestId =
		FGuid::NewGuid().ToString(
			EGuidFormats::DigitsWithHyphensLower);

	const FString Boundary =
		TEXT("----AesirBoundary") +
		FGuid::NewGuid().ToString(EGuidFormats::Digits);

	TArray<uint8> Body;

	AppendUtf8(
		Body,
		FString::Printf(
			TEXT("--%s\r\n")
			TEXT("Content-Disposition: form-data; ")
			TEXT("name=\"request_id\"\r\n\r\n")
			TEXT("%s\r\n"),
			*Boundary,
			*RequestId));

	AppendUtf8(
		Body,
		FString::Printf(
			TEXT("--%s\r\n")
			TEXT("Content-Disposition: form-data; ")
			TEXT("name=\"file\"; filename=\"voice.wav\"\r\n")
			TEXT("Content-Type: audio/wav\r\n\r\n"),
			*Boundary));

	Body.Append(WavData);

	AppendUtf8(
		Body,
		FString::Printf(
			TEXT("\r\n--%s--\r\n"),
			*Boundary));

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
		FHttpModule::Get().CreateRequest();

	Request->SetURL(
		ServiceBaseUrl + TEXT("/v1/voice/command"));
	Request->SetVerb(TEXT("POST"));
	Request->SetTimeout(3.0f);
	Request->SetHeader(
		TEXT("Content-Type"),
		FString::Printf(
			TEXT("multipart/form-data; boundary=%s"),
			*Boundary));
	Request->SetContent(Body);

	Request->OnProcessRequestComplete().BindUObject(
		this,
		&UCommandServiceSubsystem::HandleParseCommandResponse,
		RequestId);

	if (!Request->ProcessRequest())
	{
		const FString Error =
			TEXT("无法启动语音命令上传请求。");

		UE_LOG(
			LogAesirWarden,
			Warning,
			TEXT("%s"),
			*Error);

		OnCommandParsed.Broadcast(
			false, false, Error, FTacticalOrder{});
	}
}

void UCommandServiceSubsystem::ResolveTacticalIntentForTest(const FString& IntentId, const FString& TargetId,
	const FString& Timing, const FString& NormalizedText, const FCombatContext& CombatContext)
{
	const FString RequestId =
		FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);

	auto AbilityStateToString = [](EAbilityAvailability State)
	{
		switch (State)
		{
		case EAbilityAvailability::Ready:
			return FString(TEXT("ready"));
		case EAbilityAvailability::Cooldown:
			return FString(TEXT("cooldown"));
		case EAbilityAvailability::Unavailable:
			return FString(TEXT("unavailable"));
		case EAbilityAvailability::Blocked:
			return FString(TEXT("blocked"));
		default:
			return FString(TEXT("unavailable"));
		}
	};

	const TSharedRef<FJsonObject> Player = MakeShared<FJsonObject>();
	Player->SetStringField(TEXT("id"), CombatContext.Player.Id);
	Player->SetNumberField(TEXT("hp_percent"), CombatContext.Player.HpPercent);
	Player->SetBoolField(TEXT("is_downed"), CombatContext.Player.bIsDowned);
	Player->SetNumberField(
		TEXT("distance_to_boss_m"),
		CombatContext.Player.DistanceToBossMeters);

	const TSharedRef<FJsonObject> AbilityStates = MakeShared<FJsonObject>();

	for (const TPair<FString, EAbilityAvailability>& Pair :
		CombatContext.Companion.AbilityStates)
	{
		AbilityStates->SetStringField(
			Pair.Key,
			AbilityStateToString(Pair.Value));
	}

	const TSharedRef<FJsonObject> Companion = MakeShared<FJsonObject>();
	Companion->SetStringField(TEXT("id"), CombatContext.Companion.Id);
	Companion->SetNumberField(
		TEXT("hp_percent"),
		CombatContext.Companion.HpPercent);
	Companion->SetNumberField(
		TEXT("mp_percent"),
		CombatContext.Companion.MpPercent);
	Companion->SetStringField(
		TEXT("current_behavior"),
		CombatContext.Companion.CurrentBehavior);
	Companion->SetObjectField(TEXT("ability_states"), AbilityStates);

	TArray<TSharedPtr<FJsonValue>> StateTags;
	for (const FString& Tag : CombatContext.Boss.StateTags)
	{
		StateTags.Add(MakeShared<FJsonValueString>(Tag));
	}

	const TSharedRef<FJsonObject> Boss = MakeShared<FJsonObject>();
	Boss->SetStringField(TEXT("id"), CombatContext.Boss.Id);
	Boss->SetNumberField(TEXT("hp_percent"), CombatContext.Boss.HpPercent);
	Boss->SetNumberField(TEXT("stun_percent"), CombatContext.Boss.StunPercent);
	Boss->SetArrayField(TEXT("state_tags"), StateTags);
	Boss->SetNumberField(TEXT("phase"), CombatContext.Boss.Phase);
	Boss->SetBoolField(TEXT("is_enraged"), CombatContext.Boss.bIsEnraged);

	if (CombatContext.Boss.bHasStunnedRemainingSeconds)
	{
		Boss->SetNumberField(
			TEXT("stunned_remaining_seconds"),
			CombatContext.Boss.StunnedRemainingSeconds);
	}
	else
	{
		Boss->SetField(
			TEXT("stunned_remaining_seconds"),
			MakeShared<FJsonValueNull>());
	}

	const TSharedRef<FJsonObject> Context = MakeShared<FJsonObject>();
	Context->SetStringField(TEXT("encounter_id"), CombatContext.EncounterId);
	Context->SetStringField(TEXT("snapshot_id"), CombatContext.SnapshotId);
	Context->SetStringField(TEXT("captured_at"), CombatContext.CapturedAt);
	Context->SetStringField(TEXT("mode"), TEXT("combat"));
	Context->SetObjectField(TEXT("player"), Player);
	Context->SetObjectField(TEXT("companion"), Companion);
	Context->SetObjectField(TEXT("boss"), Boss);

	const TSharedRef<FJsonObject> Preferences = MakeShared<FJsonObject>();
	Preferences->SetStringField(TEXT("strength"), TEXT("unspecified"));
	Preferences->SetStringField(
		TEXT("resource_conservation"),
		TEXT("normal"));

	const TSharedRef<FJsonObject> Intent = MakeShared<FJsonObject>();
	Intent->SetStringField(TEXT("intent_id"), IntentId);
	Intent->SetStringField(TEXT("target_id"), TargetId);
	Intent->SetStringField(TEXT("timing"), Timing);
	Intent->SetStringField(TEXT("normalized_text"), NormalizedText);
	Intent->SetObjectField(TEXT("preferences"), Preferences);
	Intent->SetNumberField(TEXT("parse_confidence"), 1.0);

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("protocol_version"), TEXT("0.2"));
	Root->SetStringField(TEXT("request_id"), RequestId);
	Root->SetObjectField(TEXT("intent"), Intent);
	Root->SetObjectField(TEXT("combat_context"), Context);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer =
		TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Root, Writer);

	const TSharedRef<IHttpRequest> Request =
		FHttpModule::Get().CreateRequest();

	Request->SetURL(ServiceBaseUrl + TEXT("/v1/tactical/resolve"));
	Request->SetVerb(TEXT("POST"));
	Request->SetTimeout(5.0f);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Body);

	Request->OnProcessRequestComplete().BindUObject(
		this,
		&UCommandServiceSubsystem::HandleResolveTacticalResponse,
		RequestId);

	if (!Request->ProcessRequest())
	{
		OnTacticalResolved.Broadcast(
			false,
			TEXT("无法启动战术决策请求。"));
	}
}

void UCommandServiceSubsystem::HandleHealthResponse(
	FHttpRequestPtr,
	FHttpResponsePtr Response,
	bool bSucceeded)
{
	auto Fail = [this](const FString& Message)
	{
		UE_LOG(LogAesirWarden, Warning, TEXT("%s"), *Message);
		OnHealthChecked.Broadcast(false, Message);
	};

	if (!bSucceeded || !Response.IsValid())
	{
		Fail(TEXT("无法连接本地 AI 服务，游戏继续使用本地 AI。"));
		return;
	}

	if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		Fail(FString::Printf(
			TEXT("AI 服务健康检查失败，HTTP %d。"),
			Response->GetResponseCode()));
		return;
	}

	TSharedPtr<FJsonObject> Json;
	const TSharedRef<TJsonReader<>> Reader =
		TJsonReaderFactory<>::Create(Response->GetContentAsString());

	if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
	{
		Fail(TEXT("AI 服务健康响应不是有效 JSON。"));
		return;
	}

	FString Status;
	FString Service;
	FString ProtocolVersion;

	const bool bHasFields =
		Json->TryGetStringField(TEXT("status"), Status) &&
		Json->TryGetStringField(TEXT("service"), Service) &&
		Json->TryGetStringField(TEXT("protocol_version"), ProtocolVersion);

	if (!bHasFields)
	{
		Fail(TEXT("AI 服务健康响应缺少必要字段。"));
		return;
	}

	if (Status != TEXT("ok") ||
		Service != TEXT("aesir-ai-service") ||
		ProtocolVersion != TEXT("0.1"))
	{
		Fail(FString::Printf(
			TEXT("AI 服务契约不兼容：status=%s service=%s protocol=%s"),
			*Status, *Service, *ProtocolVersion));
		return;
	}

	const FString Message = FString::Printf(
		TEXT("AI 服务连接成功：%s，协议 %s"),
		*Service,
		*ProtocolVersion);

	UE_LOG(LogAesirWarden, Display, TEXT("%s"), *Message);
	OnHealthChecked.Broadcast(true, Message);
}

void UCommandServiceSubsystem::ParseCommand(const FString& Text)
{
	const FString RequestId =
		FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);

	const TSharedRef<FJsonObject> Agent = MakeShared<FJsonObject>();
	Agent->SetStringField(TEXT("id"), TEXT("companion.alice"));
	Agent->SetArrayField(TEXT("ability_ids"), {
		MakeShared<FJsonValueString>(TEXT("ability.alice.explosion")),
		MakeShared<FJsonValueString>(TEXT("ability.alice.basic_attack"))
	});

	const TSharedRef<FJsonObject> Context = MakeShared<FJsonObject>();
	Context->SetStringField(TEXT("catalog_revision"), TEXT("dev-001"));
	Context->SetStringField(TEXT("locale"), TEXT("zh-CN"));
	Context->SetArrayField(TEXT("agents"), {
		MakeShared<FJsonValueObject>(Agent)
	});
	Context->SetArrayField(TEXT("target_selectors"), {
		MakeShared<FJsonValueString>(TEXT("encounter.primary_hostile")),
		MakeShared<FJsonValueString>(TEXT("party.player"))
	});
	Context->SetArrayField(TEXT("state_tags"), {
		MakeShared<FJsonValueString>(TEXT("state.stunned")),
		MakeShared<FJsonValueString>(TEXT("state.phase_two"))
	});

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("protocol_version"), TEXT("0.1"));
	Root->SetStringField(TEXT("request_id"), RequestId);
	Root->SetStringField(TEXT("text"), Text);
	Root->SetObjectField(TEXT("context"), Context);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer =
		TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Root, Writer);

	const TSharedRef<IHttpRequest> Request =
		FHttpModule::Get().CreateRequest();

	Request->SetURL(ServiceBaseUrl + TEXT("/v1/commands/parse"));
	Request->SetVerb(TEXT("POST"));
	Request->SetTimeout(3.0f);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Body);

	Request->OnProcessRequestComplete().BindUObject(
		this,
		&UCommandServiceSubsystem::HandleParseCommandResponse,
		RequestId);

	if (!Request->ProcessRequest())
	{
		UE_LOG(LogAesirWarden, Warning,
			TEXT("无法启动命令请求。"));
	}
}

void UCommandServiceSubsystem::HandleParseCommandResponse(
	FHttpRequestPtr,
	FHttpResponsePtr Response,
	bool bSucceeded,
	FString ExpectedRequestId)
{
	FTacticalOrder Result;

	auto Fail = [this, &Result](const FString& Error)
	{
		UE_LOG(LogAesirWarden, Warning, TEXT("%s"), *Error);
		OnCommandParsed.Broadcast(false, false, Error, Result);
	};

	if (!bSucceeded || !Response.IsValid())
	{
		Fail(TEXT("命令服务没有响应。"));
		return;
	}

	if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		const FString ErrorBody =
			Response->GetContentAsString().Left(512);

		Fail(FString::Printf(
			TEXT("命令服务 HTTP %d：%s"),
			Response->GetResponseCode(),
			*ErrorBody));
		return;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader =
		TJsonReaderFactory<>::Create(Response->GetContentAsString());

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		Fail(TEXT("响应不是有效 JSON。"));
		return;
	}

	FString Protocol;
	FString ResponseRequestId;
	FString Message;
	FString Source;
	bool bRecognized = false;

	if (!Root->TryGetStringField(TEXT("protocol_version"), Protocol) ||
		!Root->TryGetStringField(TEXT("request_id"), ResponseRequestId) ||
		!Root->TryGetBoolField(TEXT("recognized"), bRecognized) ||
		!Root->TryGetStringField(TEXT("message"), Message) ||
		!Root->TryGetStringField(TEXT("source"), Source))
	{
		Fail(TEXT("响应缺少外层必要字段。"));
		return;
	}

	if (Protocol != TEXT("0.1") || ResponseRequestId != ExpectedRequestId)
	{
		Fail(TEXT("协议版本或 request_id 不匹配。"));
		return;
	}

	if (Source != TEXT("rule") &&
		Source != TEXT("llm") &&
		Source != TEXT("rule_fallback"))
	{
		Fail(FString::Printf(
			TEXT("响应包含不支持的 source：%s"),
			*Source));
		return;
	}

	if (!bRecognized)
	{
		OnCommandParsed.Broadcast(true, false, Message, Result);
		return;
	}

	const TSharedPtr<FJsonObject>* Order;
	if (!Root->TryGetObjectField(TEXT("order"), Order) || !Order->IsValid())
	{
		Fail(TEXT("recognized=true，但没有合法 order。"));
		return;
	}

	double Priority = 0.0;
	FString Intent;

	if (!(*Order)->TryGetStringField(TEXT("order_id"), Result.OrderId) ||
		!(*Order)->TryGetStringField(TEXT("agent_id"), Result.AgentId) ||
		!(*Order)->TryGetStringField(TEXT("intent"), Intent) ||
		!(*Order)->TryGetNumberField(TEXT("priority"), Priority))
	{
		Fail(TEXT("order 缺少必要字段。"));
		return;
	}

	Result.Priority = static_cast<int32>(Priority);

	if (Result.Priority < 0 || Result.Priority > 100)
	{
		Fail(TEXT("priority 超出 0 到 100。"));
		return;
	}

	if (Intent == TEXT("conditional_cast"))
	{
		Result.Intent = ETacticalOrderIntent::ConditionalCast;

		const TSharedPtr<FJsonObject>* WhenObject;
		if (!(*Order)->TryGetObjectField(TEXT("when"), WhenObject) ||
			!WhenObject->IsValid())
		{
			Fail(TEXT("conditional_cast 缺少 when。"));
			return;
		}

		FString WhenType;
		if (!(*WhenObject)->TryGetStringField(TEXT("type"), WhenType) ||
			WhenType != TEXT("state_entered") ||
			!(*WhenObject)->TryGetStringField(
				TEXT("subject"), Result.When.Subject) ||
			!(*WhenObject)->TryGetStringField(
				TEXT("tag"), Result.When.Tag))
		{
			Fail(TEXT("when 不符合 state_entered 契约。"));
			return;
		}

		Result.bHasWhen = true;
		Result.When.Type = ETacticalWhenType::StateEntered;

		const TSharedPtr<FJsonObject>* ThenObject;
		FString ThenType;

		if (!(*Order)->TryGetObjectField(TEXT("then"), ThenObject) ||
			!ThenObject->IsValid() ||
			!(*ThenObject)->TryGetStringField(TEXT("type"), ThenType) ||
			ThenType != TEXT("cast_ability") ||
			!(*ThenObject)->TryGetStringField(
				TEXT("ability_id"), Result.Then.AbilityId))
		{
			Fail(TEXT("then 不符合 cast_ability 契约。"));
			return;
		}

		Result.Then.Type = ETacticalThenType::CastAbility;

		if ((*ThenObject)->TryGetStringField(
			TEXT("target"), Result.Then.Target.Selector))
		{
			Result.Then.Target.Type = ETacticalTargetType::Selector;
		}
		else
		{
			const TSharedPtr<FJsonObject>* TargetObject;
			if (!(*ThenObject)->TryGetObjectField(TEXT("target"), TargetObject) ||
				!TargetObject->IsValid() ||
				!(*TargetObject)->TryGetStringField(
					TEXT("ref"), Result.Then.Target.Reference) ||
				Result.Then.Target.Reference != TEXT("when.subject"))
			{
				Fail(TEXT("cast_ability.target 不合法。"));
				return;
			}

			Result.Then.Target.Type = ETacticalTargetType::Reference;
		}
	}
	else
	{
		const TSharedPtr<FJsonValue> WhenValue =
			(*Order)->TryGetField(TEXT("when"));
		if (!WhenValue.IsValid() || WhenValue->Type != EJson::Null)
		{
			Fail(TEXT("该 intent 的 when 必须为 null。"));
			return;
		}

		const TSharedPtr<FJsonObject>* ThenObject;
		FString ThenType;
		if (!(*Order)->TryGetObjectField(TEXT("then"), ThenObject) ||
			!ThenObject->IsValid() ||
			!(*ThenObject)->TryGetStringField(TEXT("type"), ThenType))
		{
			Fail(TEXT("then 缺失或没有 type。"));
			return;
		}

		if (Intent == TEXT("hold_ability") &&
			ThenType == TEXT("hold_ability"))
		{
			Result.Intent = ETacticalOrderIntent::HoldAbility;
			Result.Then.Type = ETacticalThenType::HoldAbility;
			if (!(*ThenObject)->TryGetStringField(
					TEXT("ability_id"), Result.Then.AbilityId) ||
				!(*ThenObject)->TryGetBoolField(
					TEXT("active"), Result.Then.bActive))
			{
				Fail(TEXT("hold_ability 字段不完整。"));
				return;
			}
		}
		else if (Intent == TEXT("prioritize_attack") &&
			ThenType == TEXT("set_priority"))
		{
			Result.Intent = ETacticalOrderIntent::PrioritizeAttack;
			Result.Then.Type = ETacticalThenType::SetPriority;
			if (!(*ThenObject)->TryGetStringField(
					TEXT("mode"), Result.Then.Mode) ||
				(Result.Then.Mode != TEXT("basic_attack_first") &&
					Result.Then.Mode != TEXT("ability_first")))
			{
				Fail(TEXT("set_priority.mode 不合法。"));
				return;
			}
		}
		else if (Intent == TEXT("follow_keep_distance") &&
			ThenType == TEXT("follow"))
		{
			Result.Intent = ETacticalOrderIntent::FollowKeepDistance;
			Result.Then.Type = ETacticalThenType::Follow;
			Result.Then.Target.Type = ETacticalTargetType::Selector;
			if (!(*ThenObject)->TryGetStringField(
					TEXT("target"), Result.Then.Target.Selector) ||
				!(*ThenObject)->TryGetBoolField(
					TEXT("keep_distance"), Result.Then.bKeepDistance))
			{
				Fail(TEXT("follow 字段不完整。"));
				return;
			}
		}
		else if (Intent == TEXT("retreat") && ThenType == TEXT("retreat"))
		{
			Result.Intent = ETacticalOrderIntent::Retreat;
			Result.Then.Type = ETacticalThenType::Retreat;
		}
		else
		{
			Fail(TEXT("intent 与 then.type 不匹配。"));
			return;
		}
	}

	const TSharedPtr<FJsonObject>* ExpiresObject;
	FString ExpiresType;

	if (!(*Order)->TryGetObjectField(TEXT("expires"), ExpiresObject) ||
		!ExpiresObject->IsValid() ||
		!(*ExpiresObject)->TryGetStringField(TEXT("type"), ExpiresType) ||
		ExpiresType != TEXT("encounter_end"))
	{
		Fail(TEXT("expires 不符合 encounter_end 契约。"));
		return;
	}

	Result.Expires.Type = ETacticalExpiresType::EncounterEnd;

	UE_LOG(LogAesirWarden, Display,
		TEXT("已解析 order=%s intent=%s priority=%d source=%s"),
		*Result.OrderId,
		*Intent,
		Result.Priority,
		*Source);

	OnCommandParsed.Broadcast(true, true, Message, Result);
}

void UCommandServiceSubsystem::HandleResolveTacticalResponse(
	FHttpRequestPtr,
	FHttpResponsePtr Response,
	bool bSucceeded,
	FString ExpectedRequestId)
{
	if (!bSucceeded || !Response.IsValid())
	{
		OnTacticalResolved.Broadcast(false, TEXT("战术服务没有响应。"));
		return;
	}

	const FString Body = Response->GetContentAsString();

	if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		OnTacticalResolved.Broadcast(
			false,
			FString::Printf(
				TEXT("战术服务 HTTP %d：%s"),
				Response->GetResponseCode(),
				*Body.Left(1024)));
		return;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader =
		TJsonReaderFactory<>::Create(Body);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OnTacticalResolved.Broadcast(false, TEXT("战术响应不是有效 JSON。"));
		return;
	}

	FString ProtocolVersion;
	FString ResponseRequestId;

	if (!Root->TryGetStringField(
			TEXT("protocol_version"), ProtocolVersion) ||
		!Root->TryGetStringField(
			TEXT("request_id"), ResponseRequestId))
	{
		OnTacticalResolved.Broadcast(false, TEXT("战术响应缺少必要字段。"));
		return;
	}

	if (ProtocolVersion != TEXT("0.2") ||
		ResponseRequestId != ExpectedRequestId)
	{
		OnTacticalResolved.Broadcast(
			false,
			TEXT("战术响应的协议版本或 request_id 不匹配。"));
		return;
	}

	UE_LOG(
		LogAesirWarden,
		Display,
		TEXT("战术决策响应：%s"),
		*Body);

	OnTacticalResolved.Broadcast(true, Body);
}
