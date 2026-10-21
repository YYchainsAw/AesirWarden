// Copyright (c) 2026 YYchainsAw. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameUserSettings.h"
#include "Containers/Ticker.h"
#include "AesirSettingsMenuWidget.generated.h"

class UButton;
class UCheckBox;
class UComboBoxString;
class USlider;
class UTextBlock;
class UWidgetSwitcher;
class UWidgetTree;

USTRUCT(BlueprintType)
struct FAesirSettingsValues
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FIntPoint Resolution = FIntPoint(1920, 1080);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TEnumAsByte<EWindowMode::Type> WindowMode = EWindowMode::WindowedFullscreen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bVSync = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float FrameRateLimit = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	int32 OverallQuality = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	int32 ShadowQuality = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	int32 TextureQuality = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	int32 EffectsQuality = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MasterVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MouseSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bInvertMouseY = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAesirSettingsClosed);

/** Stages user edits until Apply, with a timed, reversible video-mode preview. */
UCLASS(Blueprintable)
class AESIRWARDEN_API UAesirSettingsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Aesir|Settings")
	void BeginSettingsSession();

	UFUNCTION(BlueprintCallable, Category = "Aesir|Settings")
	void SetPendingSettings(const FAesirSettingsValues& Values);

	UFUNCTION(BlueprintPure, Category = "Aesir|Settings")
	FAesirSettingsValues GetPendingSettings() const { return Pending; }

	UFUNCTION(BlueprintPure, Category = "Aesir|Settings")
	bool HasPendingChanges() const;

	UFUNCTION(BlueprintPure, Category = "Aesir|Settings")
	bool IsAwaitingVideoConfirmation() const { return bAwaitingVideo; }

	UFUNCTION(BlueprintPure, Category = "Aesir|Settings")
	bool IsAwaitingDiscardConfirmation() const { return bAwaitingDiscard; }

	UFUNCTION(BlueprintCallable, Category = "Aesir|Settings")
	void RequestApply();

	UFUNCTION(BlueprintCallable, Category = "Aesir|Settings")
	void ConfirmVideoMode(bool bKeep);

	UFUNCTION(BlueprintCallable, Category = "Aesir|Settings")
	void RequestClose();

	UFUNCTION(BlueprintCallable, Category = "Aesir|Settings")
	void DiscardChangesAndClose();

	UFUNCTION(BlueprintCallable, Category = "Aesir|Settings")
	void ContinueEditing();

	UFUNCTION(BlueprintCallable, Category = "Aesir|Settings")
	void RestoreDefaults();

	UFUNCTION(BlueprintCallable, Category = "Aesir|Settings")
	void SetActiveCategory(int32 Category);

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Settings")
	FAesirSettingsClosed OnClosed;

	static void BuildDefaultSettingsTree(UWidgetTree* Tree);
	/** Add missing fullscreen backdrop layers without replacing authored controls. */
	static bool EnsureSettingsBackdrop(UWidgetTree* Tree);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
	FAesirSettingsValues ReadSettings() const;
	void WriteSettings(const FAesirSettingsValues& Values, bool bApplyResolution);
	void ResolveControls();
	void PopulateOptions();
	void RefreshControls();
	void UpdateActions();
	void UpdateConfirmation();
	void CloseSession();
	void CommitSettings();
	void RollbackVideoPreview();
	void RefreshQualityPreset();
	UFUNCTION() void HandleWindowMode(FString Value, ESelectInfo::Type SelectionType);
	UFUNCTION() void HandleResolution(FString Value, ESelectInfo::Type SelectionType);
	UFUNCTION() void HandleFrameLimit(FString Value, ESelectInfo::Type SelectionType);
	UFUNCTION() void HandleOverallQuality(FString Value, ESelectInfo::Type SelectionType);
	UFUNCTION() void HandleShadowQuality(FString Value, ESelectInfo::Type SelectionType);
	UFUNCTION() void HandleTextureQuality(FString Value, ESelectInfo::Type SelectionType);
	UFUNCTION() void HandleEffectsQuality(FString Value, ESelectInfo::Type SelectionType);
	UFUNCTION() void HandleVSync(bool Checked);
	UFUNCTION() void HandleInvertY(bool Checked);
	UFUNCTION() void HandleMasterVolume(float Value);
	UFUNCTION() void HandleSensitivity(float Value);
	UFUNCTION() void HandleDisplayTab();
	UFUNCTION() void HandleQualityTab();
	UFUNCTION() void HandleAudioTab();
	UFUNCTION() void HandleControlsTab();
	UFUNCTION() void HandleConfirmYes();
	UFUNCTION() void HandleConfirmNo();

	FAesirSettingsValues Baseline;
	FAesirSettingsValues Pending;
	Scalability::FQualityLevels OriginalQuality;
	Scalability::FQualityLevels PendingQuality;
	TArray<FIntPoint> ResolutionOptions;
	TArray<TEnumAsByte<EWindowMode::Type>> WindowModes;
	TArray<float> FrameLimits;
	FTSTicker::FDelegateHandle ConfirmationTicker;
	FIntPoint VideoRestoreResolution = FIntPoint::ZeroValue;
	EWindowMode::Type VideoRestoreWindowMode = EWindowMode::WindowedFullscreen;
	double VideoDeadline = 0.0;
	bool bRefreshing = false;
	bool bSessionActive = false;
	bool bAwaitingVideo = false;
	bool bAwaitingDiscard = false;
	bool bQualityPresetEdited = false;
	int32 ActiveCategory = 0;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> TabButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> TabLabels;
	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> SettingsPages;
	UPROPERTY(Transient) TObjectPtr<UWidget> SettingsContent;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> WindowModeControl;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> ResolutionControl;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> FrameLimitControl;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> OverallQualityControl;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> ShadowQualityControl;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> TextureQualityControl;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> EffectsQualityControl;
	UPROPERTY(Transient) TObjectPtr<UCheckBox> VSyncControl;
	UPROPERTY(Transient) TObjectPtr<UCheckBox> InvertYControl;
	UPROPERTY(Transient) TObjectPtr<USlider> MasterVolumeControl;
	UPROPERTY(Transient) TObjectPtr<USlider> SensitivityControl;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> VolumeValue;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SensitivityValue;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UButton> ApplyButton;
	UPROPERTY(Transient) TObjectPtr<UButton> BackButton;
	UPROPERTY(Transient) TObjectPtr<UButton> DefaultsButton;
	UPROPERTY(Transient) TObjectPtr<UWidget> ConfirmationPanel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ConfirmationText;
	UPROPERTY(Transient) TObjectPtr<UButton> ConfirmYesButton;
	UPROPERTY(Transient) TObjectPtr<UButton> ConfirmNoButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ConfirmYesLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ConfirmNoLabel;
};
