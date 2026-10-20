// Copyright (c) 2026 YYchainsAw. All rights reserved. Third-party assets belong to their respective owners.

#include "UI/AesirMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "AesirMainMenu"

void UAesirMainMenuWidget::BuildDefaultMenuTree(UWidgetTree* InWidgetTree)
{
	if (!IsValid(InWidgetTree) || InWidgetTree->RootWidget)
	{
		return;
	}

	constexpr float GoldenX = 0.618034f;
	constexpr float GoldenY = 0.381966f;
	const FLinearColor Ivory(0.96f, 0.91f, 0.79f, 1.0f);
	const FLinearColor Gold(0.73f, 0.55f, 0.28f, 0.85f);
	UFont* MenuFont = LoadObject<UFont>(nullptr, TEXT("/Game/Aesir/UI/Fonts/F_MenuSerif.F_MenuSerif"), nullptr, LOAD_NoWarn);
	if (!MenuFont)
	{
		MenuFont = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
	}
	UTexture2D* Logo = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Aesir/UI/Art/Logo/logo.logo"), nullptr, LOAD_NoWarn);
	UTexture2D* Frame = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/Aesir/UI/Art/Kenney/FantasyUIBorders/Double/Border/T_KUI_panel_border_002.T_KUI_panel_border_002"), nullptr, LOAD_NoWarn);
	UTexture2D* Divider = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/Aesir/UI/Art/Kenney/FantasyUIBorders/Double/DividerFade/T_KUI_divider_fade_003.T_KUI_divider_fade_003"), nullptr, LOAD_NoWarn);
	const FVector2D LogoSize(640.0f, Logo && Logo->GetSizeX() > 0
		? 640.0f * Logo->GetSizeY() / Logo->GetSizeX() : 360.0f);

	UCanvasPanel* Root = InWidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MenuRoot"));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	InWidgetTree->RootWidget = Root;
	auto Place = [Root](UWidget* Widget, FVector2D Size, FVector2D Offset, FVector2D Alignment)
	{
		UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Widget);
		Slot->SetAnchors(FAnchors(GoldenX, GoldenY));
		Slot->SetAlignment(Alignment);
		Slot->SetPosition(Offset);
		Slot->SetSize(Size);
		return Slot;
	};
	auto MakeText = [InWidgetTree, MenuFont, Ivory](FName Name, FText Text, int32 Size)
	{
		UTextBlock* Label = InWidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Label->SetText(Text);
		FSlateFontInfo Font(MenuFont, Size, TEXT("Regular"));
		Font.LetterSpacing = 25;
		Font.OutlineSettings.OutlineSize = 1;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.015f, 0.018f, 0.022f, 0.9f);
		Label->SetFont(Font);
		Label->SetColorAndOpacity(FSlateColor(Ivory));
		Label->SetJustification(ETextJustify::Center);
		Label->SetShadowOffset(FVector2D(0.0f, 2.0f));
		Label->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Label;
	};
	if (Logo)
	{
		UImage* Image = InWidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_Logo"));
		Image->SetBrushFromTexture(Logo);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Image, LogoSize, FVector2D::ZeroVector, FVector2D(0.5f, 0.5f));
	}
	else
	{
		Place(MakeText(TEXT("Txt_Title"), LOCTEXT("GameTitle", "AESIR\nWARDEN"), 80),
			LogoSize, FVector2D::ZeroVector, FVector2D(0.5f, 0.5f));
	}
	UHorizontalBox* Dividers = InWidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MenuDivider"));
	Dividers->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Dividers, FVector2D(400.0f, 30.0f), FVector2D(0.0f, LogoSize.Y * 0.5f + 8.0f), FVector2D(0.5f, 0.0f));
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UImage* Line = InWidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Index == 0 ? TEXT("Img_DividerLeft") : TEXT("Img_DividerRight"));
		FSlateBrush Brush;
		Brush.SetResourceObject(Divider);
		Brush.ImageSize = FVector2D(192.0f, 36.0f);
		Brush.DrawAs = Divider ? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
		Brush.Mirroring = Index == 0 ? ESlateBrushMirrorType::NoMirror : ESlateBrushMirrorType::Horizontal;
		Line->SetBrush(Brush);
		Line->SetColorAndOpacity(Gold);
		UHorizontalBoxSlot* Slot = Dividers->AddChildToHorizontalBox(Line);
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Slot->SetPadding(FMargin(Index == 0 ? 0.0f : 8.0f, 0.0f, Index == 0 ? 8.0f : 0.0f, 0.0f));
	}

	// These containers never paint a background. Only transparent-center texture borders are drawn.
	USizeBox* PanelSize = InWidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MenuPanelSize"));
	PanelSize->SetWidthOverride(500.0f);
	Place(PanelSize, FVector2D(500.0f, 0.0f), FVector2D(0.0f, LogoSize.Y * 0.5f + 56.0f), FVector2D(0.5f, 0.0f))->SetAutoSize(true);
	UOverlay* Panel = InWidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MenuPanel"));
	PanelSize->SetContent(Panel);
	UVerticalBox* Actions = InWidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuActions"));
	Panel->AddChildToOverlay(Actions)->SetHorizontalAlignment(HAlign_Fill);

	auto AddAction = [InWidgetTree, Frame, Gold, MakeText](UVerticalBox* Parent, FName ButtonName, FName LabelName, FText Text)
	{
		USizeBox* Height = InWidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(*(ButtonName.ToString() + TEXT("_Size"))));
		Height->SetHeightOverride(88.0f);
		Parent->AddChildToVerticalBox(Height)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		UButton* Button = InWidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		FSlateBrush Outline;
		Outline.SetResourceObject(Frame);
		Outline.DrawAs = Frame ? ESlateBrushDrawType::Box : ESlateBrushDrawType::NoDrawType;
		Outline.ImageSize = FVector2D(48.0f, 48.0f);
		Outline.Margin = FMargin(0.25f);
		Outline.TintColor = FSlateColor(Gold);
		FButtonStyle Style;
		Style.Normal = Style.Hovered = Style.Pressed = Style.Disabled = Outline;
		Style.Hovered.TintColor = FSlateColor(FLinearColor(1.0f, 0.88f, 0.59f, 1.0f));
		Style.Pressed.TintColor = FSlateColor(FLinearColor(0.90f, 0.66f, 0.29f, 1.0f));
		Style.Disabled.TintColor = FSlateColor(FLinearColor(0.36f, 0.32f, 0.24f, 0.5f));
		Style.NormalPadding = FMargin(24.0f, 2.0f);
		Style.PressedPadding = FMargin(24.0f, 3.0f, 24.0f, 1.0f);
		Button->SetStyle(Style);
		Button->SetBackgroundColor(FLinearColor::White);
		Button->SetColorAndOpacity(FLinearColor::White);
		UTextBlock* Label = MakeText(LabelName, Text, 55);
		// Source Han's generous line metrics extend beyond its visible Chinese
		// glyphs. Keep the requested size and optically center the ink in the frame.
		Label->SetRenderTranslation(FVector2D(0.0f, -18.0f));
		Button->SetContent(Label);
		Height->SetContent(Button);
		return Button;
	};
	AddAction(Actions, TEXT("Btn_Start"), TEXT("Txt_Start"), LOCTEXT("StartGame", "开始游戏"))->SetIsEnabled(false);
	AddAction(Actions, TEXT("Btn_Settings"), TEXT("Txt_Settings"), LOCTEXT("Settings", "设置"));
	AddAction(Actions, TEXT("Btn_Credits"), TEXT("Txt_Credits"), LOCTEXT("Credits", "制作人员"));
	AddAction(Actions, TEXT("Btn_Quit"), TEXT("Txt_Quit"), LOCTEXT("QuitGame", "退出游戏"));
	UTextBlock* Status = MakeText(TEXT("Txt_Status"), LOCTEXT("PreparingCampfire", "正在准备营火……"), 20);
	Status->SetAutoWrapText(true);
	Actions->AddChildToVerticalBox(Status)->SetPadding(FMargin(0.0f, 4.0f));

	UVerticalBox* Details = InWidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuDetails"));
	Details->SetVisibility(ESlateVisibility::Collapsed);
	Panel->AddChildToOverlay(Details)->SetHorizontalAlignment(HAlign_Fill);
	Details->AddChildToVerticalBox(MakeText(TEXT("Txt_DetailsTitle"), FText::GetEmpty(), 55))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	UTextBlock* Body = MakeText(TEXT("Txt_DetailsBody"), FText::GetEmpty(), 28);
	Body->SetAutoWrapText(true);
	Details->AddChildToVerticalBox(Body)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	AddAction(Details, TEXT("Btn_Back"), TEXT("Txt_Back"), LOCTEXT("BackToMainMenu", "返回"));
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
		if (!Btn_Settings)
		{
			Btn_Settings = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Settings")));
		}
		if (!Btn_Credits)
		{
			Btn_Credits = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Credits")));
		}
		if (!Btn_Back)
		{
			Btn_Back = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Back")));
		}
		if (!MenuActions)
		{
			MenuActions = WidgetTree->FindWidget(TEXT("MenuActions"));
		}
		if (!MenuDetails)
		{
			MenuDetails = WidgetTree->FindWidget(TEXT("MenuDetails"));
		}
		if (!Txt_DetailsTitle)
		{
			Txt_DetailsTitle = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Txt_DetailsTitle")));
		}
		if (!Txt_DetailsBody)
		{
			Txt_DetailsBody = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Txt_DetailsBody")));
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
	if (Btn_Settings)
	{
		Btn_Settings->OnClicked.AddUniqueDynamic(this, &UAesirMainMenuWidget::HandleSettingsClicked);
	}
	if (Btn_Credits)
	{
		Btn_Credits->OnClicked.AddUniqueDynamic(this, &UAesirMainMenuWidget::HandleCreditsClicked);
	}
	if (Btn_Back)
	{
		Btn_Back->OnClicked.AddUniqueDynamic(this, &UAesirMainMenuWidget::HandleBackClicked);
	}
	RefreshAvailability();
	ReturnToMainMenu();
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
	if (Btn_Settings)
	{
		Btn_Settings->OnClicked.RemoveDynamic(this, &UAesirMainMenuWidget::HandleSettingsClicked);
	}
	if (Btn_Credits)
	{
		Btn_Credits->OnClicked.RemoveDynamic(this, &UAesirMainMenuWidget::HandleCreditsClicked);
	}
	if (Btn_Back)
	{
		Btn_Back->OnClicked.RemoveDynamic(this, &UAesirMainMenuWidget::HandleBackClicked);
	}
	Super::NativeDestruct();
}

