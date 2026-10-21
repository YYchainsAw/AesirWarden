// Copyright (c) 2026 YYchainsAw. All rights reserved.

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Settings/AesirGameUserSettings.h"
#include "Slate/SceneViewport.h"
#include "UI/AesirSettingsMenuWidget.h"
#include "UObject/UnrealType.h"
#include "Widgets/SViewport.h"

namespace AesirSettingsTests
{

// Known config globals are cache keys in UE 5.8, not filesystem paths.
// Preserve the saved/runtime layers too: restoring only InMemoryFile can leave
// test writes queued for a later config flush during engine shutdown.
struct FSettingsConfigSnapshot
{
	FString Key;
	FString Filename;
	TArray<uint8> Bytes;
	TOptional<FConfigFile> Memory;
	TOptional<FConfigFile> Combined;
	TOptional<FConfigCommandStream> Saved;
	TOptional<FConfigCommandStream> Runtime;
	bool bExisted = false;
	bool bCaptured = false;

	bool Capture(const FString& InKey)
	{
		Key = InKey;
		FConfigBranch* Branch = GConfig ? GConfig->FindBranch(NAME_None, Key) : nullptr;
		if (!Branch || Branch->IniPath.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("Settings test cannot resolve config branch %s; refusing to modify settings."), *Key);
			return false;
		}
		Filename = FPaths::ConvertRelativePathToFull(Branch->IniPath);
		Memory.Emplace(Branch->InMemoryFile);
		Combined.Emplace(Branch->FinalCombinedLayers);
		Saved.Emplace(Branch->SavedLayer);
		Runtime.Emplace(Branch->RuntimeChanges);
		bExisted = IFileManager::Get().FileExists(*Filename);
		if (bExisted && !FFileHelper::LoadFileToArray(Bytes, *Filename))
		{
			UE_LOG(LogTemp, Error, TEXT("Settings test cannot back up %s; refusing to modify settings."), *Filename);
			return false;
		}
		if (bExisted)
		{
			const FString Directory = FPaths::ProjectSavedDir() / TEXT("Settings/BeforeAutomation");
			IFileManager::Get().MakeDirectory(*Directory, true);
			const FString Backup = Directory / FString::Printf(TEXT("%s_%s.ini"), *FPaths::GetBaseFilename(Filename), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
			if (!FFileHelper::SaveArrayToFile(Bytes, *Backup))
			{
				UE_LOG(LogTemp, Error, TEXT("Settings test cannot write backup %s; refusing to modify settings."), *Backup);
				return false;
			}
		}
		UE_LOG(LogTemp, Display, TEXT("SETTINGS_CONFIG_BACKUP: key=%s; path=%s; existed=%d; bytes=%d"), *Key, *Filename, bExisted, Bytes.Num());
		bCaptured = true;
		return true;
	}

