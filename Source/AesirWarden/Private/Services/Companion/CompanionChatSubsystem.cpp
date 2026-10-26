#include "Services/Companion/CompanionChatSubsystem.h"

#include "AesirWarden.h"
#include "AI/Companion/TacticalOrderComponent.h"
#include "Characters/Companion/AesirCompanionCharacter.h"
#include "Characters/Player/AesirPlayerCharacter.h"
#include "Combat/AesirHealthComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/AesirCombatPrototypeGameMode.h"
#include "Framework/AesirCombatPrototypePlayerController.h"
#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Internationalization/Regex.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
constexpr double HeartbeatInterval = 3.0;
constexpr int32 MaxQueuedMessages = 8;
constexpr int32 MaxRequestAgeMs = 8000;
constexpr int32 MaxSnapshotAgeMs = 8000;
constexpr TCHAR PlayerTarget[] = TEXT("party.player");

FString NewId() { return FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower); }

bool IsServiceUrl(const FString& Value)
{
	// HTTP is a development-only loopback transport. Remote credentials require HTTPS.
	static const FRegexPattern Pattern(TEXT("^(https://([A-Za-z0-9.-]+|\\[[0-9A-Fa-f:]+\\])(:[0-9]{1,5})?|http://(127\\.0\\.0\\.1|localhost|\\[::1\\])(:[0-9]{1,5})?)/?$"));
	FRegexMatcher Matcher(Pattern, Value);
	return Matcher.FindNext();
}

bool IsCompanionId(const FString& Value)
{
	if (Value.IsEmpty() || Value.Len() > 100 || Value.Contains(TEXT(".."))) { return false; }
	for (TCHAR C : Value)
	{
		if (!(FChar::IsAlnum(C) || C == '.' || C == '_' || C == '-')) { return false; }
	}
	return true;
}

void OverrideEnvironment(const TCHAR* Name, FString& Value)
{
	const FString Override = FPlatformMisc::GetEnvironmentVariable(Name);
	if (!Override.IsEmpty()) { Value = Override; }
}
}

void UCompanionChatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	const TCHAR* Section = TEXT("Aesir.Companion");
	GConfig->GetString(Section, TEXT("ServiceBaseUrl"), ServiceBaseUrl, GGameIni);
	GConfig->GetString(Section, TEXT("GameId"), ExpectedGameId, GGameIni);
	GConfig->GetString(Section, TEXT("SaveId"), CurrentSaveId, GGameIni);
	GConfig->GetString(Section, TEXT("CompanionId"), SelectedCompanionId, GGameIni);
	GConfig->GetBool(Section, TEXT("RequireBearer"), bRequireBearer, GGameIni);
	OverrideEnvironment(TEXT("AESIR_NPC_BASE_URL"), ServiceBaseUrl);
	OverrideEnvironment(TEXT("AESIR_NPC_GAME_ID"), ExpectedGameId);
	OverrideEnvironment(TEXT("AESIR_NPC_SAVE_ID"), CurrentSaveId);
	OverrideEnvironment(TEXT("AESIR_NPC_COMPANION_ID"), SelectedCompanionId);
	BearerToken = FPlatformMisc::GetEnvironmentVariable(TEXT("AESIR_NPC_BEARER_TOKEN"));
	ServiceBaseUrl.RemoveFromEnd(TEXT("/"));
	FGuid SaveGuid;
	if (FGuid::Parse(CurrentSaveId, SaveGuid))
	{
		CurrentSaveId = SaveGuid.ToString(EGuidFormats::DigitsWithHyphensLower);
	}
	HeartbeatTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UCompanionChatSubsystem::TickHeartbeat), 0.1f);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UCompanionChatSubsystem::HandleWorldCleanup);
}

void UCompanionChatSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(HeartbeatTickerHandle);
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	InvalidateSession(true);
	BearerToken.Empty();
	Super::Deinitialize();
}

