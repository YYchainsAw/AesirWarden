// Copyright (c) 2026 YYchainsAw. All rights reserved. Third-party assets belong to their respective owners.

#include "UI/AesirMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "AesirMainMenu"

void UAesirMainMenuWidget::BuildDefaultMenuTree(UWidgetTree* InWidgetTree)
{
	if (!IsValid(InWidgetTree) || InWidgetTree->RootWidget)
	{
		return;
	}

	// Keep the left side open for the in-world campfire and the player character.
	const FLinearColor Ash(0.91f, 0.88f, 0.81f, 1.0f);
	const FLinearColor EmberGold(0.76f, 0.57f, 0.31f, 1.0f);
	const FLinearColor Charcoal(0.025f, 0.03f, 0.035f, 0.88f);
	const FLinearColor QuietText(0.60f, 0.60f, 0.57f, 1.0f);

	UCanvasPanel* Root = InWidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MenuRoot"));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	InWidgetTree->RootWidget = Root;

	USizeBox* PanelSize = InWidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MenuPanelSize"));
	PanelSize->SetWidthOverride(420.0f);
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PanelSize);
	PanelSlot->SetAnchors(FAnchors(1.0f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(1.0f, 0.5f));
	PanelSlot->SetPosition(FVector2D(-72.0f, 0.0f));
	PanelSlot->SetAutoSize(true);

	UBorder* Panel = InWidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuPanel"));
	Panel->SetBrushColor(Charcoal);
	Panel->SetPadding(FMargin(40.0f, 42.0f));
	PanelSize->SetContent(Panel);

	UVerticalBox* Content = InWidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuContent"));
	Panel->SetContent(Content);

	auto AddText = [InWidgetTree, Content](const FName Name, const FText& Text,
		const int32 Size, const FName Typeface, const FLinearColor& Color)
	{
		UTextBlock* Label = InWidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Label->SetText(Text);
		Label->SetFont(FCoreStyle::GetDefaultFontStyle(Typeface, Size));
		Label->SetColorAndOpacity(FSlateColor(Color));
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		Content->AddChildToVerticalBox(Label);
		return Label;
	};

	AddText(TEXT("Txt_Title"), LOCTEXT("GameTitle", "AESIR\nWARDEN"), 44, TEXT("Bold"), Ash);

	USpacer* TitleSpace = InWidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("TitleSpace"));
	TitleSpace->SetSize(FVector2D(1.0f, 28.0f));
	Content->AddChildToVerticalBox(TitleSpace);

	UBorder* EmberLine = InWidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EmberLine"));
	EmberLine->SetPadding(FMargin(0.0f));
	EmberLine->SetBrushColor(EmberGold);
	USpacer* LineHeight = InWidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("EmberLineHeight"));
	LineHeight->SetSize(FVector2D(1.0f, 2.0f));
	EmberLine->SetContent(LineHeight);
	Content->AddChildToVerticalBox(EmberLine)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 34.0f));

	auto AddAction = [InWidgetTree, Content, Ash, EmberGold](const FName ButtonName,
		const FName LabelName, const FText& Text)
	{
		UButton* Button = InWidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		Style.Normal.TintColor = FSlateColor(FLinearColor(0.09f, 0.10f, 0.11f, 0.90f));
		Style.Hovered.TintColor = FSlateColor(FLinearColor(0.24f, 0.19f, 0.13f, 1.0f));
		Style.Pressed.TintColor = FSlateColor(FLinearColor(0.36f, 0.25f, 0.13f, 1.0f));
		Style.NormalPadding = FMargin(18.0f, 14.0f);
		Style.PressedPadding = FMargin(18.0f, 15.0f, 18.0f, 13.0f);
		Button->SetStyle(Style);
		Button->SetColorAndOpacity(Ash);
		Button->SetBackgroundColor(EmberGold);
		UTextBlock* Label = InWidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
		Label->SetText(Text);
		Label->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 20));
		Label->SetColorAndOpacity(FSlateColor(Ash));
		Button->SetContent(Label);
		Content->AddChildToVerticalBox(Button)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		return Button;
	};

	UButton* StartButton = AddAction(TEXT("Btn_Start"), TEXT("Txt_Start"), LOCTEXT("StartGame", "开始游戏"));
	StartButton->SetIsEnabled(false);
	AddAction(TEXT("Btn_Quit"), TEXT("Txt_Quit"), LOCTEXT("QuitGame", "退出游戏"));

	UTextBlock* Status = AddText(TEXT("Txt_Status"), LOCTEXT("PreparingCampfire", "正在准备营火……"),
		14, TEXT("Regular"), QuietText);
	Status->SetAutoWrapText(true);
	if (UVerticalBoxSlot* StatusSlot = Cast<UVerticalBoxSlot>(Status->Slot))
	{
		StatusSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
	}
}

TSharedRef<SWidget> UAesirMainMenuWidget::RebuildWidget()
{
	BuildDefaultMenuTree(WidgetTree);
	return Super::RebuildWidget();
}

void UAesirMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Native fallback trees are created after Initialize, when BindWidget normally runs.
	if (WidgetTree)
	{
		if (!Btn_Start)
		{
			Btn_Start = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Start")));
		}
		if (!Btn_Quit)
		{
			Btn_Quit = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Quit")));
		}
		if (!Txt_Status)
		{
			Txt_Status = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Txt_Status")));
		}
	}

	if (Btn_Start)
	{
		Btn_Start->OnClicked.AddUniqueDynamic(this, &UAesirMainMenuWidget::HandleStartClicked);
	}
	if (Btn_Quit)
	{
		Btn_Quit->OnClicked.AddUniqueDynamic(this, &UAesirMainMenuWidget::HandleQuitClicked);
	}
	RefreshAvailability();
}

void UAesirMainMenuWidget::NativeDestruct()
{
	if (Btn_Start)
	{
		Btn_Start->OnClicked.RemoveDynamic(this, &UAesirMainMenuWidget::HandleStartClicked);
	}
	if (Btn_Quit)
	{
		Btn_Quit->OnClicked.RemoveDynamic(this, &UAesirMainMenuWidget::HandleQuitClicked);
	}
	Super::NativeDestruct();
}

void UAesirMainMenuWidget::SetStartAvailability(const bool bEnabled, FText InStatusText)
{
	bStartEnabled = bEnabled;
	StatusText = MoveTemp(InStatusText);
	RefreshAvailability();
}

void UAesirMainMenuWidget::RefreshAvailability()
{
	if (Btn_Start)
	{
		Btn_Start->SetIsEnabled(bStartEnabled);
	}
	if (Txt_Status)
	{
		Txt_Status->SetText(StatusText);
		Txt_Status->SetVisibility(StatusText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UAesirMainMenuWidget::FocusDefaultAction()
{
	UButton* Target = Btn_Start && Btn_Start->GetIsEnabled() && Btn_Start->IsVisible() ? Btn_Start.Get() : Btn_Quit.Get();
	if (!Target || !Target->GetIsEnabled() || !Target->IsVisible())
	{
		return;
	}
	if (APlayerController* Controller = GetOwningPlayer())
	{
		Target->SetUserFocus(Controller);
	}
	else
	{
		Target->SetKeyboardFocus();
	}
}

void UAesirMainMenuWidget::HandleStartClicked()
{
	if (bStartEnabled)
	{
		OnStartRequested.Broadcast();
	}
}

void UAesirMainMenuWidget::HandleQuitClicked()
{
	OnQuitRequested.Broadcast();
}

#undef LOCTEXT_NAMESPACE