	void Restore()
	{
		if (!bCaptured) return;
		FConfigBranch* Branch = GConfig ? GConfig->FindBranch(NAME_None, Key) : nullptr;
		if (Branch)
		{
			Branch->InMemoryFile = Memory.GetValue();
			Branch->InMemoryFile.Branch = Branch;
			Branch->FinalCombinedLayers = Combined.GetValue();
			Branch->FinalCombinedLayers.Branch = Branch;
			Branch->SavedLayer = Saved.GetValue();
			Branch->SavedLayer.Branch = Branch;
			Branch->RuntimeChanges = Runtime.GetValue();
			Branch->RuntimeChanges.Branch = Runtime.GetValue().Branch ? Branch : nullptr;
		}
		else UE_LOG(LogTemp, Error, TEXT("Settings test lost config branch %s while restoring."), *Key);
		const bool bRestored = bExisted ? FFileHelper::SaveArrayToFile(Bytes, *Filename)
			: (!IFileManager::Get().FileExists(*Filename) || IFileManager::Get().Delete(*Filename, false, true));
		if (!bRestored) UE_LOG(LogTemp, Error, TEXT("Settings test could not restore %s; use Saved/Settings/BeforeAutomation backup."), *Filename);
		UE_LOG(LogTemp, Display, TEXT("SETTINGS_CONFIG_RESTORED: key=%s; path=%s; success=%d"), *Key, *Filename, bRestored);
	}
};

// Apply/Save is a behavior under test. Capture every config property, scalability
// cache, config-cache section and on-disk byte before touching global settings.
// The fixture restores all four so a test cannot persist a user's temporary
// resolution, audio or mouse preferences, including when an assertion fails.
class FSettingsWorld
{
public:
	FSettingsWorld()
	{
		Settings = UAesirGameUserSettings::Get();
		if (!Settings)
		{
			return;
		}
		if (!GameConfig.Capture(GGameUserSettingsIni) || !EditorConfig.Capture(GEditorSettingsIni)) return;
		bReadyForChanges = true;
		for (TFieldIterator<FProperty> It(Settings->GetClass()); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Config))
			{
				FString Value;
				It->ExportText_InContainer(0, Value, Settings, Settings, Settings, PPF_None);
				OriginalProperties.Add(*It, Value);
			}
		}
		OriginalQuality = Settings->ScalabilityQuality;
		GameInstance = NewObject<UGameInstance>(GEngine);
		GameInstance->AddToRoot();
		GameInstance->InitializeStandalone(MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("AesirSettingsTest")));
		World = GameInstance->GetWorld();
		FWorldContext& Context = *GameInstance->GetWorldContext();
		Viewport = NewObject<UGameViewportClient>(GEngine);
		Context.GameViewport = Viewport;
		Viewport->Init(Context, GameInstance, false);
		SlateViewport = SNew(SViewport);
		SceneViewport = Viewport->CreateViewport(SlateViewport);
		World->InitializeActorsForPlay(FURL());
		FString PlayerError;
		ULocalPlayer* LocalPlayer = GameInstance->CreateLocalPlayer(0, PlayerError, false);
		if (LocalPlayer)
		{
			Controller = World->SpawnActor<APlayerController>();
			APawn* Pawn = World->SpawnActor<APawn>();
			Controller->SetPlayer(LocalPlayer);
			Controller->Possess(Pawn);
		}
	}

	~FSettingsWorld()
	{
		if (Menu)
		{
			if (Menu->IsAwaitingVideoConfirmation()) Menu->ConfirmVideoMode(false);
			Menu->DiscardChangesAndClose();
			Menu->RemoveFromParent();
		}
		if (Settings && bReadyForChanges)
		{
			RestoreProperties();
			Settings->ScalabilityQuality = OriginalQuality;
			Settings->ApplySettings(false);
			Settings->ApplyRuntimeSettings(World);
			RestoreProperties();
			Settings->ScalabilityQuality = OriginalQuality;
		}
		if (World)
		{
			for (AActor* Actor : FActorRange(World)) Actor->RouteEndPlay(EEndPlayReason::LevelTransition);
			Viewport->DetachViewportClient();
			SceneViewport.Reset();
			SlateViewport.Reset();
			GameInstance->Shutdown();
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
			GameInstance->RemoveFromRoot();
		}
		GameConfig.Restore();
		EditorConfig.Restore();
	}

	bool CreateMenu(FAutomationTestBase* Test)
	{
		UClass* Class = LoadClass<UAesirSettingsMenuWidget>(nullptr,
			TEXT("/Game/Aesir/UI/Menus/WBP_SettingsMenu.WBP_SettingsMenu_C"));
		if (!Test->TestNotNull(TEXT("The saved editable settings Blueprint loads with its native parent"), Class)) return false;
		Menu = CreateWidget<UAesirSettingsMenuWidget>(Controller, Class);
		if (!Test->TestNotNull(TEXT("The real settings menu can be instantiated"), Menu)) return false;
		Menu->AddToViewport();
		Menu->BeginSettingsSession();
		return true;
	}

	void RestoreProperties()
	{
		for (const TPair<FProperty*, FString>& Entry : OriginalProperties)
		{
			Entry.Key->ImportText_Direct(*Entry.Value, Entry.Key->ContainerPtrToValuePtr<void>(Settings), Settings, PPF_None);
		}
	}

	UAesirGameUserSettings* Settings = nullptr;
	UGameInstance* GameInstance = nullptr;
	UWorld* World = nullptr;
	UGameViewportClient* Viewport = nullptr;
	APlayerController* Controller = nullptr;
	UAesirSettingsMenuWidget* Menu = nullptr;
	FSettingsConfigSnapshot GameConfig;
	FSettingsConfigSnapshot EditorConfig;
	TMap<FProperty*, FString> OriginalProperties;
	Scalability::FQualityLevels OriginalQuality;
	TSharedPtr<SViewport> SlateViewport;
	TSharedPtr<FSceneViewport> SceneViewport;
	bool bReadyForChanges = false;
};

