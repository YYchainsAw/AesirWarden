// Copyright (c) 2026 YYchainsAw. All rights reserved.

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "AesirWarden.h"
#include "Abilities/AesirAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AI/Boss/AesirBossAIController.h"
#include "AI/Boss/AesirBossPolicyClientComponent.h"
#include "AI/Boss/AesirBossTelemetryComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "BrainComponent.h"
#include "Combat/AesirHealthComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/AesirCombatPrototypeGameMode.h"
#include "Framework/AesirCombatPrototypePlayerController.h"
#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "UI/AesirCombatResultWidget.h"
#include "UI/AesirMainMenuWidget.h"
#include "UObject/Package.h"
#include "UnrealClient.h"

namespace AesirMainMatchSmoke
{
class FRun final : public TSharedFromThis<FRun>
{
public:
	FRun(UWorld* InWorld, EAesirMatchResult InExpected)
		: World(InWorld), Expected(InExpected), StartedAt(FPlatformTime::Seconds())
	{
	}

	void Begin()
	{
		if (World->GetOutermost()->GetName() != TEXT("/Game/Main"))
		{
			Finish(TEXT("Load /Game/Main for this verification."));
			return;
		}

		const FString RunName = FString::Printf(TEXT("%s-%s"), CaseName(),
			*FGuid::NewGuid().ToString(EGuidFormats::Digits));
		const FString Directory = FPaths::ConvertRelativePathToFull(
			FPaths::ProjectSavedDir() / TEXT("Verification/MainMatch") / RunName);
		if (!IFileManager::Get().MakeDirectory(*Directory, true))
		{
			Finish(TEXT("Cannot create a verification screenshot directory."));
			return;
		}
		Screenshot = Directory / TEXT("Result.png");
		UE_LOG(LogAesirWarden, Display, TEXT("MATCH_MAIN_SMOKE_BEGIN: case=%s screenshot=%s"), CaseName(), *Screenshot);

		// Match presentation can pause the world. Core ticker still runs and
		// lets us verify the Slate frame and screenshot after the transition.
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateSP(AsShared(), &FRun::Tick));
	}

	bool IsFinished() const { return bFinished; }

private:
	enum class EStage : uint8
	{
		Menu,
		Exploration,
		Result,
		Screenshot
	};

	const TCHAR* CaseName() const { return Expected == EAesirMatchResult::Victory ? TEXT("Victory") : TEXT("Defeat"); }

	bool Finish(const TCHAR* Error = nullptr)
	{
		bFinished = true;
		if (AAesirCombatPrototypeGameMode* GameMode = Mode.Get())
		{
			GameMode->OnMatchFinished.Remove(MatchFinishedHandle);
		}
		if (Error)
		{
			UE_LOG(LogAesirWarden, Error, TEXT("MATCH_MAIN_SMOKE_FAIL: case=%s stage=%d detail=%s"),
				CaseName(), static_cast<int32>(Stage), Error);
		}
		else
		{
			UE_LOG(LogAesirWarden, Display,
				TEXT("MATCH_MAIN_SMOKE_PASS: case=%s boss=%s episode=%s viewport_count=1 result_events=%d retry_focused=true screenshot=%s"),
				CaseName(), *BossName, *EpisodeId, ResultEvents, *Screenshot);
		}
		FPlatformMisc::RequestExitWithStatus(false, Error ? 1 : 0);
		return false;
	}

	void OnMatchFinished(EAesirMatchResult Result)
	{
		++ResultEvents;
		ReportedResult = Result;
	}

