// Copyright (c) 2026 YYchainsAw. All rights reserved. Third-party assets belong to their respective owners.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AesirMainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;
class UWidgetTree;
class UAesirSettingsMenuWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAesirMainMenuActionRequested);

/** Campfire menu with an optional Blueprint Designer layout and a usable native fallback. */
UCLASS(Blueprintable)
class AESIRWARDEN_API UAesirMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Enable Start only once the level flow has prepared the opening area. Empty text hides status. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Main Menu")
	void SetStartAvailability(bool bEnabled, FText InStatusText);

	/** Call after adding to the viewport and setting the controller's UI input mode. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Main Menu")
	void FocusDefaultAction();

	/** Restore the four main actions after a settings or credits page. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Main Menu")
	void ReturnToMainMenu();

	UFUNCTION(BlueprintPure, Category = "Aesir|Main Menu")
	UAesirSettingsMenuWidget* GetActiveSettingsMenu() const { return ActiveSettingsMenu; }

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Main Menu")
	FAesirMainMenuActionRequested OnStartRequested;

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Main Menu")
	FAesirMainMenuActionRequested OnQuitRequested;

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Main Menu")
	FAesirMainMenuActionRequested OnSettingsRequested;

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Main Menu")
	FAesirMainMenuActionRequested OnCreditsRequested;

	/** Shared by the native fallback and the editor scaffold; leaves an authored tree intact. */
	static void BuildDefaultMenuTree(UWidgetTree* InWidgetTree);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Extend or replace the default information page in a Blueprint child. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Aesir|Main Menu", meta = (DisplayName = "On Settings Opened"))
	void ReceiveSettingsOpened();

	UFUNCTION(BlueprintImplementableEvent, Category = "Aesir|Main Menu", meta = (DisplayName = "On Credits Opened"))
	void ReceiveCreditsOpened();

	/** Keep these names in a Blueprint child to retain automatic native wiring. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Aesir|Main Menu")
	TObjectPtr<UButton> Btn_Start;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Aesir|Main Menu")
	TObjectPtr<UButton> Btn_Quit;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Aesir|Main Menu")
	TObjectPtr<UButton> Btn_Settings;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Aesir|Main Menu")
	TObjectPtr<UButton> Btn_Credits;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Aesir|Main Menu")
	TObjectPtr<UButton> Btn_Back;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Aesir|Main Menu")
	TObjectPtr<UWidget> MenuActions;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Aesir|Main Menu")
	TObjectPtr<UWidget> MenuDetails;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Aesir|Main Menu")
	TObjectPtr<UTextBlock> Txt_DetailsTitle;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Aesir|Main Menu")
	TObjectPtr<UTextBlock> Txt_DetailsBody;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Aesir|Main Menu")
	TObjectPtr<UTextBlock> Txt_Status;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aesir|Main Menu")
	bool bStartEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aesir|Main Menu")
	FText StatusText = NSLOCTEXT("AesirMainMenu", "PreparingCampfire", "正在准备营火……");

	/** Soft asset reference also keeps the editable settings page discoverable when cooking. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aesir|Main Menu")
	TSoftClassPtr<UAesirSettingsMenuWidget> SettingsMenuClass = TSoftClassPtr<UAesirSettingsMenuWidget>(
		FSoftObjectPath(TEXT("/Game/Aesir/UI/Menus/WBP_SettingsMenu.WBP_SettingsMenu_C")));

private:
	void RefreshAvailability();
	void ShowDetails(FText Title, FText Body);
	bool bShowingDetails = false;

	UPROPERTY(Transient)
	TObjectPtr<UAesirSettingsMenuWidget> ActiveSettingsMenu;

	UFUNCTION()
	void HandleSettingsClosed();

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleCreditsClicked();

	UFUNCTION()
	void HandleBackClicked();
};
