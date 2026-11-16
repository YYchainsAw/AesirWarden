// Copyright (c) 2026 YYchainsAw. All rights reserved.

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "AesirWarden.h"
#include "Abilities/AesirAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AI/Boss/AesirBossAIController.h"
#include "AI/Boss/AesirBossTelemetryComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "BrainComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Combat/AesirHealthComponent.h"
#include "Components/Button.h"
#include "Components/ChildActorComponent.h"
#include "Containers/Ticker.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/AesirCombatPrototypeGameMode.h"
#include "Framework/AesirCombatPrototypePlayerController.h"
#include "Framework/Flow/AesirCampfireStart.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/Paths.h"
#include "Tags/AesirGameplayTags.h"
#include "UI/AesirCombatHUDWidget.h"
#include "UI/AesirCombatResultWidget.h"
#include "UI/AesirMainMenuWidget.h"
#include "UObject/Package.h"
#include "UnrealClient.h"

namespace AesirMainRetrySmoke
{
class FRun final : public TSharedFromThis<FRun>
{
public:
	explicit FRun(UWorld* InitialWorld)
		: GameInstance(InitialWorld->GetGameInstance()), World(InitialWorld), StartedAt(FPlatformTime::Seconds())
	{
	}

	void Begin()
	{
		ScreenshotDirectory = FPaths::ConvertRelativePathToFull(
			FPaths::ProjectSavedDir() / TEXT("Verification/Retry") / FGuid::NewGuid().ToString(EGuidFormats::Digits));
		IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
		CleanupHandle = FWorldDelegates::OnWorldCleanup.AddSP(AsShared(), &FRun::OnWorldCleanup);
		UE_LOG(LogAesirWarden, Display, TEXT("MATCH_RETRY_SMOKE_BEGIN: Main, two retries and one return to menu"));
		// The core ticker stays alive while the result screen pauses gameplay and
		// when OpenLevel replaces the world in the same game instance.
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateSP(AsShared(), &FRun::Tick));
	}

private:
	enum class EStage : uint8 { Menu, Exploration, BossCombat, Result, Travel };

	static bool IsMain(const UWorld* InWorld)
	{
		return InWorld && InWorld->WorldType == EWorldType::Game &&
			InWorld->GetOutermost()->GetName() == TEXT("/Game/Main");
	}

	static UAbilitySystemComponent* AbilitySystem(AActor* Actor)
	{
		return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	}

	static bool HasFreshHealth(const AActor* Actor, const UAbilitySystemComponent* ASC)
	{
		const auto* Health = Actor ? Actor->FindComponentByClass<UAesirHealthComponent>() : nullptr;
		return Health && ASC && !Health->IsDead() && !Health->IsInvulnerable() &&
			FMath::IsNearlyEqual(Health->GetCurrentHealth(), Health->GetMaxHealth()) &&
			FMath::IsNearlyEqual(ASC->GetNumericAttribute(UAesirAttributeSet::GetHealthAttribute()),
				ASC->GetNumericAttribute(UAesirAttributeSet::GetMaxHealthAttribute())) &&
			!ASC->HasMatchingGameplayTag(AesirGameplayTags::State_Dead);
	}

	bool Finish(const TCHAR* Error = nullptr)
	{
		FWorldDelegates::OnWorldCleanup.Remove(CleanupHandle);
		if (Mode.IsValid())
		{
			Mode->OnMatchFinished.Remove(MatchFinishedHandle);
		}
		if (Error)
		{
			UE_LOG(LogAesirWarden, Error, TEXT("MATCH_RETRY_SMOKE_FAIL: round=%d stage=%d detail=%s"),
				Round, static_cast<int32>(Stage), Error);
		}
		else
		{
			UE_LOG(LogAesirWarden, Display,
				TEXT("MATCH_RETRY_SMOKE_PASS: retries=2 returned_to_menu=true restarted=true defeats=%d unique_episodes=%d retired_worlds_checked=%d stale_delegates_replayed=%d"),
				Defeats, Episodes.Num(), RetiredWorldsChecked, StaleDelegatesReplayed);
		}
		FPlatformMisc::RequestExitWithStatus(false, Error ? 1 : 0);
		return false;
	}

	void OnMatchFinished(EAesirMatchResult Result)
	{
		++ResultEvents;
		EventResult = Result;
	}

