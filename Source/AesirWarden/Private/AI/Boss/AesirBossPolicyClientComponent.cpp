// Copyright © 2026 YYchainsAw. All Rights Reserved.

#include "AI/Boss/AesirBossPolicyClientComponent.h"

#include "AesirWarden.h"
#include "AI/Boss/AesirBossObservationComponent.h"
#include "AI/Boss/AesirBossObservationTypes.h"
#include "AI/Boss/AesirBossTelemetryComponent.h"
#include "AI/Boss/AesirBossAIController.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"

UAesirBossPolicyClientComponent::UAesirBossPolicyClientComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAesirBossPolicyClientComponent::Initialize(
	AAesirBossAIController* InController,
	UAesirBossObservationComponent* InObservationComponent)
{
	Controller = InController;
	ObservationComponent = InObservationComponent;
}

void UAesirBossPolicyClientComponent::Start(AActor* InitialTarget)
{
	Stop();
	TargetActor = InitialTarget;
	ConsecutiveFailures = 0;
	bRunning = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DecisionTimerHandle,
			this,
			&UAesirBossPolicyClientComponent::RequestDecision,
			DecisionIntervalSeconds,
			true,
			0.05f);
	}
}

void UAesirBossPolicyClientComponent::Stop()
{
	bRunning = false;
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(DecisionTimerHandle);

	if (ActiveRequest.IsValid())
	{
		ActiveRequest->CancelRequest();
		ActiveRequest.Reset();
	}
}

bool UAesirBossPolicyClientComponent::IsRunning() const
{
	return bRunning;
}

void UAesirBossPolicyClientComponent::RequestDecision()
{
	if (!bRunning || ActiveRequest.IsValid() ||
		!IsValid(Controller) || !IsValid(ObservationComponent))
	{
		return;
	}

	AActor* Target = ResolveTarget();
	if (!IsValid(Target))
		return;

	TargetActor = Target;
	const FAesirBossObservation Observation =
		ObservationComponent->CaptureObservation(Target);
	if (!Observation.bValid ||
		Observation.SchemaVersion !=
			FAesirBossObservation::CurrentSchemaVersion)
	{
		RegisterFailure(TEXT("invalid_observation"));
		return;
	}
	PendingObservation = Observation;

	TArray<TSharedPtr<FJsonValue>> ObservationValues;
	for (const float Value : Observation.ToFeatureVector())
	{
		ObservationValues.Add(
			MakeShared<FJsonValueNumber>(
				FMath::Clamp(Value, 0.0f, 1.0f)));
	}

	const FString RequestId = FGuid::NewGuid().ToString(
		EGuidFormats::DigitsWithHyphensLower);
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("protocol_version"), TEXT("1.0"));
	Root->SetStringField(TEXT("request_id"), RequestId);
	Root->SetNumberField(
		TEXT("schema_version"),
		FAesirBossObservation::CurrentSchemaVersion);
	Root->SetNumberField(TEXT("sequence"), Observation.Sequence);
	Root->SetArrayField(TEXT("observation"), ObservationValues);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer =
		TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Root, Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
		FHttpModule::Get().CreateRequest();
	Request->SetURL(
		ServiceBaseUrl + TEXT("/v1/boss/policy/decide"));
	Request->SetVerb(TEXT("POST"));
	Request->SetTimeout(RequestTimeoutSeconds);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Body);

	const double RequestStartedSeconds = FPlatformTime::Seconds();
	Request->OnProcessRequestComplete().BindUObject(
		this,
		&UAesirBossPolicyClientComponent::HandleDecisionResponse,
		RequestId,
		Observation.Sequence,
		RequestStartedSeconds);

	ActiveRequest = Request;
	if (!Request->ProcessRequest())
	{
		ActiveRequest.Reset();
		RegisterFailure(TEXT("request_not_started"));
	}
}

