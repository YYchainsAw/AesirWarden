#include "AI/Boss/AesirBossTelemetryComponent.h"

#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
FString GetPolicySourceName(EAesirBossPolicySource Value)
{
	return StaticEnum<EAesirBossPolicySource>()->GetNameStringByValue(
		static_cast<int64>(Value));
}

FString GetEpisodeResultName(EAesirBossEpisodeResult Value)
{
	return StaticEnum<EAesirBossEpisodeResult>()->GetNameStringByValue(
		static_cast<int64>(Value));
}

FString GetActionName(EAesirBossAction Value)
{
	return StaticEnum<EAesirBossAction>()->GetNameStringByValue(
		static_cast<int64>(Value));
}

FString GetActionResultName(EAesirBossActionResult Value)
{
	return StaticEnum<EAesirBossActionResult>()->GetNameStringByValue(
		static_cast<int64>(Value));
}

FString GetRewardReasonName(EAesirBossRewardReason Value)
{
	return StaticEnum<EAesirBossRewardReason>()->GetNameStringByValue(
		static_cast<int64>(Value));
}

TSharedPtr<FJsonObject> BuildSummaryJson(
	const FAesirBossEpisodeSummary& Summary)
{
	TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("episode_id"), Summary.EpisodeId);
	Json->SetStringField(
		TEXT("policy_source"),
		GetPolicySourceName(Summary.PolicySource));
	Json->SetStringField(
		TEXT("policy_identifier"),
		Summary.PolicyIdentifier);
	Json->SetStringField(
		TEXT("result"),
		GetEpisodeResultName(Summary.Result));
	Json->SetNumberField(TEXT("duration_seconds"), Summary.DurationSeconds);
	Json->SetNumberField(TEXT("decision_count"), Summary.DecisionCount);
	Json->SetNumberField(
		TEXT("accepted_action_count"),
		Summary.AcceptedActionCount);
	Json->SetNumberField(
		TEXT("rejected_action_count"),
		Summary.RejectedActionCount);
	Json->SetNumberField(TEXT("total_reward"), Summary.TotalReward);
	Json->SetNumberField(TEXT("damage_dealt"), Summary.DamageDealt);
	Json->SetNumberField(TEXT("damage_received"), Summary.DamageReceived);
	Json->SetNumberField(
		TEXT("rejected_reward_count"),
		Summary.RejectedRewardCount);
	Json->SetNumberField(
		TEXT("repeated_action_penalty_count"),
		Summary.RepeatedActionPenaltyCount);
	return Json;
}

TSharedPtr<FJsonObject> BuildDecisionJson(
	const FAesirBossDecisionRecord& Record)
{
	TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetNumberField(TEXT("decision_index"), Record.DecisionIndex);
	Json->SetStringField(
		TEXT("policy_source"),
		GetPolicySourceName(Record.PolicySource));
	Json->SetNumberField(
		TEXT("observation_schema_version"),
		Record.Observation.SchemaVersion);
	Json->SetNumberField(
		TEXT("observation_sequence"),
		Record.Observation.Sequence);
	Json->SetNumberField(
		TEXT("observation_world_time_seconds"),
		Record.Observation.WorldTimeSeconds);
	Json->SetBoolField(
		TEXT("observation_valid"),
		Record.Observation.bValid);

	TArray<TSharedPtr<FJsonValue>> FeatureValues;
	for (const float Feature : Record.Observation.ToFeatureVector())
	{
		FeatureValues.Add(MakeShared<FJsonValueNumber>(Feature));
	}
	Json->SetArrayField(TEXT("observation"), FeatureValues);

	Json->SetNumberField(
		TEXT("action_id"),
		static_cast<uint8>(Record.Outcome.Action));
	Json->SetStringField(
		TEXT("action"),
		GetActionName(Record.Outcome.Action));
	Json->SetStringField(
		TEXT("action_result"),
		GetActionResultName(Record.Outcome.Result));
	Json->SetStringField(
		TEXT("target"),
		GetNameSafe(Record.Outcome.Target));
	Json->SetNumberField(
		TEXT("action_world_time_seconds"),
		Record.Outcome.WorldTimeSeconds);
	Json->SetNumberField(
		TEXT("policy_evaluation_milliseconds"),
		Record.PolicyEvaluationMilliseconds);
	Json->SetNumberField(
		TEXT("action_request_milliseconds"),
		Record.ActionRequestMilliseconds);
	Json->SetNumberField(TEXT("reward"), Record.Reward);
	return Json;
}

TSharedPtr<FJsonObject> BuildRewardEventJson(
	const FAesirBossRewardEvent& Event)
{
	TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetNumberField(TEXT("decision_index"), Event.DecisionIndex);
	Json->SetStringField(TEXT("reason"), GetRewardReasonName(Event.Reason));
	Json->SetNumberField(TEXT("source_magnitude"), Event.SourceMagnitude);
	Json->SetNumberField(TEXT("reward"), Event.Reward);
	Json->SetNumberField(
		TEXT("world_time_seconds"),
		Event.WorldTimeSeconds);
	return Json;
}
}

