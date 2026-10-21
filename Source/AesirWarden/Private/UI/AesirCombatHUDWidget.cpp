// Copyright (c) 2026 YYchainsAw. All rights reserved.

#include "UI/AesirCombatHUDWidget.h"

#include "Abilities/AesirAttributeSet.h"
#include "Abilities/AesirPlayerAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AI/Boss/AesirBossAIController.h"
#include "Blueprint/WidgetTree.h"
#include "Combat/AesirHealthComponent.h"
#include "Combat/AesirTargetingComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"

#define LOCTEXT_NAMESPACE "AesirCombatHUD"

namespace AesirHUD
{
	TArray<FGameplayAttribute> PlayerAttributes()
	{
		return {UAesirAttributeSet::GetHealthAttribute(), UAesirAttributeSet::GetMaxHealthAttribute(),
			UAesirPlayerAttributeSet::GetGuardPressureAttribute(), UAesirPlayerAttributeSet::GetMaxGuardPressureAttribute()};
	}

	TArray<FGameplayAttribute> BossAttributes()
	{
		return {UAesirAttributeSet::GetHealthAttribute(), UAesirAttributeSet::GetMaxHealthAttribute()};
	}

	bool HasHealth(UAbilitySystemComponent* ASC)
	{
		return IsValid(ASC) && ASC->HasAttributeSetForAttribute(UAesirAttributeSet::GetHealthAttribute());
	}

	bool IsLiveBoss(AActor* Actor, const UWorld* World)
	{
		if (!IsValid(Actor) || Actor->IsActorBeingDestroyed() || Actor->GetWorld() != World)
		{
			return false;
		}
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
		return HasHealth(ASC) && ASC->GetNumericAttribute(UAesirAttributeSet::GetHealthAttribute()) > 0.0f &&
			ASC->GetNumericAttribute(UAesirAttributeSet::GetMaxHealthAttribute()) > 0.0f;
	}

	FText HealthText(float Health, float MaxHealth)
	{
		if (!FMath::IsFinite(Health) || !FMath::IsFinite(MaxHealth) || MaxHealth <= 0.0f)
		{
			return LOCTEXT("NoHealth", "— / —");
		}
		FNumberFormattingOptions Format;
		Format.SetMaximumFractionalDigits(0);
		return FText::Format(LOCTEXT("HealthValue", "{0} / {1}"),
			FText::AsNumber(FMath::Clamp(Health, 0.0f, MaxHealth), &Format), FText::AsNumber(MaxHealth, &Format));
	}
}

float UAesirCombatHUDWidget::SafeRatio(float Value, float Maximum)
{
	return FMath::IsFinite(Value) && FMath::IsFinite(Maximum) && Maximum > 0.0f
		? FMath::Clamp(Value / Maximum, 0.0f, 1.0f) : 0.0f;
}

TSharedRef<SWidget> UAesirCombatHUDWidget::RebuildWidget()
{
	BuildDefaultHUDTree(WidgetTree);
	return Super::RebuildWidget();
}

void UAesirCombatHUDWidget::ResolveWidgets()
{
	PlayerHealthBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("PB_PlayerHealth")));
	PlayerHealthTrail = Cast<UProgressBar>(GetWidgetFromName(TEXT("PB_PlayerHealthTrail")));
	GuardPressureBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("PB_GuardPressure")));
	HealthValueText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Txt_PlayerHealth")));
	GuardStateText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Txt_GuardState")));
	CachedBossPanel = GetWidgetFromName(TEXT("BossPanel"));
	BossHealthBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("PB_BossHealth")));
	BossNameText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Txt_BossName")));
	AbilityPanel = Cast<UHorizontalBox>(GetWidgetFromName(TEXT("HB_Abilities")));
}

void UAesirCombatHUDWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ResolveWidgets();
	if (PlayerHealthBar) PlayerHealthBar->SetFillColorAndOpacity(HealthColor);
	if (GuardPressureBar) GuardPressureBar->SetFillColorAndOpacity(GuardColor);
	if (BossHealthBar) BossHealthBar->SetFillColorAndOpacity(HealthColor);
	if (IsDesignTime())
	{
		if (PlayerHealthBar) PlayerHealthBar->SetPercent(PreviewHealthRatio);
		if (PlayerHealthTrail) PlayerHealthTrail->SetPercent(PreviewHealthRatio);
		if (GuardPressureBar) GuardPressureBar->SetPercent(PreviewGuardRatio);
		if (HealthValueText) HealthValueText->SetText(AesirHUD::HealthText(PreviewHealthRatio * 100.0f, 100.0f));
		if (CachedBossPanel) CachedBossPanel->SetVisibility(bPreviewBoss ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (BossHealthBar) BossHealthBar->SetPercent(PreviewHealthRatio);
	}
}

void UAesirCombatHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveWidgets();
	RebuildAbilitySlots();
	RefreshHUDSources();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RefreshTimer, this, &UAesirCombatHUDWidget::RefreshHUDSources, 0.1f, true);
	}
}

void UAesirCombatHUDWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(RefreshTimer);
	UnbindPlayer();
	UnbindBoss();
	Super::NativeDestruct();
}

void UAesirCombatHUDWidget::BindPlayer(APawn* Pawn)
{
	UAbilitySystemComponent* ASC = IsValid(Pawn) ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn) : nullptr;
	if (BoundPlayer.Get() == Pawn && PlayerASC.Get() == ASC) return;
	UnbindPlayer();
	BoundPlayer = Pawn;
	PlayerASC = ASC;
	bHealthInitialized = false;
	if (ASC)
	{
		for (const FGameplayAttribute& Attribute : AesirHUD::PlayerAttributes())
		{
			PlayerAttributeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(
				this, &UAesirCombatHUDWidget::HandlePlayerAttributeChanged));
		}
	}
}

void UAesirCombatHUDWidget::UnbindPlayer()
{
	// The migrated Blueprint's original Construct graph also observes its first
	// pawn's HealthComponent. Release that legacy callback before switching pawns
	// or removing the HUD so it cannot write an obsolete health percentage.
	if (APawn* Pawn = BoundPlayer.Get())
	{
		if (UAesirHealthComponent* Health = Pawn->FindComponentByClass<UAesirHealthComponent>())
		{
			Health->OnHealthChanged.RemoveAll(this);
		}
	}
	if (UAbilitySystemComponent* ASC = PlayerASC.Get())
	{
		const TArray<FGameplayAttribute> Attributes = AesirHUD::PlayerAttributes();
		for (int32 Index = 0; Index < PlayerAttributeHandles.Num(); ++Index)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Attributes[Index]).Remove(PlayerAttributeHandles[Index]);
		}
	}
	PlayerAttributeHandles.Reset();
	PlayerASC.Reset();
	BoundPlayer.Reset();
}

void UAesirCombatHUDWidget::BindBoss(AActor* Boss)
{
	UAbilitySystemComponent* ASC = IsValid(Boss) ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Boss) : nullptr;
	if (DisplayedBoss.Get() == Boss && BossASC.Get() == ASC) return;
	UnbindBoss();
	DisplayedBoss = Boss;
	BossASC = ASC;
	if (ASC)
	{
		for (const FGameplayAttribute& Attribute : AesirHUD::BossAttributes())
		{
			BossAttributeHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(
				this, &UAesirCombatHUDWidget::HandleBossAttributeChanged));
		}
	}
}

void UAesirCombatHUDWidget::UnbindBoss()
{
	if (UAbilitySystemComponent* ASC = BossASC.Get())
	{
		const TArray<FGameplayAttribute> Attributes = AesirHUD::BossAttributes();
		for (int32 Index = 0; Index < BossAttributeHandles.Num(); ++Index)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Attributes[Index]).Remove(BossAttributeHandles[Index]);
		}
	}
	BossAttributeHandles.Reset();
	BossASC.Reset();
	DisplayedBoss.Reset();
}

void UAesirCombatHUDWidget::SetBossTarget(AActor* Target, FText DisplayName)
{
	bBossPinned = IsValid(Target) && Target->GetWorld() == GetWorld();
	PinnedBoss = bBossPinned ? Target : nullptr;
	PinnedBossName = MoveTemp(DisplayName);
	RefreshHUDSources();
}