	bool BeginMatch(AAesirCombatPrototypeGameMode* GameMode, AAesirCombatPrototypePlayerController* Controller)
	{
		AActor* Boss = GameMode->GetMatchBoss();
		if (!IsValid(Boss)) { return true; }
		APawn* BossPawn = Cast<APawn>(Boss);
		AAesirBossAIController* BossAI = BossPawn ? Cast<AAesirBossAIController>(BossPawn->GetController()) : nullptr;
		if (!IsValid(BossAI)) { return true; }
		const UAesirBossTelemetryComponent* Telemetry = BossAI->GetBossTelemetryComponent();
		if (!Telemetry || !Telemetry->IsEpisodeActive()) { return true; }

		AActor* Loser = Expected == EAesirMatchResult::Victory ? Boss : static_cast<AActor*>(Controller->GetPawn());
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Loser);
		const FGameplayAttribute HealthAttribute = UAesirAttributeSet::GetHealthAttribute();
		const UAesirHealthComponent* Health = Loser ? Loser->FindComponentByClass<UAesirHealthComponent>() : nullptr;
		if (!ASC || !ASC->HasAttributeSetForAttribute(HealthAttribute) || !Health ||
			Health->IsDead() || ASC->GetNumericAttribute(HealthAttribute) <= 0.0f)
		{
			return Finish(TEXT("The selected actor needs a living initialized GAS health component."));
		}
		if (!Director->BeginBossCombat(Boss) || Director->GetFlowState() != EAesirLevelFlowState::BossCombat)
		{
			return Finish(TEXT("Main cannot begin the designated Boss encounter."));
		}

		BossController = BossAI;
		BossName = Boss->GetPathName();
		EpisodeId = Telemetry->GetEpisodeSummary().EpisodeId;
		if (EpisodeId.IsEmpty())
		{
			return Finish(TEXT("The active Boss episode has no identity."));
		}
		UE_LOG(LogAesirWarden, Display, TEXT("MATCH_MAIN_SMOKE_ENCOUNTER: case=%s boss=%s loser=%s episode=%s"),
			CaseName(), *BossName, *Loser->GetPathName(), *EpisodeId);