UAesirBossTelemetryComponent::UAesirBossTelemetryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FString UAesirBossTelemetryComponent::StartEpisode(
	EAesirBossPolicySource PolicySource,
	const FString& RequestedEpisodeId)
{
	DecisionRecords.Reset();
	RewardEvents.Reset();
	EpisodeSummary = FAesirBossEpisodeSummary();
	EpisodeSummary.PolicySource = PolicySource;
	EpisodeSummary.EpisodeId = RequestedEpisodeId.IsEmpty()
		? FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower)
		: RequestedEpisodeId;
	EpisodeStartTimeSeconds = GetWorld()
		? GetWorld()->GetTimeSeconds()
		: 0.0f;
	bEpisodeActive = true;
	return EpisodeSummary.EpisodeId;
}

void UAesirBossTelemetryComponent::RecordDecision(
	const FAesirBossDecisionRecord& Record)
{
	if (!bEpisodeActive || DecisionRecords.Num() >= MaxDecisionRecords)
		return;

	FAesirBossDecisionRecord StoredRecord = Record;
	StoredRecord.DecisionIndex = DecisionRecords.Num();
	StoredRecord.PolicySource = EpisodeSummary.PolicySource;
	DecisionRecords.Add(MoveTemp(StoredRecord));

	++EpisodeSummary.DecisionCount;
	if (Record.Outcome.WasAccepted())
		++EpisodeSummary.AcceptedActionCount;
	else
		++EpisodeSummary.RejectedActionCount;

	EpisodeSummary.TotalReward += Record.Reward;
}

void UAesirBossTelemetryComponent::SetPolicyIdentifier(
	const FString& PolicyIdentifier)
{
	if (bEpisodeActive && !PolicyIdentifier.IsEmpty())
	{
		EpisodeSummary.PolicyIdentifier = PolicyIdentifier;
	}
}

bool UAesirBossTelemetryComponent::AddRewardToLatestDecision(
	float RewardDelta,
	EAesirBossRewardReason Reason,
	float SourceMagnitude)
{
	if (!bEpisodeActive || DecisionRecords.IsEmpty())
		return false;

	DecisionRecords.Last().Reward += RewardDelta;
	EpisodeSummary.TotalReward += RewardDelta;

	FAesirBossRewardEvent RewardEvent;
	RewardEvent.DecisionIndex = DecisionRecords.Last().DecisionIndex;
	RewardEvent.Reason = Reason;
	RewardEvent.SourceMagnitude = FMath::Max(SourceMagnitude, 0.0f);
	RewardEvent.Reward = RewardDelta;
	RewardEvent.WorldTimeSeconds = GetWorld()
		? GetWorld()->GetTimeSeconds()
		: 0.0f;
	RewardEvents.Add(RewardEvent);

	switch (Reason)
	{
	case EAesirBossRewardReason::DamageDealt:
		EpisodeSummary.DamageDealt += RewardEvent.SourceMagnitude;
		break;

	case EAesirBossRewardReason::DamageReceived:
		EpisodeSummary.DamageReceived += RewardEvent.SourceMagnitude;
		break;

	case EAesirBossRewardReason::RejectedAction:
		++EpisodeSummary.RejectedRewardCount;
		break;

	case EAesirBossRewardReason::RepeatedAction:
		++EpisodeSummary.RepeatedActionPenaltyCount;
		break;

	default:
		break;
	}

	return true;
}

FAesirBossEpisodeSummary UAesirBossTelemetryComponent::EndEpisode(
	EAesirBossEpisodeResult Result)
{
	if (!bEpisodeActive)
		return EpisodeSummary;

	const float EndTimeSeconds = GetWorld()
		? GetWorld()->GetTimeSeconds()
		: EpisodeStartTimeSeconds;
	EpisodeSummary.DurationSeconds = FMath::Max(
		EndTimeSeconds - EpisodeStartTimeSeconds,
		0.0f);
	EpisodeSummary.Result = Result;
	bEpisodeActive = false;

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Boss episode: id=%s policy=%s result=%s "
			"duration=%.2f decisions=%d accepted=%d rejected=%d "
			"damage_dealt=%.1f damage_received=%.1f repeats=%d reward=%.3f"),
		*EpisodeSummary.EpisodeId,
		*StaticEnum<EAesirBossPolicySource>()->GetNameStringByValue(
			static_cast<int64>(EpisodeSummary.PolicySource)),
		*StaticEnum<EAesirBossEpisodeResult>()->GetNameStringByValue(
			static_cast<int64>(EpisodeSummary.Result)),
		EpisodeSummary.DurationSeconds,
		EpisodeSummary.DecisionCount,
		EpisodeSummary.AcceptedActionCount,
		EpisodeSummary.RejectedActionCount,
		EpisodeSummary.DamageDealt,
		EpisodeSummary.DamageReceived,
		EpisodeSummary.RepeatedActionPenaltyCount,
		EpisodeSummary.TotalReward);

	if (bExportCompletedEpisodes && !ExportEpisodeToDisk())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Failed to export Boss episode telemetry: %s"),
			*EpisodeSummary.EpisodeId);
	}

	return EpisodeSummary;
}