bool UCompanionChatSubsystem::ConfigureSession(const FString& BaseUrl, const FString& GameId, const FString& SaveId)
{
	if (bInvalidating) { return false; }
	FGuid SaveGuid;
	if (!FGuid::Parse(SaveId, SaveGuid) || GameId.IsEmpty() || !IsServiceUrl(BaseUrl))
	{
		OnRequestFailed.Broadcast(TEXT("队友服务配置无效：需要服务地址、游戏标识和存档 UUID。"));
		return false;
	}
	InvalidateSession(true);
	ServiceBaseUrl = BaseUrl;
	ServiceBaseUrl.RemoveFromEnd(TEXT("/"));
	ExpectedGameId = GameId;
	CurrentSaveId = SaveGuid.ToString(EGuidFormats::DigitsWithHyphensLower);
	BearerToken = FPlatformMisc::GetEnvironmentVariable(TEXT("AESIR_NPC_BEARER_TOKEN"));
	bReconnectRequired = false;
	return true;
}

void UCompanionChatSubsystem::SetSelectedCompanionId(const FString& CompanionId)
{
	if (bInvalidating) { return; }
	if (!IsCompanionId(CompanionId)) { OnRequestFailed.Broadcast(TEXT("队友标识无效。")); return; }
	if (CompanionId == SelectedCompanionId) { return; }
	// One run is shared by all NPCs in this save; only the selected NPC's epoch changes.
	SelectedCompanionId = CompanionId;
	InvalidateSession(false, true);
	bReconnectRequired = false;
}

void UCompanionChatSubsystem::ResetConversation()
{
	InvalidateSession(true);
	bReconnectRequired = false;
}

void UCompanionChatSubsystem::HandleWorldCleanup(UWorld* World, bool, bool)
{
	if (SessionWorld.Get() == World) { InvalidateSession(true); }
}

bool UCompanionChatSubsystem::IsExplorationActive() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || !World->IsGameWorld()) { return false; }
	if (const AAesirCombatPrototypeGameMode* Mode = World->GetAuthGameMode<AAesirCombatPrototypeGameMode>())
	{
		if (Mode->GetMatchResult() != EAesirMatchResult::None) { return false; }
	}
	for (TActorIterator<AAesirLevelFlowDirector> It(World); It; ++It)
	{
		if (IsValid(*It)) { return It->GetFlowState() == EAesirLevelFlowState::Exploration; }
	}
	return true; // Maps without a director are exploration sandboxes.
}

bool UCompanionChatSubsystem::RefreshLocalContext()
{
	if (bInvalidating) { return false; }
	const FString NewToken = FPlatformMisc::GetEnvironmentVariable(TEXT("AESIR_NPC_BEARER_TOKEN"));
	if (NewToken != BearerToken)
	{
		InvalidateSession(true);
		BearerToken = NewToken;
		bReconnectRequired = false;
	}
	UWorld* World = GetWorld();
	APawn* Player = IsValid(World) ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
	AAesirCompanionCharacter* Companion = nullptr;
	if (IsExplorationActive())
	{
		for (TActorIterator<AAesirCompanionCharacter> It(World); It; ++It)
		{
			if (IsValid(*It) && It->GetCompanionId().ToString() == SelectedCompanionId) { Companion = *It; break; }
		}
	}
	const UAesirHealthComponent* Health = IsValid(Companion) ? Companion->FindComponentByClass<UAesirHealthComponent>() : nullptr;
	bool bAlive = IsValid(Player) && IsValid(Health) && Health->GetCurrentHealth() > 0.0f;
	if (const AAesirPlayerCharacter* AesirPlayer = Cast<AAesirPlayerCharacter>(Player))
	{
		bAlive &= AesirPlayer->GetGASHealth() > 0.0f;
	}
	const UTacticalOrderComponent* Orders = IsValid(Companion) ? Companion->FindComponentByClass<UTacticalOrderComponent>() : nullptr;
	if (IsValid(Orders) && Orders->IsEncounterActive()) { bAlive = false; }
	if (!bAlive)
	{
		if (bContextActive || bConnecting || bBindingReady || !DesiredRunId.IsEmpty()) { InvalidateSession(true); }
		return false;
	}
	if (bContextActive && (SessionWorld.Get() != World || SessionPlayer.Get() != Player || SessionCompanion.Get() != Companion))
	{
		InvalidateSession(true);
	}
	if (!bContextActive)
	{
		SessionWorld = World;
		SessionPlayer = Player;
		SessionCompanion = Companion;
		bContextActive = true;
		if (DesiredRunId.IsEmpty()) { DesiredRunId = NewId(); }
	}
	return true;
}