class FExerciseSettings final : public IAutomationLatentCommand
{
public:
	FExerciseSettings(FAutomationTestBase* InTest, TSharedPtr<FSettingsWorld> InFixture, FString InMode)
		: Test(InTest), Fixture(MoveTemp(InFixture)), Mode(MoveTemp(InMode))
	{
	}
	virtual bool Update() override;

private:
	FAutomationTestBase* Test;
	TSharedPtr<FSettingsWorld> Fixture;
	FString Mode;
	FAesirSettingsValues Baseline;
	FAesirSettingsValues Edited;
	double PreviewStartedAt = 0.0;
	int32 Stage = 0;
};

bool FExerciseSettings::Update()
{
	Fixture->World->Tick(LEVELTICK_All, 0.125f);
	UAesirSettingsMenuWidget* Menu = Fixture->Menu;
	UAesirGameUserSettings* Settings = Fixture->Settings;
	if (Stage == 0)
	{
		Baseline = Menu->GetPendingSettings();
		Edited = Baseline;
		Edited.MasterVolume = Baseline.MasterVolume > 0.5f ? 0.3f : 0.8f;
		Edited.MouseSensitivity = Baseline.MouseSensitivity > 1.5f ? 0.75f : 2.25f;
		Edited.bInvertMouseY = !Baseline.bInvertMouseY;
		Test->TestFalse(TEXT("A new settings session has no unapplied edits"), Menu->HasPendingChanges());
		if (Mode == TEXT("ControlEvents"))
		{
			UCheckBox* VSync = Cast<UCheckBox>(Menu->GetWidgetFromName(TEXT("Chk_VSync")));
			UCheckBox* InvertY = Cast<UCheckBox>(Menu->GetWidgetFromName(TEXT("Chk_InvertY")));
			UTextBlock* VSyncValue = Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("Txt_VSyncValue")));
			UTextBlock* InvertYValue = Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("Txt_InvertYValue")));
			UComboBoxString* Overall = Cast<UComboBoxString>(Menu->GetWidgetFromName(TEXT("Cmb_OverallQuality")));
			UComboBoxString* Shadow = Cast<UComboBoxString>(Menu->GetWidgetFromName(TEXT("Cmb_ShadowQuality")));
			USlider* Volume = Cast<USlider>(Menu->GetWidgetFromName(TEXT("Sld_MasterVolume")));
			USlider* Sensitivity = Cast<USlider>(Menu->GetWidgetFromName(TEXT("Sld_MouseSensitivity")));
			UTextBlock* VolumeValue = Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("Txt_VolumeValue")));
			UTextBlock* SensitivityValue = Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("Txt_SensitivityValue")));
			if (!Test->TestNotNull(TEXT("The real Blueprint contains the VSync check box"), VSync) ||
				!Test->TestNotNull(TEXT("The real Blueprint contains the inverted-look check box"), InvertY) ||
				!Test->TestNotNull(TEXT("The real Blueprint contains the VSync state label"), VSyncValue) ||
				!Test->TestNotNull(TEXT("The real Blueprint contains the inverted-look state label"), InvertYValue) ||
				!Test->TestNotNull(TEXT("The real Blueprint contains the overall quality selector"), Overall) ||
				!Test->TestNotNull(TEXT("The real Blueprint contains the shadow quality selector"), Shadow) ||
				!Test->TestNotNull(TEXT("The real Blueprint contains the volume slider"), Volume) ||
				!Test->TestNotNull(TEXT("The real Blueprint contains the sensitivity slider"), Sensitivity) ||
				!Test->TestNotNull(TEXT("The real Blueprint contains the volume value label"), VolumeValue) ||
				!Test->TestNotNull(TEXT("The real Blueprint contains the sensitivity value label"), SensitivityValue)) return true;

			// Dispatch the saved controls' public input events. This case never calls
			// SetPendingSettings, so missing native event wiring cannot be masked.
			for (bool bChecked : {true, false})
			{
				VSync->OnCheckStateChanged.Broadcast(bChecked);
				InvertY->OnCheckStateChanged.Broadcast(bChecked);
				Test->TestEqual(TEXT("VSync input stages the selected value"), Menu->GetPendingSettings().bVSync, bChecked);
				Test->TestEqual(TEXT("Inverted-look input stages the selected value"), Menu->GetPendingSettings().bInvertMouseY, bChecked);
				Test->TestEqual(TEXT("The VSync state label follows check-box input immediately"), VSyncValue->GetText().ToString(), FString(bChecked ? TEXT("开启") : TEXT("关闭")));
				Test->TestEqual(TEXT("The inverted-look state label follows check-box input immediately"), InvertYValue->GetText().ToString(), FString(bChecked ? TEXT("开启") : TEXT("关闭")));
			}
			const int32 Preset = Baseline.OverallQuality == 2 ? 1 : 2;
			Overall->SetSelectedIndex(Preset);
			Test->TestEqual(TEXT("Changing the real overall selector stages its preset"), Menu->GetPendingSettings().OverallQuality, Preset);
			Test->TestEqual(TEXT("The selected preset also updates the shadow selector"), Shadow->GetSelectedIndex(), Preset);
			Test->TestEqual(TEXT("The selected preset stages texture quality"), Menu->GetPendingSettings().TextureQuality, Preset);
			Test->TestEqual(TEXT("The selected preset stages effects quality"), Menu->GetPendingSettings().EffectsQuality, Preset);
			const int32 CustomShadow = Preset + 1;
			Shadow->SetSelectedIndex(CustomShadow);
			Test->TestEqual(TEXT("Changing the real shadow selector stages a custom value"), Menu->GetPendingSettings().ShadowQuality, CustomShadow);
			Test->TestEqual(TEXT("A changed individual quality marks the preset custom"), Menu->GetPendingSettings().OverallQuality, -1);
			Test->TestEqual(TEXT("The overall selector visibly changes to Custom"), Overall->GetSelectedOption(), FString(TEXT("自定义")));

			Volume->OnValueChanged.Broadcast(Edited.MasterVolume);
			Sensitivity->OnValueChanged.Broadcast(Edited.MouseSensitivity);
			Test->TestTrue(TEXT("Real volume slider input stages its value"), FMath::IsNearlyEqual(Menu->GetPendingSettings().MasterVolume, Edited.MasterVolume));
			Test->TestTrue(TEXT("Real sensitivity slider input stages its value"), FMath::IsNearlyEqual(Menu->GetPendingSettings().MouseSensitivity, Edited.MouseSensitivity));
			Test->TestEqual(TEXT("The volume label follows real slider input"), VolumeValue->GetText().ToString(), FText::AsPercent(Edited.MasterVolume).ToString());
			Test->TestEqual(TEXT("The sensitivity label follows real slider input"), SensitivityValue->GetText().ToString(), FString::Printf(TEXT("%.2f"), Edited.MouseSensitivity));
			Test->TestTrue(TEXT("Control events mark settings dirty"), Menu->HasPendingChanges());
			Test->TestTrue(TEXT("Control events leave live volume unapplied"), FMath::IsNearlyEqual(Settings->GetMasterVolume(), Baseline.MasterVolume));
			Test->TestTrue(TEXT("Control events leave live sensitivity unapplied"), FMath::IsNearlyEqual(Settings->GetMouseSensitivity(), Baseline.MouseSensitivity));
			Test->TestEqual(TEXT("Control events leave live VSync unapplied"), Settings->IsVSyncEnabled(), Baseline.bVSync);
			Test->TestEqual(TEXT("Control events leave live inverted look unapplied"), Settings->GetInvertMouseY(), Baseline.bInvertMouseY);
			Test->TestTrue(TEXT("Control events leave every live scalability group unapplied"), Settings->ScalabilityQuality == Fixture->OriginalQuality);
			Menu->BeginSettingsSession();
			Test->TestTrue(TEXT("Repeating Begin on an active session retains staged slider input"), FMath::IsNearlyEqual(Menu->GetPendingSettings().MasterVolume, Edited.MasterVolume));
			Test->TestTrue(TEXT("Repeating Begin on an active session does not clear dirty state"), Menu->HasPendingChanges());
			return true;
		}
		Menu->SetPendingSettings(Edited);
		Test->TestTrue(TEXT("Changing staged values marks the session dirty"), Menu->HasPendingChanges());
		Test->TestTrue(TEXT("Staging volume does not change live settings"), FMath::IsNearlyEqual(Settings->GetMasterVolume(), Baseline.MasterVolume));
		Test->TestTrue(TEXT("Staging sensitivity does not change live input preferences"), FMath::IsNearlyEqual(Settings->GetMouseSensitivity(), Baseline.MouseSensitivity));
		Test->TestEqual(TEXT("Staging inverted Y does not change live input preferences"), Settings->GetInvertMouseY(), Baseline.bInvertMouseY);
		if (Mode == TEXT("PendingAndDiscard"))
		{
			Menu->RequestClose();
			Test->TestTrue(TEXT("Closing dirty settings asks before discarding edits"), Menu->IsAwaitingDiscardConfirmation());
			Test->TestTrue(TEXT("The dirty settings menu stays available while confirming"), Menu->IsInViewport());
			Menu->ContinueEditing();
			Test->TestFalse(TEXT("Continue Editing closes only the discard question"), Menu->IsAwaitingDiscardConfirmation());
			Test->TestTrue(TEXT("Continue Editing retains pending changes"), Menu->HasPendingChanges());
			Menu->RequestClose();
			Menu->DiscardChangesAndClose();
			Test->TestTrue(TEXT("Discard leaves applied volume unchanged"), FMath::IsNearlyEqual(Settings->GetMasterVolume(), Baseline.MasterVolume));
			Test->TestTrue(TEXT("Discard leaves applied mouse sensitivity unchanged"), FMath::IsNearlyEqual(Settings->GetMouseSensitivity(), Baseline.MouseSensitivity));
			Menu->BeginSettingsSession();
			Test->TestFalse(TEXT("A fresh session discards prior unapplied values"), Menu->HasPendingChanges());
			Test->TestTrue(TEXT("A fresh session reloads applied volume"), FMath::IsNearlyEqual(Menu->GetPendingSettings().MasterVolume, Baseline.MasterVolume));
			return true;
		}
		if (Mode == TEXT("RestoreDefaults"))
		{
			Menu->RestoreDefaults();
			const FAesirSettingsValues Defaults = Menu->GetPendingSettings();
			Test->TestTrue(TEXT("Restore Defaults stages normal volume"), FMath::IsNearlyEqual(Defaults.MasterVolume, 1.0f));
			Test->TestTrue(TEXT("Restore Defaults stages normal sensitivity"), FMath::IsNearlyEqual(Defaults.MouseSensitivity, 1.0f));
			Test->TestFalse(TEXT("Restore Defaults stages uninverted look"), Defaults.bInvertMouseY);
			Test->TestTrue(TEXT("Restore Defaults does not apply the volume while editing"), FMath::IsNearlyEqual(Settings->GetMasterVolume(), Baseline.MasterVolume));
			Test->TestTrue(TEXT("Restore Defaults does not apply video resolution while editing"), Settings->GetScreenResolution() == Baseline.Resolution);
			return true;
		}
		if (Mode == TEXT("ApplyAndPersistence"))
		{
			Menu->RequestApply();
			Test->TestFalse(TEXT("Audio and input edits do not ask for video confirmation"), Menu->IsAwaitingVideoConfirmation());
			Test->TestFalse(TEXT("Applying non-video changes clears the dirty state"), Menu->HasPendingChanges());
			Test->TestTrue(TEXT("Apply updates master volume"), FMath::IsNearlyEqual(Settings->GetMasterVolume(), Edited.MasterVolume));
			Test->TestTrue(TEXT("Apply updates mouse sensitivity"), FMath::IsNearlyEqual(Settings->GetMouseSensitivity(), Edited.MouseSensitivity));
			Test->TestEqual(TEXT("Apply updates inverted look"), Settings->GetInvertMouseY(), Edited.bInvertMouseY);
			Settings->SetMasterVolume(Baseline.MasterVolume);
			Settings->SetMouseSensitivity(Baseline.MouseSensitivity);
			Settings->SetInvertMouseY(Baseline.bInvertMouseY);
			Settings->LoadSettings(true);
			Test->TestTrue(TEXT("Loading the saved settings restores committed master volume"), FMath::IsNearlyEqual(Settings->GetMasterVolume(), Edited.MasterVolume));
			Test->TestTrue(TEXT("Loading the saved settings restores committed sensitivity"), FMath::IsNearlyEqual(Settings->GetMouseSensitivity(), Edited.MouseSensitivity));
			Menu->BeginSettingsSession();
			Test->TestTrue(TEXT("A new session starts from the committed settings"), FMath::IsNearlyEqual(Menu->GetPendingSettings().MouseSensitivity, Edited.MouseSensitivity));
			return true;
		}
		if (Mode == TEXT("WindowChangedDuringSession"))
		{
			// A desktop resize can update the backend while the menu is open. The
			// player edits only window mode here; the old pending resolution was
			// never selected again and must not overwrite that newer actual size.
			const FIntPoint ActualResolution = Baseline.Resolution == FIntPoint(1600, 900)
				? FIntPoint(1920, 1080) : FIntPoint(1600, 900);
			Settings->SetScreenResolution(ActualResolution);
			Edited.WindowMode = Baseline.WindowMode == EWindowMode::Windowed
				? EWindowMode::WindowedFullscreen : EWindowMode::Windowed;
			Menu->SetPendingSettings(Edited);
			Menu->RequestApply();
			Test->TestTrue(TEXT("Changing window mode after a desktop resize still asks for display confirmation"), Menu->IsAwaitingVideoConfirmation());
			Test->TestTrue(TEXT("Display preview preserves the actual size when resolution was not edited"), Settings->GetScreenResolution() == ActualResolution);
			Menu->ConfirmVideoMode(false);
			Test->TestTrue(TEXT("Rejecting display preview restores the size immediately preceding preview"), Settings->GetScreenResolution() == ActualResolution);
			Test->TestEqual(TEXT("Rejecting display preview restores the window mode preceding preview"), Settings->GetFullscreenMode(), Baseline.WindowMode.GetValue());
			Test->TestTrue(TEXT("Rejecting synchronizes pending resolution to the restored actual size"), Menu->GetPendingSettings().Resolution == ActualResolution);
			Test->TestTrue(TEXT("Rejecting display preserves unrelated pending volume"), FMath::IsNearlyEqual(Menu->GetPendingSettings().MasterVolume, Edited.MasterVolume));
			Test->TestTrue(TEXT("Rejecting display leaves unrelated audio unapplied"), FMath::IsNearlyEqual(Settings->GetMasterVolume(), Baseline.MasterVolume));
			FAesirSettingsValues OnlyRestoredDisplay = Baseline;
			OnlyRestoredDisplay.Resolution = ActualResolution;
			Menu->SetPendingSettings(OnlyRestoredDisplay);
			Test->TestFalse(TEXT("The restored video snapshot is now the clean baseline after unrelated edits are reverted"), Menu->HasPendingChanges());
			return true;
		}
		Edited.Resolution = Baseline.Resolution == FIntPoint(1280, 720) ? FIntPoint(1600, 900) : FIntPoint(1280, 720);
		Edited.WindowMode = EWindowMode::Windowed;
		Menu->SetPendingSettings(Edited);
		Menu->RequestApply();
		Test->TestTrue(TEXT("A changed video mode starts a confirmation preview"), Menu->IsAwaitingVideoConfirmation());
		Test->TestTrue(TEXT("Video confirmation does not prematurely commit audio preferences"), FMath::IsNearlyEqual(Settings->GetMasterVolume(), Baseline.MasterVolume));
		if (Mode == TEXT("VideoReject"))
		{
			Menu->ConfirmVideoMode(false);
			Test->TestFalse(TEXT("Rejecting video mode closes its confirmation"), Menu->IsAwaitingVideoConfirmation());
			Test->TestTrue(TEXT("Rejected video mode restores the prior resolution"), Settings->GetScreenResolution() == Baseline.Resolution);
			Test->TestEqual(TEXT("Rejected video mode restores the prior window mode"), Settings->GetFullscreenMode(), Baseline.WindowMode.GetValue());
			Test->TestTrue(TEXT("Rejecting display preview retains unrelated edits for the player to continue adjusting"),
				FMath::IsNearlyEqual(Menu->GetPendingSettings().MasterVolume, Edited.MasterVolume) && Menu->HasPendingChanges());
			return true;
		}
		if (Mode == TEXT("VideoKeep"))
		{
			Menu->ConfirmVideoMode(true);
			Test->TestFalse(TEXT("Accepting video preview closes its confirmation"), Menu->IsAwaitingVideoConfirmation());
			Test->TestFalse(TEXT("Accepting video preview commits the complete pending session"), Menu->HasPendingChanges());
			Test->TestTrue(TEXT("Accepted video preview keeps the requested resolution"), Settings->GetScreenResolution() == Edited.Resolution);
			Test->TestEqual(TEXT("Accepted video preview keeps the requested window mode"), Settings->GetFullscreenMode(), Edited.WindowMode.GetValue());
			Test->TestTrue(TEXT("Accepting the display also commits the pending audio preference"), FMath::IsNearlyEqual(Settings->GetMasterVolume(), Edited.MasterVolume));
			Settings->LoadSettings(true);
			Test->TestTrue(TEXT("Accepted video mode is saved for a new session"), Settings->GetScreenResolution() == Edited.Resolution);
			return true;
		}
		if (Mode == TEXT("RemovedDuringPreview"))
		{
			Menu->RemoveFromParent();
			Test->TestFalse(TEXT("Removing the settings widget cannot leave a live video confirmation"), Menu->IsAwaitingVideoConfirmation());
			Test->TestTrue(TEXT("Removing a preview restores the prior resolution"), Settings->GetScreenResolution() == Baseline.Resolution);
			Test->TestEqual(TEXT("Removing a preview restores the prior window mode"), Settings->GetFullscreenMode(), Baseline.WindowMode.GetValue());
			return true;
		}
		PreviewStartedAt = FPlatformTime::Seconds();
		Stage = 1;
		return false;
	}
	const double WaitingTime = FPlatformTime::Seconds() - PreviewStartedAt;
	if (Menu->IsAwaitingVideoConfirmation())
	{
		if (WaitingTime > 20.0)
		{
			Test->AddError(TEXT("The unconfirmed video mode did not roll back within 20 real seconds."));
			return true;
		}
		return false;
	}
	Test->TestTrue(TEXT("The video preview provides about 15 real seconds to confirm"), WaitingTime >= 14.5);
	Test->TestTrue(TEXT("An unattended video preview restores the original resolution"), Settings->GetScreenResolution() == Baseline.Resolution);
	Test->TestEqual(TEXT("An unattended preview restores the original window mode"), Settings->GetFullscreenMode(), Baseline.WindowMode.GetValue());
	Test->TestTrue(TEXT("An unattended preview never commits unrelated pending audio"), FMath::IsNearlyEqual(Settings->GetMasterVolume(), Baseline.MasterVolume));
	return true;
}

} // namespace AesirSettingsTests

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FAesirSettingsSessionTest, "Aesir.Settings.Session",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FAesirSettingsSessionTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	for (const TCHAR* Mode : { TEXT("ControlEvents"), TEXT("PendingAndDiscard"), TEXT("RestoreDefaults"), TEXT("ApplyAndPersistence"), TEXT("WindowChangedDuringSession"), TEXT("VideoReject"), TEXT("VideoKeep"), TEXT("RemovedDuringPreview"), TEXT("VideoTimeout") })
	{
		OutBeautifiedNames.Add(Mode);
		OutTestCommands.Add(Mode);
	}
}

bool FAesirSettingsSessionTest::RunTest(const FString& Parameters)
{
	using namespace AesirSettingsTests;
	TSharedPtr<FSettingsWorld> Fixture = MakeShared<FSettingsWorld>();
	if (!TestNotNull(TEXT("The project uses UAesirGameUserSettings"), Fixture->Settings) ||
		!TestNotNull(TEXT("A local controller is available"), Fixture->Controller) || !Fixture->CreateMenu(this)) return false;
	AddCommand(new FExerciseSettings(this, Fixture, Parameters));
	return true;
}

#endif // WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