	void OnWorldCleanup(UWorld* RetiringWorld, bool, bool)
	{
		if (Stage != EStage::Travel || RetiringWorld != World.Get())
		{
			return;
		}
		const auto* OldMode = RetiredMode.Get(true);
		const auto* OldPlayer = RetiredPlayerHealth.Get(true);
		const auto* OldBoss = RetiredBossHealth.Get(true);
		const bool bPlayerBound = OldMode && OldPlayer && OldPlayer->OnDeath.Contains(OldMode, TEXT("HandlePlayerDeath"));
		const bool bBossBound = OldMode && OldBoss && OldBoss->OnDeath.Contains(OldMode, TEXT("HandleEnemyDeath"));
		bRetiredWorldStillBound |= bPlayerBound || bBossBound;
		if (LastRetiredRound != Round)
		{
			LastRetiredRound = Round;
			++RetiredWorldsChecked;
		}
		UE_LOG(LogAesirWarden, Display, TEXT("MATCH_RETRY_SMOKE_CLEANUP: round=%d count=%d player_bound=%d boss_bound=%d"),
			Round, RetiredWorldsChecked, bPlayerBound, bBossBound);
	}

	bool CaptureCheckpoint(const TCHAR* Name)
	{
		const FString Filename = ScreenshotDirectory / (FString(Name) + TEXT(".png"));
		if (IFileManager::Get().FileSize(*Filename) > 0)
		{
			UE_LOG(LogAesirWarden, Display, TEXT("MATCH_RETRY_SMOKE_SCREENSHOT: %s"), *Filename);
			return true;
		}
		if (PendingScreenshot != Filename)
		{
			PendingScreenshot = Filename;
			FScreenshotRequest::RequestScreenshot(Filename, true, false);
		}
		return false;
	}