void UAesirCombatHUDWidget::ClearBossTarget()
{
	bBossPinned = false;
	PinnedBoss.Reset();
	PinnedBossName = FText::GetEmpty();
	RefreshHUDSources();
}

void UAesirCombatHUDWidget::RefreshHUDSources()
{
	BindPlayer(GetOwningPlayerPawn());
	RefreshPlayerVitals();
	AActor* Candidate = bBossPinned ? PinnedBoss.Get() : nullptr;
	if (!bBossPinned && bFollowLockedBoss && BoundPlayer.IsValid())
	{
		const UAesirTargetingComponent* Targeting = BoundPlayer->FindComponentByClass<UAesirTargetingComponent>();
		APawn* TargetPawn = Targeting ? Cast<APawn>(Targeting->GetCurrentTarget()) : nullptr;
		if (TargetPawn && Cast<AAesirBossAIController>(TargetPawn->GetController())) Candidate = TargetPawn;
	}
	BindBoss(AesirHUD::IsLiveBoss(Candidate, GetWorld()) ? Candidate : nullptr);
	RefreshBossVitals();
	RefreshAbilitySlots();
}

void UAesirCombatHUDWidget::HandlePlayerAttributeChanged(const FOnAttributeChangeData& Data)
{
	RefreshPlayerVitals();
}

void UAesirCombatHUDWidget::HandleBossAttributeChanged(const FOnAttributeChangeData& Data)
{
	RefreshBossVitals();
}

void UAesirCombatHUDWidget::RefreshPlayerVitals()
{
	UAbilitySystemComponent* ASC = PlayerASC.Get();
	const bool bHasHealth = AesirHUD::HasHealth(ASC);
	const float Health = bHasHealth ? ASC->GetNumericAttribute(UAesirAttributeSet::GetHealthAttribute()) : 0.0f;
	const float Maximum = bHasHealth ? ASC->GetNumericAttribute(UAesirAttributeSet::GetMaxHealthAttribute()) : 0.0f;
	const float Ratio = SafeRatio(Health, Maximum);
	if (!bHealthInitialized || Ratio >= HealthTrailRatio || Ratio > LastHealthRatio)
	{
		HealthTrailRatio = Ratio;
		HealthTrailDelay = 0.0f;
		bHealthInitialized = true;
	}
	else if (Ratio < LastHealthRatio)
	{
		HealthTrailDelay = 0.35f;
	}
	LastHealthRatio = Ratio;
	if (PlayerHealthBar)
	{
		PlayerHealthBar->SetPercent(Ratio);
		PlayerHealthBar->SetFillColorAndOpacity(HealthColor);
	}
	if (PlayerHealthTrail) PlayerHealthTrail->SetPercent(HealthTrailRatio);
	if (HealthValueText) HealthValueText->SetText(AesirHUD::HealthText(Health, Maximum));
	const bool bHasGuard = ASC && ASC->HasAttributeSetForAttribute(UAesirPlayerAttributeSet::GetGuardPressureAttribute());
	const float Guard = bHasGuard ? ASC->GetNumericAttribute(UAesirPlayerAttributeSet::GetGuardPressureAttribute()) : 0.0f;
	const float MaxGuard = bHasGuard ? ASC->GetNumericAttribute(UAesirPlayerAttributeSet::GetMaxGuardPressureAttribute()) : 0.0f;
	const FGameplayTag BrokenTag = FGameplayTag::RequestGameplayTag(TEXT("state.guard_broken"), false);
	const bool bBroken = ASC && BrokenTag.IsValid() && ASC->HasMatchingGameplayTag(BrokenTag);
	if (GuardPressureBar)
	{
		GuardPressureBar->SetPercent(bBroken ? 1.0f : SafeRatio(Guard, MaxGuard));
		GuardPressureBar->SetFillColorAndOpacity(bBroken ? BrokenGuardColor : GuardColor);
	}
	if (GuardStateText)
	{
		GuardStateText->SetText(bBroken ? LOCTEXT("GuardBroken", "破防") : LOCTEXT("GuardPressure", "防御压力"));
		GuardStateText->SetColorAndOpacity(FSlateColor(bBroken ? BrokenGuardColor : FLinearColor(0.88f, 0.80f, 0.63f)));
	}
}

