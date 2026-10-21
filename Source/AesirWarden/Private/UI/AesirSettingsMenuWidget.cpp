// Copyright (c) 2026 YYchainsAw. All rights reserved.

#include "UI/AesirSettingsMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Settings/AesirGameUserSettings.h"

#define LOCTEXT_NAMESPACE "AesirSettings"

namespace AesirSettings
{
	const TArray<FString>& QualityNames()
	{
		static const TArray<FString> Names = {TEXT("低"), TEXT("中"), TEXT("高"), TEXT("极高"), TEXT("影视")};
		return Names;
	}

	FString ResolutionText(FIntPoint Size) { return FString::Printf(TEXT("%d × %d"), Size.X, Size.Y); }

	bool SameValues(const FAesirSettingsValues& A, const FAesirSettingsValues& B)
	{
		return A.Resolution == B.Resolution && A.WindowMode == B.WindowMode && A.bVSync == B.bVSync &&
			FMath::IsNearlyEqual(A.FrameRateLimit, B.FrameRateLimit) && A.OverallQuality == B.OverallQuality &&
			A.ShadowQuality == B.ShadowQuality && A.TextureQuality == B.TextureQuality && A.EffectsQuality == B.EffectsQuality &&
			FMath::IsNearlyEqual(A.MasterVolume, B.MasterVolume) && FMath::IsNearlyEqual(A.MouseSensitivity, B.MouseSensitivity) &&
			A.bInvertMouseY == B.bInvertMouseY;
	}
}

TSharedRef<SWidget> UAesirSettingsMenuWidget::RebuildWidget()
{
	BuildDefaultSettingsTree(WidgetTree);
	return Super::RebuildWidget();
}

void UAesirSettingsMenuWidget::ResolveControls()
{
	auto Find = [this](const TCHAR* Name) { return GetWidgetFromName(FName(Name)); };
	TabButtons = {Cast<UButton>(Find(TEXT("Btn_TabDisplay"))), Cast<UButton>(Find(TEXT("Btn_TabQuality"))),
		Cast<UButton>(Find(TEXT("Btn_TabAudio"))), Cast<UButton>(Find(TEXT("Btn_TabControls")))};
	TabLabels = {Cast<UTextBlock>(Find(TEXT("Txt_TabDisplay"))), Cast<UTextBlock>(Find(TEXT("Txt_TabQuality"))),
		Cast<UTextBlock>(Find(TEXT("Txt_TabAudio"))), Cast<UTextBlock>(Find(TEXT("Txt_TabControls")))};
	SettingsPages = Cast<UWidgetSwitcher>(Find(TEXT("SettingsPages")));
	SettingsContent = Find(TEXT("SettingsContent"));
	WindowModeControl = Cast<UComboBoxString>(Find(TEXT("Cmb_WindowMode")));
	ResolutionControl = Cast<UComboBoxString>(Find(TEXT("Cmb_Resolution")));
	FrameLimitControl = Cast<UComboBoxString>(Find(TEXT("Cmb_FrameLimit")));
	OverallQualityControl = Cast<UComboBoxString>(Find(TEXT("Cmb_OverallQuality")));
	ShadowQualityControl = Cast<UComboBoxString>(Find(TEXT("Cmb_ShadowQuality")));
	TextureQualityControl = Cast<UComboBoxString>(Find(TEXT("Cmb_TextureQuality")));
	EffectsQualityControl = Cast<UComboBoxString>(Find(TEXT("Cmb_EffectsQuality")));
	VSyncControl = Cast<UCheckBox>(Find(TEXT("Chk_VSync")));
	InvertYControl = Cast<UCheckBox>(Find(TEXT("Chk_InvertY")));
	MasterVolumeControl = Cast<USlider>(Find(TEXT("Sld_MasterVolume")));
	SensitivityControl = Cast<USlider>(Find(TEXT("Sld_MouseSensitivity")));
	VolumeValue = Cast<UTextBlock>(Find(TEXT("Txt_VolumeValue")));
	SensitivityValue = Cast<UTextBlock>(Find(TEXT("Txt_SensitivityValue")));
	StatusText = Cast<UTextBlock>(Find(TEXT("Txt_SettingsStatus")));
	ApplyButton = Cast<UButton>(Find(TEXT("Btn_Apply")));
	BackButton = Cast<UButton>(Find(TEXT("Btn_Back")));
	DefaultsButton = Cast<UButton>(Find(TEXT("Btn_Defaults")));
	ConfirmationPanel = Find(TEXT("ConfirmPanel"));
	ConfirmationText = Cast<UTextBlock>(Find(TEXT("Txt_Confirmation")));
	ConfirmYesButton = Cast<UButton>(Find(TEXT("Btn_ConfirmYes")));
	ConfirmNoButton = Cast<UButton>(Find(TEXT("Btn_ConfirmNo")));
	ConfirmYesLabel = Cast<UTextBlock>(Find(TEXT("Txt_ConfirmYes")));
	ConfirmNoLabel = Cast<UTextBlock>(Find(TEXT("Txt_ConfirmNo")));
}

void UAesirSettingsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	ResolveControls();
	if (TabButtons.IsValidIndex(0) && TabButtons[0]) TabButtons[0]->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDisplayTab);
	if (TabButtons.IsValidIndex(1) && TabButtons[1]) TabButtons[1]->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleQualityTab);
	if (TabButtons.IsValidIndex(2) && TabButtons[2]) TabButtons[2]->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleAudioTab);
	if (TabButtons.IsValidIndex(3) && TabButtons[3]) TabButtons[3]->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleControlsTab);
	if (ApplyButton) ApplyButton->OnClicked.AddUniqueDynamic(this, &ThisClass::RequestApply);
	if (BackButton) BackButton->OnClicked.AddUniqueDynamic(this, &ThisClass::RequestClose);
	if (DefaultsButton) DefaultsButton->OnClicked.AddUniqueDynamic(this, &ThisClass::RestoreDefaults);
	if (ConfirmYesButton) ConfirmYesButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirmYes);
	if (ConfirmNoButton) ConfirmNoButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirmNo);
	if (WindowModeControl) WindowModeControl->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleWindowMode);
	if (ResolutionControl) ResolutionControl->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleResolution);
	if (FrameLimitControl) FrameLimitControl->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleFrameLimit);
	if (OverallQualityControl) OverallQualityControl->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleOverallQuality);
	if (ShadowQualityControl) ShadowQualityControl->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleShadowQuality);
	if (TextureQualityControl) TextureQualityControl->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleTextureQuality);
	if (EffectsQualityControl) EffectsQualityControl->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleEffectsQuality);
	if (VSyncControl) VSyncControl->OnCheckStateChanged.AddUniqueDynamic(this, &ThisClass::HandleVSync);
	if (InvertYControl) InvertYControl->OnCheckStateChanged.AddUniqueDynamic(this, &ThisClass::HandleInvertY);
	if (MasterVolumeControl) MasterVolumeControl->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleMasterVolume);
	if (SensitivityControl) SensitivityControl->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleSensitivity);
	BeginSettingsSession();
}

void UAesirSettingsMenuWidget::NativeDestruct()
{
	if (bAwaitingVideo) RollbackVideoPreview();
	FTSTicker::RemoveTicker(ConfirmationTicker);
	ConfirmationTicker.Reset();
	for (UButton* Button : TabButtons) if (Button) Button->OnClicked.RemoveAll(this);
	for (UButton* Button : {ApplyButton.Get(), BackButton.Get(), DefaultsButton.Get(), ConfirmYesButton.Get(), ConfirmNoButton.Get()})
		if (Button) Button->OnClicked.RemoveAll(this);
	for (UComboBoxString* Combo : {WindowModeControl.Get(), ResolutionControl.Get(), FrameLimitControl.Get(), OverallQualityControl.Get(),
		ShadowQualityControl.Get(), TextureQualityControl.Get(), EffectsQualityControl.Get()})
		if (Combo) Combo->OnSelectionChanged.RemoveAll(this);
	if (VSyncControl) VSyncControl->OnCheckStateChanged.RemoveAll(this);
	if (InvertYControl) InvertYControl->OnCheckStateChanged.RemoveAll(this);
	if (MasterVolumeControl) MasterVolumeControl->OnValueChanged.RemoveAll(this);
	if (SensitivityControl) SensitivityControl->OnValueChanged.RemoveAll(this);
	bSessionActive = false;
	Super::NativeDestruct();
}

