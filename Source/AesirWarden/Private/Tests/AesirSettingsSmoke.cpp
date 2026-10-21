// Copyright (c) 2026 YYchainsAw. All rights reserved.

#if WITH_EDITOR && !UE_BUILD_SHIPPING

#include "AesirWarden.h"
#include "AI/Boss/AesirBossAIController.h"
#include "AI/Boss/AesirBossTelemetryComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Combat/AesirHealthComponent.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Settings/AesirGameUserSettings.h"
#include "TimerManager.h"
#include "UI/AesirMainMenuWidget.h"
#include "UI/AesirSettingsMenuWidget.h"
#include "UnrealClient.h"
#include "UObject/UnrealType.h"

namespace AesirSettingsSmoke
{

class FRun final : public TSharedFromThis<FRun>
{
public:
	explicit FRun(UWorld* InWorld) : World(InWorld), StartedAt(FPlatformTime::Seconds())
	{
		OutputDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Settings/Preview"));
	}

	void Begin()
	{
		IFileManager::Get().MakeDirectory(*OutputDirectory, true);
		for (const TCHAR* Name : {TEXT("Display"), TEXT("Quality"), TEXT("Audio"), TEXT("Controls"), TEXT("Discard")})
		{
			IFileManager::Get().Delete(*(OutputDirectory / FString(Name) + TEXT(".png")), false, true);
		}
		World->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateSP(AsShared(), &FRun::Tick), 0.25f, true);
		UE_LOG(LogAesirWarden, Display, TEXT("SETTINGS_MAIN_SMOKE_BEGIN: waiting for the real Main menu; no settings will be applied."));
	}

	bool IsFinished() const { return bFinished; }

private:
	enum class EStage : uint8 { Waiting, Tabs, CleanBack, Reopen, DiscardScreenshot, Final };

	void Finish(bool bSuccess)
	{
		bFinished = true;
		if (UWorld* TestWorld = World.Get()) TestWorld->GetTimerManager().ClearTimer(Timer);
		if (SettingsMenu.IsValid()) SettingsMenu->DiscardChangesAndClose();
		FFileHelper::SaveStringToFile(bSuccess ? TEXT("PASS\n") : TEXT("FAIL\n"), *(OutputDirectory / TEXT("result.txt")));
		// This command refuses editor/PIE worlds before beginning. Never close an editor.
		if (!GIsEditor) FPlatformMisc::RequestExit(false);
	}

	void Fail(const FString& Reason)
	{
		UE_LOG(LogAesirWarden, Error, TEXT("SETTINGS_MAIN_SMOKE_FAIL: %s"), *Reason);
		Finish(false);
	}