bool UAesirBossTelemetryComponent::ExportEpisodeToDisk() const
{
	const FString OutputDirectory = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("AesirTelemetry"),
		TEXT("BossEpisodes"));
	if (!IFileManager::Get().MakeDirectory(*OutputDirectory, true) &&
		!IFileManager::Get().DirectoryExists(*OutputDirectory))
	{
		return false;
	}

	TSharedPtr<FJsonObject> RootJson = MakeShared<FJsonObject>();
	RootJson->SetNumberField(TEXT("telemetry_schema_version"), 1);
	RootJson->SetStringField(
		TEXT("exported_at_utc"),
		FDateTime::UtcNow().ToIso8601());
	RootJson->SetNumberField(
		TEXT("observation_schema_version"),
		FAesirBossObservation::CurrentSchemaVersion);

	static const TCHAR* ObservationFeatureNames[] = {
		TEXT("boss_health_ratio"),
		TEXT("target_health_ratio"),
		TEXT("normalized_distance"),
		TEXT("facing_alignment"),
		TEXT("has_line_of_sight"),
		TEXT("boss_stunned"),
		TEXT("boss_attacking"),
		TEXT("target_blocking"),
		TEXT("target_attacking"),
		TEXT("target_dead"),
		TEXT("boss_poise_ratio"),
		TEXT("target_guard_pressure_ratio"),
		TEXT("target_dodging"),
		TEXT("recent_target_attack_rate"),
		TEXT("recent_target_block_rate"),
		TEXT("recent_target_dodge_rate"),
		TEXT("distance_trend"),
		TEXT("light_attack_available"),
		TEXT("heavy_attack_available"),
		TEXT("defend_available"),
		TEXT("dodge_available"),
		TEXT("pursue_available"),
		TEXT("disengage_available"),
		TEXT("use_ability_available"),
		TEXT("gap_closer_skill_available"),
		TEXT("unblockable_area_skill_available")
	};
	static_assert(
		UE_ARRAY_COUNT(ObservationFeatureNames) ==
			FAesirBossObservation::FeatureCount,
		"Telemetry feature names must match the observation schema.");
	TArray<TSharedPtr<FJsonValue>> FeatureNamesJson;
	FeatureNamesJson.Reserve(UE_ARRAY_COUNT(ObservationFeatureNames));
	for (const TCHAR* FeatureName : ObservationFeatureNames)
	{
		FeatureNamesJson.Add(MakeShared<FJsonValueString>(FeatureName));
	}
	RootJson->SetArrayField(
		TEXT("observation_feature_names"),
		FeatureNamesJson);
	RootJson->SetObjectField(
		TEXT("episode"),
		BuildSummaryJson(EpisodeSummary));

	TArray<TSharedPtr<FJsonValue>> DecisionsJson;
	DecisionsJson.Reserve(DecisionRecords.Num());
	for (const FAesirBossDecisionRecord& Record : DecisionRecords)
	{
		DecisionsJson.Add(
			MakeShared<FJsonValueObject>(BuildDecisionJson(Record)));
	}
	RootJson->SetArrayField(TEXT("decisions"), DecisionsJson);

	TArray<TSharedPtr<FJsonValue>> RewardEventsJson;
	RewardEventsJson.Reserve(RewardEvents.Num());
	for (const FAesirBossRewardEvent& Event : RewardEvents)
	{
		RewardEventsJson.Add(
			MakeShared<FJsonValueObject>(BuildRewardEventJson(Event)));
	}
	RootJson->SetArrayField(TEXT("reward_events"), RewardEventsJson);

	FString JsonText;
	const TSharedRef<TJsonWriter<>> Writer =
		TJsonWriterFactory<>::Create(&JsonText);
	if (!FJsonSerializer::Serialize(RootJson.ToSharedRef(), Writer))
	{
		return false;
	}

	const FString FileName = FString::Printf(
		TEXT("%s.json"),
		*FPaths::MakeValidFileName(EpisodeSummary.EpisodeId));
	const FString OutputPath = FPaths::Combine(OutputDirectory, FileName);
	const bool bSaved = FFileHelper::SaveStringToFile(
		JsonText,
		*OutputPath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	if (bSaved)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("Boss telemetry exported: %s"),
			*OutputPath);
	}
	return bSaved;
}

bool UAesirBossTelemetryComponent::IsEpisodeActive() const
{
	return bEpisodeActive;
}

TArray<FAesirBossDecisionRecord>
UAesirBossTelemetryComponent::GetDecisionRecords() const
{
	return DecisionRecords;
}

TArray<FAesirBossRewardEvent>
UAesirBossTelemetryComponent::GetRewardEvents() const
{
	return RewardEvents;
}

FAesirBossEpisodeSummary
UAesirBossTelemetryComponent::GetEpisodeSummary() const
{
	return EpisodeSummary;
}