bool UCompanionChatSubsystem::TickHeartbeat(float)
{
	if (!RefreshLocalContext()) { return true; }
	if (!PendingSteps.IsEmpty()) { PumpQueue(); }
	const double Now = FPlatformTime::Seconds();
	if (Now >= NextHeartbeatAt && !ActiveStep.IsValid() && PendingSteps.IsEmpty() &&
		!bConnecting && !bReconnectRequired && !UncertainStep.IsValid())
	{
		FGuid SaveGuid;
		if (FGuid::Parse(CurrentSaveId, SaveGuid))
		{
			auto Step = MakeShared<FPendingStep>();
			Step->RequestId = NewId();
			PendingSteps.Add(Step);
			PumpQueue();
		}
		NextHeartbeatAt = Now + HeartbeatInterval;
	}
	return true;
}

void UCompanionChatSubsystem::SendChatMessage(const FString& Text)
{
	if (bInvalidating) { return; }
	const FString Trimmed = Text.TrimStartAndEnd();
	if (Trimmed.IsEmpty() || Trimmed.Len() > 500) { OnRequestFailed.Broadcast(TEXT("请输入 1 到 500 个字符。")); return; }
	FGuid SaveGuid;
	if (!FGuid::Parse(CurrentSaveId, SaveGuid))
	{
		OnRequestFailed.Broadcast(TEXT("队友服务尚未关联当前存档，请先完成连接配置。")); return;
	}
	if (!RefreshLocalContext())
	{
		OnRequestFailed.Broadcast(TEXT("当前无法对话：请在探索阶段选择存活的队友。")); return;
	}
	if (bRequireBearer && BearerToken.IsEmpty())
	{
		OnRequestFailed.Broadcast(TEXT("队友服务缺少登录凭据，请重新连接。")); return;
	}
	if (UncertainStep.IsValid())
	{
		OnRequestFailed.Broadcast(TEXT("上一条请求结果未确认；可在原预算内重试，或取消后再发送新指令。")); return;
	}
	if (PendingSteps.Num() >= MaxQueuedMessages)
	{
		OnRequestFailed.Broadcast(TEXT("待发送消息已满，请等待当前对话完成。")); return;
	}
	bReconnectRequired = false;
	auto Step = MakeShared<FPendingStep>();
	Step->RequestId = NewId();
	Step->Text = Trimmed;
	Step->bPlayerCommand = true;
	PendingSteps.Add(Step);
	PumpQueue();
}

void UCompanionChatSubsystem::PumpQueue()
{
	if (bConnecting || ActiveStep.IsValid() || bReconnectRequired || UncertainStep.IsValid() || PendingSteps.IsEmpty() || !bContextActive) { return; }
	if (!IsServiceUrl(ServiceBaseUrl) || !IsCompanionId(SelectedCompanionId))
	{
		FailSession(TEXT("队友服务配置无效：远程服务需要 HTTPS，本地开发使用回环 HTTP 地址。")); return;
	}
	if (!bBindingReady) { DiscoverContext(); return; }
	ActiveStep = PendingSteps[0];
	PendingSteps.RemoveAt(0);
	SendAgentStep(ActiveStep);
}