		// Exercise the actual character GAS delegate and match adjudication. A
		// direct match/Flow setter would bypass the death path being verified.
		DeathFrame = GFrameCounter;
		ASC->SetNumericAttributeBase(HealthAttribute, 0.0f);
		if (!Health->IsDead() || ASC->GetNumericAttribute(HealthAttribute) > 0.0f)
		{
			return Finish(TEXT("The GAS health change did not reach the character health component."));
		}
		if (GameMode->GetMatchResult() != EAesirMatchResult::None || ResultEvents != 0)
		{
			return Finish(TEXT("The result was published before the death frame ended."));
		}
		Stage = EStage::Result;
		return true;
	}

	bool VerifyResult(AAesirCombatPrototypeGameMode* GameMode, AAesirCombatPrototypePlayerController* Controller)
	{
		if (GameMode->GetMatchResult() == EAesirMatchResult::None) { return true; }
		if (GameMode->GetMatchResult() != Expected || ReportedResult != Expected || ResultEvents != 1 ||
			GFrameCounter <= DeathFrame || Director->GetFlowState() != EAesirLevelFlowState::Results)
		{
			return Finish(TEXT("The expected result, one finish event and the Results phase must agree after the death frame."));
		}

		UAesirCombatResultWidget* Widget = Controller->GetCombatResultWidget();
		if (!Widget || Widget->GetClass()->GetPathName() != TEXT("/Game/Aesir/UI/Results/WBP_CombatResult.WBP_CombatResult_C") ||
			Widget->GetDisplayedResult() != Expected || Widget->GetOwningPlayer() != Controller ||
			!Widget->IsInViewport() || !Widget->IsVisible())
		{
			return Finish(TEXT("The local controller must display the authored WBP_CombatResult with the matching result."));
		}
		TArray<UUserWidget*> Widgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World.Get(), Widgets, UAesirCombatResultWidget::StaticClass(), true);
		if (Widgets.Num() != 1 || Widgets[0] != Widget)
		{
			return Finish(TEXT("Expected exactly one result page in the viewport."));
		}
		if (!Controller->IsMoveInputIgnored() || !Controller->IsLookInputIgnored() ||
			!Controller->bShowMouseCursor || Controller->IsCompanionChatOpen())
		{
			return Finish(TEXT("Results must lock gameplay input, close chat and show the UI cursor."));
		}
		const UButton* Retry = Cast<UButton>(Widget->GetWidgetFromName(TEXT("BTN_Restart")));
		const UButton* Return = Cast<UButton>(Widget->GetWidgetFromName(TEXT("BTN_ReturnToMenu")));
		const UButton* Quit = Cast<UButton>(Widget->GetWidgetFromName(TEXT("BTN_Quit")));
		const UTextBlock* Label = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("TXT_Result")));
		const FText ExpectedLabel = Expected == EAesirMatchResult::Victory
			? NSLOCTEXT("AesirResult", "Victory", "VICTORY") : NSLOCTEXT("AesirResult", "Defeat", "DEFEAT");
		if (!Retry || !Retry->IsVisible() || !Retry->GetIsEnabled() ||
			!Return || !Return->IsVisible() || !Return->GetIsEnabled() || Return->GetParent() != Retry->GetParent() ||
			!Quit || !Quit->IsVisible() || !Quit->GetIsEnabled() || Quit->GetParent() != Retry->GetParent() || !Label ||
			!Label->IsVisible() || !Label->GetText().EqualTo(ExpectedLabel))
		{
			return Finish(TEXT("The result treatment or default action is unavailable."));
		}

		const AAesirBossAIController* BossAI = BossController.Get();
		if (!BossAI) { return Finish(TEXT("The designated Boss controller disappeared before its result was verified.")); }
		const UBrainComponent* Brain = BossAI->GetBrainComponent();
		const UAesirBossTelemetryComponent* Telemetry = BossAI->GetBossTelemetryComponent();
		const UAesirBossPolicyClientComponent* Policy = BossAI->GetBossPolicyClientComponent();
		const EAesirBossEpisodeResult EpisodeResult = Expected == EAesirMatchResult::Victory
			? EAesirBossEpisodeResult::BossDefeat : EAesirBossEpisodeResult::BossVictory;
		if ((Brain && Brain->IsRunning()) || !Policy || Policy->IsRunning() ||
			!Telemetry || Telemetry->IsEpisodeActive() || Telemetry->GetEpisodeSummary().Result != EpisodeResult ||
			Telemetry->GetEpisodeSummary().EpisodeId != EpisodeId)
		{
			return Finish(TEXT("Boss decisions must stop and the original episode must record the correct terminal result."));
		}

		// Let Slate consume the focus request after the result page is attached.
		if (!Retry->HasUserFocus(Controller)) { return true; }
		if (Stage == EStage::Result)
		{
			AActor* Survivor = Expected == EAesirMatchResult::Victory
				? static_cast<AActor*>(Controller->GetPawn()) : GameMode->GetMatchBoss();
			auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Survivor);
			if (!ASC) { return Finish(TEXT("Cannot verify late damage on the surviving fighter.")); }
			const FGameplayAttribute Health = UAesirAttributeSet::GetHealthAttribute();
			const float Before = ASC->GetNumericAttribute(Health);
			auto* LateDamage = NewObject<UGameplayEffect>();
			LateDamage->DurationPolicy = EGameplayEffectDurationType::Instant;
			FGameplayModifierInfo& Modifier = LateDamage->Modifiers.AddDefaulted_GetRef();
			Modifier.Attribute = UAesirAttributeSet::GetDamageAttribute();
			Modifier.ModifierOp = EGameplayModOp::Additive;
			Modifier.ModifierMagnitude = FScalableFloat(25.0f);
			ASC->ApplyGameplayEffectToSelf(LateDamage, 1.0f, ASC->MakeEffectContext());
			if (Before <= 0.0f || ASC->GetNumericAttribute(Health) != Before || ResultEvents != 1)
			{
				return Finish(TEXT("A late GAS damage effect changed health or duplicated the terminal result."));
			}
			FScreenshotRequest::RequestScreenshot(Screenshot, true, false);
			Stage = EStage::Screenshot;
			return true;
		}
		return IFileManager::Get().FileSize(*Screenshot) > 0 ? Finish() : true;
	}

	bool Tick(float)
	{
		if (!World.IsValid() || FPlatformTime::Seconds() - StartedAt > 90.0)
		{
			return Finish(TEXT("Timed out waiting for Main, the result page/default focus or a rendered screenshot."));
		}
		AAesirCombatPrototypeGameMode* GameMode = World->GetAuthGameMode<AAesirCombatPrototypeGameMode>();
		auto* Controller = Cast<AAesirCombatPrototypePlayerController>(World->GetFirstPlayerController());
		if (!GameMode || !Controller) { return true; }
		if (!Director.IsValid())
		{
			for (TActorIterator<AAesirLevelFlowDirector> It(World.Get()); It; ++It) { Director = *It; break; }
			if (!Director.IsValid()) { return true; }
		}

		const EAesirLevelFlowState Phase = Director->GetFlowState();
		if (Stage == EStage::Menu)
		{
			if (Phase != EAesirLevelFlowState::MainMenu) { return true; }
			TArray<UUserWidget*> Menus;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World.Get(), Menus, UAesirMainMenuWidget::StaticClass(), true);
			const UWidget* Start = Menus.Num() == 1 ? Menus[0]->GetWidgetFromName(TEXT("Btn_Start")) : nullptr;
			if (!Start || !Start->GetIsEnabled()) { return true; }
			Mode = GameMode;
			MatchFinishedHandle = GameMode->OnMatchFinished.AddSP(AsShared(), &FRun::OnMatchFinished);
			if (!Director->RequestStartGame()) { return Finish(TEXT("The prepared Main menu rejected Start.")); }
			Stage = EStage::Exploration;
			return true;
		}

		if (Stage == EStage::Exploration)
		{
			if (Phase == EAesirLevelFlowState::OpeningCinematic && !bSkippedOpening)
			{
				bSkippedOpening = Director->SkipOpeningCinematic();
			}
			return Phase == EAesirLevelFlowState::Exploration && Controller->GetPawn()
				? BeginMatch(GameMode, Controller) : true;
		}
		return VerifyResult(GameMode, Controller);
	}

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AAesirCombatPrototypeGameMode> Mode;
	TWeakObjectPtr<AAesirLevelFlowDirector> Director;
	TWeakObjectPtr<AAesirBossAIController> BossController;
	FDelegateHandle MatchFinishedHandle;
	FString Screenshot;
	FString BossName;
	FString EpisodeId;
	EAesirMatchResult Expected;
	EAesirMatchResult ReportedResult = EAesirMatchResult::None;
	EStage Stage = EStage::Menu;
	double StartedAt;
	uint64 DeathFrame = 0;
	int32 ResultEvents = 0;
	bool bSkippedOpening = false;
	bool bFinished = false;
};

