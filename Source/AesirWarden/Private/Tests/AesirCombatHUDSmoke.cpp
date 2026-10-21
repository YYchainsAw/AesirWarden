// Copyright (c) 2026 YYchainsAw. All rights reserved.

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "AesirWarden.h"
#include "Abilities/AesirAttributeSet.h"
#include "Abilities/AesirPlayerAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AI/Boss/AesirBossAIController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Characters/Player/AesirPlayerCharacter.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UI/AesirCombatHUDWidget.h"
#include "UI/AesirMainMenuWidget.h"
#include "UnrealClient.h"

namespace AesirCombatHUDSmoke
{
enum class EStage : uint8
{
	AwaitMenu,
	AwaitMenuImage,
	AwaitExploration,
	AwaitGameplayImage,
	AwaitVitalsRender,
	AwaitVitalsImage
};

class FRun : public TSharedFromThis<FRun>
{
public:
	explicit FRun(UWorld* InWorld) : World(InWorld), StartedAt(FPlatformTime::Seconds()) {}

	void Begin()
	{
		FString MapName = World->GetMapName();
		MapName.RemoveFromStart(World->StreamingLevelsPrefix);
		if (MapName != TEXT("Main"))
		{
			Finish(FString::Printf(TEXT("Load the real /Game/Main before running this smoke (current map: %s)."), *MapName));
			return;
		}
		Output = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("HUD/Preview"));
		if (!IFileManager::Get().MakeDirectory(*Output, true))
		{
			Finish(TEXT("Could not create Saved/HUD/Preview."));
			return;
		}
		for (const TCHAR* Name : {TEXT("Menu.png"), TEXT("Gameplay.png"), TEXT("VitalsAndBoss.png")})
		{
			const FString Filename = Output / Name;
			if (FPaths::FileExists(Filename) && !IFileManager::Get().Delete(*Filename, false, true))
			{
				Finish(FString::Printf(TEXT("Could not retire the previous smoke screenshot: %s"), *Filename));
				return;
			}
		}
		World->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateSP(AsShared(), &FRun::Tick), 0.1f, true);
	}

	bool IsFinished() const { return bFinished; }

