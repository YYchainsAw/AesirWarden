// Copyright (c) 2026 YYchainsAw. All rights reserved. Third-party assets belong to their owners.

#include "UI/AesirSettingsMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "AesirSettingsLayout"

void UAesirSettingsMenuWidget::BuildDefaultSettingsTree(UWidgetTree* Tree)
{
	if (!IsValid(Tree) || Tree->RootWidget) return;

	const FLinearColor Ivory(0.96f, 0.91f, 0.79f);
	const FLinearColor Gold(0.73f, 0.55f, 0.28f);
	const FLinearColor BrightGold(0.95f, 0.79f, 0.45f);
	const FLinearColor Muted(0.61f, 0.63f, 0.60f);
	const FLinearColor Popup(0.045f, 0.052f, 0.060f, 0.98f);
	UFont* Font = LoadObject<UFont>(nullptr, TEXT("/Game/Aesir/UI/Fonts/F_MenuSerif.F_MenuSerif"), nullptr, LOAD_NoWarn);
	if (!Font) Font = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
	UTexture2D* Frame = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/Aesir/UI/Art/Kenney/FantasyUIBorders/Double/Border/T_KUI_panel_border_002.T_KUI_panel_border_002"), nullptr, LOAD_NoWarn);
	UTexture2D* Divider = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/Aesir/UI/Art/Kenney/FantasyUIBorders/Double/DividerFade/T_KUI_divider_fade_003.T_KUI_divider_fade_003"), nullptr, LOAD_NoWarn);

	auto Place = [](UCanvasPanel* Parent, UWidget* Child, FVector2D Position, FVector2D Size,
		FVector2D Anchor = FVector2D::ZeroVector, FVector2D Alignment = FVector2D::ZeroVector)
	{
		UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Child);
		Slot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
		Slot->SetAlignment(Alignment);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		return Slot;
	};
	auto FillOverlay = [](UOverlay* Parent, UWidget* Child)
	{
		UOverlaySlot* Slot = Parent->AddChildToOverlay(Child);
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	};
	auto Text = [Tree, Font, Ivory](FName Name, FText Value, int32 Points, ETextJustify::Type Justify = ETextJustify::Left)
	{
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Label->bIsVariable = false;
		Label->SetText(Value);
		FSlateFontInfo Info(Font, Points, TEXT("Regular"));
		Info.OutlineSettings.OutlineSize = 1;
		Info.OutlineSettings.OutlineColor = FLinearColor(0.015f, 0.018f, 0.022f, 0.8f);
		Label->SetFont(Info);
		Label->SetColorAndOpacity(Ivory);
		Label->SetJustification(Justify);
		Label->SetShadowOffset(FVector2D(0.0f, 1.0f));
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Label;
	};
	auto Outline = [](FLinearColor Color, float Width = 1.0f)
	{
		return FSlateRoundedBoxBrush(FLinearColor::Transparent, 0.0f, Color, Width);
	};
	auto ButtonStyle = [Outline, Gold, BrightGold, Muted, Ivory](bool bBorder)
	{
		FButtonStyle Style;
		Style.Normal = Outline(bBorder ? Gold.CopyWithNewOpacity(0.65f) : FLinearColor::Transparent);
		Style.Hovered = Outline(BrightGold, 1.5f);
		Style.Pressed = Outline(Ivory, 1.5f);
		Style.Disabled = Outline(bBorder ? Muted.CopyWithNewOpacity(0.35f) : FLinearColor::Transparent);
		Style.NormalPadding = FMargin(12.0f, 4.0f);
		Style.PressedPadding = Style.NormalPadding;
		return Style;
	};
	auto Button = [Tree, Text, ButtonStyle](FName Name, FName LabelName, FText Value, int32 Points, bool bBorder)
	{
		UButton* Control = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Control->bIsVariable = false;
		Control->SetStyle(ButtonStyle(bBorder));
		Control->SetContent(Text(LabelName, Value, Points, ETextJustify::Center));
		return Control;
	};
	auto Decoration = [Tree](FName Name, UTexture2D* Texture, FLinearColor Color, bool bBox)
	{
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		Image->bIsVariable = false;
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = Texture ? (bBox ? ESlateBrushDrawType::Box : ESlateBrushDrawType::Image) : ESlateBrushDrawType::NoDrawType;
		Brush.Margin = FMargin(0.25f);
		Image->SetBrush(Brush);
		Image->SetColorAndOpacity(Color);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Image;
	};
	auto Rule = [Tree](FName Name, FLinearColor Color)
	{
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		Image->bIsVariable = false;
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.TintColor = Color;
		Image->SetBrush(Brush);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Image;
	};
	auto Combo = [Tree, Font, ButtonStyle, Outline, Popup, Ivory, Gold, BrightGold](FName Name)
	{
		UComboBoxString* Control = Tree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), Name);
		Control->bIsVariable = false;
		FComboBoxStyle Style = Control->GetWidgetStyle();
		Style.ComboButtonStyle.ButtonStyle = ButtonStyle(true);
		Style.ComboButtonStyle.MenuBorderBrush = FSlateRoundedBoxBrush(Popup, 0.0f, Gold, 1.0f);
		Style.ComboButtonStyle.DownArrowImage.TintColor = Ivory;
		Style.ContentPadding = FMargin(12.0f, 6.0f);
		Style.MenuRowPadding = FMargin(10.0f, 6.0f);
		Control->SetWidgetStyle(Style);
		FTableRowStyle Rows = Control->GetItemStyle();
		Rows.EvenRowBackgroundBrush = FSlateRoundedBoxBrush(Popup, 0.0f);
		Rows.OddRowBackgroundBrush = Rows.EvenRowBackgroundBrush;
		Rows.EvenRowBackgroundHoveredBrush = FSlateRoundedBoxBrush(FLinearColor(0.13f, 0.12f, 0.09f), 0.0f);
		Rows.OddRowBackgroundHoveredBrush = Rows.EvenRowBackgroundHoveredBrush;
		Rows.ActiveBrush = Rows.EvenRowBackgroundHoveredBrush;
		Rows.ActiveHoveredBrush = Rows.ActiveBrush;
		Rows.InactiveBrush = Rows.ActiveBrush;
		Rows.InactiveHoveredBrush = Rows.ActiveBrush;
		Rows.SelectorFocusedBrush = Outline(BrightGold);
		Rows.TextColor = Ivory;
		Rows.SelectedTextColor = BrightGold;
		Control->SetItemStyle(Rows);
		Control->SetContentPadding(FMargin(12.0f, 6.0f));
		Control->SetMaxListHeight(360.0f);
		Control->SetEnableGamepadNavigationMode(true);
		// UE 5.8 exposes these construction-only properties without public setters.
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		Control->Font = FSlateFontInfo(Font, 30, TEXT("Regular"));
		Control->ForegroundColor = Ivory;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		return Control;
	};
	auto Check = [Tree, Outline, Gold, BrightGold, Ivory](FName Name)
	{
		UCheckBox* Control = Tree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), Name);
		Control->bIsVariable = false;
		FCheckBoxStyle Style;
		Style.CheckBoxType = ESlateCheckBoxType::CheckBox;
		Style.UncheckedImage = Outline(Gold.CopyWithNewOpacity(0.55f));
		Style.UncheckedHoveredImage = Outline(BrightGold, 2.0f);
		Style.UncheckedPressedImage = Outline(Ivory, 2.0f);
		Style.CheckedImage = Outline(BrightGold, 4.0f);
		Style.CheckedHoveredImage = Outline(Ivory, 4.0f);
		Style.CheckedPressedImage = Outline(Gold, 4.0f);
		Style.UndeterminedImage = Style.CheckedImage;
		Style.UndeterminedHoveredImage = Style.CheckedHoveredImage;
		Style.UndeterminedPressedImage = Style.CheckedPressedImage;
		for (FSlateBrush* Brush : {&Style.UncheckedImage, &Style.UncheckedHoveredImage, &Style.UncheckedPressedImage,
			&Style.CheckedImage, &Style.CheckedHoveredImage, &Style.CheckedPressedImage,
			&Style.UndeterminedImage, &Style.UndeterminedHoveredImage, &Style.UndeterminedPressedImage})
		{
			Brush->ImageSize = FVector2D(32.0f, 32.0f);
		}
		Style.BackgroundImage.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.BackgroundHoveredImage.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.BackgroundPressedImage.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Padding = FMargin(0.0f);
		Control->SetWidgetStyle(Style);
		return Control;
	};
	auto Slider = [Tree, Gold, BrightGold, Muted](FName Name)
	{
		USlider* Control = Tree->ConstructWidget<USlider>(USlider::StaticClass(), Name);
		Control->bIsVariable = false;
		FSliderStyle Style;
		Style.NormalBarImage = FSlateRoundedBoxBrush(Gold.CopyWithNewOpacity(0.65f), 0.0f);
		Style.HoveredBarImage = FSlateRoundedBoxBrush(BrightGold, 0.0f);
		Style.DisabledBarImage = FSlateRoundedBoxBrush(Muted.CopyWithNewOpacity(0.3f), 0.0f);
		Style.NormalThumbImage = FSlateRoundedBoxBrush(BrightGold, 0.0f, FVector2D(14.0f, 14.0f));
		Style.HoveredThumbImage = FSlateRoundedBoxBrush(FLinearColor(1.0f, 0.92f, 0.72f), 0.0f, FVector2D(16.0f, 16.0f));
		Style.DisabledThumbImage = FSlateRoundedBoxBrush(Muted, 0.0f, FVector2D(14.0f, 14.0f));
		Style.BarThickness = 2.0f;
		Control->SetWidgetStyle(Style);
		Control->SetStepSize(0.01f);
		Control->SetIndentHandle(true);
		return Control;
	};

	UCanvasPanel* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SettingsRoot"));
	Root->bIsVariable = false;
	Tree->RootWidget = Root;
	UOverlay* Surface = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SettingsSurface"));
	Surface->bIsVariable = false;
	Place(Root, Surface, FVector2D::ZeroVector, FVector2D(1280.0f, 760.0f), FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f));
	FillOverlay(Surface, Decoration(TEXT("Img_SettingsFrame"), Frame, Gold.CopyWithNewOpacity(0.8f), true));
	UCanvasPanel* Content = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SettingsContent"));
	Content->bIsVariable = false;
	FillOverlay(Surface, Content);
	Place(Content, Text(TEXT("Txt_SettingsTitle"), LOCTEXT("Title", "设 置"), 55, ETextJustify::Center), FVector2D(80.0f, 42.0f), FVector2D(1120.0f, 80.0f));
	Place(Content, Decoration(TEXT("Img_TopDivider"), Divider, Gold, false), FVector2D(88.0f, 146.0f), FVector2D(1104.0f, 16.0f));
	Place(Content, Rule(TEXT("Img_CategoryDivider"), Muted.CopyWithNewOpacity(0.45f)), FVector2D(314.0f, 206.0f), FVector2D(1.0f, 346.0f));
	const TCHAR* TabNames[] = {TEXT("Display"), TEXT("Quality"), TEXT("Audio"), TEXT("Controls")};
	const FText TabValues[] = {LOCTEXT("Display", "显示"), LOCTEXT("Quality", "画质"), LOCTEXT("Audio", "声音"), LOCTEXT("Controls", "操作")};
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Place(Content, Button(FName(*FString::Printf(TEXT("Btn_Tab%s"), TabNames[Index])),
			FName(*FString::Printf(TEXT("Txt_Tab%s"), TabNames[Index])), TabValues[Index], 34, false),
			FVector2D(90.0f, 204.0f + Index * 86.0f), FVector2D(196.0f, 64.0f));
	}
	UWidgetSwitcher* Pages = Tree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("SettingsPages"));
	Pages->bIsVariable = false;
	Place(Content, Pages, FVector2D(388.0f, 204.0f), FVector2D(788.0f, 382.0f));
	UCanvasPanel* PagePanels[4];
	for (int32 Index = 0; Index < 4; ++Index)
	{
		PagePanels[Index] = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),
			FName(*FString::Printf(TEXT("Page_%s"), TabNames[Index])));
		PagePanels[Index]->bIsVariable = false;
		Pages->AddChild(PagePanels[Index]);
	}
	auto RowLabel = [Place, Text](UCanvasPanel* Page, const TCHAR* Name, FText Value, float Y)
	{
		Place(Page, Text(FName(Name), Value, 30), FVector2D(0.0f, Y + 6.0f), FVector2D(316.0f, 46.0f));
	};
	auto ComboRow = [Place, RowLabel, Combo](UCanvasPanel* Page, const TCHAR* LabelName, FText Value, const TCHAR* Name, float Y)
	{
		RowLabel(Page, LabelName, Value, Y);
		Place(Page, Combo(FName(Name)), FVector2D(330.0f, Y), FVector2D(414.0f, 56.0f));
	};
	ComboRow(PagePanels[0], TEXT("Txt_WindowModeLabel"), LOCTEXT("WindowMode", "窗口模式"), TEXT("Cmb_WindowMode"), 0.0f);
	ComboRow(PagePanels[0], TEXT("Txt_ResolutionLabel"), LOCTEXT("Resolution", "分辨率"), TEXT("Cmb_Resolution"), 86.0f);
	UTextBlock* ResolutionHint = Text(TEXT("Txt_ResolutionHint"), LOCTEXT("ResolutionHint", "无边框模式使用桌面分辨率"), 20, ETextJustify::Center);
	ResolutionHint->SetColorAndOpacity(Muted);
	Place(PagePanels[0], ResolutionHint, FVector2D(330.0f, 146.0f), FVector2D(414.0f, 30.0f));
	RowLabel(PagePanels[0], TEXT("Txt_VSyncLabel"), LOCTEXT("VSync", "垂直同步"), 200.0f);
	Place(PagePanels[0], Check(TEXT("Chk_VSync")), FVector2D(350.0f, 212.0f), FVector2D(36.0f, 36.0f));
	Place(PagePanels[0], Text(TEXT("Txt_VSyncValue"), LOCTEXT("VSyncOff", "关闭"), 28), FVector2D(402.0f, 214.0f), FVector2D(300.0f, 36.0f));
	ComboRow(PagePanels[0], TEXT("Txt_FrameLimitLabel"), LOCTEXT("FrameLimit", "帧率上限"), TEXT("Cmb_FrameLimit"), 286.0f);
	const TCHAR* QualityNames[] = {TEXT("OverallQuality"), TEXT("ShadowQuality"), TEXT("TextureQuality"), TEXT("EffectsQuality")};
	const FText QualityLabels[] = {LOCTEXT("OverallQuality", "整体画质"), LOCTEXT("ShadowQuality", "阴影质量"), LOCTEXT("TextureQuality", "纹理质量"), LOCTEXT("EffectsQuality", "特效质量")};
	for (int32 Index = 0; Index < 4; ++Index)
	{
		ComboRow(PagePanels[1], *FString::Printf(TEXT("Txt_%sLabel"), QualityNames[Index]), QualityLabels[Index],
			*FString::Printf(TEXT("Cmb_%s"), QualityNames[Index]), Index * 86.0f);
	}
	RowLabel(PagePanels[2], TEXT("Txt_MasterVolumeLabel"), LOCTEXT("Volume", "总音量"), 30.0f);
	Place(PagePanels[2], Slider(TEXT("Sld_MasterVolume")), FVector2D(330.0f, 30.0f), FVector2D(310.0f, 54.0f));
	Place(PagePanels[2], Text(TEXT("Txt_VolumeValue"), LOCTEXT("VolumePreview", "100%"), 30, ETextJustify::Right), FVector2D(650.0f, 36.0f), FVector2D(94.0f, 46.0f));
	RowLabel(PagePanels[3], TEXT("Txt_MouseSensitivityLabel"), LOCTEXT("Sensitivity", "鼠标灵敏度"), 30.0f);
	Place(PagePanels[3], Slider(TEXT("Sld_MouseSensitivity")), FVector2D(330.0f, 30.0f), FVector2D(310.0f, 54.0f));
	Place(PagePanels[3], Text(TEXT("Txt_SensitivityValue"), LOCTEXT("SensitivityPreview", "1.00"), 30, ETextJustify::Right), FVector2D(650.0f, 36.0f), FVector2D(94.0f, 46.0f));
	RowLabel(PagePanels[3], TEXT("Txt_InvertYLabel"), LOCTEXT("InvertY", "反转鼠标 Y 轴"), 136.0f);
	Place(PagePanels[3], Check(TEXT("Chk_InvertY")), FVector2D(350.0f, 148.0f), FVector2D(36.0f, 36.0f));
	Place(PagePanels[3], Text(TEXT("Txt_InvertYValue"), LOCTEXT("InvertYOff", "关闭"), 28), FVector2D(402.0f, 150.0f), FVector2D(300.0f, 36.0f));
	Pages->SetActiveWidgetIndex(0);
	UTextBlock* Status = Text(TEXT("Txt_SettingsStatus"), LOCTEXT("Status", "调整完成后，选择“应用”保存设置。"), 20);
	Status->SetColorAndOpacity(Muted);
	Place(Content, Status, FVector2D(388.0f, 590.0f), FVector2D(788.0f, 58.0f));
	Place(Content, Decoration(TEXT("Img_BottomDivider"), Divider, Gold, false), FVector2D(88.0f, 648.0f), FVector2D(1104.0f, 16.0f));
	Place(Content, Button(TEXT("Btn_Defaults"), TEXT("Txt_Defaults"), LOCTEXT("Defaults", "恢复默认"), 30, false), FVector2D(88.0f, 674.0f), FVector2D(220.0f, 60.0f));
	Place(Content, Button(TEXT("Btn_Apply"), TEXT("Txt_Apply"), LOCTEXT("Apply", "应用"), 30, true), FVector2D(822.0f, 674.0f), FVector2D(160.0f, 60.0f));
	Place(Content, Button(TEXT("Btn_Back"), TEXT("Txt_Back"), LOCTEXT("Back", "返回"), 30, true), FVector2D(1028.0f, 674.0f), FVector2D(160.0f, 60.0f));

	// This sibling remains interactive while the runtime disables SettingsContent.
	UCanvasPanel* Confirm = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ConfirmPanel"));
	Confirm->bIsVariable = false;
	Confirm->SetVisibility(ESlateVisibility::Collapsed);
	FillOverlay(Surface, Confirm);
	Place(Confirm, Decoration(TEXT("Img_ConfirmFrame"), Frame, BrightGold, true), FVector2D(176.0f, 204.0f), FVector2D(928.0f, 352.0f));
	UTextBlock* Confirmation = Text(TEXT("Txt_Confirmation"), LOCTEXT("Confirmation", "是否保留当前设置？"), 30, ETextJustify::Center);
	Confirmation->SetAutoWrapText(true);
	Place(Confirm, Confirmation, FVector2D(224.0f, 252.0f), FVector2D(832.0f, 142.0f));
	Place(Confirm, Button(TEXT("Btn_ConfirmYes"), TEXT("Txt_ConfirmYes"), LOCTEXT("ConfirmYes", "保留"), 30, true), FVector2D(396.0f, 446.0f), FVector2D(220.0f, 64.0f));
	Place(Confirm, Button(TEXT("Btn_ConfirmNo"), TEXT("Txt_ConfirmNo"), LOCTEXT("ConfirmNo", "还原"), 30, true), FVector2D(664.0f, 446.0f), FVector2D(220.0f, 64.0f));
}

#undef LOCTEXT_NAMESPACE
