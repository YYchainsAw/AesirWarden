#include "Services/Companion/CompanionScopedProtocol.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Services/Companion/CompanionChatSubsystem.h"

#include <initializer_list>

namespace CompanionScoped
{
namespace
{
constexpr double MaxExactJsonInteger = 9007199254740991.0;

bool Fail(FString& Error, const TCHAR* Message)
{
	Error = Message;
	return false;
}

bool String(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, FString& Out)
{
	const TSharedPtr<FJsonValue> Value = Object.IsValid() ? Object->TryGetField(Key) : nullptr;
	return Value.IsValid() && Value->Type == EJson::String && Value->TryGetString(Out);
}

bool Object(const TSharedPtr<FJsonObject>& Parent, const TCHAR* Key, TSharedPtr<FJsonObject>& Out)
{
	const TSharedPtr<FJsonObject>* Value = nullptr;
	if (!Parent.IsValid() || !Parent->TryGetObjectField(Key, Value) || !Value || !Value->IsValid())
		return false;
	Out = *Value;
	return true;
}

bool Guid(const FString& Value)
{
	FGuid Parsed;
	return FGuid::ParseExact(Value, EGuidFormats::DigitsWithHyphens, Parsed);
}

bool SameGuid(const FString& Actual, const FString& Expected)
{
	FGuid ParsedActual;
	FGuid ParsedExpected;
	return FGuid::ParseExact(Actual, EGuidFormats::DigitsWithHyphens, ParsedActual) &&
		FGuid::ParseExact(Expected, EGuidFormats::DigitsWithHyphens, ParsedExpected) &&
		ParsedActual == ParsedExpected;
}

bool GuidField(const TSharedPtr<FJsonObject>& Root, const TCHAR* Key, FString& Out)
{
	return String(Root, Key, Out) && Guid(Out);
}

bool MatchGuid(const TSharedPtr<FJsonObject>& Root, const TCHAR* Key, const FString& Expected)
{
	FString Actual;
	return String(Root, Key, Actual) && SameGuid(Actual, Expected);
}

bool MatchString(const TSharedPtr<FJsonObject>& Root, const TCHAR* Key, const FString& Expected)
{
	FString Actual;
	return String(Root, Key, Actual) && Actual == Expected;
}

bool Null(const TSharedPtr<FJsonObject>& Root, const TCHAR* Key)
{
	const TSharedPtr<FJsonValue> Value = Root.IsValid() ? Root->TryGetField(Key) : nullptr;
	return Value.IsValid() && Value->Type == EJson::Null;
}

bool NullableGuid(const TSharedPtr<FJsonObject>& Root, const TCHAR* Key, FString& Out)
{
	if (Null(Root, Key))
	{
		Out.Reset();
		return true;
	}
	return GuidField(Root, Key, Out);
}

bool Integer(const TSharedPtr<FJsonObject>& Root, const TCHAR* Key, int64& Out)
{
	const TSharedPtr<FJsonValue> Value = Root.IsValid() ? Root->TryGetField(Key) : nullptr;
	double Number = -1.0;
	if (!Value.IsValid() || Value->Type != EJson::Number || !Value->TryGetNumber(Number) ||
		!FMath::IsFinite(Number) || Number < 0.0 || Number > MaxExactJsonInteger ||
		FMath::FloorToDouble(Number) != Number)
		return false;
	Out = static_cast<int64>(Number);
	return true;
}

bool StringArray(const TSharedPtr<FJsonObject>& Root, const TCHAR* Key)
{
	const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
	if (!Root->TryGetArrayField(Key, Values) || !Values)
		return false;
	for (const TSharedPtr<FJsonValue>& Value : *Values)
	{
		if (!Value.IsValid() || Value->Type != EJson::String)
			return false;
	}
	return true;
}

bool OnlyKeys(const TSharedPtr<FJsonObject>& Root, std::initializer_list<const TCHAR*> Allowed)
{
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
	{
		bool bKnown = false;
		for (const TCHAR* Key : Allowed)
			bKnown |= Pair.Key == Key;
		if (!bKnown)
			return false;
	}
	return true;
}

bool ControlState(const TSharedPtr<FJsonObject>& Root, FBinding& Out)
{
	return MatchString(Root, TEXT("protocol_version"), TEXT("0.4")) &&
		GuidField(Root, TEXT("service_instance_id"), Out.ServiceInstanceId) &&
		Integer(Root, TEXT("control_revision"), Out.ControlRevision) &&
		Integer(Root, TEXT("encounter_revision"), Out.EncounterRevision) &&
		NullableGuid(Root, TEXT("run_id"), Out.RunId) &&
		NullableGuid(Root, TEXT("run_binding_id"), Out.RunBindingId) &&
		NullableGuid(Root, TEXT("encounter_id"), Out.EncounterId) &&
		(Out.RunId.IsEmpty() == Out.RunBindingId.IsEmpty()) &&
		(Out.EncounterId.IsEmpty() || !Out.RunId.IsEmpty());
}

bool ParsePresentation(const TSharedPtr<FJsonObject>& Directive, FCompanionChatReply& Reply)
{
	if (Null(Directive, TEXT("presentation")))
		return true;
	TSharedPtr<FJsonObject> Presentation;
	if (!Object(Directive, TEXT("presentation"), Presentation) ||
		!OnlyKeys(Presentation, {TEXT("reply_text"), TEXT("emotion_id"), TEXT("gesture_id"),
			TEXT("facial_expression_id"), TEXT("interruptible"), TEXT("gaze_target_id")}) ||
		!String(Presentation, TEXT("reply_text"), Reply.ReplyText) ||
		Reply.ReplyText.IsEmpty() || Reply.ReplyText.Len() > 500 ||
		!String(Presentation, TEXT("emotion_id"), Reply.EmotionId) ||
		!String(Presentation, TEXT("gesture_id"), Reply.GestureId) ||
		!String(Presentation, TEXT("facial_expression_id"), Reply.FacialExpressionId) ||
		!Presentation->TryGetBoolField(TEXT("interruptible"), Reply.bInterruptible))
		return false;
	if (!Null(Presentation, TEXT("gaze_target_id")))
	{
		FString Target;
		if (!String(Presentation, TEXT("gaze_target_id"), Target) || Target != TEXT("party.player"))
			return false;
	}
	return true;
}
}

bool ParseContext(
	const TSharedPtr<FJsonObject>& Root, const FString& ExpectedSaveId,
	const FString& ExpectedCompanionId, const FString& ExpectedGameId,
	FBinding& OutBinding, FString& Error)
{
	Error.Reset();
	FBinding Parsed;
	TSharedPtr<FJsonObject> Scope;
	if (!Root.IsValid() ||
		!OnlyKeys(Root, {TEXT("protocol_version"), TEXT("scope"), TEXT("service_instance_id"),
			TEXT("control_revision"), TEXT("run_id"), TEXT("run_binding_id"),
			TEXT("encounter_revision"), TEXT("encounter_id"), TEXT("memory_epoch")}) ||
		!ControlState(Root, Parsed) || !Object(Root, TEXT("scope"), Scope) ||
		!OnlyKeys(Scope, {TEXT("game_id"), TEXT("player_id"), TEXT("save_id"), TEXT("companion_id")}) ||
		ExpectedGameId.IsEmpty() || !MatchString(Scope, TEXT("game_id"), ExpectedGameId) ||
		ExpectedCompanionId.IsEmpty() || !MatchString(Scope, TEXT("companion_id"), ExpectedCompanionId) ||
		!MatchGuid(Scope, TEXT("save_id"), ExpectedSaveId) ||
		!GuidField(Scope, TEXT("player_id"), Parsed.PlayerId) ||
		!GuidField(Root, TEXT("memory_epoch"), Parsed.MemoryEpoch))
		return Fail(Error, TEXT("队友服务上下文的版本、身份或控制状态无效。"));
	Parsed.GameId = ExpectedGameId;
	Parsed.SaveId = ExpectedSaveId;
	Parsed.CompanionId = ExpectedCompanionId;
	OutBinding = MoveTemp(Parsed);
	return true;
}

bool ParseBoundControl(
	const TSharedPtr<FJsonObject>& Root, const FBinding& PreviousBinding,
	const FString& ExpectedRequestId, const FString& ExpectedRunId,
	FBinding& OutBinding, FString& Error)
{
	Error.Reset();
	FBinding Parsed = PreviousBinding;
	TSharedPtr<FJsonObject> Scope;
	if (!Root.IsValid() ||
		!OnlyKeys(Root, {TEXT("protocol_version"), TEXT("scope"), TEXT("service_instance_id"),
			TEXT("control_revision"), TEXT("run_id"), TEXT("run_binding_id"),
			TEXT("encounter_revision"), TEXT("encounter_id"), TEXT("request_id")}) ||
		!ControlState(Root, Parsed) ||
		!MatchGuid(Root, TEXT("request_id"), ExpectedRequestId) ||
		!SameGuid(Parsed.ServiceInstanceId, PreviousBinding.ServiceInstanceId) ||
		!SameGuid(Parsed.RunId, ExpectedRunId) || Parsed.RunBindingId.IsEmpty() ||
		(!PreviousBinding.RunBindingId.IsEmpty() && SameGuid(Parsed.RunBindingId, PreviousBinding.RunBindingId)) ||
		PreviousBinding.ControlRevision >= static_cast<int64>(MaxExactJsonInteger) ||
		Parsed.ControlRevision != PreviousBinding.ControlRevision + 1 ||
		Parsed.EncounterRevision != 0 || !Parsed.EncounterId.IsEmpty() ||
		!Object(Root, TEXT("scope"), Scope) ||
		!OnlyKeys(Scope, {TEXT("game_id"), TEXT("player_id"), TEXT("save_id")}) ||
		!MatchString(Scope, TEXT("game_id"), PreviousBinding.GameId) ||
		!MatchGuid(Scope, TEXT("player_id"), PreviousBinding.PlayerId) ||
		!MatchGuid(Scope, TEXT("save_id"), PreviousBinding.SaveId))
		return Fail(Error, TEXT("队友服务绑定响应与本次请求或预期控制版本不匹配。"));
	OutBinding = MoveTemp(Parsed);
	return true;
}

bool ValidateAgentResponse(
	const TSharedPtr<FJsonObject>& Root, const FBinding& Binding,
	const FString& RequestId, const FString& SnapshotId,
	double FirstSentAt, double Now,
	FCompanionChatReply& OutReply, FCompanionAgentDirective& OutDirective,
	bool& bOutHasDirective, FString& Error)
{
	bOutHasDirective = false;
	Error.Reset();
	TSharedPtr<FJsonObject> Scope;
	TSharedPtr<FJsonObject> Timing;
	TSharedPtr<FJsonObject> Observability;
	FString ObservabilitySource;
	int64 ValidForMs = 0;
	if (!Root.IsValid() ||
		!OnlyKeys(Root, {TEXT("protocol_version"), TEXT("request_id"), TEXT("scope"),
			TEXT("service_instance_id"), TEXT("run_id"), TEXT("run_binding_id"), TEXT("memory_epoch"),
			TEXT("session_id"), TEXT("encounter_id"), TEXT("used_snapshot_id"), TEXT("timing"),
			TEXT("action"), TEXT("directive"), TEXT("reply_text"), TEXT("observability")}) ||
		!MatchString(Root, TEXT("protocol_version"), TEXT("0.4")) ||
		!MatchGuid(Root, TEXT("request_id"), RequestId) ||
		!MatchGuid(Root, TEXT("service_instance_id"), Binding.ServiceInstanceId) ||
		!MatchGuid(Root, TEXT("run_id"), Binding.RunId) ||
		!MatchGuid(Root, TEXT("run_binding_id"), Binding.RunBindingId) ||
		!MatchGuid(Root, TEXT("memory_epoch"), Binding.MemoryEpoch) ||
		!MatchGuid(Root, TEXT("session_id"), Binding.RunId) ||
		!Binding.EncounterId.IsEmpty() || !Null(Root, TEXT("encounter_id")) ||
		!MatchGuid(Root, TEXT("used_snapshot_id"), SnapshotId) ||
		!Object(Root, TEXT("scope"), Scope) ||
		!OnlyKeys(Scope, {TEXT("game_id"), TEXT("player_id"), TEXT("save_id"), TEXT("companion_id")}) ||
		!MatchString(Scope, TEXT("game_id"), Binding.GameId) ||
		!MatchGuid(Scope, TEXT("player_id"), Binding.PlayerId) ||
		!MatchGuid(Scope, TEXT("save_id"), Binding.SaveId) ||
		!MatchString(Scope, TEXT("companion_id"), Binding.CompanionId) ||
		!Object(Root, TEXT("timing"), Timing) ||
		!OnlyKeys(Timing, {TEXT("valid_for_ms")}) || !Integer(Timing, TEXT("valid_for_ms"), ValidForMs) ||
		!Object(Root, TEXT("observability"), Observability) ||
		!MatchGuid(Observability, TEXT("used_snapshot_id"), SnapshotId) ||
		!String(Observability, TEXT("source"), ObservabilitySource) ||
		(ObservabilitySource != TEXT("rule") && ObservabilitySource != TEXT("llm") &&
			ObservabilitySource != TEXT("rule_fallback")) ||
		!StringArray(Observability, TEXT("reason_codes")))
		return Fail(Error, TEXT("队友 AI 响应版本、作用域、绑定或快照不匹配。"));

	if (!FMath::IsFinite(FirstSentAt) || !FMath::IsFinite(Now) || FirstSentAt <= 0.0 ||
		Now < FirstSentAt || ValidForMs <= 0 || ValidForMs > 120000 ||
		(Now - FirstSentAt) * 1000.0 >= static_cast<double>(ValidForMs))
		return Fail(Error, TEXT("队友 AI 响应已过期或首次发送计时不可用。"));

	FCompanionChatReply Reply;
	Reply.ProtocolVersion = TEXT("0.4");
	Reply.CompanionId = Binding.CompanionId;
	Reply.Source = ObservabilitySource;
	if (!String(Root, TEXT("reply_text"), Reply.ReplyText))
		return Fail(Error, TEXT("队友 AI 回复文本类型无效。"));

	FString Action;
	if (!String(Root, TEXT("action"), Action))
		return Fail(Error, TEXT("队友 AI 响应缺少动作类型。"));
	if (Action == TEXT("none"))
	{
		if (!Null(Root, TEXT("directive")))
			return Fail(Error, TEXT("队友 AI 空动作响应意外包含指令。"));
		OutReply = MoveTemp(Reply);
		OutDirective = FCompanionAgentDirective();
		return true;
	}
	if (Action != TEXT("directive"))
		return Fail(Error, TEXT("队友 AI 响应动作类型未知。"));

	TSharedPtr<FJsonObject> Directive;
	TSharedPtr<FJsonObject> Expires;
	TSharedPtr<FJsonObject> Payload;
	FCompanionAgentDirective Parsed;
	int64 Priority = 0;
	if (!Object(Root, TEXT("directive"), Directive) ||
		!OnlyKeys(Directive, {TEXT("protocol_version"), TEXT("directive_id"), TEXT("agent_id"),
			TEXT("domain"), TEXT("action_type"), TEXT("priority"), TEXT("expires"), TEXT("source"),
			TEXT("reason_codes"), TEXT("policy_revision"), TEXT("payload"), TEXT("presentation")}) ||
		!MatchString(Directive, TEXT("protocol_version"), TEXT("0.4")) ||
		!GuidField(Directive, TEXT("directive_id"), Parsed.DirectiveId) ||
		!String(Directive, TEXT("agent_id"), Parsed.AgentId) || Parsed.AgentId != Binding.CompanionId ||
		!String(Directive, TEXT("domain"), Parsed.Domain) || Parsed.Domain != TEXT("exploration") ||
		!String(Directive, TEXT("action_type"), Parsed.ActionType) ||
		(Parsed.ActionType != TEXT("follow") && Parsed.ActionType != TEXT("wait")) ||
		!String(Directive, TEXT("source"), Parsed.Source) ||
		(Parsed.Source != TEXT("player_command") && Parsed.Source != TEXT("autonomy") &&
			Parsed.Source != TEXT("event") && Parsed.Source != TEXT("fallback")) ||
		!Integer(Directive, TEXT("priority"), Priority) || Priority > 100 ||
		!String(Directive, TEXT("policy_revision"), Parsed.PolicyRevision) ||
		!StringArray(Directive, TEXT("reason_codes")) ||
		!Object(Directive, TEXT("expires"), Expires) ||
		!String(Expires, TEXT("type"), Parsed.ExpiresType) ||
		!Object(Directive, TEXT("payload"), Payload))
		return Fail(Error, TEXT("队友 AI 指令的身份、场景、动作或来源无效。"));

	double ValidUntilSeconds = FirstSentAt + static_cast<double>(ValidForMs) / 1000.0;
	if (Parsed.ExpiresType == TEXT("immediate"))
	{
		if (!OnlyKeys(Expires, {TEXT("type")}))
			return Fail(Error, TEXT("队友 AI 即时指令有效期格式无效。"));
	}
	else if (Parsed.ExpiresType == TEXT("before_seconds"))
	{
		double Seconds = 0.0;
		const TSharedPtr<FJsonValue> Value = Expires->TryGetField(TEXT("remaining_seconds"));
		if (!OnlyKeys(Expires, {TEXT("type"), TEXT("remaining_seconds")}) ||
			!Value.IsValid() || Value->Type != EJson::Number || !Value->TryGetNumber(Seconds) ||
			!FMath::IsFinite(Seconds) || Seconds <= 0.0)
			return Fail(Error, TEXT("队友 AI 指令有效期无效。"));
		// A shorter action lifetime can only tighten the original send deadline.
		// Neither a longer response budget nor a replay may renew this lifetime.
		ValidUntilSeconds = FMath::Min(ValidUntilSeconds, FirstSentAt + Seconds);
	}
	else
		return Fail(Error, TEXT("探索指令不能使用遭遇有效期或未知有效期。"));
	if (Now >= ValidUntilSeconds)
		return Fail(Error, TEXT("队友 AI 指令已过期。"));

	if (Parsed.ActionType == TEXT("follow"))
	{
		if (Payload->Values.Num() != 1 || !MatchString(Payload, TEXT("target_id"), TEXT("party.player")))
			return Fail(Error, TEXT("跟随指令必须且只能引用当前玩家。"));
		Parsed.TargetId = TEXT("party.player");
		Parsed.Payload.Add(TEXT("target_id"), Parsed.TargetId);
	}
	else if (!Payload->Values.IsEmpty())
		return Fail(Error, TEXT("等待指令不能携带目标、坐标或其他执行参数。"));

	if (!ParsePresentation(Directive, Reply))
		return Fail(Error, TEXT("队友 AI 表现文本或注视目标无效。"));
	Parsed.Priority = static_cast<int32>(Priority);
	Parsed.RequestId = RequestId;
	Parsed.RunId = Binding.RunId;
	Parsed.ValidUntilSeconds = ValidUntilSeconds;
	Parsed.RemainingSeconds = static_cast<float>(Parsed.ValidUntilSeconds - Now);
	OutReply = MoveTemp(Reply);
	// The text exploration slice executes player commands only. Other valid
	// service sources may produce display text, but never an executable output.
	bOutHasDirective = Parsed.Source == TEXT("player_command");
	OutDirective = bOutHasDirective ? MoveTemp(Parsed) : FCompanionAgentDirective();
	return true;
}

TSharedRef<FJsonObject> BuildScopedBody(const FBinding& Binding, const FString& RequestId)
{
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("protocol_version"), TEXT("0.4"));
	Body->SetStringField(TEXT("request_id"), RequestId);
	Body->SetStringField(TEXT("save_id"), Binding.SaveId);
	Body->SetStringField(TEXT("companion_id"), Binding.CompanionId);
	Body->SetStringField(TEXT("service_instance_id"), Binding.ServiceInstanceId);
	Body->SetStringField(TEXT("run_id"), Binding.RunId);
	Body->SetStringField(TEXT("run_binding_id"), Binding.RunBindingId);
	Body->SetStringField(TEXT("memory_epoch"), Binding.MemoryEpoch);
	Body->SetStringField(TEXT("session_id"), Binding.RunId);
	Body->SetField(TEXT("encounter_id"), MakeShared<FJsonValueNull>());
	return Body;
}

TSharedRef<FJsonObject> BuildBindBody(
	const FBinding& Binding, const FString& RequestId, const FString& NewRunId)
{
	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("protocol_version"), TEXT("0.4"));
	Body->SetStringField(TEXT("request_id"), RequestId);
	Body->SetStringField(TEXT("save_id"), Binding.SaveId);
	Body->SetStringField(TEXT("service_instance_id"), Binding.ServiceInstanceId);
	Body->SetNumberField(TEXT("expected_control_revision"), static_cast<double>(Binding.ControlRevision));
	if (Binding.RunId.IsEmpty())
		Body->SetField(TEXT("expected_run_id"), MakeShared<FJsonValueNull>());
	else
		Body->SetStringField(TEXT("expected_run_id"), Binding.RunId);
	Body->SetStringField(TEXT("run_id"), NewRunId);
	return Body;
}
}
