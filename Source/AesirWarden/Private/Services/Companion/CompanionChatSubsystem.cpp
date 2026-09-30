#include "Services/Companion/CompanionChatSubsystem.h"

#include "Framework/AesirCombatPrototypePlayerController.h"
#include "Characters/Player/AesirPlayerCharacter.h"
#include "Characters/Companion/AesirCompanionCharacter.h"
#include "AesirWarden.h"
#include "Combat/AesirHealthComponent.h"
#include "AI/Companion/TacticalOrderComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "Http.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	constexpr float HeartbeatIntervalSeconds = 3.0f;
	constexpr TCHAR PlayerId[] = TEXT("party.player");

	float HealthPercent(const UAesirHealthComponent* Health)
	{
		return FMath::Clamp(
			100.0f * Health->GetCurrentHealth() / Health->GetMaxHealth(),
			0.0f,
			100.0f);
	}
}

void UCompanionChatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	HeartbeatTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UCompanionChatSubsystem::TickHeartbeat),
		HeartbeatIntervalSeconds);
}

void UCompanionChatSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(HeartbeatTickerHandle);
	bHeartbeatInFlight = false;
	Super::Deinitialize();
}

bool UCompanionChatSubsystem::TickHeartbeat(float)
{
	if (!bHeartbeatInFlight)
	{
		SendAgentStep(FString(), false);
	}
	return true;
}

void UCompanionChatSubsystem::SendChatMessage(const FString& Text)
{
	const FString TrimmedText = Text.TrimStartAndEnd();
	if (TrimmedText.IsEmpty())
	{
		OnRequestFailed.Broadcast(TEXT("请输入指令。"));
		return;
	}

	if (TrimmedText.Len() > 500)
	{
		OnRequestFailed.Broadcast(TEXT("指令不能超过 500 个字符。"));
		return;
	}

	SendAgentStep(TrimmedText, true);
}

bool UCompanionChatSubsystem::BuildWorldContext(
	TSharedRef<FJsonObject>& OutContext,
	TSet<FString>& OutAllowedTargetIds,
	FString& OutCompanionId,
	FString& OutSnapshotId,
	FString& OutScene,
	bool& bOutEncounterActive) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || !World->IsGameWorld())
	{
		return false;
	}

	AAesirCompanionCharacter* CompanionActor = nullptr;
	for (TActorIterator<AAesirCompanionCharacter> It(World); It; ++It)
	{
		if (IsValid(*It) && !It->GetCompanionId().IsNone())
		{
			CompanionActor = *It;
			break;
		}
	}

	ACharacter* PlayerActor = UGameplayStatics::GetPlayerCharacter(World, 0);
	if (!IsValid(PlayerActor) || !IsValid(CompanionActor))
	{
		return false;
	}

	const UAesirHealthComponent* CompanionHealth =
		CompanionActor->FindComponentByClass<UAesirHealthComponent>();
	if (!IsValid(CompanionHealth) || CompanionHealth->GetMaxHealth() <= 0.0f)
	{
		return false;
	}

	float PlayerCurrentHealth = 0.0f;
	float PlayerMaxHealth = 0.0f;
	if (const AAesirPlayerCharacter* AesirPlayer =
		Cast<AAesirPlayerCharacter>(PlayerActor))
	{
		PlayerCurrentHealth = AesirPlayer->GetGASHealth();
		PlayerMaxHealth = AesirPlayer->GetGASMaxHealth();
	}

	if (PlayerMaxHealth <= 0.0f)
	{
		const UAesirHealthComponent* PlayerHealth =
			PlayerActor->FindComponentByClass<UAesirHealthComponent>();
		if (!IsValid(PlayerHealth) || PlayerHealth->GetMaxHealth() <= 0.0f)
		{
			return false;
		}
		PlayerCurrentHealth = PlayerHealth->GetCurrentHealth();
		PlayerMaxHealth = PlayerHealth->GetMaxHealth();
	}

	OutCompanionId = CompanionActor->GetCompanionId().ToString();
	OutSnapshotId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
	OutAllowedTargetIds.Add(PlayerId);
	OutAllowedTargetIds.Add(OutCompanionId);

	const UTacticalOrderComponent* TacticalOrders =
		CompanionActor->FindComponentByClass<UTacticalOrderComponent>();
	bOutEncounterActive = IsValid(TacticalOrders) &&
		TacticalOrders->IsEncounterActive();
	OutScene = bOutEncounterActive ? TEXT("combat") : TEXT("exploration");

	const TSharedRef<FJsonObject> Player = MakeShared<FJsonObject>();
	Player->SetStringField(TEXT("id"), PlayerId);
	Player->SetNumberField(
		TEXT("hp_percent"),
		FMath::Clamp(100.0f * PlayerCurrentHealth / PlayerMaxHealth, 0.0f, 100.0f));
	Player->SetBoolField(TEXT("is_downed"), PlayerCurrentHealth <= 0.0f);

	const TSharedRef<FJsonObject> Companion = MakeShared<FJsonObject>();
	Companion->SetStringField(TEXT("id"), OutCompanionId);
	Companion->SetNumberField(TEXT("hp_percent"), HealthPercent(CompanionHealth));
	// Alice has no MP AttributeSet yet. Zero means no available MP, never a ready skill.
	Companion->SetNumberField(TEXT("mp_percent"), 0.0);
	Companion->SetStringField(
		TEXT("current_behavior"),
		CompanionActor->GetVelocity().SizeSquared2D() > 25.0f
			? TEXT("moving") : TEXT("idle"));
	Companion->SetBoolField(TEXT("is_casting"), false);

	const AAesirCombatPrototypePlayerController* PlayerController =
		Cast<AAesirCombatPrototypePlayerController>(
			UGameplayStatics::GetPlayerController(World, 0));

	OutContext->SetStringField(TEXT("snapshot_id"), OutSnapshotId);
	OutContext->SetStringField(
		TEXT("captured_at"), FDateTime::UtcNow().ToIso8601());
	OutContext->SetStringField(TEXT("scene"), OutScene);
	OutContext->SetObjectField(TEXT("player"), Player);
	OutContext->SetObjectField(TEXT("companion"), Companion);
	OutContext->SetBoolField(
		TEXT("ui_popup"),
		IsValid(PlayerController) && PlayerController->IsCompanionChatOpen());
	// No interactable observation or complete combat snapshot is wired yet.
	OutContext->SetArrayField(TEXT("interactables"), {});
	return true;
}