void UAesirBossPolicyClientComponent::HandleDecisionResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bSucceeded,
	FString ExpectedRequestId,
	int64 ExpectedSequence,
	double RequestStartedSeconds)
{
	if (ActiveRequest != Request)
		return;
	ActiveRequest.Reset();
	if (!bRunning)
		return;

	if (!bSucceeded || !Response.IsValid() ||
		Response->GetResponseCode() != 200)
	{
		RegisterFailure(TEXT("http_failure"));
		return;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader =
		TJsonReaderFactory<>::Create(Response->GetContentAsString());
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		RegisterFailure(TEXT("invalid_json"));
		return;
	}

	FString ResponseRequestId;
	FString ProtocolVersion;
	FString ModelId;
	double ResponseSequence = -1.0;
	double SchemaVersion = -1.0;
	bool bActionable = false;
	if (!Root->TryGetStringField(TEXT("protocol_version"), ProtocolVersion) ||
		!Root->TryGetStringField(TEXT("request_id"), ResponseRequestId) ||
		!Root->TryGetStringField(TEXT("model_id"), ModelId) ||
		!Root->TryGetNumberField(TEXT("sequence"), ResponseSequence) ||
		!Root->TryGetNumberField(TEXT("schema_version"), SchemaVersion) ||
		!Root->TryGetBoolField(TEXT("actionable"), bActionable) ||
		ProtocolVersion != TEXT("1.0") ||
		ResponseRequestId != ExpectedRequestId ||
		static_cast<int64>(ResponseSequence) != ExpectedSequence ||
		PendingObservation.Sequence != ExpectedSequence ||
		ModelId.IsEmpty() ||
		static_cast<int32>(SchemaVersion) !=
			FAesirBossObservation::CurrentSchemaVersion)
	{
		RegisterFailure(TEXT("protocol_mismatch"));
		return;
	}

	if (UAesirBossTelemetryComponent* Telemetry =
		Controller->GetBossTelemetryComponent())
	{
		Telemetry->SetPolicyIdentifier(ModelId);
	}

	ResetFailures();
	if (!bActionable)
		return;

	double ActionValue = -1.0;
	if (!Root->TryGetNumberField(TEXT("action_id"), ActionValue))
	{
		RegisterFailure(TEXT("missing_action"));
		return;
	}

	const int32 ActionId = static_cast<int32>(ActionValue);
	if (!FMath::IsNearlyEqual(ActionValue, static_cast<double>(ActionId)) ||
		ActionId < static_cast<int32>(EAesirBossAction::LightAttack) ||
		ActionId > static_cast<int32>(
			EAesirBossAction::UnblockableAreaSkill))
	{
		RegisterFailure(TEXT("invalid_action"));
		return;
	}

	AActor* Target = ResolveTarget();
	if (!IsValid(Target))
		return;

	const float RoundTripMilliseconds = static_cast<float>(
		(FPlatformTime::Seconds() - RequestStartedSeconds) * 1000.0);
	Controller->ExecuteBossPolicyDecision(
		static_cast<EAesirBossAction>(ActionId),
		Target,
		RoundTripMilliseconds,
		PendingObservation);
}

AActor* UAesirBossPolicyClientComponent::ResolveTarget() const
{
	if (IsValid(TargetActor))
		return TargetActor;
	return UGameplayStatics::GetPlayerPawn(this, 0);
}

void UAesirBossPolicyClientComponent::RegisterFailure(
	const FString& Reason)
{
	++ConsecutiveFailures;
	UE_LOG(
		LogAesirWarden,
		Warning,
		TEXT("Boss policy request failed: reason=%s count=%d/%d"),
		*Reason,
		ConsecutiveFailures,
		MaxConsecutiveFailures);

	if (ConsecutiveFailures >= MaxConsecutiveFailures &&
		IsValid(Controller))
	{
		Stop();
		Controller->ActivateBehaviorTreeFallback(Reason);
	}
}

void UAesirBossPolicyClientComponent::ResetFailures()
{
	ConsecutiveFailures = 0;
}
