#include "UI/AesirCompanionChatWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/AesirCombatPrototypePlayerController.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Services/Companion/CompanionChatSubsystem.h"

void UAesirCompanionChatWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildFallbackPanel();
	}
	if (ChatInput)
	{
		ChatInput->OnTextCommitted.AddUniqueDynamic(this, &UAesirCompanionChatWidget::HandleTextCommitted);
	}
}

void UAesirCompanionChatWidget::BuildFallbackPanel()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ChatCanvas"));
	WidgetTree->RootWidget = Canvas;
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.045f, 0.97f));
	Panel->SetPadding(FMargin(20.0f));
	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f));
	PanelSlot->SetSize(FVector2D(640.0f, 460.0f));

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Content);
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	Content->AddChildToVerticalBox(Header);
	CompanionName = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CompanionName"));
	Header->AddChildToHorizontalBox(CompanionName)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UButton* Close = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>();
	CloseLabel->SetText(NSLOCTEXT("CompanionChat", "Close", "关闭 (Esc)"));
	Close->SetContent(CloseLabel);
	Close->OnClicked.AddUniqueDynamic(this, &UAesirCompanionChatWidget::RequestClose);
	Header->AddChildToHorizontalBox(Close);

	TranscriptScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("TranscriptScroll"));
	UVerticalBoxSlot* ScrollSlot = Content->AddChildToVerticalBox(TranscriptScroll);
	ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ScrollSlot->SetPadding(FMargin(0.0f, 12.0f));
	Transcript = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Transcript"));
	Transcript->SetAutoWrapText(true);
	TranscriptScroll->AddChild(Transcript);

	UHorizontalBox* Composer = WidgetTree->ConstructWidget<UHorizontalBox>();
	Content->AddChildToVerticalBox(Composer);
	ChatInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("ChatInput"));
	ChatInput->SetHintText(NSLOCTEXT("CompanionChat", "Hint", "聊天，或输入“艾莉 跟随我”“艾莉 原地等我”…"));
	ChatInput->SetClearKeyboardFocusOnCommit(false);
	Composer->AddChildToHorizontalBox(ChatInput)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UButton* Send = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* SendLabel = WidgetTree->ConstructWidget<UTextBlock>();
	SendLabel->SetText(NSLOCTEXT("CompanionChat", "Send", "发送"));
	Send->SetContent(SendLabel);
	Send->OnClicked.AddUniqueDynamic(this, &UAesirCompanionChatWidget::SendMessage);
	Composer->AddChildToHorizontalBox(Send)->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));

	UHorizontalBox* Recovery = WidgetTree->ConstructWidget<UHorizontalBox>();
	Content->AddChildToVerticalBox(Recovery)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
	UButton* Retry = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* RetryLabel = WidgetTree->ConstructWidget<UTextBlock>();
	RetryLabel->SetText(NSLOCTEXT("CompanionChat", "Retry", "重试上一条"));
	Retry->SetContent(RetryLabel);
	Retry->OnClicked.AddUniqueDynamic(this, &UAesirCompanionChatWidget::RetryMessage);
	Recovery->AddChildToHorizontalBox(Retry);
	UButton* Cancel = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* CancelLabel = WidgetTree->ConstructWidget<UTextBlock>();
	CancelLabel->SetText(NSLOCTEXT("CompanionChat", "Cancel", "取消请求"));
	Cancel->SetContent(CancelLabel);
	Cancel->OnClicked.AddUniqueDynamic(this, &UAesirCompanionChatWidget::CancelMessage);
	Recovery->AddChildToHorizontalBox(Cancel)->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
}

void UAesirCompanionChatWidget::ActivateChat()
{
	DeactivateChat();
	if (UGameInstance* Instance = GetGameInstance())
	{
		BoundChat = Instance->GetSubsystem<UCompanionChatSubsystem>();
	}
	if (UCompanionChatSubsystem* Chat = BoundChat.Get())
	{
		Chat->OnReplyReceived.AddUniqueDynamic(this, &UAesirCompanionChatWidget::HandleReply);
		Chat->OnRequestFailed.AddUniqueDynamic(this, &UAesirCompanionChatWidget::HandleFailure);
		Chat->OnConversationReset.AddUniqueDynamic(this, &UAesirCompanionChatWidget::ResetConversation);
	}
	ResetConversation();
	FocusChatInput();
}