FReply UAesirMainMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bShowingDetails && (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right))
	{
		ReturnToMainMenu();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
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
	UButton* Target = bShowingDetails ? Btn_Back.Get()
		: (Btn_Start && Btn_Start->GetIsEnabled() && Btn_Start->IsVisible() ? Btn_Start.Get() : Btn_Quit.Get());
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

void UAesirMainMenuWidget::ReturnToMainMenu()
{
	bShowingDetails = false;
	if (MenuDetails)
	{
		MenuDetails->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MenuActions)
	{
		MenuActions->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	FocusDefaultAction();
}

void UAesirMainMenuWidget::ShowDetails(FText Title, FText Body)
{
	if (!MenuDetails)
	{
		return;
	}
	if (Txt_DetailsTitle)
	{
		Txt_DetailsTitle->SetText(MoveTemp(Title));
	}
	if (Txt_DetailsBody)
	{
		Txt_DetailsBody->SetText(MoveTemp(Body));
	}
	if (MenuActions)
	{
		MenuActions->SetVisibility(ESlateVisibility::Collapsed);
	}
	MenuDetails->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	bShowingDetails = true;
	FocusDefaultAction();
}

void UAesirMainMenuWidget::HandleStartClicked()
{
	if (bStartEnabled && !bShowingDetails)
	{
		OnStartRequested.Broadcast();
	}
}

void UAesirMainMenuWidget::HandleQuitClicked()
{
	OnQuitRequested.Broadcast();
}

void UAesirMainMenuWidget::HandleSettingsClicked()
{
	ShowDetails(LOCTEXT("SettingsTitle", "设置"),
		LOCTEXT("SettingsComingSoon", "画面 / 声音 / 操作\n更多设置将在后续开放"));
	OnSettingsRequested.Broadcast();
	ReceiveSettingsOpened();
}

void UAesirMainMenuWidget::HandleCreditsClicked()
{
	ShowDetails(LOCTEXT("CreditsTitle", "制作人员"),
		LOCTEXT("CreditsComingSoon", "AESIR WARDEN\n制作名单待补充"));
	OnCreditsRequested.Broadcast();
	ReceiveCreditsOpened();
}

void UAesirMainMenuWidget::HandleBackClicked()
{
	ReturnToMainMenu();
}

#undef LOCTEXT_NAMESPACE