void UAesirCombatHUDWidget::RefreshBossVitals()
{
	const bool bAlive = AesirHUD::IsLiveBoss(DisplayedBoss.Get(), GetWorld());
	if (CachedBossPanel) CachedBossPanel->SetVisibility(bAlive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!bAlive)
	{
		UnbindBoss();
		if (BossHealthBar) BossHealthBar->SetPercent(0.0f);
		return;
	}
	if (BossHealthBar)
	{
		BossHealthBar->SetPercent(SafeRatio(BossASC->GetNumericAttribute(UAesirAttributeSet::GetHealthAttribute()),
			BossASC->GetNumericAttribute(UAesirAttributeSet::GetMaxHealthAttribute())));
		BossHealthBar->SetFillColorAndOpacity(HealthColor);
	}
	if (BossNameText) BossNameText->SetText(bBossPinned && !PinnedBossName.IsEmpty() ? PinnedBossName : LOCTEXT("Boss", "首领"));
}

void UAesirCombatHUDWidget::NativeTick(const FGeometry& MyGeometry, float DeltaSeconds)
{
	Super::NativeTick(MyGeometry, DeltaSeconds);
	if (!PlayerHealthBar || !PlayerHealthTrail) return;
	HealthTrailDelay = FMath::Max(0.0f, HealthTrailDelay - DeltaSeconds);
	if (HealthTrailDelay <= 0.0f)
	{
		HealthTrailRatio = FMath::FInterpConstantTo(HealthTrailRatio, PlayerHealthBar->GetPercent(), DeltaSeconds, 0.65f);
		PlayerHealthTrail->SetPercent(HealthTrailRatio);
	}
}

void UAesirCombatHUDWidget::RebuildAbilitySlots()
{
	if (!AbilityPanel || !WidgetTree || bAbilitySlotsBuilt) return;
	bAbilitySlotsBuilt = true;
	AbilityPanel->ClearChildren();
	UFont* Font = LoadObject<UFont>(nullptr, TEXT("/Game/Aesir/UI/Fonts/F_MenuSerif.F_MenuSerif"), nullptr, LOAD_NoWarn);
	UTexture2D* Frame = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/Aesir/UI/Art/Kenney/FantasyUIBorders/Double/Border/T_KUI_panel_border_002.T_KUI_panel_border_002"), nullptr, LOAD_NoWarn);
	for (int32 Index = 0; Index < AbilityEntries.Num(); ++Index)
	{
		const FAesirHUDAbilityEntry& Entry = AbilityEntries[Index];
		auto Name = [Index](const TCHAR* Part) { return FName(*FString::Printf(TEXT("Ability_%d_%s"), Index, Part)); };
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name(TEXT("Column")));
		AbilityPanel->AddChildToHorizontalBox(Column)->SetPadding(FMargin(8.0f, 0.0f));
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), Name(TEXT("Size")));
		Size->SetWidthOverride(64.0f);
		Size->SetHeightOverride(64.0f);
		Column->AddChildToVerticalBox(Size);
		UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), Name(TEXT("Overlay")));
		Size->SetContent(Overlay);
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name(TEXT("Icon")));
		Icon->SetBrushFromTexture(Entry.Icon);
		Icon->SetColorAndOpacity(FLinearColor(0.96f, 0.91f, 0.79f));
		UOverlaySlot* IconSlot = Overlay->AddChildToOverlay(Icon);
		IconSlot->SetHorizontalAlignment(HAlign_Fill);
		IconSlot->SetVerticalAlignment(VAlign_Fill);
		IconSlot->SetPadding(FMargin(12.0f));
		UProgressBar* Shade = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), Name(TEXT("Cooldown")));
		FProgressBarStyle Style;
		Style.BackgroundImage.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.FillImage.DrawAs = ESlateBrushDrawType::Image;
		Style.FillImage.TintColor = FSlateColor(FLinearColor::White);
		Shade->SetWidgetStyle(Style);
		Shade->SetBarFillType(EProgressBarFillType::BottomToTop);
		Shade->SetBarFillStyle(EProgressBarFillStyle::Scale);
		Shade->SetFillColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
		Shade->SetBorderPadding(FVector2D::ZeroVector);
		UOverlaySlot* ShadeSlot = Overlay->AddChildToOverlay(Shade);
		ShadeSlot->SetHorizontalAlignment(HAlign_Fill);
		ShadeSlot->SetVerticalAlignment(VAlign_Fill);
		ShadeSlot->SetPadding(FMargin(5.0f));
		UImage* Border = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name(TEXT("Frame")));
		FSlateBrush Brush;
		Brush.SetResourceObject(Frame);
		Brush.DrawAs = Frame ? ESlateBrushDrawType::Box : ESlateBrushDrawType::NoDrawType;
		Brush.Margin = FMargin(0.25f);
		Brush.TintColor = FSlateColor(FLinearColor(0.73f, 0.55f, 0.28f, 0.8f));
		Border->SetBrush(Brush);
		UOverlaySlot* BorderSlot = Overlay->AddChildToOverlay(Border);
		BorderSlot->SetHorizontalAlignment(HAlign_Fill);
		BorderSlot->SetVerticalAlignment(VAlign_Fill);
		auto MakeLabel = [this, Font](FName WidgetName, FText Text, int32 PointSize)
		{
			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), WidgetName);
			Label->SetText(Text);
			if (Font) Label->SetFont(FSlateFontInfo(Font, PointSize, TEXT("Regular")));
			Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.91f, 0.79f)));
			Label->SetJustification(ETextJustify::Center);
			Label->SetShadowOffset(FVector2D(1.0f, 1.0f));
			return Label;
		};
		UTextBlock* Time = MakeLabel(Name(TEXT("Time")), FText::GetEmpty(), 24);
		UOverlaySlot* TimeSlot = Overlay->AddChildToOverlay(Time);
		TimeSlot->SetHorizontalAlignment(HAlign_Center);
		TimeSlot->SetVerticalAlignment(VAlign_Center);
		Column->AddChildToVerticalBox(MakeLabel(Name(TEXT("Key")), Entry.InputHint, 16));
		Column->AddChildToVerticalBox(MakeLabel(Name(TEXT("Label")), Entry.Label, 16));
	}
}

