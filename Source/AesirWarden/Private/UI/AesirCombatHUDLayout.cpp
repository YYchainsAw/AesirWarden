// Copyright (c) 2026 YYchainsAw. All rights reserved. Third-party assets belong to their respective owners.

#include "UI/AesirCombatHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SafeZone.h"
#include "Components/SafeZoneSlot.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "AesirCombatHUDLayout"

void UAesirCombatHUDWidget::BuildDefaultHUDTree(UWidgetTree* Tree)
{
	if (!IsValid(Tree) || Tree->RootWidget)
	{
		return;
	}

	const FLinearColor Ivory(0.96f, 0.91f, 0.79f, 1.0f);
	const FLinearColor Gold(0.73f, 0.55f, 0.28f, 0.85f);
	const FLinearColor DarkRed(0.53f, 0.055f, 0.065f, 1.0f);
	const FLinearColor Amber(0.83f, 0.52f, 0.16f, 1.0f);
	UFont* Font = LoadObject<UFont>(nullptr, TEXT("/Game/Aesir/UI/Fonts/F_MenuSerif.F_MenuSerif"), nullptr, LOAD_NoWarn);
	if (!Font)
	{
		Font = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
	}
	UTexture2D* Frame = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/Aesir/UI/Art/Kenney/FantasyUIBorders/Double/Border/T_KUI_panel_border_002.T_KUI_panel_border_002"), nullptr, LOAD_NoWarn);
	UTexture2D* Divider = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/Aesir/UI/Art/Kenney/FantasyUIBorders/Double/DividerFade/T_KUI_divider_fade_003.T_KUI_divider_fade_003"), nullptr, LOAD_NoWarn);

	USafeZone* SafeArea = Tree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), TEXT("HUDSafeArea"));
	SafeArea->bIsVariable = false;
	SafeArea->SetSidesToPad(true, true, true, true);
	SafeArea->SetVisibility(ESlateVisibility::HitTestInvisible);
	Tree->RootWidget = SafeArea;
	UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HUDCanvas"));
	Canvas->bIsVariable = false;
	Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	SafeArea->SetContent(Canvas);
	if (USafeZoneSlot* SafeSlot = Cast<USafeZoneSlot>(Canvas->Slot))
	{
		SafeSlot->SetHorizontalAlignment(HAlign_Fill);
		SafeSlot->SetVerticalAlignment(VAlign_Fill);
	}

	auto Place = [](UCanvasPanel* Parent, UWidget* Widget, FVector2D Position, FVector2D Size,
		FVector2D Anchor = FVector2D::ZeroVector, FVector2D Alignment = FVector2D::ZeroVector)
	{
		UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Widget);
		Slot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
		Slot->SetAlignment(Alignment);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		return Slot;
	};
	auto MakeText = [Tree, Font, Ivory](FName Name, const FText& Text, int32 Size, ETextJustify::Type Justify)
	{
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Label->bIsVariable = false;
		Label->SetText(Text);
		FSlateFontInfo FontInfo(Font, Size, TEXT("Regular"));
		FontInfo.OutlineSettings.OutlineSize = 1;
		FontInfo.OutlineSettings.OutlineColor = FLinearColor(0.015f, 0.018f, 0.022f, 0.9f);
		Label->SetFont(FontInfo);
		Label->SetColorAndOpacity(FSlateColor(Ivory));
		Label->SetShadowOffset(FVector2D(0.0f, 1.0f));
		Label->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
		Label->SetJustification(Justify);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Label;
	};
	auto MakeIcon = [Tree, Gold](FName Name, const TCHAR* Path)
	{
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		Image->bIsVariable = false;
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, Path, nullptr, LOAD_NoWarn);
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = FVector2D(128.0f, 128.0f);
		Brush.DrawAs = Texture ? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
		Image->SetBrush(Brush);
		Image->SetColorAndOpacity(Gold);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Image;
	};
	auto MakeBar = [Tree](FName Name, FLinearColor Color, float Percent, bool bPaintTrack)
	{
		UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), Name);
		// The old SetPlayerHealth graph gets this generated variable by its original name/GUID.
		Bar->bIsVariable = Name == TEXT("PB_PlayerHealth");
		FProgressBarStyle Style;
		Style.BackgroundImage.DrawAs = bPaintTrack ? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
		Style.BackgroundImage.TintColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.35f));
		Style.FillImage.DrawAs = ESlateBrushDrawType::Image;
		Style.FillImage.TintColor = FSlateColor(FLinearColor::White);
		Style.MarqueeImage.DrawAs = ESlateBrushDrawType::NoDrawType;
		Bar->SetWidgetStyle(Style);
		Bar->SetFillColorAndOpacity(Color);
		Bar->SetBarFillType(EProgressBarFillType::LeftToRight);
		Bar->SetBarFillStyle(EProgressBarFillStyle::Scale);
		Bar->SetBorderPadding(FVector2D::ZeroVector);
		Bar->SetPercent(Percent);
		Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Bar;
	};
	auto AddFrame = [Tree, Frame, Gold](UOverlay* Parent, FName Name)
	{
		UImage* Border = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		Border->bIsVariable = false;
		FSlateBrush Brush;
		Brush.SetResourceObject(Frame);
		Brush.DrawAs = Frame ? ESlateBrushDrawType::Box : ESlateBrushDrawType::NoDrawType;
		Brush.Margin = FMargin(0.25f);
		Brush.ImageSize = FVector2D(48.0f, 48.0f);
		Border->SetBrush(Brush);
		Border->SetColorAndOpacity(Gold);
		Border->SetVisibility(ESlateVisibility::HitTestInvisible);
		UOverlaySlot* Slot = Parent->AddChildToOverlay(Border);
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	};
	auto AddBarLayer = [](UOverlay* Parent, UProgressBar* Bar)
	{
		UOverlaySlot* Slot = Parent->AddChildToOverlay(Bar);
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
		Slot->SetPadding(FMargin(3.0f, 3.0f));
	};

	// No full panel paints a background. Only the local bar tracks and transparent borders draw.
	UCanvasPanel* PlayerPanel = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PlayerVitals"));
	PlayerPanel->bIsVariable = false;
	Place(Canvas, PlayerPanel, FVector2D(72.0f, 64.0f), FVector2D(436.0f, 116.0f));
	Place(PlayerPanel, MakeIcon(TEXT("Img_Health"), TEXT("/Game/Aesir/UI/Art/Emberhold/T_EH_heart.T_EH_heart")),
		FVector2D(0.0f, 18.0f), FVector2D(30.0f, 30.0f));
	Place(PlayerPanel, MakeText(TEXT("Txt_PlayerHealth"), LOCTEXT("HealthPreview", "720 / 1000"), 20, ETextJustify::Right),
		FVector2D(44.0f, -8.0f), FVector2D(380.0f, 32.0f));
	UOverlay* Health = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PlayerHealthFrame"));
	Health->bIsVariable = false;
	Place(PlayerPanel, Health, FVector2D(44.0f, 24.0f), FVector2D(380.0f, 22.0f));
	AddBarLayer(Health, MakeBar(TEXT("PB_PlayerHealthTrail"), FLinearColor(0.81f, 0.66f, 0.39f, 0.9f), 0.72f, true));
	AddBarLayer(Health, MakeBar(TEXT("PB_PlayerHealth"), DarkRed, 0.72f, false));
	AddFrame(Health, TEXT("Img_HealthFrame"));

	Place(PlayerPanel, MakeIcon(TEXT("Img_Guard"), TEXT("/Game/Aesir/UI/Art/Emberhold/T_EH_shield.T_EH_shield")),
		FVector2D(4.0f, 59.0f), FVector2D(22.0f, 22.0f));
	UOverlay* Guard = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("GuardPressureFrame"));
	Guard->bIsVariable = false;
	Place(PlayerPanel, Guard, FVector2D(44.0f, 65.0f), FVector2D(300.0f, 10.0f));
	// The pressure track is intentionally thinner; the inset leaves a four-pixel fill.
	AddBarLayer(Guard, MakeBar(TEXT("PB_GuardPressure"), Amber, 0.28f, true));
	AddFrame(Guard, TEXT("Img_GuardFrame"));
	Place(PlayerPanel, MakeText(TEXT("Txt_GuardState"), FText::GetEmpty(), 18, ETextJustify::Left),
		FVector2D(44.0f, 80.0f), FVector2D(380.0f, 32.0f));

	UCanvasPanel* Boss = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BossPanel"));
	Boss->bIsVariable = false;
	Boss->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Canvas, Boss, FVector2D(0.0f, -64.0f), FVector2D(720.0f, 78.0f), FVector2D(0.5f, 1.0f), FVector2D(0.5f, 1.0f));
	Place(Boss, MakeText(TEXT("Txt_BossName"), LOCTEXT("BossPreview", "守望者"), 26, ETextJustify::Center),
		FVector2D(0.0f, -4.0f), FVector2D(720.0f, 42.0f));
	UOverlay* BossHealth = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("BossHealthFrame"));
	BossHealth->bIsVariable = false;
	Place(Boss, BossHealth, FVector2D(0.0f, 44.0f), FVector2D(720.0f, 18.0f));
	AddBarLayer(BossHealth, MakeBar(TEXT("PB_BossHealth"), DarkRed, 0.85f, true));
	AddFrame(BossHealth, TEXT("Img_BossHealthFrame"));
	UImage* BossDivider = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_BossDivider"));
	BossDivider->bIsVariable = false;
	FSlateBrush DividerBrush;
	DividerBrush.SetResourceObject(Divider);
	DividerBrush.DrawAs = Divider ? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
	DividerBrush.ImageSize = FVector2D(192.0f, 36.0f);
	BossDivider->SetBrush(DividerBrush);
	BossDivider->SetColorAndOpacity(Gold);
	BossDivider->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Boss, BossDivider, FVector2D(200.0f, 63.0f), FVector2D(320.0f, 12.0f));

	UHorizontalBox* Abilities = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HB_Abilities"));
	Abilities->bIsVariable = false;
	Abilities->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* AbilitySlot = Place(Canvas, Abilities, FVector2D(-72.0f, -64.0f),
		FVector2D::ZeroVector, FVector2D(1.0f, 1.0f), FVector2D(1.0f, 1.0f));
	AbilitySlot->SetAutoSize(true);
}

#undef LOCTEXT_NAMESPACE
