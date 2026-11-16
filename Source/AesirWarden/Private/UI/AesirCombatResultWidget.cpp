// Copyright (c) 2026 YYchainsAw. All rights reserved.

#include "UI/AesirCombatResultWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"

TSharedRef<SWidget> UAesirCombatResultWidget::RebuildWidget()
{
	// A missing optional Blueprint still leaves a usable result page.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ResultRoot"));
		WidgetTree->RootWidget = Canvas;
		UVerticalBox* Actions = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VerticalBox_Result"));
		UCanvasPanelSlot* Panel = Canvas->AddChildToCanvas(Actions);
		Panel->SetAnchors(FAnchors(0.5f));
		Panel->SetAlignment(FVector2D(0.5f));
		Panel->SetAutoSize(true);
		TXT_Result = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TXT_Result"));
		TXT_Result->SetJustification(ETextJustify::Center);
		Actions->AddChildToVerticalBox(TXT_Result)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
		auto AddAction = [this, Actions](FName Name, FText Text)
		{
			UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
			Label->SetText(Text);
			Label->SetJustification(ETextJustify::Center);
			Button->SetContent(Label);
			Actions->AddChildToVerticalBox(Button)->SetPadding(FMargin(12.0f, 8.0f));
			return Button;
		};
		BTN_Restart = AddAction(TEXT("BTN_Restart"), NSLOCTEXT("AesirResult", "Retry", "重试"));
		BTN_ReturnToMenu = AddAction(TEXT("BTN_ReturnToMenu"), NSLOCTEXT("AesirResult", "ReturnToMenu", "返回菜单"));
		BTN_Quit = AddAction(TEXT("BTN_Quit"), NSLOCTEXT("AesirResult", "Quit", "退出游戏"));
	}
	return Super::RebuildWidget();
}

void UAesirCombatResultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetColorAndOpacity(FLinearColor::White);
	SetRenderOpacity(1.0f);
	if (auto* Background = Cast<UBorder>(GetWidgetFromName(TEXT("Border_Background"))))
	{
		// The border colors the backdrop through its brush. Content must
		// retain its own colors so that the controls stay readable.
		Background->SetContentColorAndOpacity(FLinearColor::White);
	}
	if (BTN_Restart)
	{
		// The old WBP click opened L_AesirCombatTest. Native match routing owns
		// this action now; keeping that binding would trigger two level loads.
		BTN_Restart->OnClicked.Clear();
		BTN_Restart->OnClicked.AddUniqueDynamic(this, &UAesirCombatResultWidget::HandleRetry);
	}
	if (BTN_ReturnToMenu)
	{
		BTN_ReturnToMenu->OnClicked.AddUniqueDynamic(this, &UAesirCombatResultWidget::HandleReturnToMenu);
	}
	if (BTN_Quit)
	{
		BTN_Quit->OnClicked.AddUniqueDynamic(this, &UAesirCombatResultWidget::HandleQuit);
	}
	RefreshResult();
}

void UAesirCombatResultWidget::NativeDestruct()
{
	if (BTN_Restart) { BTN_Restart->OnClicked.RemoveDynamic(this, &UAesirCombatResultWidget::HandleRetry); }
	if (BTN_ReturnToMenu) { BTN_ReturnToMenu->OnClicked.RemoveDynamic(this, &UAesirCombatResultWidget::HandleReturnToMenu); }
	if (BTN_Quit) { BTN_Quit->OnClicked.RemoveDynamic(this, &UAesirCombatResultWidget::HandleQuit); }
	Super::NativeDestruct();
}

void UAesirCombatResultWidget::SetMatchResult(EAesirMatchResult Result)
{
	if (DisplayedResult != EAesirMatchResult::None && DisplayedResult != Result) { return; }
	DisplayedResult = Result;
	RefreshResult();
}

void UAesirCombatResultWidget::RefreshResult()
{
	if (TXT_Result)
	{
		TXT_Result->SetText(DisplayedResult == EAesirMatchResult::Victory
			? NSLOCTEXT("AesirResult", "Victory", "VICTORY")
			: DisplayedResult == EAesirMatchResult::Defeat
				? NSLOCTEXT("AesirResult", "Defeat", "DEFEAT") : FText::GetEmpty());
		TXT_Result->SetColorAndOpacity(FSlateColor(DisplayedResult == EAesirMatchResult::Victory
			? FLinearColor(0.61f, 0.9f, 0.66f) : FLinearColor(0.96f, 0.55f, 0.49f)));
	}
	const bool bTerminal = DisplayedResult != EAesirMatchResult::None;
	if (BTN_Restart) { BTN_Restart->SetIsEnabled(bTerminal); }
	if (BTN_ReturnToMenu) { BTN_ReturnToMenu->SetIsEnabled(bTerminal); }
	if (BTN_Quit) { BTN_Quit->SetIsEnabled(bTerminal); }
}

void UAesirCombatResultWidget::FocusDefaultAction()
{
	if (BTN_Restart && BTN_Restart->GetIsEnabled()) { BTN_Restart->SetUserFocus(GetOwningPlayer()); }
	else { SetUserFocus(GetOwningPlayer()); }
}

void UAesirCombatResultWidget::HandleRetry()
{
	if (DisplayedResult != EAesirMatchResult::None) { OnRetryRequested.Broadcast(); }
}

void UAesirCombatResultWidget::HandleReturnToMenu()
{
	if (DisplayedResult != EAesirMatchResult::None) { OnReturnToMenuRequested.Broadcast(); }
}

void UAesirCombatResultWidget::HandleQuit()
{
	if (DisplayedResult != EAesirMatchResult::None) { OnQuitRequested.Broadcast(); }
}