FHttpRequestPtr UCompanionChatSubsystem::MakeRequest(const FString& Path, const TSharedPtr<FJsonObject>& Body)
{
	FHttpRequestPtr Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(ServiceBaseUrl + TEXT("/v1/scoped") + Path);
	Request->SetVerb(Body.IsValid() ? TEXT("POST") : TEXT("GET"));
	Request->SetTimeout(8.0f);
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	if (!BearerToken.IsEmpty()) { Request->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + BearerToken); }
	if (Body.IsValid())
	{
		FString Json;
		FJsonSerializer::Serialize(Body.ToSharedRef(), TJsonWriterFactory<>::Create(&Json));
		Request->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
		Request->SetContentAsString(Json);
	}
	return Request;
}

bool UCompanionChatSubsystem::ReadResponse(FHttpResponsePtr Response, bool bSucceeded, TSharedPtr<FJsonObject>& Root, FString& Error) const
{
	if (!bSucceeded || !Response.IsValid())
	{
		Error = TEXT("队友服务连接中断；请求可能已提交，结果未确认。"); return false;
	}
	const bool bJson = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response->GetContentAsString()), Root) && Root.IsValid();
	if (Response->GetResponseCode() < 200 || Response->GetResponseCode() >= 300)
	{
		FString Code;
		const TSharedPtr<FJsonObject>* Detail;
		if (bJson && Root->TryGetObjectField(TEXT("detail"), Detail)) { (*Detail)->TryGetStringField(TEXT("code"), Code); }
		// Never echo remote bodies or messages, including authentication errors.
		bool bSafeCode = Code.Len() <= 80;
		for (TCHAR C : Code) { bSafeCode &= ((C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9') || C == '_'); }
		if (!bSafeCode) { Code.Empty(); }
		Error = FString::Printf(TEXT("队友服务 HTTP %d %s；请检查凭据、存档权限或当前绑定。"), Response->GetResponseCode(), *Code);
		return false;
	}
	if (!bJson) { Error = TEXT("队友服务返回了无效 JSON。"); return false; }
	return true;
}

void UCompanionChatSubsystem::DiscoverContext()
{
	if (bRequireBearer && BearerToken.IsEmpty()) { FailSession(TEXT("队友服务缺少 Bearer 凭据。")); return; }
	bConnecting = true;
	const int64 Generation = ContextGeneration;
	ActiveHttpRequest = MakeRequest(FString::Printf(TEXT("/context?save_id=%s&companion_id=%s"),
		*FGenericPlatformHttp::UrlEncode(CurrentSaveId), *FGenericPlatformHttp::UrlEncode(SelectedCompanionId)));
	ActiveHttpRequest->OnProcessRequestComplete().BindWeakLambda(this,
		[this, Generation](FHttpRequestPtr, FHttpResponsePtr Response, bool bSucceeded)
		{
			if (Generation != ContextGeneration || !RefreshLocalContext() || Generation != ContextGeneration) { return; }
			ActiveHttpRequest.Reset();
			TSharedPtr<FJsonObject> Root;
			FString Error;
			CompanionScoped::FBinding Discovered;
			if (!ReadResponse(Response, bSucceeded, Root, Error) ||
				!CompanionScoped::ParseContext(Root, CurrentSaveId, SelectedCompanionId, ExpectedGameId, Discovered, Error))
			{
				FailSession(Error); return;
			}
			if (Discovered.RunId == DesiredRunId && !Discovered.RunBindingId.IsEmpty() && Discovered.EncounterId.IsEmpty())
			{
				Binding = Discovered;
				bBindingReady = true;
				bConnecting = false;
				PumpQueue();
			}
			else { BindRun(Discovered); }
		});
	if (!ActiveHttpRequest->ProcessRequest()) { ActiveHttpRequest.Reset(); FailSession(TEXT("无法发现队友服务。")); }
}

void UCompanionChatSubsystem::BindRun(const CompanionScoped::FBinding& Discovered)
{
	const FString ControlRequestId = NewId();
	const FString NewRunId = DesiredRunId;
	const int64 Generation = ContextGeneration;
	ActiveHttpRequest = MakeRequest(TEXT("/runs/bind"), CompanionScoped::BuildBindBody(Discovered, ControlRequestId, NewRunId));
	ActiveHttpRequest->OnProcessRequestComplete().BindWeakLambda(this,
		[this, Discovered, Generation, ControlRequestId, NewRunId](FHttpRequestPtr, FHttpResponsePtr Response, bool bSucceeded)
		{
			if (Generation != ContextGeneration || !RefreshLocalContext() || Generation != ContextGeneration) { return; }
			ActiveHttpRequest.Reset();
			TSharedPtr<FJsonObject> Root;
			FString Error;
			CompanionScoped::FBinding Bound;
			if (!ReadResponse(Response, bSucceeded, Root, Error) ||
				!CompanionScoped::ParseBoundControl(Root, Discovered, ControlRequestId, NewRunId, Bound, Error))
			{
				FailSession(Error); return;
			}
			Binding = Bound;
			bBindingReady = true;
			bConnecting = false;
			UE_LOG(LogAesirWarden, Display, TEXT("Companion scoped run bound: run=%s companion=%s"), *Binding.RunId, *Binding.CompanionId);
			PumpQueue();
		});
	if (!ActiveHttpRequest->ProcessRequest()) { ActiveHttpRequest.Reset(); FailSession(TEXT("无法绑定队友服务。")); }
}

bool UCompanionChatSubsystem::BuildWorldContext(TSharedRef<FJsonObject>& OutContext, FString& OutCompanionId, FString& OutSnapshotId) const
{
	const AAesirCompanionCharacter* Companion = SessionCompanion.Get();
	const APawn* Player = SessionPlayer.Get();
	if (!IsValid(Companion) || !IsValid(Player) || !IsExplorationActive()) { return false; }
	const UAesirHealthComponent* Health = Companion->FindComponentByClass<UAesirHealthComponent>();
	const AAesirPlayerCharacter* AesirPlayer = Cast<AAesirPlayerCharacter>(Player);
	if (!IsValid(Health) || Health->GetMaxHealth() <= 0.0f || !AesirPlayer || AesirPlayer->GetGASMaxHealth() <= 0.0f) { return false; }
	OutCompanionId = Companion->GetCompanionId().ToString();
	OutSnapshotId = NewId();
	auto PlayerJson = MakeShared<FJsonObject>();
	PlayerJson->SetStringField(TEXT("id"), PlayerTarget);
	PlayerJson->SetNumberField(TEXT("hp_percent"), FMath::Clamp(100.0f * AesirPlayer->GetGASHealth() / AesirPlayer->GetGASMaxHealth(), 0.0f, 100.0f));
	PlayerJson->SetBoolField(TEXT("is_downed"), AesirPlayer->GetGASHealth() <= 0.0f);
	auto CompanionJson = MakeShared<FJsonObject>();
	CompanionJson->SetStringField(TEXT("id"), OutCompanionId);
	CompanionJson->SetNumberField(TEXT("hp_percent"), FMath::Clamp(100.0f * Health->GetCurrentHealth() / Health->GetMaxHealth(), 0.0f, 100.0f));
	CompanionJson->SetNumberField(TEXT("mp_percent"), 0.0);
	CompanionJson->SetStringField(TEXT("current_behavior"), Companion->GetVelocity().SizeSquared2D() > 25.0f ? TEXT("moving") : TEXT("idle"));
	CompanionJson->SetBoolField(TEXT("is_casting"), false);
	OutContext->SetStringField(TEXT("snapshot_id"), OutSnapshotId);
	OutContext->SetStringField(TEXT("captured_at"), FDateTime::UtcNow().ToIso8601());
	OutContext->SetStringField(TEXT("scene"), TEXT("exploration"));
	OutContext->SetObjectField(TEXT("player"), PlayerJson);
	OutContext->SetObjectField(TEXT("companion"), CompanionJson);
	const AAesirCombatPrototypePlayerController* PC = Cast<AAesirCombatPrototypePlayerController>(Player->GetController());
	OutContext->SetBoolField(TEXT("ui_popup"), IsValid(PC) && PC->IsCompanionChatOpen());
	OutContext->SetArrayField(TEXT("interactables"), {});
	return true;
}

void UCompanionChatSubsystem::SendAgentStep(TSharedPtr<FPendingStep> Step)
{
	if (!Step->WorldContext.IsValid())
	{
		Step->CapturedAt = FPlatformTime::Seconds();
		TSharedRef<FJsonObject> WorldContext = MakeShared<FJsonObject>();
		FString CompanionId;
		if (!BuildWorldContext(WorldContext, CompanionId, Step->SnapshotId) || CompanionId != Binding.CompanionId)
		{
			ReportFailure(Step->bPlayerCommand, TEXT("无法读取当前探索场景。")); CompleteStep(Step); return;
		}
		Step->WorldContext = WorldContext;
		Step->FirstSentAt = FPlatformTime::Seconds();
		Step->Binding = Binding;
		Step->Generation = ContextGeneration;
	}
	const double Now = FPlatformTime::Seconds();
	const double ElapsedMs = (Now - Step->FirstSentAt) * 1000.0;
	const double CaptureAgeMs = (Now - Step->CapturedAt) * 1000.0;
	if (ElapsedMs >= MaxRequestAgeMs || CaptureAgeMs >= MaxSnapshotAgeMs || Step->Generation != ContextGeneration)
	{
		ReportFailure(Step->bPlayerCommand, TEXT("原请求预算已过期，旧请求不会重新执行。")); CompleteStep(Step); return;
	}
	auto Body = CompanionScoped::BuildScopedBody(Step->Binding, Step->RequestId);
	Body->SetObjectField(TEXT("world_context"), Step->WorldContext);
	if (Step->bPlayerCommand) { Body->SetStringField(TEXT("text"), Step->Text); }
	auto Timing = MakeShared<FJsonObject>();
	Timing->SetNumberField(TEXT("capture_age_ms"), FMath::FloorToInt(CaptureAgeMs));
	Timing->SetNumberField(TEXT("request_age_ms"), FMath::FloorToInt(ElapsedMs));
	Timing->SetNumberField(TEXT("max_snapshot_age_ms"), MaxSnapshotAgeMs);
	Timing->SetNumberField(TEXT("max_request_age_ms"), MaxRequestAgeMs);
	Body->SetObjectField(TEXT("timing"), Timing);
	if (Step->bPlayerCommand)
	{
		UE_LOG(LogAesirWarden, Display, TEXT("Companion scoped request: request=%s run=%s companion=%s age_ms=%.1f"),
			*Step->RequestId, *Step->Binding.RunId, *Step->Binding.CompanionId, ElapsedMs);
	}
	ActiveHttpRequest = MakeRequest(TEXT("/agent/step"), Body);
	ActiveHttpRequest->OnProcessRequestComplete().BindUObject(this, &UCompanionChatSubsystem::HandleAgentStepResponse, Step);
	if (!ActiveHttpRequest->ProcessRequest())
	{
		ActiveHttpRequest.Reset();
		if (Step->bPlayerCommand)
		{
			ActiveStep.Reset(); UncertainStep = Step;
			ReportFailure(true, TEXT("队友请求未能发送；可重试原请求或取消。"));
		}
		else { FailSession(TEXT("队友心跳未能发送；发送消息时将重新连接。")); }
	}
}

bool UCompanionChatSubsystem::RetryLastMessage()
{
	if (!RefreshLocalContext() || !UncertainStep.IsValid() || ActiveStep.IsValid() || UncertainStep->Generation != ContextGeneration) { return false; }
	ActiveStep = UncertainStep;
	UncertainStep.Reset();
	SendAgentStep(ActiveStep);
	return true;
}

void UCompanionChatSubsystem::HandleAgentStepResponse(FHttpRequestPtr, FHttpResponsePtr Response, bool bSucceeded, TSharedPtr<FPendingStep> Step)
{
	if (Step->Generation != ContextGeneration || ActiveStep != Step || !RefreshLocalContext() || Step->Generation != ContextGeneration) { return; }
	ActiveHttpRequest.Reset();
	TSharedPtr<FJsonObject> Root;
	FString Error;
	if (!ReadResponse(Response, bSucceeded, Root, Error))
	{
		UE_LOG(LogAesirWarden, Display, TEXT("Companion scoped response unavailable: request=%s elapsed_ms=%.1f"),
			*Step->RequestId, (FPlatformTime::Seconds() - Step->FirstSentAt) * 1000.0);
		if (!Step->bPlayerCommand)
		{
			// A failed background probe cannot monopolize the player's recovery slot.
			FailSession(Error);
		}
		else if (!bSucceeded || !Response.IsValid() || Response->GetResponseCode() >= 500 ||
			(Response->GetResponseCode() >= 200 && Response->GetResponseCode() < 300))
		{
			ActiveStep.Reset(); UncertainStep = Step;
			ReportFailure(Step->bPlayerCommand, Error + TEXT(" 可重试原请求或取消，不会自动换 ID 重做。"));
		}
		else { FailSession(Error); }
		return;
	}
	FCompanionChatReply Reply;
	FCompanionAgentDirective Directive;
	bool bHasDirective = false;
	const double Now = FPlatformTime::Seconds();
	if ((Now - Step->CapturedAt) * 1000.0 >= MaxSnapshotAgeMs ||
		!CompanionScoped::ValidateAgentResponse(Root, Step->Binding, Step->RequestId, Step->SnapshotId, Step->FirstSentAt, Now, Reply, Directive, bHasDirective, Error))
	{
		ReportFailure(Step->bPlayerCommand, Error.IsEmpty() ? TEXT("队友响应使用的快照已过期。") : Error); CompleteStep(Step); return;
	}
	if (bHasDirective)
	{
		Directive.ContextGeneration = ContextGeneration;
		if (IsDirectiveCurrent(Directive) && !AppliedDirectiveIds.Contains(Directive.DirectiveId))
		{
			if (AppliedDirectiveIds.Num() >= 4096) { FailSession(TEXT("当前对话指令记录已满，请重新开始对话。")); return; }
			AppliedDirectiveIds.Add(Directive.DirectiveId);
			OnDirectiveReceived.Broadcast(Directive);
		}
	}
	if (Step->Generation != ContextGeneration) { return; }
	bHeartbeatFailureLogged = false;
	UE_LOG(LogAesirWarden, Display, TEXT("Companion scoped response: request=%s run=%s companion=%s directive=%s elapsed_ms=%.1f"),
		*Step->RequestId, *Step->Binding.RunId, *Step->Binding.CompanionId, bHasDirective ? *Directive.DirectiveId : TEXT("none"), (Now - Step->FirstSentAt) * 1000.0);
	if (Step->bPlayerCommand)
	{
		if (Reply.ReplyText.IsEmpty()) { Reply.ReplyText = bHasDirective ? TEXT("收到。") : TEXT("暂时没有可执行指令。"); }
		OnReplyReceived.Broadcast(Reply);
	}
	CompleteStep(Step);
}

bool UCompanionChatSubsystem::IsDirectiveCurrent(const FCompanionAgentDirective& Directive) const
{
	return bBindingReady && bContextActive && Directive.ContextGeneration == ContextGeneration &&
		Directive.AgentId == SelectedCompanionId && Directive.RunId == Binding.RunId &&
		FPlatformTime::Seconds() < Directive.ValidUntilSeconds && IsExplorationActive();
}

void UCompanionChatSubsystem::CompleteStep(TSharedPtr<FPendingStep> Step)
{
	if (ActiveStep == Step) { ActiveStep.Reset(); }
	if (UncertainStep == Step) { UncertainStep.Reset(); }
	NextHeartbeatAt = FPlatformTime::Seconds() + HeartbeatInterval;
	PumpQueue();
}

void UCompanionChatSubsystem::SendCancellation(const TSharedPtr<FPendingStep>& Step)
{
	if (!Step.IsValid() || Step->Binding.RunBindingId.IsEmpty()) { return; }
	auto Body = CompanionScoped::BuildScopedBody(Step->Binding, NewId());
	Body->SetStringField(TEXT("target_request_id"), Step->RequestId);
	Body->SetStringField(TEXT("operation"), TEXT("agent-step"));
	Body->SetStringField(TEXT("reason_code"), TEXT("USER_CANCELLED"));
	MakeRequest(TEXT("/requests/cancel"), Body)->ProcessRequest();
}

void UCompanionChatSubsystem::SendRunEnd()
{
	if (Binding.RunBindingId.IsEmpty()) { return; }
	auto Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("protocol_version"), TEXT("0.4"));
	Body->SetStringField(TEXT("request_id"), NewId());
	Body->SetStringField(TEXT("save_id"), Binding.SaveId);
	Body->SetStringField(TEXT("service_instance_id"), Binding.ServiceInstanceId);
	Body->SetStringField(TEXT("run_id"), Binding.RunId);
	Body->SetStringField(TEXT("run_binding_id"), Binding.RunBindingId);
	Body->SetNumberField(TEXT("expected_control_revision"), Binding.ControlRevision);
	MakeRequest(TEXT("/runs/end"), Body)->ProcessRequest();
}

void UCompanionChatSubsystem::InvalidateSession(bool bEndRun, bool bPreserveRun)
{
	if (bInvalidating) { return; }
	TGuardValue<bool> InvalidationGuard(bInvalidating, true);
	++ContextGeneration; // Invalidate local callbacks before any remote cancellation.
	if (ActiveHttpRequest.IsValid())
	{
		ActiveHttpRequest->OnProcessRequestComplete().Unbind();
		ActiveHttpRequest->CancelRequest();
		ActiveHttpRequest.Reset();
	}
	SendCancellation(ActiveStep);
	SendCancellation(UncertainStep);
	if (bEndRun) { SendRunEnd(); }
	ActiveStep.Reset(); UncertainStep.Reset(); PendingSteps.Empty(); AppliedDirectiveIds.Empty();
	if (!bPreserveRun) { Binding = CompanionScoped::FBinding(); }
	bBindingReady = false; bConnecting = false; bContextActive = false;
	if (!bPreserveRun) { SessionWorld.Reset(); SessionPlayer.Reset(); }
	SessionCompanion.Reset();
	if (!bPreserveRun) { DesiredRunId.Empty(); }
	OnConversationReset.Broadcast();
}

void UCompanionChatSubsystem::FailSession(const FString& Message)
{
	const bool bHadPlayerMessage = (ActiveStep.IsValid() && ActiveStep->bPlayerCommand) ||
		PendingSteps.ContainsByPredicate([](const TSharedPtr<FPendingStep>& Step) { return Step->bPlayerCommand; });
	InvalidateSession(false);
	bReconnectRequired = true;
	ReportFailure(bHadPlayerMessage, Message);
}

void UCompanionChatSubsystem::ReportFailure(bool bPlayerCommand, const FString& Message)
{
	if (bPlayerCommand) { OnRequestFailed.Broadcast(Message); }
	else if (!bHeartbeatFailureLogged)
	{
		UE_LOG(LogAesirWarden, Warning, TEXT("%s"), *Message);
		bHeartbeatFailureLogged = true;
	}
}