private:
	void RestoreVitals()
	{
		if (bVitalsChanged && PlayerASC.IsValid())
		{
			PlayerASC->SetNumericAttributeBase(UAesirAttributeSet::GetHealthAttribute(), OriginalHealthBase);
			PlayerASC->SetNumericAttributeBase(UAesirPlayerAttributeSet::GetGuardPressureAttribute(), OriginalGuardBase);
			bVitalsChanged = false;
		}
		if (HUD.IsValid())
		{
			HUD->ClearBossTarget();
			HUD->RefreshHUDSources();
		}
	}

	void Finish(const FString& Failure = FString())
	{
		if (bFinished) { return; }
		bFinished = true;
		if (World.IsValid()) { World->GetTimerManager().ClearTimer(Timer); }
		RestoreVitals();
		if (!Failure.IsEmpty())
		{
			UE_LOG(LogAesirWarden, Error, TEXT("HUD_MAIN_SMOKE_FAIL: stage=%d; %s"), static_cast<int32>(Stage), *Failure);
		}
		else
		{
			UE_LOG(LogAesirWarden, Display, TEXT("HUD_MAIN_SMOKE_PASS: real WBP_CombatHUD viewport_count=1; menu_hidden=true; real_Start_clicked=true; opening_hidden=%s; exploration_visible=true; initial_health_ratio=%.4f; preview_health_ratio=%.4f; preview_guard_ratio=%.4f; boss=%s; player_attributes_restored=true; maps_and_assets_saved=false; screenshots=%s/Menu.png,%s/Gameplay.png,%s/VitalsAndBoss.png"),
				bSawOpening ? TEXT("true") : TEXT("false"), InitialHealthRatio, PreviewHealthRatio, PreviewGuardRatio,
				bBossFound ? TEXT("real BossController pawn pinned temporarily") : TEXT("not loaded (optional)"), *Output, *Output, *Output);
		}
		// Keep ActiveRun alive until the callback returns. Ending PIE must never
		// close the user's editor; a separate -game process may exit normally.
		if (GIsEditor && GEditor)
		{
			GEditor->RequestEndPlayMap();
		}
		else
		{
			FPlatformMisc::RequestExit(false);
		}
	}

	bool ImageWritten(const TCHAR* Name) const
	{
		const FString Filename = Output / Name;
		return FPaths::FileExists(Filename) && IFileManager::Get().FileSize(*Filename) > 0;
	}

	bool ResolveRealHUD()
	{
		TArray<UUserWidget*> Widgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World.Get(), Widgets, UAesirCombatHUDWidget::StaticClass(), true);
		if (Widgets.Num() > 1)
		{
			Finish(FString::Printf(TEXT("The real viewport contains %d combat HUD instances; expected one existing HUD."), Widgets.Num()));
			return false;
		}
		if (Widgets.IsEmpty()) { return false; }
		UAesirCombatHUDWidget* Found = Cast<UAesirCombatHUDWidget>(Widgets[0]);
		if (!Found || Found->GetClass()->GetPathName() != TEXT("/Game/Aesir/UI/HealthBars/WBP_CombatHUD.WBP_CombatHUD_C"))
		{
			Finish(TEXT("Viewport HUD is not the saved WBP_CombatHUD."));
			return false;
		}
		if (HUD.IsValid() && HUD.Get() != Found)
		{
			Finish(TEXT("The original HUD instance was replaced during the menu/gameplay handoff."));
			return false;
		}
		HUD = Found;
		return true;
	}

	bool VerifyPlayerVitals(bool bRecordInitial)
	{
		APlayerController* Controller = World->GetFirstPlayerController();
		AAesirPlayerCharacter* Player = Controller ? Cast<AAesirPlayerCharacter>(Controller->GetPawn()) : nullptr;
		UAbilitySystemComponent* ASC = Player ? Player->GetAbilitySystemComponent() : nullptr;
		if (!ASC || !ASC->HasAttributeSetForAttribute(UAesirAttributeSet::GetHealthAttribute()) ||
			!ASC->HasAttributeSetForAttribute(UAesirPlayerAttributeSet::GetGuardPressureAttribute()))
		{
			Finish(TEXT("The real possessed player has no usable health/guard GAS attributes."));
			return false;
		}
		PlayerASC = ASC;
		HUD->RefreshHUDSources();
		const UProgressBar* Health = Cast<UProgressBar>(HUD->GetWidgetFromName(TEXT("PB_PlayerHealth")));
		const UProgressBar* Guard = Cast<UProgressBar>(HUD->GetWidgetFromName(TEXT("PB_GuardPressure")));
		const float HealthRatio = UAesirCombatHUDWidget::SafeRatio(ASC->GetNumericAttribute(UAesirAttributeSet::GetHealthAttribute()),
			ASC->GetNumericAttribute(UAesirAttributeSet::GetMaxHealthAttribute()));
		const float GuardRatio = UAesirCombatHUDWidget::SafeRatio(ASC->GetNumericAttribute(UAesirPlayerAttributeSet::GetGuardPressureAttribute()),
			ASC->GetNumericAttribute(UAesirPlayerAttributeSet::GetMaxGuardPressureAttribute()));
		if (!Health || !Guard || !FMath::IsNearlyEqual(Health->GetPercent(), HealthRatio, 0.001f) ||
			(!bRecordInitial && !FMath::IsNearlyEqual(Guard->GetPercent(), GuardRatio, 0.001f)))
		{
			Finish(TEXT("The visible HUD progress bars do not match the real possessed player's GAS values."));
			return false;
		}
		if (bRecordInitial)
		{
			InitialHealthRatio = HealthRatio;
		}
		else
		{
			PreviewHealthRatio = HealthRatio;
			PreviewGuardRatio = GuardRatio;
		}
		return true;
	}

	void PrepareTemporaryVitals()
	{
		if (!PlayerASC.IsValid())
		{
			Finish(TEXT("The real player ASC disappeared before the preview."));
			return;
		}
		OriginalHealthBase = PlayerASC->GetNumericAttributeBase(UAesirAttributeSet::GetHealthAttribute());
		OriginalGuardBase = PlayerASC->GetNumericAttributeBase(UAesirPlayerAttributeSet::GetGuardPressureAttribute());
		bVitalsChanged = true;
		PlayerASC->SetNumericAttributeBase(UAesirAttributeSet::GetHealthAttribute(), OriginalHealthBase * 0.6f);
		PlayerASC->SetNumericAttributeBase(UAesirPlayerAttributeSet::GetGuardPressureAttribute(),
			PlayerASC->GetNumericAttribute(UAesirPlayerAttributeSet::GetMaxGuardPressureAttribute()) * 0.4f);
		for (TActorIterator<AAesirBossAIController> It(World.Get()); It; ++It)
		{
			APawn* Pawn = It->GetPawn();
			UAbilitySystemComponent* ASC = IsValid(Pawn) ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn) : nullptr;
			if (ASC && ASC->HasAttributeSetForAttribute(UAesirAttributeSet::GetHealthAttribute()) &&
				ASC->GetNumericAttribute(UAesirAttributeSet::GetHealthAttribute()) > 0.0f)
			{
				HUD->SetBossTarget(Pawn, NSLOCTEXT("AesirCombatHUDSmoke", "Boss", "首领"));
				bBossFound = HUD->GetDisplayedBoss() == Pawn;
				break;
			}
		}
		HUD->RefreshHUDSources();
		VitalsChangedAt = FPlatformTime::Seconds();
		Stage = EStage::AwaitVitalsRender;
	}

	void Tick()
	{
		if (bFinished) { return; }
		if (!World.IsValid() || FPlatformTime::Seconds() - StartedAt > 90.0)
		{
			Finish(TEXT("Timed out after 90 seconds waiting for real Main flow, HUD or screenshot output."));
			return;
		}
		if (!Flow.IsValid())
		{
			for (TActorIterator<AAesirLevelFlowDirector> It(World.Get()); It; ++It) { Flow = *It; break; }
			if (!Flow.IsValid()) { return; }
		}
		if (!ResolveRealHUD()) { return; }
		if (Stage == EStage::AwaitMenu)
		{
			if (Flow->GetFlowState() != EAesirLevelFlowState::MainMenu) { return; }
			TArray<UUserWidget*> Menus;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World.Get(), Menus, UAesirMainMenuWidget::StaticClass(), true);
			if (Menus.Num() != 1) { return; }
			UAesirMainMenuWidget* Menu = Cast<UAesirMainMenuWidget>(Menus[0]);
			UButton* Start = Menu ? Cast<UButton>(Menu->GetWidgetFromName(TEXT("Btn_Start"))) : nullptr;
			if (!Menu || !Menu->IsVisible() || !Start || !Start->GetIsEnabled()) { return; }
			if (HUD->IsVisible())
			{
				Finish(TEXT("The ready Main menu did not hide the real combat HUD."));
				return;
			}
			// The first ready menu can precede the renderer's first settled world
			// frame. Keep the capture useful without changing production startup.
			if (MenuReadyAt == 0.0) { MenuReadyAt = FPlatformTime::Seconds(); }
			if (FPlatformTime::Seconds() - MenuReadyAt < 1.5) { return; }
			StartButton = Start;
			FScreenshotRequest::RequestScreenshot(Output / TEXT("Menu.png"), true, false);
			Stage = EStage::AwaitMenuImage;
			return;
		}
		if (Stage == EStage::AwaitMenuImage)
		{
			if (HUD->IsVisible() || Flow->GetFlowState() != EAesirLevelFlowState::MainMenu)
			{
				Finish(TEXT("The menu phase/HUD visibility changed before the real Start click."));
				return;
			}
			if (!ImageWritten(TEXT("Menu.png"))) { return; }
			if (!StartButton.IsValid() || !StartButton->GetIsEnabled())
			{
				Finish(TEXT("The real enabled Start button was lost."));
				return;
			}
			Stage = EStage::AwaitExploration;
			StartButton->OnClicked.Broadcast();
			bSawOpening = Flow->GetFlowState() == EAesirLevelFlowState::OpeningCinematic;
			if (!bSawOpening)
			{
				Finish(TEXT("The real Start button did not enter the configured OpeningCinematic phase."));
			}
			else if (HUD->IsVisible())
			{
				Finish(TEXT("The real HUD became visible when the opening started."));
			}
			return;
		}
		if (Stage == EStage::AwaitExploration)
		{
			if (Flow->GetFlowState() == EAesirLevelFlowState::OpeningCinematic)
			{
				bSawOpening = true;
				if (HUD->IsVisible()) { Finish(TEXT("The opening exposed the combat HUD before gameplay handoff.")); }
				return;
			}
			if (Flow->GetFlowState() != EAesirLevelFlowState::Exploration) { return; }
			if (!HUD->IsVisible())
			{
				Finish(TEXT("Exploration did not restore the same combat HUD instance."));
				return;
			}
			if (!VerifyPlayerVitals(true)) { return; }
			FScreenshotRequest::RequestScreenshot(Output / TEXT("Gameplay.png"), true, false);
			Stage = EStage::AwaitGameplayImage;
			return;
		}
		if (Stage == EStage::AwaitGameplayImage)
		{
			if (ImageWritten(TEXT("Gameplay.png"))) { PrepareTemporaryVitals(); }
			return;
		}
		if (Stage == EStage::AwaitVitalsRender)
		{
			if (FPlatformTime::Seconds() - VitalsChangedAt < 0.3) { return; }
			if (!HUD->IsVisible() || !VerifyPlayerVitals(false))
			{
				if (!bFinished) { Finish(TEXT("The temporary vitals preview lost its visible gameplay HUD.")); }
				return;
			}
			FScreenshotRequest::RequestScreenshot(Output / TEXT("VitalsAndBoss.png"), true, false);
			Stage = EStage::AwaitVitalsImage;
			return;
		}
		if (Stage == EStage::AwaitVitalsImage && ImageWritten(TEXT("VitalsAndBoss.png")))
		{
			RestoreVitals();
			if (!PlayerASC.IsValid() ||
				!FMath::IsNearlyEqual(PlayerASC->GetNumericAttributeBase(UAesirAttributeSet::GetHealthAttribute()), OriginalHealthBase, 0.001f) ||
				!FMath::IsNearlyEqual(PlayerASC->GetNumericAttributeBase(UAesirPlayerAttributeSet::GetGuardPressureAttribute()), OriginalGuardBase, 0.001f))
			{
				Finish(TEXT("The preview did not restore the player's original GAS base attributes."));
				return;
			}
			Finish();
		}
	}

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AAesirLevelFlowDirector> Flow;
	TWeakObjectPtr<UAesirCombatHUDWidget> HUD;
	TWeakObjectPtr<UButton> StartButton;
	TWeakObjectPtr<UAbilitySystemComponent> PlayerASC;
	FTimerHandle Timer;
	FString Output;
	double StartedAt;
	double MenuReadyAt = 0.0;
	double VitalsChangedAt = 0.0;
	float OriginalHealthBase = 0.0f;
	float OriginalGuardBase = 0.0f;
	float InitialHealthRatio = 0.0f;
	float PreviewHealthRatio = 0.0f;
	float PreviewGuardRatio = 0.0f;
	EStage Stage = EStage::AwaitMenu;
	bool bSawOpening = false;
	bool bVitalsChanged = false;
	bool bBossFound = false;
	bool bFinished = false;
};

TSharedPtr<FRun> ActiveRun;
FAutoConsoleCommandWithWorld RunCommand(TEXT("Aesir.HUD.Smoke"),
	TEXT("Verify the existing HUD in real Main via its Start button; save three screenshots, restore temporary vitals, and exit only the game session. Never saves maps/assets."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (!World || !World->IsGameWorld() || (ActiveRun.IsValid() && !ActiveRun->IsFinished())) { return; }
		ActiveRun = MakeShared<FRun>(World);
		ActiveRun->Begin();
	}));
} // namespace AesirCombatHUDSmoke

#endif
