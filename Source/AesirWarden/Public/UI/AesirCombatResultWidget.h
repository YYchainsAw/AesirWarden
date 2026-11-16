// Copyright (c) 2026 YYchainsAw. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/AesirCombatPrototypeGameMode.h"
#include "AesirCombatResultWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAesirResultActionRequested);

/** Native wiring for the existing editable WBP_CombatResult. */
UCLASS(Blueprintable)
class AESIRWARDEN_API UAesirCombatResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Aesir|Match")
	void SetMatchResult(EAesirMatchResult Result);

	UFUNCTION(BlueprintPure, Category = "Aesir|Match")
	EAesirMatchResult GetDisplayedResult() const { return DisplayedResult; }

	UFUNCTION(BlueprintCallable, Category = "Aesir|Match")
	void FocusDefaultAction();

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Match")
	FAesirResultActionRequested OnRetryRequested;

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Match")
	FAesirResultActionRequested OnReturnToMenuRequested;

	UPROPERTY(BlueprintAssignable, Category = "Aesir|Match")
	FAesirResultActionRequested OnQuitRequested;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Aesir|Match")
	TObjectPtr<UTextBlock> TXT_Result;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Aesir|Match")
	TObjectPtr<UButton> BTN_Restart;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Aesir|Match")
	TObjectPtr<UButton> BTN_ReturnToMenu;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Aesir|Match")
	TObjectPtr<UButton> BTN_Quit;

private:
	void RefreshResult();
	UFUNCTION()
	void HandleRetry();
	UFUNCTION()
	void HandleReturnToMenu();
	UFUNCTION()
	void HandleQuit();

	UPROPERTY(Transient)
	EAesirMatchResult DisplayedResult = EAesirMatchResult::None;
};