void UAesirCombatHUDWidget::RefreshAbilitySlots()
{
	if (!AbilityPanel) return;
	UAbilitySystemComponent* ASC = PlayerASC.Get();
	AbilityPanel->SetVisibility(ASC && !AbilityEntries.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	for (int32 Index = 0; Index < AbilityEntries.Num(); ++Index)
	{
		const FGameplayTag Tag = AbilityEntries[Index].CooldownTag;
		float Remaining = 0.0f;
		float Duration = 0.0f;
		bool bIndefinite = false;
		if (ASC && Tag.IsValid())
		{
			const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Tag));
			for (const TPair<float, float>& Effect : ASC->GetActiveEffectsTimeRemainingAndDuration(Query))
			{
				bIndefinite |= Effect.Key < 0.0f;
				if (Effect.Key > Remaining) { Remaining = Effect.Key; Duration = Effect.Value; }
			}
			// Loose cooldown tags can block activation without supplying an expiry.
			bIndefinite |= Remaining <= 0.0f && ASC->HasMatchingGameplayTag(Tag);
		}
		const FString Prefix = FString::Printf(TEXT("Ability_%d_"), Index);
		if (UProgressBar* Shade = Cast<UProgressBar>(GetWidgetFromName(FName(*(Prefix + TEXT("Cooldown"))))))
		{
			Shade->SetPercent(bIndefinite ? 1.0f : SafeRatio(Remaining, Duration));
		}
		if (UTextBlock* Time = Cast<UTextBlock>(GetWidgetFromName(FName(*(Prefix + TEXT("Time"))))))
		{
			Time->SetText(bIndefinite ? LOCTEXT("UnknownCooldown", "…")
				: Remaining > 0.0f ? FText::AsNumber(FMath::CeilToInt(Remaining)) : FText::GetEmpty());
		}
	}
}

#undef LOCTEXT_NAMESPACE
