#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "AesirCompanionChatWidget.generated.h"

class UCompanionChatSubsystem;
class UScrollBox;
class UTextBlock;
struct FCompanionChatReply;

/** Native fallback chat panel. Authored widgets may inherit it and bind the optional controls. */
UCLASS()
class AESIRWARDEN_API UAesirCompanionChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ActivateChat();
	void DeactivateChat();
	void FocusChatInput();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> ChatInput;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Transcript;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CompanionName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> TranscriptScroll;

private:
	void BuildFallbackPanel();
	void AppendLine(const FString& Speaker, const FString& Message);

	UFUNCTION()
	void SendMessage();

	UFUNCTION()
	void RequestClose();

	UFUNCTION()
	void RetryMessage();

	UFUNCTION()
	void CancelMessage();

	UFUNCTION()
	void HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleReply(const FCompanionChatReply& Reply);

	UFUNCTION()
	void HandleFailure(const FString& Error);

	UFUNCTION()
	void ResetConversation();

	TWeakObjectPtr<UCompanionChatSubsystem> BoundChat;
	TArray<FString> TranscriptLines;
};