void UAesirCompanionChatWidget::DeactivateChat()
{
	if (UCompanionChatSubsystem* Chat = BoundChat.Get())
	{
		Chat->OnReplyReceived.RemoveDynamic(this, &UAesirCompanionChatWidget::HandleReply);
		Chat->OnRequestFailed.RemoveDynamic(this, &UAesirCompanionChatWidget::HandleFailure);
		Chat->OnConversationReset.RemoveDynamic(this, &UAesirCompanionChatWidget::ResetConversation);
	}
	BoundChat.Reset();
}

void UAesirCompanionChatWidget::FocusChatInput()
{
	if (ChatInput)
	{
		ChatInput->SetUserFocus(GetOwningPlayer());
		ChatInput->SetKeyboardFocus();
	}
	else
	{
		SetKeyboardFocus();
	}
}

void UAesirCompanionChatWidget::ResetConversation()
{
	TranscriptLines.Reset();
	if (Transcript)
	{
		Transcript->SetText(FText::GetEmpty());
	}
	if (ChatInput)
	{
		ChatInput->SetText(FText::GetEmpty());
	}
	if (CompanionName)
	{
		const UCompanionChatSubsystem* Chat = BoundChat.Get();
		CompanionName->SetText(FText::FromString(Chat ? Chat->GetSelectedCompanionId() : FString()));
	}
}

void UAesirCompanionChatWidget::AppendLine(const FString& Speaker, const FString& Message)
{
	if (Message.IsEmpty())
	{
		return;
	}
	TranscriptLines.Add(Speaker + TEXT(": ") + Message);
	if (TranscriptLines.Num() > 100)
	{
		TranscriptLines.RemoveAt(0, TranscriptLines.Num() - 100);
	}
	if (Transcript)
	{
		Transcript->SetText(FText::FromString(FString::Join(TranscriptLines, TEXT("\n\n"))));
	}
	if (TranscriptScroll)
	{
		TranscriptScroll->ScrollToEnd();
	}
}

void UAesirCompanionChatWidget::SendMessage()
{
	UCompanionChatSubsystem* Chat = BoundChat.Get();
	if (!ChatInput || !Chat || !IsVisible())
	{
		return;
	}
	const FString Text = ChatInput->GetText().ToString().TrimStartAndEnd();
	if (Text.IsEmpty())
	{
		return;
	}
	if (Text.Len() > 500)
	{
		HandleFailure(TEXT("消息不能超过 500 个字符。"));
		return;
	}
	AppendLine(TEXT("你"), Text);
	ChatInput->SetText(FText::GetEmpty());
	Chat->SendChatMessage(Text);
	FocusChatInput();
}

void UAesirCompanionChatWidget::HandleTextCommitted(const FText&, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		SendMessage();
	}
}

void UAesirCompanionChatWidget::RetryMessage()
{
	if (UCompanionChatSubsystem* Chat = BoundChat.Get())
	{
		if (!Chat->RetryLastMessage())
		{
			HandleFailure(TEXT("没有可重试的请求，或原请求预算已过期；可取消请求后重新发送。"));
		}
	}
	FocusChatInput();
}

void UAesirCompanionChatWidget::CancelMessage()
{
	if (UCompanionChatSubsystem* Chat = BoundChat.Get())
	{
		Chat->ResetConversation();
		AppendLine(TEXT("提示"), TEXT("已清除本地等待；旧回包将被忽略，可发送新消息。"));
	}
	FocusChatInput();
}

void UAesirCompanionChatWidget::HandleReply(const FCompanionChatReply& Reply)
{
	const UCompanionChatSubsystem* Chat = BoundChat.Get();
	if (Chat && IsVisible() && Reply.CompanionId == Chat->GetSelectedCompanionId())
	{
		AppendLine(Reply.CompanionId, Reply.ReplyText);
	}
}

void UAesirCompanionChatWidget::HandleFailure(const FString& Error)
{
	if (BoundChat.IsValid() && IsVisible())
	{
		AppendLine(TEXT("提示"), Error);
	}
}

void UAesirCompanionChatWidget::RequestClose()
{
	if (AAesirCombatPrototypePlayerController* Controller = Cast<AAesirCombatPrototypePlayerController>(GetOwningPlayer()))
	{
		Controller->CloseCompanionChat();
	}
}

FReply UAesirCompanionChatWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Escape)
	{
		RequestClose();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

void UAesirCompanionChatWidget::NativeDestruct()
{
	DeactivateChat();
	Super::NativeDestruct();
}