	bool HasOldResult(AAesirCombatPrototypePlayerController* Controller) const
	{
		TArray<UUserWidget*> Results;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World.Get(), Results, UAesirCombatResultWidget::StaticClass(), true);
		return Controller->GetCombatResultWidget() || Results.Num() != 0;
	}

	bool AwaitMenu(AAesirCombatPrototypePlayerController* Controller)
	{
		const EAesirLevelFlowState Phase = Director->GetFlowState();
		if (Phase == EAesirLevelFlowState::Uninitialized) return true;
		if (Phase != EAesirLevelFlowState::MainMenu)
		{
			return Finish(TEXT("A round entered gameplay before the start button was pressed."));
		}

		TArray<UUserWidget*> Menus;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World.Get(), Menus, UAesirMainMenuWidget::StaticClass(), true);
		auto* Start = Menus.Num() == 1 ? Cast<UButton>(Menus[0]->GetWidgetFromName(TEXT("Btn_Start"))) : nullptr;
		if (!Start || !Start->GetIsEnabled() || !Start->IsVisible() || !Start->HasUserFocus(Controller)) return true;

		const auto* Campfire = Cast<AAesirCampfireStart>(Director.Get());
		const AActor* Camera = Campfire && Campfire->MenuCameraChild ? Campfire->MenuCameraChild->GetChildActor() : nullptr;
		if (!Camera || Controller->GetViewTarget() != Camera || !Controller->bShowMouseCursor ||
			!Controller->IsMoveInputIgnored() || !Controller->IsLookInputIgnored() ||
			Mode->GetMatchResult() != EAesirMatchResult::None || HasOldResult(Controller))
		{
			return Finish(TEXT("The campfire menu did not reclaim camera, focus and input from the previous game."));
		}
		if (Round == 3 && !CaptureCheckpoint(TEXT("ReturnToMenu"))) return true;

		UE_LOG(LogAesirWarden, Display, TEXT("MATCH_RETRY_SMOKE_MENU: round=%d start_focused=true camera=%s"),
			Round, *Camera->GetName());
		Start->OnClicked.Broadcast();
		Stage = EStage::Exploration;
		return true;
	}

	bool AwaitExploration(AAesirCombatPrototypePlayerController* Controller)
	{
		const EAesirLevelFlowState Phase = Director->GetFlowState();
		const bool bIsRetry = Round == 1 || Round == 2;
		if (bIsRetry && (Phase == EAesirLevelFlowState::MainMenu || Phase == EAesirLevelFlowState::OpeningCinematic))
		{
			return Finish(TEXT("Retry exposed a menu or opening cinematic instead of proceeding directly to exploration."));
		}
		if (!bIsRetry && Phase == EAesirLevelFlowState::OpeningCinematic && !bSkippedOpening)
		{
			bSkippedOpening = Director->SkipOpeningCinematic();
		}
		if (Phase != EAesirLevelFlowState::Exploration) return true;

		auto* Player = Cast<ACharacter>(Controller->GetPawn());
		auto* Boss = Cast<APawn>(Mode->GetMatchBoss());
		auto* BossController = Boss ? Cast<AAesirBossAIController>(Boss->GetController()) : nullptr;
		if (!Player || !BossController) return true;
		if (Boss == RetiredBoss.Get())
		{
			return Finish(TEXT("Exploration retained the previous round's Boss actor."));
		}
		auto* PlayerASC = AbilitySystem(Player);
		auto* BossASC = AbilitySystem(Boss);
		auto* Telemetry = BossController->GetBossTelemetryComponent();
		auto* Brain = BossController->GetBrainComponent();
		if (!HasFreshHealth(Player, PlayerASC) || !HasFreshHealth(Boss, BossASC) ||
			PlayerASC->HasMatchingGameplayTag(CooldownTag) || Player->GetCharacterMovement()->MovementMode == MOVE_None ||
			HasOldResult(Controller) || Controller->IsCompanionChatOpen() ||
			Controller->IsMoveInputIgnored() || Controller->IsLookInputIgnored() ||
			Controller->bShowMouseCursor || Controller->GetViewTarget() != Player ||
			Mode->GetMatchResult() != EAesirMatchResult::None || ResultEvents != 0)
		{
			return Finish(TEXT("Fresh exploration retained combat state, a cooldown, result UI, or locked player controls."));
		}
		if (!Telemetry || Telemetry->IsEpisodeActive() || (Brain && Brain->IsRunning()))
		{
			return Finish(TEXT("Boss AI or an Episode started before the new encounter."));
		}
		TArray<UUserWidget*> HUD;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World.Get(), HUD, UAesirCombatHUDWidget::StaticClass(), true);
		if (HUD.Num() != 1 || !HUD[0]->IsVisible() || HUD[0]->GetOwningPlayer() != Controller)
		{
			return Finish(TEXT("Exploration did not restore the player's combat HUD."));
		}

		const FVector PlayerLocation = Player->GetActorLocation();
		if (Round == 0)
		{
			FirstCampfireSpawn = PlayerLocation;
		}
		else if (FVector::Dist2D(PlayerLocation, FirstCampfireSpawn) > 150.0 ||
			FMath::Abs(PlayerLocation.Z - FirstCampfireSpawn.Z) > 300.0)
		{
			return Finish(TEXT("The new player did not start at the original campfire."));
		}
		if (bIsRetry)
		{
			if (const auto* Camera = Controller->PlayerCameraManager.Get(); Camera && Camera->bEnableFading)
			{
				return true;
			}
			if (!CaptureCheckpoint(Round == 1 ? TEXT("Retry1") : TEXT("Retry2"))) return true;
		}

		UE_LOG(LogAesirWarden, Display,
			TEXT("MATCH_RETRY_SMOKE_FRESH: round=%d spawn=%s player_hp=%.1f boss_hp=%.1f old_cooldowns=0 old_results=0"),
			Round, *PlayerLocation.ToCompactString(),
			PlayerASC->GetNumericAttribute(UAesirAttributeSet::GetHealthAttribute()),
			BossASC->GetNumericAttribute(UAesirAttributeSet::GetHealthAttribute()));
		if (!Director->BeginBossCombat(Boss) || Director->GetFlowState() != EAesirLevelFlowState::BossCombat ||
			Director->BeginBossCombat(Boss))
		{
			return Finish(TEXT("The Boss encounter could not be entered exactly once."));
		}
		CurrentBoss = Boss;
		CurrentBossController = BossController;
		Stage = EStage::BossCombat;
		return true;
	}

	bool AwaitBossCombat(AAesirCombatPrototypePlayerController* Controller)
	{
		auto* BossController = CurrentBossController.Get();
		auto* Telemetry = BossController ? BossController->GetBossTelemetryComponent() : nullptr;
		if (!Telemetry || !Telemetry->IsEpisodeActive()) return true;
		const FAesirBossEpisodeSummary Summary = Telemetry->GetEpisodeSummary();
		const auto* Brain = BossController->GetBrainComponent();
		if (!Brain || !Brain->IsRunning() || Summary.PolicySource != EAesirBossPolicySource::BehaviorTree ||
			Summary.Result != EAesirBossEpisodeResult::InProgress || Summary.EpisodeId.IsEmpty() ||
			Episodes.Contains(Summary.EpisodeId) || Mode->GetMatchResult() != EAesirMatchResult::None)
		{
			return Finish(TEXT("Encounter did not start a fresh running Behavior Tree Episode."));
		}
		Episodes.Add(Summary.EpisodeId);
		CurrentEpisode = Summary.EpisodeId;
		UE_LOG(LogAesirWarden, Display, TEXT("MATCH_RETRY_SMOKE_EPISODE: round=%d episode=%s"), Round, *CurrentEpisode);

		// After Return to Menu, Start must reach a playable fourth encounter.
		if (Round == 3)
		{
			return Defeats == 3 && Episodes.Num() == 4
				? Finish() : Finish(TEXT("Expected three defeats and four unique encounters."));
		}

		APawn* Player = Controller->GetPawn();
		auto* PlayerASC = AbilitySystem(Player);
		auto* BossASC = AbilitySystem(CurrentBoss.Get());
		auto* Health = Player ? Player->FindComponentByClass<UAesirHealthComponent>() : nullptr;
		if (!PlayerASC || !BossASC || !Health || Health->IsDead())
		{
			return Finish(TEXT("The fight needs two live GAS fighters before injecting defeat."));
		}

		// Leave persistent per-run state to prove world replacement removes it.
		auto* Cooldown = NewObject<UGameplayEffect>();
		Cooldown->DurationPolicy = EGameplayEffectDurationType::Infinite;
		FInheritedTagContainer GrantedTags;
		GrantedTags.Added.AddTag(CooldownTag);
		Cooldown->AddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(GrantedTags);
		const FActiveGameplayEffectHandle Handle =
			PlayerASC->ApplyGameplayEffectToSelf(Cooldown, 1.0f, PlayerASC->MakeEffectContext());
		if (!Handle.IsValid() || !PlayerASC->HasMatchingGameplayTag(CooldownTag))
		{
			return Finish(TEXT("GAS refused the persistent cooldown test fixture."));
		}
		const FGameplayAttribute HealthAttribute = UAesirAttributeSet::GetHealthAttribute();
		BossASC->SetNumericAttributeBase(HealthAttribute,
			FMath::Max(1.0f, BossASC->GetNumericAttribute(HealthAttribute) - 25.0f));

		// Force a different defeat location. A reset in place cannot pass the
		// next world's distance check. This is not a traversal test.
		Player->SetActorLocation(FirstCampfireSpawn + FVector(500.0, 0.0, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
		if (FVector::Dist2D(Player->GetActorLocation(), FirstCampfireSpawn) < 300.0)
		{
			return Finish(TEXT("Cannot set a defeat location outside the campfire start area."));
		}

		PlayerASC->SetNumericAttributeBase(HealthAttribute, 0.0f);
		if (!Health->IsDead() || Mode->GetMatchResult() != EAesirMatchResult::None)
		{
			return Finish(TEXT("The lethal GAS change did not reach the death delegate and deferred match adjudication."));
		}
		Stage = EStage::Result;
		return true;
	}

	bool AwaitResult(AAesirCombatPrototypePlayerController* Controller)
	{
		if (Mode->GetMatchResult() == EAesirMatchResult::None) return true;
		const auto* BossController = CurrentBossController.Get();
		const auto* Telemetry = BossController ? BossController->GetBossTelemetryComponent() : nullptr;
		auto* Widget = Controller->GetCombatResultWidget();
		auto* Button = Widget ? Cast<UButton>(Widget->GetWidgetFromName(Round < 2
			? TEXT("BTN_Restart") : TEXT("BTN_ReturnToMenu"))) : nullptr;
		if (Mode->GetMatchResult() != EAesirMatchResult::Defeat || ResultEvents != 1 ||
			EventResult != EAesirMatchResult::Defeat || Director->GetFlowState() != EAesirLevelFlowState::Results ||
			!Widget || Widget->GetDisplayedResult() != EAesirMatchResult::Defeat || !Widget->IsInViewport() ||
			!Button || !Button->IsVisible() || !Button->GetIsEnabled() || !Telemetry || Telemetry->IsEpisodeActive() ||
			Telemetry->GetEpisodeSummary().EpisodeId != CurrentEpisode ||
			Telemetry->GetEpisodeSummary().Result != EAesirBossEpisodeResult::BossVictory)
		{
			return Finish(TEXT("Defeat did not present exactly one result and finalize the Boss Episode."));
		}

		RetiredMode = Mode;
		RetiredController = Controller;
		RetiredPlayer = Controller->GetPawn();
		RetiredBoss = CurrentBoss;
		RetiredWidget = Widget;
		RetiredPlayerHealth = Controller->GetPawn()->FindComponentByClass<UAesirHealthComponent>();
		RetiredBossHealth = CurrentBoss->FindComponentByClass<UAesirHealthComponent>();

		// Click the actual UMG button wiring. A repeated same-frame request
		// and a competing route must not replace the accepted travel.
		Button->OnClicked.Broadcast();
		Button->OnClicked.Broadcast();
		if (Round < 2 ? Mode->RequestReturnToMenu() : Mode->RequestRetryMatch())
		{
			return Finish(TEXT("A competing travel action was accepted after the result button click."));
		}
		++Defeats;
		Stage = EStage::Travel;
		UE_LOG(LogAesirWarden, Display, TEXT("MATCH_RETRY_SMOKE_TRAVEL: round=%d action=%s"),
			Round, Round < 2 ? TEXT("retry") : TEXT("return_to_menu"));
		return true;
	}

	bool AwaitTravel(UWorld* NextWorld)
	{
		if (!NextWorld || NextWorld == World.Get() || !NextWorld->HasBegunPlay()) return true;
		if (!IsMain(NextWorld))
		{
			return Finish(TEXT("Result action opened a different map instead of reloading Main."));
		}
		if (bRetiredWorldStillBound || RetiredWorldsChecked != Round + 1)
		{
			UE_LOG(LogAesirWarden, Error, TEXT("Retry smoke expected %d cleanup(s), saw %d; death bindings remain: %d."),
				Round + 1, RetiredWorldsChecked, bRetiredWorldStillBound);
			return Finish(TEXT("The retired world did not clear its match death bindings."));
		}
		auto* NextMode = NextWorld->GetAuthGameMode<AAesirCombatPrototypeGameMode>();
		auto* NextController = Cast<AAesirCombatPrototypePlayerController>(NextWorld->GetFirstPlayerController());
		if (!NextMode || !NextController || !NextController->GetPawn()) return true;
		if (NextMode == RetiredMode.Get() || NextController == RetiredController.Get() ||
			NextController->GetPawn() == RetiredPlayer.Get() ||
			(RetiredBoss.IsValid() && NextMode->GetMatchBoss() == RetiredBoss.Get()) ||
			(RetiredWidget.IsValid() && RetiredWidget->IsInViewport()))
		{
			return Finish(TEXT("The old game mode, controller, player, Boss or viewport widget survived reload."));
		}
		const int32 PreviousResultEvents = ResultEvents;
		for (UAesirHealthComponent* Health : {RetiredPlayerHealth.Get(), RetiredBossHealth.Get()})
		{
			// Old UObject instances may legitimately have been collected.
			if (Health && IsValid(Health->GetOwner()))
			{
				Health->OnDeath.Broadcast(nullptr, nullptr);
				++StaleDelegatesReplayed;
			}
		}
		if (NextMode->GetMatchResult() != EAesirMatchResult::None || ResultEvents != PreviousResultEvents)
		{
			return Finish(TEXT("A new world inherited the previous match result."));
		}
		World = NextWorld;
		Mode.Reset();
		Director.Reset();
		CurrentBoss.Reset();
		CurrentBossController.Reset();
		bSkippedOpening = false;
		ResultEvents = 0;
		EventResult = EAesirMatchResult::None;
		++Round;
		Stage = Round == 3 ? EStage::Menu : EStage::Exploration;
		return true;
	}

	bool Tick(float)
	{
		if (!GameInstance.IsValid() || FPlatformTime::Seconds() - StartedAt > 150.0)
		{
			return Finish(TEXT("Timed out waiting for the new world, restored controls, or the result action."));
		}
		UWorld* CurrentWorld = GameInstance->GetWorld();
		if (Stage == EStage::Travel) return AwaitTravel(CurrentWorld);
		if (!IsMain(CurrentWorld) || CurrentWorld != World.Get())
		{
			return Finish(TEXT("The game world changed while no result action was pending."));
		}

		auto* GameMode = CurrentWorld->GetAuthGameMode<AAesirCombatPrototypeGameMode>();
		auto* Controller = Cast<AAesirCombatPrototypePlayerController>(CurrentWorld->GetFirstPlayerController());
		if (!GameMode || !Controller || !Controller->GetPawn()) return true;
		if (!Mode.IsValid())
		{
			Mode = GameMode;
			MatchFinishedHandle = Mode->OnMatchFinished.AddSP(AsShared(), &FRun::OnMatchFinished);
		}
		if (!Director.IsValid())
		{
			for (TActorIterator<AAesirLevelFlowDirector> It(CurrentWorld); It; ++It) { Director = *It; break; }
			if (!Director.IsValid()) return true;
		}
		if (!CooldownTag.IsValid()) return Finish(TEXT("Missing Cooldown.Player.TestHeal gameplay tag."));
		switch (Stage)
		{
		case EStage::Menu: return AwaitMenu(Controller);
		case EStage::Exploration: return AwaitExploration(Controller);
		case EStage::BossCombat: return AwaitBossCombat(Controller);
		case EStage::Result: return AwaitResult(Controller);
		default: return Finish(TEXT("Unexpected smoke test stage."));
		}
	}

	TWeakObjectPtr<UGameInstance> GameInstance;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AAesirCombatPrototypeGameMode> Mode;
	TWeakObjectPtr<AAesirLevelFlowDirector> Director;
	TWeakObjectPtr<APawn> CurrentBoss;
	TWeakObjectPtr<AAesirBossAIController> CurrentBossController;
	TWeakObjectPtr<AAesirCombatPrototypeGameMode> RetiredMode;
	TWeakObjectPtr<AAesirCombatPrototypePlayerController> RetiredController;
	TWeakObjectPtr<APawn> RetiredPlayer;
	TWeakObjectPtr<APawn> RetiredBoss;
	TWeakObjectPtr<UAesirCombatResultWidget> RetiredWidget;
	TWeakObjectPtr<UAesirHealthComponent> RetiredPlayerHealth;
	TWeakObjectPtr<UAesirHealthComponent> RetiredBossHealth;
	FDelegateHandle MatchFinishedHandle;
	FDelegateHandle CleanupHandle;
	const double StartedAt;
	const FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Player.TestHeal"), false);
	TSet<FString> Episodes;
	FString CurrentEpisode;
	FString ScreenshotDirectory;
	FString PendingScreenshot;
	FVector FirstCampfireSpawn = FVector::ZeroVector;
	int32 Round = 0;
	int32 Defeats = 0;
	int32 ResultEvents = 0;
	int32 StaleDelegatesReplayed = 0;
	int32 RetiredWorldsChecked = 0;
	int32 LastRetiredRound = INDEX_NONE;
	bool bRetiredWorldStillBound = false;
	bool bSkippedOpening = false;
	EAesirMatchResult EventResult = EAesirMatchResult::None;
	EStage Stage = EStage::Menu;
};

TSharedPtr<FRun> ActiveRun;

FAutoConsoleCommandWithWorldAndArgs RunCommand(
	TEXT("Aesir.Match.RetrySmoke"),
	TEXT("Verify two consecutive retries, Return to Menu, and Start in standalone Main."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (GIsEditor || !World || World->WorldType != EWorldType::Game ||
			World->GetOutermost()->GetName() != TEXT("/Game/Main") || !World->GetGameInstance() || !Args.IsEmpty())
		{
			UE_LOG(LogAesirWarden, Error, TEXT("MATCH_RETRY_SMOKE_FAIL: run Aesir.Match.RetrySmoke in standalone /Game/Main."));
			FPlatformMisc::RequestExitWithStatus(false, 1);
			return;
		}
		if (!ActiveRun)
		{
			ActiveRun = MakeShared<FRun>(World);
			ActiveRun->Begin();
		}
	}));
}

#endif