void UCompanionChatSubsystem::SendAgentStep(
	const FString& Text,
	bool bPlayerCommand)
{
	TSharedRef<FJsonObject> Context = MakeShared<FJsonObject>();
	TSet<FString> AllowedTargetIds;
	FString CompanionId;
	FString SnapshotId;
	FString Scene;
	bool bEncounterActive = false;
	if (!BuildWorldContext(
		Context,
		AllowedTargetIds,
		CompanionId,
		SnapshotId,
		Scene,
		bEncounterActive))
	{
		if (bPlayerCommand)
		{
			ReportFailure(true, TEXT("无法读取当前玩家或队友状态。"));
		}
		return;
	}

	const FString RequestId =
		FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
	const uint64 RequestSequence = ++NextRequestSequence;
	if (bPlayerCommand)
	{
		LatestPlayerCommandSequence = RequestSequence;
	}

	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("protocol_version"), TEXT("0.3"));
	Body->SetStringField(TEXT("request_id"), RequestId);
	Body->SetStringField(TEXT("companion_id"), CompanionId);
	Body->SetObjectField(TEXT("world_context"), Context);
	if (bPlayerCommand)
	{
		Body->SetStringField(TEXT("text"), Text);
	}

	FString JsonBody;
	if (!FJsonSerializer::Serialize(
		Body,
		TJsonWriterFactory<>::Create(&JsonBody)))
	{
		ReportFailure(bPlayerCommand, TEXT("无法创建队友 AI 请求。"));
		return;
	}

	const TSharedRef<IHttpRequest> Request =
		FHttpModule::Get().CreateRequest();
	Request->SetURL(ServiceBaseUrl + TEXT("/v1/agent/step"));
	Request->SetVerb(TEXT("POST"));
	Request->SetTimeout(bPlayerCommand ? 5.0f : 2.5f);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	Request->SetContentAsString(JsonBody);
	Request->OnProcessRequestComplete().BindUObject(
		this,
		&UCompanionChatSubsystem::HandleAgentStepResponse,
		RequestId,
		SnapshotId,
		CompanionId,
		Scene,
		AllowedTargetIds,
		bPlayerCommand,
		bEncounterActive,
		RequestSequence,
		FPlatformTime::Seconds());

	if (!bPlayerCommand)
	{
		bHeartbeatInFlight = true;
	}
	if (!Request->ProcessRequest())
	{
		if (!bPlayerCommand)
		{
			bHeartbeatInFlight = false;
		}
		ReportFailure(bPlayerCommand, TEXT("无法启动队友 AI 请求。"));
	}
}

void UCompanionChatSubsystem::ReportFailure(
	bool bPlayerCommand,
	const FString& Message)
{
	if (bPlayerCommand)
	{
		OnRequestFailed.Broadcast(Message);
	}
	else if (!bHeartbeatFailureLogged)
	{
		UE_LOG(LogAesirWarden, Warning, TEXT("%s"), *Message);
		bHeartbeatFailureLogged = true;
	}
}