TSharedPtr<FRun> ActiveRun;

FAutoConsoleCommandWithWorldAndArgs RunCommand(
	TEXT("Aesir.Match.Smoke"),
	TEXT("Aesir.Match.Smoke Victory|Defeat. Verify the authored Main result page and exit the standalone -game process."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (GIsEditor || !World || World->WorldType != EWorldType::Game)
		{
			UE_LOG(LogAesirWarden, Error, TEXT("MATCH_MAIN_SMOKE_FAIL: use a standalone UnrealEditor /Game/Main -game process."));
			return;
		}
		if (ActiveRun && !ActiveRun->IsFinished())
		{
			UE_LOG(LogAesirWarden, Warning, TEXT("A Main match smoke test is already running."));
			return;
		}
		if (Args.Num() != 1 || (!Args[0].Equals(TEXT("Victory"), ESearchCase::IgnoreCase) &&
			!Args[0].Equals(TEXT("Defeat"), ESearchCase::IgnoreCase)))
		{
			UE_LOG(LogAesirWarden, Error, TEXT("MATCH_MAIN_SMOKE_FAIL: expected one argument, Victory or Defeat."));
			FPlatformMisc::RequestExitWithStatus(false, 1);
			return;
		}
		const EAesirMatchResult Expected = Args[0].Equals(TEXT("Victory"), ESearchCase::IgnoreCase)
			? EAesirMatchResult::Victory : EAesirMatchResult::Defeat;
		ActiveRun = MakeShared<FRun>(World, Expected);
		ActiveRun->Begin();
	}));
}

#endif