FAesirSettingsValues UAesirSettingsMenuWidget::ReadSettings() const
{
	FAesirSettingsValues Values;
	if (const UAesirGameUserSettings* Settings = UAesirGameUserSettings::Get())
	{
		Values.Resolution = Settings->GetScreenResolution();
		Values.WindowMode = Settings->GetFullscreenMode();
		Values.bVSync = Settings->IsVSyncEnabled();
		Values.FrameRateLimit = Settings->GetFrameRateLimit();
		Values.OverallQuality = Settings->GetOverallScalabilityLevel();
		Values.ShadowQuality = Settings->GetShadowQuality();
		Values.TextureQuality = Settings->GetTextureQuality();
		Values.EffectsQuality = Settings->GetVisualEffectQuality();
		Values.MasterVolume = Settings->GetMasterVolume();
		Values.MouseSensitivity = Settings->GetMouseSensitivity();
		Values.bInvertMouseY = Settings->GetInvertMouseY();
	}
	return Values;
}

void UAesirSettingsMenuWidget::BeginSettingsSession()
{
	if (bSessionActive) { RefreshControls(); UpdateConfirmation(); return; }
	Baseline = Pending = ReadSettings();
	if (const UAesirGameUserSettings* Settings = UAesirGameUserSettings::Get())
	{
		OriginalQuality = PendingQuality = Settings->ScalabilityQuality;
	}
	bSessionActive = true;
	bAwaitingDiscard = false;
	bQualityPresetEdited = false;
	PopulateOptions();
	RefreshControls();
	SetActiveCategory(0);
	UpdateConfirmation();
	if (TabButtons.IsValidIndex(0) && TabButtons[0] && GetOwningPlayer()) TabButtons[0]->SetUserFocus(GetOwningPlayer());
}

void UAesirSettingsMenuWidget::PopulateOptions()
{
	TGuardValue<bool> Guard(bRefreshing, true);
	WindowModes = {EWindowMode::Fullscreen, EWindowMode::WindowedFullscreen, EWindowMode::Windowed};
	if (WindowModeControl)
	{
		WindowModeControl->ClearOptions();
		for (const TCHAR* Name : {TEXT("全屏"), TEXT("无边框窗口"), TEXT("窗口")}) WindowModeControl->AddOption(Name);
	}
	ResolutionOptions.Reset();
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(ResolutionOptions);
	ResolutionOptions.RemoveAll([](FIntPoint Size) { return Size.X < 640 || Size.Y < 360; });
	if (Pending.Resolution.X > 0 && Pending.Resolution.Y > 0) ResolutionOptions.AddUnique(Pending.Resolution);
	if (const UAesirGameUserSettings* Settings = UAesirGameUserSettings::Get())
	{
		const FIntPoint Desktop = Settings->GetDesktopResolution();
		if (Desktop.X > 0 && Desktop.Y > 0) ResolutionOptions.AddUnique(Desktop);
	}
	ResolutionOptions.Sort([](FIntPoint A, FIntPoint B) { return A.X == B.X ? A.Y < B.Y : A.X < B.X; });
	if (ResolutionControl)
	{
		ResolutionControl->ClearOptions();
		for (FIntPoint Size : ResolutionOptions) ResolutionControl->AddOption(AesirSettings::ResolutionText(Size));
	}
	FrameLimits = {30.0f, 60.0f, 120.0f, 0.0f};
	if (!FrameLimits.Contains(Pending.FrameRateLimit)) FrameLimits.Add(Pending.FrameRateLimit);
	if (FrameLimitControl)
	{
		FrameLimitControl->ClearOptions();
		for (float Limit : FrameLimits) FrameLimitControl->AddOption(Limit <= 0.0f ? TEXT("不限") : FString::Printf(TEXT("%.0f"), Limit));
	}
	for (UComboBoxString* Combo : {OverallQualityControl.Get(), ShadowQualityControl.Get(), TextureQualityControl.Get(), EffectsQualityControl.Get()})
	{
		if (!Combo) continue;
		Combo->ClearOptions();
		for (const FString& Name : AesirSettings::QualityNames()) Combo->AddOption(Name);
	}
	if (OverallQualityControl) OverallQualityControl->AddOption(TEXT("自定义"));
	if (MasterVolumeControl) { MasterVolumeControl->SetMinValue(0.0f); MasterVolumeControl->SetMaxValue(1.0f); }
	if (SensitivityControl) { SensitivityControl->SetMinValue(0.1f); SensitivityControl->SetMaxValue(3.0f); }
}

