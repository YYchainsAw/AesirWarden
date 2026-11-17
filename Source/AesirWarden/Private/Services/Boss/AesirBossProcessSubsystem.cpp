// Copyright (c) 2026 YYchainsAw. All rights reserved.

#include "Services/Boss/AesirBossProcessSubsystem.h"

#include "AesirWarden.h"
#include "AesirBossRuntimeValidation.h"
#include "Async/Async.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformProperties.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "IPAddress.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

void UAesirBossProcessSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	const TCHAR* Section = TEXT("Aesir.BossProcess");
	GConfig->GetBool(Section, TEXT("Enabled"), bEnabled, GGameIni);
	bEnabled |= FParse::Param(FCommandLine::Get(), TEXT("AesirBossManaged"));
	if (!bEnabled) return;

	GConfig->GetString(Section, TEXT("ExecutablePath"), ExecutablePath, GGameIni);
	GConfig->GetString(Section, TEXT("ExecutableSHA256"), ExecutableSha256, GGameIni);
	GConfig->GetString(Section, TEXT("ManifestSHA256"), ManifestSha256, GGameIni);
	GConfig->GetInt(Section, TEXT("Port"), Port, GGameIni);
	GConfig->GetFloat(Section, TEXT("StartupTimeoutSeconds"), StartupTimeoutSeconds, GGameIni);
	GConfig->GetFloat(Section, TEXT("HealthTimeoutSeconds"), HealthTimeoutSeconds, GGameIni);
	FParse::Value(FCommandLine::Get(), TEXT("AesirBossExecutable="), ExecutablePath);
	if (ExecutablePath.IsEmpty())
	{
		ExecutablePath = FPlatformProperties::RequiresCookedData()
			? FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("BossPolicy/aesir-boss-policy.exe"))
			: FPaths::ProjectDir() / TEXT("ThirdParty/BossPolicy/Windows/aesir-boss-policy.exe");
	}
	ExecutablePath = FPaths::ConvertRelativePathToFull(ExecutablePath);
	StartupTimeoutSeconds = FMath::Clamp(StartupTimeoutSeconds, 1.0f, 300.0f);
	HealthTimeoutSeconds = FMath::Clamp(HealthTimeoutSeconds, 0.1f, 10.0f);
	State = EAesirBossProcessState::Stopped;
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &ThisClass::Tick), 0.25f);
	Start();
}

void UAesirBossProcessSubsystem::Start()
{
	if (!bEnabled || bDeinitializing || State == EAesirBossProcessState::Starting || IsReady()) return;
	ReleaseProcess();
	FailureReason.Reset();
	if (Port < 1024 || Port > 65535)
	{
		Fail(TEXT("invalid_port"));
		return;
	}
	State = EAesirBossProcessState::Starting;
	StartedAt = FPlatformTime::Seconds();
	const uint64 LaunchGeneration = ++Generation;
	const FString Path = ExecutablePath;
	const FString ExeHash = ExecutableSha256;
	const FString ManifestHash = ManifestSha256;
	const TWeakObjectPtr<ThisClass> WeakThis(this);
	UE_LOG(LogAesirWarden, Display, TEXT("Boss process validating: executable=%s"), *Path);
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis, LaunchGeneration, Path, ExeHash, ManifestHash]()
	{
		AesirBossRuntime::FIdentity Identity;
		FString Error;
		const bool bValid = AesirBossRuntime::ValidateBundle(Path, ExeHash, ManifestHash, Identity, Error);
		AsyncTask(ENamedThreads::GameThread, [WeakThis, LaunchGeneration, bValid, Identity, Error]()
		{
			ThisClass* Host = WeakThis.Get();
			if (!Host || Host->bDeinitializing || Host->Generation != LaunchGeneration ||
				Host->State != EAesirBossProcessState::Starting) return;
			if (!bValid)
			{
				Host->Fail(Error);
				return;
			}
			Host->ModelId = Identity.ModelId;
			Host->ModelSha256 = Identity.ModelSha256;
			Host->ReleaseId = Identity.ReleaseId;
			Host->SimulationRevision = Identity.SimulationRevision;
			Host->RewardRevision = Identity.RewardRevision;
			Host->LaunchProcess();
		});
	});
}