	bool CollectHiddenHUD(AAesirLevelFlowDirector* Flow)
	{
		const FArrayProperty* Classes = FindFProperty<FArrayProperty>(Flow->GetClass(), TEXT("GameplayHUDWidgetClasses"));
		const FObjectPropertyBase* ClassProperty = Classes ? CastField<FObjectPropertyBase>(Classes->Inner) : nullptr;
		if (!Classes || !ClassProperty)
		{
			Fail(TEXT("The configured gameplay HUD classes are unavailable."));
			return false;
		}
		FScriptArrayHelper ClassArray(Classes, Classes->ContainerPtrToValuePtr<void>(Flow));
		for (int32 Index = 0; Index < ClassArray.Num(); ++Index)
		{
			UClass* Class = Cast<UClass>(ClassProperty->GetObjectPropertyValue(ClassArray.GetRawPtr(Index)));
			if (!Class) continue;
			TArray<UUserWidget*> Widgets;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World.Get(), Widgets, Class, true);
			for (UUserWidget* Widget : Widgets) HiddenHUD.AddUnique(Widget);
		}
		if (HiddenHUD.IsEmpty())
		{
			Fail(TEXT("No configured gameplay HUD was available to check menu visibility."));
			return false;
		}
		return true;
	}

	bool CheckMenuIsolation()
	{
		AAesirLevelFlowDirector* Flow = Director.Get();
		APlayerController* Controller = UGameplayStatics::GetPlayerController(World.Get(), 0);
		UGameViewportClient* Viewport = World->GetGameViewport();
		if (!Flow || Flow->GetFlowState() != EAesirLevelFlowState::MainMenu || !Controller || !Viewport ||
			!Controller->IsMoveInputIgnored() || !Controller->IsLookInputIgnored() ||
			!Viewport->IgnoreInput() || !Controller->bShowMouseCursor || Controller->GetViewTarget() != InitialViewTarget.Get())
		{
			Fail(TEXT("Opening/closing Settings changed the startup flow, campfire camera, cursor or gameplay input locks."));
			return false;
		}
		for (const TWeakObjectPtr<UUserWidget>& Widget : HiddenHUD)
		{
			if (!Widget.IsValid() || Widget->GetVisibility() != ESlateVisibility::Collapsed)
			{
				Fail(TEXT("A gameplay HUD became visible during the settings flow."));
				return false;
			}
		}
		if (!Health.IsValid() || !FMath::IsNearlyEqual(Health->GetCurrentHealth(), InitialHealth))
		{
			Fail(TEXT("Player health changed while Main/Settings remained open."));
			return false;
		}
		for (TActorIterator<AAesirBossAIController> It(World.Get()); It; ++It)
		{
			const UAesirBossTelemetryComponent* Telemetry = It->GetBossTelemetryComponent();
			if (!Telemetry || Telemetry->IsEpisodeActive())
			{
				Fail(TEXT("A boss entered combat before the Start button was clicked."));
				return false;
			}
		}
		return true;
	}

	bool CheckReturnedToMain()
	{
		UButton* SettingsButton = MainMenu.IsValid() ? Cast<UButton>(MainMenu->GetWidgetFromName(TEXT("Btn_Settings"))) : nullptr;
		APlayerController* Controller = UGameplayStatics::GetPlayerController(World.Get(), 0);
		if (!MainMenu.IsValid() || !MainMenu->IsInViewport() || !MainMenu->IsVisible() ||
			MainMenu->GetActiveSettingsMenu() || (SettingsMenu.IsValid() && SettingsMenu->IsInViewport()) ||
			!SettingsButton || !SettingsButton->HasUserFocus(Controller))
		{
			Fail(TEXT("Closing settings did not restore the real main menu and focus its Settings button."));
			return false;
		}
		return CheckMenuIsolation();
	}

	bool OpenFromRealButton()
	{
		UButton* Button = MainMenu.IsValid() ? Cast<UButton>(MainMenu->GetWidgetFromName(TEXT("Btn_Settings"))) : nullptr;
		if (!Button || !Button->GetIsEnabled())
		{
			Fail(TEXT("The authored main menu has no enabled Settings button."));
			return false;
		}
		Button->OnClicked.Broadcast();
		SettingsMenu = MainMenu->GetActiveSettingsMenu();
		if (!SettingsMenu.IsValid() || !SettingsMenu->IsInViewport() || !SettingsMenu->IsVisible() || MainMenu->IsVisible())
		{
			Fail(TEXT("The real Settings button did not open a settings page and hide the entire main menu/logo."));
			return false;
		}
		if (SettingsMenu->GetClass()->GetPathName() != TEXT("/Game/Aesir/UI/Menus/WBP_SettingsMenu.WBP_SettingsMenu_C"))
		{
			Fail(TEXT("The Settings button fell back to a native tree instead of loading the saved Designer Blueprint."));
			return false;
		}
		return true;
	}

	void RequestPreview(const TCHAR* Name)
	{
		PendingScreenshot = OutputDirectory / (FString(Name) + TEXT(".png"));
		FScreenshotRequest::RequestScreenshot(PendingScreenshot, true, false);
	}

	void Tick()
	{
		if (bFinished) return;
		if (!World.IsValid()) { Fail(TEXT("The game world was destroyed before the smoke test completed.")); return; }
		if (FPlatformTime::Seconds() - StartedAt > 120.0) { Fail(TEXT("The settings smoke test exceeded 120 seconds.")); return; }
		if (Stage == EStage::Waiting)
		{
			// Let the startup fade, exposure and streamed scene settle before visual captures.
			if (FPlatformTime::Seconds() - StartedAt < 8.0) return;
			for (TActorIterator<AAesirLevelFlowDirector> It(World.Get()); It; ++It) Director = *It;
			AAesirLevelFlowDirector* Flow = Director.Get();
			APlayerController* Controller = UGameplayStatics::GetPlayerController(World.Get(), 0);
			if (!Flow || !Controller || !Controller->GetPawn() || Flow->GetFlowState() == EAesirLevelFlowState::Uninitialized) return;
			if (Flow->GetFlowState() != EAesirLevelFlowState::MainMenu) { Fail(TEXT("The level left MainMenu before Settings was opened.")); return; }
			const FObjectPropertyBase* MenuProperty = FindFProperty<FObjectPropertyBase>(Flow->GetClass(), TEXT("MainMenuRef"));
			MainMenu = MenuProperty ? Cast<UAesirMainMenuWidget>(MenuProperty->GetObjectPropertyValue_InContainer(Flow)) : nullptr;
			const FBoolProperty* Ready = FindFProperty<FBoolProperty>(Flow->GetClass(), TEXT("bStartupStreamingReady"));
			UButton* Start = MainMenu.IsValid() ? Cast<UButton>(MainMenu->GetWidgetFromName(TEXT("Btn_Start"))) : nullptr;
			if (!Ready || !Ready->GetPropertyValue_InContainer(Flow) || !Start || !Start->GetIsEnabled()) return;
			UGameViewportClient* Viewport = World->GetGameViewport();
			if (!Viewport || !Viewport->Viewport || Viewport->Viewport->GetSizeXY() != FIntPoint(1920, 1080))
			{
				Fail(TEXT("Run the Main smoke at 1920x1080 using -windowed -ResX=1920 -ResY=1080."));
				return;
			}
			InitialViewTarget = Controller->GetViewTarget();
			Health = Controller->GetPawn()->FindComponentByClass<UAesirHealthComponent>();
			if (!Health.IsValid() || Health->GetCurrentHealth() <= 0.0f) { Fail(TEXT("A living player is required to verify the menu combat gate.")); return; }
			InitialHealth = Health->GetCurrentHealth();
			if (!CollectHiddenHUD(Flow) || !CheckMenuIsolation() || !OpenFromRealButton()) return;
			Baseline = SettingsMenu->GetPendingSettings();
			Stage = EStage::Tabs;
			return;
		}
		if (!CheckMenuIsolation()) return;
		if (Stage == EStage::Tabs)
		{
			if (!SettingsMenu.IsValid() || MainMenu->IsVisible()) { Fail(TEXT("Settings lost its active page or the main menu became visible over it.")); return; }
			if (!PendingScreenshot.IsEmpty())
			{
				if (!FPaths::FileExists(PendingScreenshot)) return;
				PendingScreenshot.Reset();
				++Category;
			}
			if (Category == 4)
			{
				if (SettingsMenu->HasPendingChanges()) { Fail(TEXT("Visiting settings categories created unapplied changes.")); return; }
				SettingsMenu->RequestClose();
				Stage = EStage::CleanBack;
				return;
			}
			const TCHAR* Names[] = {TEXT("Display"), TEXT("Quality"), TEXT("Audio"), TEXT("Controls")};
			UButton* Tab = Cast<UButton>(SettingsMenu->GetWidgetFromName(FName(*FString::Printf(TEXT("Btn_Tab%s"), Names[Category]))));
			UWidgetSwitcher* Pages = Cast<UWidgetSwitcher>(SettingsMenu->GetWidgetFromName(TEXT("SettingsPages")));
			if (!Tab || !Pages) { Fail(TEXT("The saved Settings Blueprint is missing a category button or page switcher.")); return; }
			Tab->OnClicked.Broadcast();
			if (Pages->GetActiveWidgetIndex() != Category) { Fail(TEXT("A real category button failed to switch the settings page.")); return; }
			RequestPreview(Names[Category]);
			return;
		}
		if (Stage == EStage::CleanBack)
		{
			if (!CheckReturnedToMain()) return;
			Stage = EStage::Reopen;
			return;
		}
		if (Stage == EStage::Reopen)
		{
			if (!OpenFromRealButton()) return;
			FAesirSettingsValues Edited = SettingsMenu->GetPendingSettings();
			Edited.MasterVolume = Baseline.MasterVolume > 0.5f ? 0.3f : 0.8f;
			Edited.MouseSensitivity = Baseline.MouseSensitivity > 1.5f ? 0.75f : 2.25f;
			SettingsMenu->SetPendingSettings(Edited);
			SettingsMenu->RequestClose();
			if (!SettingsMenu->IsAwaitingDiscardConfirmation()) { Fail(TEXT("Closing changed settings failed to show the discard question.")); return; }
			RequestPreview(TEXT("Discard"));
			Stage = EStage::DiscardScreenshot;
			return;
		}
		if (Stage == EStage::DiscardScreenshot)
		{
			if (!FPaths::FileExists(PendingScreenshot)) return;
			UButton* Discard = Cast<UButton>(SettingsMenu->GetWidgetFromName(TEXT("Btn_ConfirmYes")));
			if (!Discard) { Fail(TEXT("The discard confirmation is missing its real affirmative button.")); return; }
			Discard->OnClicked.Broadcast();
			Stage = EStage::Final;
			return;
		}
		if (!CheckReturnedToMain()) return;
		const UAesirGameUserSettings* Settings = UAesirGameUserSettings::Get();
		if (!Settings || !FMath::IsNearlyEqual(Settings->GetMasterVolume(), Baseline.MasterVolume) ||
			!FMath::IsNearlyEqual(Settings->GetMouseSensitivity(), Baseline.MouseSensitivity) ||
			Settings->GetInvertMouseY() != Baseline.bInvertMouseY || Settings->GetScreenResolution() != Baseline.Resolution ||
			Settings->GetFullscreenMode() != Baseline.WindowMode.GetValue())
		{
			Fail(TEXT("Discarding settings changed applied audio, input or display preferences."));
			return;
		}
		UE_LOG(LogAesirWarden, Display, TEXT("SETTINGS_MAIN_SMOKE_PASS: real settings button and four tabs; main logo/HUD hidden; unchanged close and dirty discard returned focus; camera/input/combat gate preserved; 5 screenshots: %s"), *OutputDirectory);
		Finish(true);
	}

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AAesirLevelFlowDirector> Director;
	TWeakObjectPtr<UAesirMainMenuWidget> MainMenu;
	TWeakObjectPtr<UAesirSettingsMenuWidget> SettingsMenu;
	TWeakObjectPtr<UAesirHealthComponent> Health;
	TWeakObjectPtr<AActor> InitialViewTarget;
	TArray<TWeakObjectPtr<UUserWidget>> HiddenHUD;
	FAesirSettingsValues Baseline;
	FTimerHandle Timer;
	FString OutputDirectory;
	FString PendingScreenshot;
	double StartedAt;
	float InitialHealth = 0.0f;
	int32 Category = 0;
	EStage Stage = EStage::Waiting;
	bool bFinished = false;
};

TSharedPtr<FRun> ActiveRun;

FAutoConsoleCommandWithWorldAndArgs RunCommand(
	TEXT("Aesir.Settings.Smoke"),
	TEXT("Verify Main->Settings tabs/close/discard at 1920x1080, save Saved/Settings/Preview and exit the standalone game. Refuses editor/PIE; never applies settings."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (GIsEditor || !World || !World->IsGameWorld() || World->WorldType == EWorldType::PIE)
		{
			UE_LOG(LogAesirWarden, Error, TEXT("SETTINGS_MAIN_SMOKE_FAIL: use a separate UnrealEditor project /Game/Main -game process; this command will not close an editor or PIE session."));
			return;
		}
		if (ActiveRun.IsValid() && !ActiveRun->IsFinished())
		{
			UE_LOG(LogAesirWarden, Warning, TEXT("A settings smoke test is already running."));
			return;
		}
		ActiveRun = MakeShared<FRun>(World);
		ActiveRun->Begin();
	}));

} // namespace AesirSettingsSmoke

#endif // WITH_EDITOR && !UE_BUILD_SHIPPING