void UAesirSettingsMenuWidget::RefreshControls()
{
	TGuardValue<bool> Guard(bRefreshing, true);
	if (WindowModeControl) WindowModeControl->SetSelectedIndex(WindowModes.IndexOfByKey(Pending.WindowMode));
	if (ResolutionControl)
	{
		ResolutionControl->SetSelectedOption(AesirSettings::ResolutionText(Pending.Resolution));
		ResolutionControl->SetIsEnabled(Pending.WindowMode != EWindowMode::WindowedFullscreen);
	}
	if (FrameLimitControl) FrameLimitControl->SetSelectedIndex(FrameLimits.IndexOfByKey(Pending.FrameRateLimit));
	if (OverallQualityControl) OverallQualityControl->SetSelectedIndex(Pending.OverallQuality < 0 ? 5 : Pending.OverallQuality);
	if (ShadowQualityControl) ShadowQualityControl->SetSelectedIndex(Pending.ShadowQuality);
	if (TextureQualityControl) TextureQualityControl->SetSelectedIndex(Pending.TextureQuality);
	if (EffectsQualityControl) EffectsQualityControl->SetSelectedIndex(Pending.EffectsQuality);
	if (VSyncControl) VSyncControl->SetIsChecked(Pending.bVSync);
	if (InvertYControl) InvertYControl->SetIsChecked(Pending.bInvertMouseY);
	if (UTextBlock* Label = Cast<UTextBlock>(GetWidgetFromName(TEXT("Txt_VSyncValue"))))
		Label->SetText(Pending.bVSync ? LOCTEXT("On", "开启") : LOCTEXT("Off", "关闭"));
	if (UTextBlock* Label = Cast<UTextBlock>(GetWidgetFromName(TEXT("Txt_InvertYValue"))))
		Label->SetText(Pending.bInvertMouseY ? LOCTEXT("On", "开启") : LOCTEXT("Off", "关闭"));
	if (MasterVolumeControl) MasterVolumeControl->SetValue(Pending.MasterVolume);
	if (SensitivityControl) SensitivityControl->SetValue(Pending.MouseSensitivity);
	if (VolumeValue) VolumeValue->SetText(FText::AsPercent(Pending.MasterVolume));
	if (SensitivityValue) SensitivityValue->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), Pending.MouseSensitivity)));
	UpdateActions();
}

void UAesirSettingsMenuWidget::SetPendingSettings(const FAesirSettingsValues& Values)
{
	if (!bSessionActive || bAwaitingVideo || bAwaitingDiscard) return;
	const int32 PreviousOverall = Pending.OverallQuality;
	Pending = Values;
	if (Pending.WindowMode != EWindowMode::Fullscreen && Pending.WindowMode != EWindowMode::WindowedFullscreen && Pending.WindowMode != EWindowMode::Windowed)
		Pending.WindowMode = Baseline.WindowMode;
	if (Pending.Resolution.X <= 0 || Pending.Resolution.Y <= 0) Pending.Resolution = Baseline.Resolution;
	Pending.MasterVolume = FMath::IsFinite(Pending.MasterVolume) ? FMath::Clamp(Pending.MasterVolume, 0.0f, 1.0f) : Baseline.MasterVolume;
	Pending.MouseSensitivity = FMath::IsFinite(Pending.MouseSensitivity) ? FMath::Clamp(Pending.MouseSensitivity, 0.1f, 3.0f) : Baseline.MouseSensitivity;
	Pending.FrameRateLimit = FMath::IsFinite(Pending.FrameRateLimit) ? FMath::Max(Pending.FrameRateLimit, 0.0f) : Baseline.FrameRateLimit;
	Pending.OverallQuality = FMath::Clamp(Pending.OverallQuality, -1, 4);
	Pending.ShadowQuality = FMath::Clamp(Pending.ShadowQuality, 0, 4);
	Pending.TextureQuality = FMath::Clamp(Pending.TextureQuality, 0, 4);
	Pending.EffectsQuality = FMath::Clamp(Pending.EffectsQuality, 0, 4);
	if (Pending.OverallQuality >= 0 && Pending.OverallQuality != PreviousOverall)
	{
		PendingQuality.SetFromSingleQualityLevel(Pending.OverallQuality);
		bQualityPresetEdited = true;
	}
	PendingQuality.ShadowQuality = Pending.ShadowQuality;
	PendingQuality.TextureQuality = Pending.TextureQuality;
	PendingQuality.EffectsQuality = Pending.EffectsQuality;
	RefreshQualityPreset();
	RefreshControls();
}