void UAesirBossProcessSubsystem::LaunchProcess()
{
	ISocketSubsystem* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	FSocket* Probe = Sockets ? Sockets->CreateSocket(NAME_Stream, TEXT("Boss service port reservation"), false) : nullptr;
	if (!Probe)
	{
		Fail(TEXT("port_probe_failed"));
		return;
	}
	TSharedRef<FInternetAddr> Address = Sockets->CreateInternetAddr();
	bool bValidAddress = false;
	Address->SetIp(TEXT("127.0.0.1"), bValidAddress);
	Address->SetPort(Port);
	Probe->SetReuseAddr(false);
	const bool bAvailable = bValidAddress && Probe->Bind(*Address);
	Sockets->DestroySocket(Probe);
	if (!bAvailable)
	{
		Fail(TEXT("port_in_use"));
		return;
	}
	ProcessLogPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectLogDir()
		/ FString::Printf(TEXT("BossProcess-%s.log"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(ProcessLogPath), true) ||
		!FPlatformProcess::CreatePipe(OutputRead, OutputWrite))
	{
		Fail(TEXT("process_log_unavailable"));
		return;
	}
	const FString Arguments = FString::Printf(TEXT("--host 127.0.0.1 --port %d"), Port);
	OwnedProcess = FPlatformProcess::CreateProc(*ExecutablePath, *Arguments, true, true, true,
		&OwnedProcessId, 0, *FPaths::GetPath(ExecutablePath), OutputWrite, nullptr, OutputWrite);
	FPlatformProcess::ClosePipe(nullptr, OutputWrite);
	OutputWrite = nullptr;
	if (!OwnedProcess.IsValid())
	{
		Fail(TEXT("process_launch_failed"));
		return;
	}
	UE_LOG(LogAesirWarden, Display, TEXT("Boss process started: pid=%u port=%d release=%s log=%s"),
		OwnedProcessId, Port, *ReleaseId, *ProcessLogPath);
}

bool UAesirBossProcessSubsystem::Tick(float)
{
	DrainOutput();
	if (OwnedProcess.IsValid() && !FPlatformProcess::IsProcRunning(OwnedProcess))
	{
		int32 ExitCode = -1;
		FPlatformProcess::GetProcReturnCode(OwnedProcess, &ExitCode);
		Fail(FString::Printf(TEXT("process_exited_%d"), ExitCode));
		return true;
	}
	if (State == EAesirBossProcessState::Starting)
	{
		if (FPlatformTime::Seconds() - StartedAt >= StartupTimeoutSeconds)
		{
			Fail(OwnedProcess.IsValid() ? TEXT("readiness_timeout") : TEXT("validation_timeout"));
		}
		else if (OwnedProcess.IsValid() && !HealthRequest.IsValid())
		{
			PollReadiness();
		}
	}
	return true;
}

void UAesirBossProcessSubsystem::PollReadiness()
{
	HealthRequest = FHttpModule::Get().CreateRequest();
	HealthRequest->SetURL(GetServiceBaseUrl() + TEXT("/health"));
	HealthRequest->SetVerb(TEXT("GET"));
	HealthRequest->SetTimeout(HealthTimeoutSeconds);
	HealthRequest->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnGameThread);
	HealthRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::HandleReadiness, Generation);
	if (!HealthRequest->ProcessRequest()) HealthRequest.Reset();
}