void UCompanionChatSubsystem::HandleAgentStepResponse(
	FHttpRequestPtr,
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
	double SentAtSeconds)
{
	if (!bPlayerCommand)
	{
		bHeartbeatInFlight = false;
	}
	if (RequestSequence < LatestPlayerCommandSequence)
	{
		return;
	}

	auto Fail = [this, bPlayerCommand](const FString& Message)
	{
		ReportFailure(bPlayerCommand, Message);
	};

	if (!bSucceeded || !Response.IsValid())
	{
		Fail(TEXT("队友 AI 服务没有响应。"));
		return;
	}
	if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		Fail(FString::Printf(
			TEXT("队友 AI 服务 HTTP %d：%s"),
			Response->GetResponseCode(),
			*Response->GetContentAsString().Left(256)));
		return;
	}

	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(
		TJsonReaderFactory<>::Create(Response->GetContentAsString()),
		Root) || !Root.IsValid())
	{
		Fail(TEXT("队友 AI 响应不是有效 JSON。"));
		return;
	}

	FString ProtocolVersion;
	FString RequestId;
	FString CompanionId;
	FString Action;
	const TSharedPtr<FJsonObject>* Observability;
	FString UsedSnapshotId;
	if (!Root->TryGetStringField(TEXT("protocol_version"), ProtocolVersion) ||
		ProtocolVersion != TEXT("0.3") ||
		!Root->TryGetStringField(TEXT("request_id"), RequestId) ||
		RequestId != ExpectedRequestId ||
		!Root->TryGetStringField(TEXT("companion_id"), CompanionId) ||
		CompanionId != ExpectedCompanionId ||
		!Root->TryGetStringField(TEXT("action"), Action) ||
		!Root->TryGetObjectField(TEXT("observability"), Observability) ||
		!(*Observability)->TryGetStringField(
			TEXT("used_snapshot_id"), UsedSnapshotId) ||
		UsedSnapshotId != ExpectedSnapshotId)
	{
		Fail(TEXT("队友 AI 响应版本、身份或快照不匹配。"));
		return;
	}

	FCompanionChatReply ChatReply;
	ChatReply.ProtocolVersion = ProtocolVersion;
	ChatReply.CompanionId = CompanionId;
	(*Observability)->TryGetStringField(TEXT("source"), ChatReply.Source);
	Root->TryGetStringField(TEXT("reply_text"), ChatReply.ReplyText);

	const TSharedPtr<FJsonValue> DirectiveValue =
		Root->TryGetField(TEXT("directive"));
	if (Action == TEXT("none"))
	{
		if (DirectiveValue.IsValid() && DirectiveValue->Type != EJson::Null)
		{
			Fail(TEXT("空动作响应意外包含指令。"));
			return;
		}
	}
	else if (Action == TEXT("directive"))
	{
		const TSharedPtr<FJsonObject>* DirectiveObject;
		if (!Root->TryGetObjectField(TEXT("directive"), DirectiveObject))
		{
			Fail(TEXT("队友 AI 响应缺少指令。"));
			return;
		}

		FCompanionAgentDirective Directive;
		double Priority = -1.0;
		FGuid ParsedDirectiveId;
		if (!(*DirectiveObject)->TryGetStringField(
				TEXT("directive_id"), Directive.DirectiveId) ||
			!FGuid::Parse(Directive.DirectiveId, ParsedDirectiveId) ||
			!(*DirectiveObject)->TryGetStringField(
				TEXT("agent_id"), Directive.AgentId) ||
			Directive.AgentId != ExpectedCompanionId ||
			!(*DirectiveObject)->TryGetStringField(
				TEXT("domain"), Directive.Domain) ||
			Directive.Domain != ExpectedScene ||
			!(*DirectiveObject)->TryGetStringField(
				TEXT("action_type"), Directive.ActionType) ||
			Directive.ActionType.IsEmpty() ||
			!(*DirectiveObject)->TryGetStringField(
				TEXT("source"), Directive.Source) ||
			!(*DirectiveObject)->TryGetNumberField(TEXT("priority"), Priority) ||
			!FMath::IsFinite(Priority) || Priority < 0.0 || Priority > 100.0)
		{
			Fail(TEXT("队友 AI 指令身份、场景或动作不合法。"));
			return;
		}
		Directive.Priority = static_cast<int32>(Priority);
		(*DirectiveObject)->TryGetStringField(
			TEXT("policy_revision"), Directive.PolicyRevision);

		const TSharedPtr<FJsonObject>* ExpiresObject;
		if (!(*DirectiveObject)->TryGetObjectField(
				TEXT("expires"), ExpiresObject) ||
			!(*ExpiresObject)->TryGetStringField(
				TEXT("type"), Directive.ExpiresType))
		{
			Fail(TEXT("队友 AI 指令缺少有效期。"));
			return;
		}
		if (Directive.ExpiresType == TEXT("before_seconds"))
		{
			double RemainingSeconds = -1.0;
			if (!(*ExpiresObject)->TryGetNumberField(
					TEXT("remaining_seconds"), RemainingSeconds) ||
				!FMath::IsFinite(RemainingSeconds) ||
				RemainingSeconds <= 0.0 ||
				FPlatformTime::Seconds() - SentAtSeconds >= RemainingSeconds)
			{
				Fail(TEXT("队友 AI 指令已过期。"));
				return;
			}
			Directive.RemainingSeconds = static_cast<float>(
				RemainingSeconds - (FPlatformTime::Seconds() - SentAtSeconds));
		}
		else if (Directive.ExpiresType == TEXT("encounter_end"))
		{
			if (!bEncounterActive)
			{
				Fail(TEXT("非战斗状态收到了持续到战斗结束的指令。"));
				return;
			}
		}
		else if (Directive.ExpiresType != TEXT("immediate"))
		{
			Fail(TEXT("队友 AI 指令有效期类型未知。"));
			return;
		}

		const TSharedPtr<FJsonObject>* PayloadObject;
		if (!(*DirectiveObject)->TryGetObjectField(TEXT("payload"), PayloadObject))
		{
			Fail(TEXT("队友 AI 指令缺少 payload。"));
			return;
		}
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair :
			(*PayloadObject)->Values)
		{
			if (!Pair.Value.IsValid() || Pair.Value->Type != EJson::String)
			{
				Fail(TEXT("队友 AI 指令 payload 类型不合法。"));
				return;
			}
			Directive.Payload.Add(Pair.Key, Pair.Value->AsString());
		}

		for (const FString& TargetKey : {
			FString(TEXT("target_id")), FString(TEXT("object_id"))})
		{
			if (const FString* TargetId = Directive.Payload.Find(TargetKey))
			{
				if (!TargetId->IsEmpty() && !AllowedTargetIds.Contains(*TargetId))
				{
					Fail(TEXT("队友 AI 指令引用了快照外的目标。"));
					return;
				}
				if (TargetKey == TEXT("target_id"))
				{
					Directive.TargetId = *TargetId;
				}
			}
		}

		const TSharedPtr<FJsonObject>* PresentationObject;
		if ((*DirectiveObject)->TryGetObjectField(
				TEXT("presentation"), PresentationObject))
		{
			FString GazeTargetId;
			if ((*PresentationObject)->TryGetStringField(
					TEXT("gaze_target_id"), GazeTargetId) &&
				!GazeTargetId.IsEmpty() &&
				!AllowedTargetIds.Contains(GazeTargetId))
			{
				Fail(TEXT("队友 AI 表现指令引用了快照外的目标。"));
				return;
			}
			(*PresentationObject)->TryGetStringField(
				TEXT("reply_text"), ChatReply.ReplyText);
			(*PresentationObject)->TryGetStringField(
				TEXT("emotion_id"), ChatReply.EmotionId);
			(*PresentationObject)->TryGetStringField(
				TEXT("gesture_id"), ChatReply.GestureId);
			(*PresentationObject)->TryGetStringField(
				TEXT("facial_expression_id"),
				ChatReply.FacialExpressionId);
			(*PresentationObject)->TryGetBoolField(
				TEXT("interruptible"), ChatReply.bInterruptible);
		}
		OnDirectiveReceived.Broadcast(Directive);
	}
	else
	{
		Fail(TEXT("队友 AI 响应动作类型未知。"));
		return;
	}

	bHeartbeatFailureLogged = false;
	if (bPlayerCommand || Action == TEXT("directive"))
	{
		TArray<FString> ReasonCodes;
		(*Observability)->TryGetStringArrayField(TEXT("reason_codes"), ReasonCodes);
		UE_LOG(LogAesirWarden, Display,
			TEXT("Companion agent step: companion=%s scene=%s action=%s reasons=%s request=%s"),
			*CompanionId, *ExpectedScene, *Action,
			*FString::Join(ReasonCodes, TEXT(",")), *RequestId);
	}
	if (bPlayerCommand)
	{
		if (ChatReply.ReplyText.IsEmpty())
		{
			ChatReply.ReplyText = Action == TEXT("directive")
				? TEXT("收到。") : TEXT("暂时没有可执行指令。");
		}
		OnReplyReceived.Broadcast(ChatReply);
	}
}