bool UAesirSettingsMenuWidget::HasPendingChanges() const
{
	return bSessionActive && (!AesirSettings::SameValues(Baseline, Pending) || !(OriginalQuality == PendingQuality));
}

void UAesirSettingsMenuWidget::UpdateActions()
{
	if (ApplyButton) ApplyButton->SetIsEnabled(HasPendingChanges() && !bAwaitingVideo && !bAwaitingDiscard && UAesirGameUserSettings::Get());
	if (StatusText) StatusText->SetText(UAesirGameUserSettings::Get()
		? HasPendingChanges() ? LOCTEXT("Unsaved", "有尚未应用的修改") : FText::GetEmpty()
		: LOCTEXT("Unavailable", "设置暂不可用，请重新启动游戏。"));
}

void UAesirSettingsMenuWidget::WriteSettings(const FAesirSettingsValues& Values, bool bApplyResolution)
{
	if (UAesirGameUserSettings* Settings = UAesirGameUserSettings::Get())
	{
		if (Values.WindowMode != Baseline.WindowMode) Settings->SetFullscreenMode(Values.WindowMode);
		if (Values.Resolution != Baseline.Resolution) Settings->SetScreenResolution(Values.Resolution);
		if (Values.bVSync != Baseline.bVSync) Settings->SetVSyncEnabled(Values.bVSync);
		if (!FMath::IsNearlyEqual(Values.FrameRateLimit, Baseline.FrameRateLimit)) Settings->SetFrameRateLimit(Values.FrameRateLimit);
		if (bQualityPresetEdited) Settings->ScalabilityQuality = PendingQuality;
		else
		{
			if (Values.ShadowQuality != Baseline.ShadowQuality) Settings->SetShadowQuality(Values.ShadowQuality);
			if (Values.TextureQuality != Baseline.TextureQuality) Settings->SetTextureQuality(Values.TextureQuality);
			if (Values.EffectsQuality != Baseline.EffectsQuality) Settings->SetVisualEffectQuality(Values.EffectsQuality);
		}
		if (!FMath::IsNearlyEqual(Values.MasterVolume, Baseline.MasterVolume)) Settings->SetMasterVolume(Values.MasterVolume);
		if (!FMath::IsNearlyEqual(Values.MouseSensitivity, Baseline.MouseSensitivity)) Settings->SetMouseSensitivity(Values.MouseSensitivity);
		if (Values.bInvertMouseY != Baseline.bInvertMouseY) Settings->SetInvertMouseY(Values.bInvertMouseY);
		if (bApplyResolution) Settings->ApplyResolutionSettings(false);
		Settings->ApplyNonResolutionSettings();
		Settings->ApplyRuntimeSettings(GetWorld());
	}
}