void UAesirBossProcessSubsystem::HandleReadiness(
	FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSucceeded, uint64 ExpectedGeneration)
{
	if (bDeinitializing || Generation != ExpectedGeneration || Request != HealthRequest) return;
	HealthRequest.Reset();
	if (State != EAesirBossProcessState::Starting || !OwnedProcess.IsValid() ||
		!FPlatformProcess::IsProcRunning(OwnedProcess)) return;
	if (!bSucceeded || !Response.IsValid() || Response->GetResponseCode() != 200) return;
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response->GetContentAsString()), Root) || !Root.IsValid())
	{
		Fail(TEXT("invalid_health_json"));
		return;
	}
	auto Matches = [&Root](const TCHAR* Key, const FString& Expected)
	{
		FString Value;
		return Root->TryGetStringField(Key, Value) && Value == Expected;
	};
	bool bLoaded = false;
	double Schema = 0, Observations = 0, Actions = 0;
	if (!Matches(TEXT("status"), TEXT("ok")) || !Matches(TEXT("service"), TEXT("aesir-boss-policy-service")) ||
		!Matches(TEXT("protocol_version"), TEXT("1.0")) || !Matches(TEXT("model_id"), ModelId) ||
		!Matches(TEXT("release_id"), ReleaseId) || !Matches(TEXT("model_sha256"), ModelSha256) ||
		!Matches(TEXT("simulation_revision"), SimulationRevision) || !Matches(TEXT("reward_revision"), RewardRevision) ||
		!Root->TryGetBoolField(TEXT("loaded"), bLoaded) || !bLoaded ||
		!Root->TryGetNumberField(TEXT("schema_version"), Schema) || Schema != 4 ||
		!Root->TryGetNumberField(TEXT("observation_dimension"), Observations) || Observations != 26 ||
		!Root->TryGetNumberField(TEXT("action_count"), Actions) || Actions != 9)
	{
		Fail(TEXT("health_identity_mismatch"));
		return;
	}
	State = EAesirBossProcessState::Ready;
	UE_LOG(LogAesirWarden, Display, TEXT("Boss process ready: pid=%u release=%s model=%s sha256=%s startup_seconds=%.2f"),
		OwnedProcessId, *ReleaseId, *ModelId, *ModelSha256, FPlatformTime::Seconds() - StartedAt);
}

void UAesirBossProcessSubsystem::DrainOutput()
{
	if (!OutputRead) return;
	const FString Output = FPlatformProcess::ReadPipe(OutputRead);
	if (!Output.IsEmpty())
	{
		FFileHelper::SaveStringToFile(Output, *ProcessLogPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
			&IFileManager::Get(), FILEWRITE_Append);
	}
}

void UAesirBossProcessSubsystem::ReleaseProcess()
{
	++Generation;
	if (HealthRequest.IsValid())
	{
		HealthRequest->OnProcessRequestComplete().Unbind();
		HealthRequest->CancelRequest();
		HealthRequest.Reset();
	}
	if (OwnedProcess.IsValid())
	{
		UE_LOG(LogAesirWarden, Display, TEXT("Boss process stopping: owned_pid=%u"), OwnedProcessId);
		FPlatformProcess::TerminateProc(OwnedProcess, true);
		FPlatformProcess::CloseProc(OwnedProcess);
		OwnedProcessId = 0;
	}
	DrainOutput();
	FPlatformProcess::ClosePipe(OutputRead, OutputWrite);
	OutputRead = OutputWrite = nullptr;
}

void UAesirBossProcessSubsystem::Fail(const FString& Reason)
{
	ReleaseProcess();
	FailureReason = Reason;
	State = EAesirBossProcessState::Failed;
	UE_LOG(LogAesirWarden, Warning, TEXT("Boss process unavailable: reason=%s log=%s"), *Reason, *ProcessLogPath);
}

void UAesirBossProcessSubsystem::Stop()
{
	ReleaseProcess();
	State = bEnabled ? EAesirBossProcessState::Stopped : EAesirBossProcessState::Disabled;
}

void UAesirBossProcessSubsystem::Deinitialize()
{
	bDeinitializing = true;
	FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	Stop();
	Super::Deinitialize();
}

FString UAesirBossProcessSubsystem::GetServiceBaseUrl() const
{
	return FString::Printf(TEXT("http://127.0.0.1:%d"), Port);
}

bool UAesirBossProcessSubsystem::MatchesModel(
	const FString& InModelId, const FString& InReleaseId, const FString& InModelSha256) const
{
	return IsReady() && ModelId == InModelId && ReleaseId == InReleaseId && ModelSha256 == InModelSha256;
}