void UAesirSettingsMenuWidget::RequestApply()
{
	UAesirGameUserSettings* Settings = UAesirGameUserSettings::Get();
	if (!bSessionActive || !Settings || !HasPendingChanges() || bAwaitingVideo || bAwaitingDiscard) return;
	if (Pending.WindowMode != Baseline.WindowMode || Pending.Resolution != Baseline.Resolution)
	{
		// Apply only the display preview. Other pending changes remain unapplied
		// until the player confirms, and no unconfirmed values are written to disk.
		// Preserve any external window resize since this settings session opened.
		VideoRestoreResolution = Settings->GetScreenResolution();
		VideoRestoreWindowMode = Settings->GetFullscreenMode();
		if (Pending.Resolution == Baseline.Resolution) Pending.Resolution = VideoRestoreResolution;
		if (Pending.WindowMode == Baseline.WindowMode) Pending.WindowMode = VideoRestoreWindowMode;
		Settings->SetFullscreenMode(Pending.WindowMode);
		Settings->SetScreenResolution(Pending.Resolution);
		Settings->ApplyResolutionSettings(false);
		bAwaitingVideo = true;
		VideoDeadline = FPlatformTime::Seconds() + 15.0;
		ConfirmationTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this,
			[this](float DeltaTime) { UpdateConfirmation(); return bAwaitingVideo; }), 0.1f);
		UpdateConfirmation();
		if (ConfirmNoButton && GetOwningPlayer()) ConfirmNoButton->SetUserFocus(GetOwningPlayer());
	}
	else CommitSettings();
}

void UAesirSettingsMenuWidget::CommitSettings()
{
	UAesirGameUserSettings* Settings = UAesirGameUserSettings::Get();
	if (!Settings) return;
	WriteSettings(Pending, false);
	if (Pending.WindowMode != Baseline.WindowMode || Pending.Resolution != Baseline.Resolution) Settings->ConfirmVideoMode();
	Settings->SaveSettings();
	Baseline = Pending = ReadSettings();
	OriginalQuality = PendingQuality = Settings->ScalabilityQuality;
	bQualityPresetEdited = false;
	RefreshControls();
	if (StatusText) StatusText->SetText(LOCTEXT("Saved", "设置已保存"));
}

void UAesirSettingsMenuWidget::RollbackVideoPreview()
{
	if (!bAwaitingVideo) return;
	bAwaitingVideo = false;
	FTSTicker::RemoveTicker(ConfirmationTicker);
	ConfirmationTicker.Reset();
	if (UAesirGameUserSettings* Settings = UAesirGameUserSettings::Get())
	{
		Settings->SetFullscreenMode(VideoRestoreWindowMode);
		Settings->SetScreenResolution(VideoRestoreResolution);
		Settings->ApplyResolutionSettings(false);
	}
	Pending.WindowMode = Baseline.WindowMode = VideoRestoreWindowMode;
	Pending.Resolution = Baseline.Resolution = VideoRestoreResolution;
	PopulateOptions();
	RefreshControls();
}

void UAesirSettingsMenuWidget::ConfirmVideoMode(bool bKeep)
{
	if (!bAwaitingVideo) return;
	if (bKeep && FPlatformTime::Seconds() < VideoDeadline)
	{
		bAwaitingVideo = false;
		FTSTicker::RemoveTicker(ConfirmationTicker);
		ConfirmationTicker.Reset();
		CommitSettings();
	}
	else RollbackVideoPreview();
	UpdateConfirmation();
	if (BackButton && GetOwningPlayer()) BackButton->SetUserFocus(GetOwningPlayer());
}

void UAesirSettingsMenuWidget::UpdateConfirmation()
{
	if (bAwaitingVideo && FPlatformTime::Seconds() >= VideoDeadline)
	{
		ConfirmVideoMode(false);
		if (StatusText) StatusText->SetText(LOCTEXT("VideoReverted", "显示设置已还原"));
		return;
	}
	const bool bModal = bAwaitingVideo || bAwaitingDiscard;
	if (SettingsContent) SettingsContent->SetVisibility(bModal ? ESlateVisibility::Hidden : ESlateVisibility::SelfHitTestInvisible);
	if (ConfirmationPanel) ConfirmationPanel->SetVisibility(bModal ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (ConfirmationText) ConfirmationText->SetText(bAwaitingVideo
		? FText::Format(LOCTEXT("KeepVideo", "是否保留显示设置？\n{0} 秒后自动还原"), FText::AsNumber(FMath::Max(0, FMath::CeilToInt(VideoDeadline - FPlatformTime::Seconds()))))
		: LOCTEXT("DiscardPrompt", "有尚未应用的修改。\n是否放弃修改并返回？"));
	if (ConfirmYesLabel) ConfirmYesLabel->SetText(bAwaitingVideo ? LOCTEXT("Keep", "保留") : LOCTEXT("Discard", "放弃修改"));
	if (ConfirmNoLabel) ConfirmNoLabel->SetText(bAwaitingVideo ? LOCTEXT("Revert", "还原") : LOCTEXT("Continue", "继续调整"));
	UpdateActions();
}

void UAesirSettingsMenuWidget::RequestClose()
{
	if (!bSessionActive) return;
	if (bAwaitingVideo) { ConfirmVideoMode(false); return; }
	if (HasPendingChanges())
	{
		bAwaitingDiscard = true;
		UpdateConfirmation();
		if (ConfirmNoButton && GetOwningPlayer()) ConfirmNoButton->SetUserFocus(GetOwningPlayer());
	}
	else CloseSession();
}

void UAesirSettingsMenuWidget::ContinueEditing()
{
	if (!bAwaitingDiscard) return;
	bAwaitingDiscard = false;
	UpdateConfirmation();
	if (BackButton && GetOwningPlayer()) BackButton->SetUserFocus(GetOwningPlayer());
}

void UAesirSettingsMenuWidget::DiscardChangesAndClose()
{
	if (bAwaitingVideo) RollbackVideoPreview();
	Pending = Baseline;
	PendingQuality = OriginalQuality;
	bAwaitingDiscard = false;
	CloseSession();
}

void UAesirSettingsMenuWidget::CloseSession()
{
	if (!bSessionActive) return;
	bSessionActive = false;
	RemoveFromParent();
	OnClosed.Broadcast();
}

void UAesirSettingsMenuWidget::RestoreDefaults()
{
	if (!bSessionActive || bAwaitingDiscard || bAwaitingVideo) return;
	Pending = FAesirSettingsValues();
	if (const UAesirGameUserSettings* Settings = UAesirGameUserSettings::Get())
	{
		const FIntPoint Desktop = Settings->GetDesktopResolution();
		Pending.Resolution = Desktop.X > 0 && Desktop.Y > 0 ? Desktop : Baseline.Resolution;
	}
	PendingQuality.SetFromSingleQualityLevel(2);
	bQualityPresetEdited = true;
	PopulateOptions();
	RefreshControls();
}

void UAesirSettingsMenuWidget::SetActiveCategory(int32 Category)
{
	if (bAwaitingVideo || bAwaitingDiscard) return;
	ActiveCategory = FMath::Clamp(Category, 0, 3);
	if (SettingsPages) SettingsPages->SetActiveWidgetIndex(ActiveCategory);
	for (int32 Index = 0; Index < TabLabels.Num(); ++Index)
	{
		if (TabLabels[Index]) TabLabels[Index]->SetColorAndOpacity(FSlateColor(Index == ActiveCategory
			? FLinearColor(1.0f, 0.84f, 0.53f) : FLinearColor(0.75f, 0.72f, 0.64f)));
	}
}

void UAesirSettingsMenuWidget::RefreshQualityPreset()
{
	Pending.OverallQuality = PendingQuality.GetSingleQualityLevel();
}

FReply UAesirSettingsMenuWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Gamepad_FaceButton_Right)
	{
		if (bAwaitingDiscard) ContinueEditing(); else RequestClose();
		return FReply::Handled();
	}
	if (Event.GetKey() == EKeys::Gamepad_LeftShoulder || Event.GetKey() == EKeys::Gamepad_RightShoulder)
	{
		SetActiveCategory((ActiveCategory + (Event.GetKey() == EKeys::Gamepad_RightShoulder ? 1 : 3)) % 4);
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

void UAesirSettingsMenuWidget::HandleWindowMode(FString Value, ESelectInfo::Type SelectionType)
{
	if (bRefreshing || !WindowModeControl || !WindowModes.IsValidIndex(WindowModeControl->GetSelectedIndex())) return;
	Pending.WindowMode = WindowModes[WindowModeControl->GetSelectedIndex()];
	if (Pending.WindowMode == EWindowMode::WindowedFullscreen)
	{
		if (UAesirGameUserSettings* Settings = UAesirGameUserSettings::Get())
		{
			const FIntPoint Desktop = Settings->GetDesktopResolution();
			if (Desktop.X > 0 && Desktop.Y > 0) Pending.Resolution = Desktop;
		}
	}
	RefreshControls();
}
void UAesirSettingsMenuWidget::HandleResolution(FString Value, ESelectInfo::Type SelectionType)
{
	if (!bRefreshing && ResolutionControl && ResolutionOptions.IsValidIndex(ResolutionControl->GetSelectedIndex()))
	{
		Pending.Resolution = ResolutionOptions[ResolutionControl->GetSelectedIndex()];
		UpdateActions();
	}
}
void UAesirSettingsMenuWidget::HandleFrameLimit(FString Value, ESelectInfo::Type SelectionType)
{
	if (!bRefreshing && FrameLimitControl && FrameLimits.IsValidIndex(FrameLimitControl->GetSelectedIndex()))
	{
		Pending.FrameRateLimit = FrameLimits[FrameLimitControl->GetSelectedIndex()];
		UpdateActions();
	}
}
void UAesirSettingsMenuWidget::HandleOverallQuality(FString Value, ESelectInfo::Type SelectionType)
{
	if (bRefreshing || !OverallQualityControl) return;
	const int32 Index = OverallQualityControl->GetSelectedIndex();
	if (Index >= 0 && Index <= 4)
	{
		PendingQuality.SetFromSingleQualityLevel(Index);
		bQualityPresetEdited = true;
		Pending.OverallQuality = Index;
		Pending.ShadowQuality = PendingQuality.ShadowQuality;
		Pending.TextureQuality = PendingQuality.TextureQuality;
		Pending.EffectsQuality = PendingQuality.EffectsQuality;
	}
	RefreshControls();
}
void UAesirSettingsMenuWidget::HandleShadowQuality(FString Value, ESelectInfo::Type SelectionType)
{
	if (bRefreshing || !ShadowQualityControl) return;
	PendingQuality.ShadowQuality = Pending.ShadowQuality = FMath::Clamp(ShadowQualityControl->GetSelectedIndex(), 0, 4);
	RefreshQualityPreset(); RefreshControls();
}
void UAesirSettingsMenuWidget::HandleTextureQuality(FString Value, ESelectInfo::Type SelectionType)
{
	if (bRefreshing || !TextureQualityControl) return;
	PendingQuality.TextureQuality = Pending.TextureQuality = FMath::Clamp(TextureQualityControl->GetSelectedIndex(), 0, 4);
	RefreshQualityPreset(); RefreshControls();
}
void UAesirSettingsMenuWidget::HandleEffectsQuality(FString Value, ESelectInfo::Type SelectionType)
{
	if (bRefreshing || !EffectsQualityControl) return;
	PendingQuality.EffectsQuality = Pending.EffectsQuality = FMath::Clamp(EffectsQualityControl->GetSelectedIndex(), 0, 4);
	RefreshQualityPreset(); RefreshControls();
}
void UAesirSettingsMenuWidget::HandleVSync(bool Checked) { if (!bRefreshing) { Pending.bVSync = Checked; RefreshControls(); } }
void UAesirSettingsMenuWidget::HandleInvertY(bool Checked) { if (!bRefreshing) { Pending.bInvertMouseY = Checked; RefreshControls(); } }
void UAesirSettingsMenuWidget::HandleMasterVolume(float Value) { if (!bRefreshing) { Pending.MasterVolume = Value; RefreshControls(); } }
void UAesirSettingsMenuWidget::HandleSensitivity(float Value) { if (!bRefreshing) { Pending.MouseSensitivity = Value; RefreshControls(); } }
void UAesirSettingsMenuWidget::HandleDisplayTab() { SetActiveCategory(0); }
void UAesirSettingsMenuWidget::HandleQualityTab() { SetActiveCategory(1); }
void UAesirSettingsMenuWidget::HandleAudioTab() { SetActiveCategory(2); }
void UAesirSettingsMenuWidget::HandleControlsTab() { SetActiveCategory(3); }
void UAesirSettingsMenuWidget::HandleConfirmYes() { if (bAwaitingVideo) ConfirmVideoMode(true); else DiscardChangesAndClose(); }
void UAesirSettingsMenuWidget::HandleConfirmNo() { if (bAwaitingVideo) ConfirmVideoMode(false); else ContinueEditing(); }

#undef LOCTEXT_NAMESPACE
