// Copyright (c) 2026 YYchainsAw. All rights reserved.

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Abilities/AesirAttributeSet.h"
#include "Abilities/AesirPlayerAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/Enemy/AesirEnemyCharacter.h"
#include "Characters/Player/AesirPlayerCharacter.h"
#include "Components/HorizontalBox.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "Slate/SceneViewport.h"
#include "UI/AesirCombatHUDWidget.h"
#include "UObject/UnrealType.h"
#include "Widgets/SViewport.h"

#include <limits>

namespace AesirCombatHUDTests
{

class FHUDWorld
{
public:
	FHUDWorld()
	{
		GameInstance = NewObject<UGameInstance>(GEngine);
		GameInstance->AddToRoot();
		GameInstance->InitializeStandalone(MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("AesirCombatHUDTest")));
		World = GameInstance->GetWorld();
		FWorldContext& Context = *GameInstance->GetWorldContext();
		Viewport = NewObject<UGameViewportClient>(GEngine);
		Context.GameViewport = Viewport;
		Viewport->Init(Context, GameInstance, false);
		SlateViewport = SNew(SViewport);
		SceneViewport = Viewport->CreateViewport(SlateViewport);
		World->InitializeActorsForPlay(FURL());
		FString Error;
		ULocalPlayer* LocalPlayer = GameInstance->CreateLocalPlayer(0, Error, false);
		if (!LocalPlayer)
		{
			return;
		}
		Controller = World->SpawnActor<APlayerController>();
		if (Controller)
		{
			Controller->SetPlayer(LocalPlayer);
			Player = CreatePlayer();
		}
	}

	~FHUDWorld()
	{
		if (HUD)
		{
			HUD->RemoveFromParent();
		}
		for (AActor* Actor : FActorRange(World))
		{
			Actor->RouteEndPlay(EEndPlayReason::LevelTransition);
		}
		Viewport->DetachViewportClient();
		SceneViewport.Reset();
		SlateViewport.Reset();
		GameInstance->Shutdown();
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		GameInstance->RemoveFromRoot();
	}

	AAesirPlayerCharacter* CreatePlayer()
	{
		AAesirPlayerCharacter* Character = World->SpawnActor<AAesirPlayerCharacter>();
		if (!Character)
		{
			return nullptr;
		}
		if (FClassProperty* Effect = FindFProperty<FClassProperty>(Character->GetClass(), TEXT("DefaultAttributesEffect")))
		{
			Effect->SetObjectPropertyValue_InContainer(Character, UGameplayEffect::StaticClass());
		}
		Character->GetCharacterMovement()->DisableMovement();
		Controller->Possess(Character);
		Character->DispatchBeginPlay();
		return Character;
	}

	AAesirEnemyCharacter* CreateBoss()
	{
		// Initialize only the enemy's actual GAS data. A combat AI/BT and damage
		// reactions are outside HUD observation and must not run in this fixture.
		FActorSpawnParameters Parameters;
		Parameters.bDeferConstruction = true;
		AAesirEnemyCharacter* Boss = World->SpawnActor<AAesirEnemyCharacter>(
			AAesirEnemyCharacter::StaticClass(), FVector(1000.0f, 0.0f, 0.0f), FRotator::ZeroRotator, Parameters);
		if (!Boss)
		{
			return nullptr;
		}
		Boss->AutoPossessAI = EAutoPossessAI::Disabled;
		Boss->FinishSpawning(Boss->GetActorTransform());
		Boss->GetCharacterMovement()->DisableMovement();
		UAbilitySystemComponent* ASC = Boss->GetAbilitySystemComponent();
		ASC->InitAbilityActorInfo(Boss, Boss);
		ASC->SetNumericAttributeBase(UAesirAttributeSet::GetMaxHealthAttribute(), 500.0f);
		ASC->SetNumericAttributeBase(UAesirAttributeSet::GetHealthAttribute(), 500.0f);
		return Boss;
	}

	bool CreateHUD(FAutomationTestBase* Test, bool bConfigureAbility = false)
	{
		UClass* HUDClass = LoadClass<UAesirCombatHUDWidget>(nullptr,
			TEXT("/Game/Aesir/UI/HealthBars/WBP_CombatHUD.WBP_CombatHUD_C"));
		if (!Test->TestNotNull(TEXT("The real migrated WBP_CombatHUD loads with its native parent"), HUDClass))
		{
			return false;
		}
		HUD = CreateWidget<UAesirCombatHUDWidget>(Controller, HUDClass);
		if (!Test->TestNotNull(TEXT("The real combat HUD can be created"), HUD))
		{
			return false;
		}
		if (bConfigureAbility)
		{
			FAesirHUDAbilityEntry Entry;
			Entry.Label = FText::FromString(TEXT("测试治疗"));
			Entry.InputHint = FText::FromString(TEXT("H"));
			Entry.CooldownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Player.TestHeal"));
			HUD->AbilityEntries = { Entry };
		}
		HUD->AddToViewport();
		HUD->RefreshHUDSources();
		return true;
	}

	UGameInstance* GameInstance = nullptr;
	UWorld* World = nullptr;
	UGameViewportClient* Viewport = nullptr;
	APlayerController* Controller = nullptr;
	AAesirPlayerCharacter* Player = nullptr;
	UAesirCombatHUDWidget* HUD = nullptr;
	TSharedPtr<SViewport> SlateViewport;
	TSharedPtr<FSceneViewport> SceneViewport;
};

template <typename TWidget>
TWidget* Widget(UAesirCombatHUDWidget* HUD, const TCHAR* Name)
{
	return Cast<TWidget>(HUD->GetWidgetFromName(Name));
}

class FExerciseHUD final : public IAutomationLatentCommand
{
public:
	FExerciseHUD(FAutomationTestBase* InTest, TSharedPtr<FHUDWorld> InFixture, FString InMode)
		: Test(InTest), Fixture(MoveTemp(InFixture)), Mode(MoveTemp(InMode))
	{
	}
	virtual bool Update() override;

private:
	FAutomationTestBase* Test;
	TSharedPtr<FHUDWorld> Fixture;
	FString Mode;
	AAesirEnemyCharacter* Boss = nullptr;
	AAesirPlayerCharacter* PreviousPlayer = nullptr;
	float InitialCooldownRatio = 0.0f;
	int32 Frames = 0;
	int32 Stage = 0;
};

bool FExerciseHUD::Update()
{
	// TimerManager first activates pending timers after its first tick, and
	// dispatches only when InternalTime > ExpireTime (not on equality). Step
	// beyond the HUD's 0.1 s cadence so the next frame observes a real timer
	// callback rather than landing exactly on the pending timer's deadline.
	Fixture->World->Tick(LEVELTICK_All, 0.125f);
	if (++Frames > 40)
	{
		Test->AddError(FString::Printf(TEXT("HUD case %s did not finish within 40 ticks."), *Mode));
		return true;
	}
	UAesirCombatHUDWidget* HUD = Fixture->HUD;
	UAbilitySystemComponent* ASC = Fixture->Player->GetAbilitySystemComponent();
	UProgressBar* Health = Widget<UProgressBar>(HUD, TEXT("PB_PlayerHealth"));
	UProgressBar* Guard = Widget<UProgressBar>(HUD, TEXT("PB_GuardPressure"));
	UProgressBar* Trail = Widget<UProgressBar>(HUD, TEXT("PB_PlayerHealthTrail"));
	UTextBlock* HealthText = Widget<UTextBlock>(HUD, TEXT("Txt_PlayerHealth"));
	UTextBlock* GuardText = Widget<UTextBlock>(HUD, TEXT("Txt_GuardState"));
	UWidget* BossPanel = HUD->GetWidgetFromName(TEXT("BossPanel"));
	UProgressBar* BossHealth = Widget<UProgressBar>(HUD, TEXT("PB_BossHealth"));
	UTextBlock* BossName = Widget<UTextBlock>(HUD, TEXT("Txt_BossName"));
	UHorizontalBox* Abilities = Widget<UHorizontalBox>(HUD, TEXT("HB_Abilities"));
	if (Stage == 0 && (!Test->TestNotNull(TEXT("The saved HUD has its player health bar"), Health) ||
		!Test->TestNotNull(TEXT("The saved HUD has its guard-pressure bar"), Guard) ||
		!Test->TestNotNull(TEXT("The saved HUD has a damage trail"), Trail) ||
		!Test->TestNotNull(TEXT("The saved HUD has readable health text"), HealthText) ||
		!Test->TestNotNull(TEXT("The saved HUD has guard state text"), GuardText) ||
		!Test->TestNotNull(TEXT("The saved HUD has the boss container"), BossPanel) ||
		!Test->TestNotNull(TEXT("The saved HUD has boss health"), BossHealth) ||
		!Test->TestNotNull(TEXT("The saved HUD has a boss name"), BossName) ||
		!Test->TestNotNull(TEXT("The saved HUD has an ability container"), Abilities)))
	{
		return true;
	}
	if (Mode == TEXT("PlayerVitals"))
	{
		if (Stage == 0)
		{
			Test->TestTrue(TEXT("Initial native health appears full"), FMath::IsNearlyEqual(Health->GetPercent(), 1.0f));
			Test->TestTrue(TEXT("Guard pressure starts empty rather than pretending to be stamina"), FMath::IsNearlyZero(Guard->GetPercent()));
			Test->TestFalse(TEXT("An unconfigured ability panel adds no placeholder skills"), Abilities->IsVisible());
			ASC->SetNumericAttributeBase(UAesirAttributeSet::GetHealthAttribute(), 60.0f);
			Test->TestTrue(TEXT("A health change updates the existing widget through GAS without replacing the player"), FMath::IsNearlyEqual(Health->GetPercent(), 0.6f));
			Test->TestTrue(TEXT("Damage initially leaves a delayed health trail"), Trail->GetPercent() >= Health->GetPercent());
			ASC->SetNumericAttributeBase(UAesirAttributeSet::GetMaxHealthAttribute(), 200.0f);
			Test->TestTrue(TEXT("Changing maximum health refreshes the ratio"), FMath::IsNearlyEqual(Health->GetPercent(), 0.3f));
			Test->TestTrue(TEXT("Current health remains readable after the maximum changes"), HealthText->GetText().ToString().Contains(TEXT("60")));
			Test->TestTrue(TEXT("Maximum health remains readable"), HealthText->GetText().ToString().Contains(TEXT("200")));
			ASC->SetNumericAttributeBase(UAesirPlayerAttributeSet::GetGuardPressureAttribute(), 65.0f);
			Test->TestTrue(TEXT("Guard pressure fills upward as pressure accumulates"), FMath::IsNearlyEqual(Guard->GetPercent(), 0.65f));
			ASC->SetNumericAttributeBase(UAesirPlayerAttributeSet::GetMaxGuardPressureAttribute(), 200.0f);
			Test->TestTrue(TEXT("Maximum guard pressure changes update its denominator"), FMath::IsNearlyEqual(Guard->GetPercent(), 0.325f));
			ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("state.guard_broken")));
			Stage = 1;
			return false;
		}
		Test->TestTrue(TEXT("Guard-broken state changes the displayed bar color"), Guard->GetFillColorAndOpacity().Equals(HUD->BrokenGuardColor));
		Test->TestFalse(TEXT("Guard-broken state has a readable label"), GuardText->GetText().IsEmpty());
		ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("state.guard_broken")));
		return true;
	}
	if (Mode == TEXT("HiddenAndRebind"))
	{
		if (Stage == 0)
		{
			HUD->SetVisibility(ESlateVisibility::Collapsed);
			ASC->SetNumericAttributeBase(UAesirAttributeSet::GetHealthAttribute(), 70.0f);
			HUD->RefreshHUDSources();
			Test->TestEqual(TEXT("HUD data refresh cannot unhide a menu-suppressed HUD"), HUD->GetVisibility(), ESlateVisibility::Collapsed);
			HUD->SetVisibility(ESlateVisibility::Visible);
			HUD->RemoveFromParent();
			Stage = 1;
			return false;
		}
		if (Stage == 1)
		{
			Test->TestFalse(TEXT("Removing the HUD unbinds its health observer"), ASC->GetGameplayAttributeValueChangeDelegate(UAesirAttributeSet::GetHealthAttribute()).IsBoundToObject(HUD));
			HUD->AddToViewport();
			HUD->RefreshHUDSources();
			ASC->SetNumericAttributeBase(UAesirAttributeSet::GetHealthAttribute(), 50.0f);
			Test->TestTrue(TEXT("Readding the same widget reconnects live values"), FMath::IsNearlyEqual(Health->GetPercent(), 0.5f));
			PreviousPlayer = Fixture->Player;
			Fixture->Player = Fixture->CreatePlayer();
			Fixture->Player->GetAbilitySystemComponent()->SetNumericAttributeBase(UAesirAttributeSet::GetMaxHealthAttribute(), 200.0f);
			Fixture->Player->GetAbilitySystemComponent()->SetNumericAttributeBase(UAesirAttributeSet::GetHealthAttribute(), 150.0f);
			Stage = 2;
			return false;
		}
		Test->TestTrue(TEXT("Automatic source refresh follows a newly possessed player"), FMath::IsNearlyEqual(Health->GetPercent(), 0.75f));
		PreviousPlayer->GetAbilitySystemComponent()->SetNumericAttributeBase(UAesirAttributeSet::GetHealthAttribute(), 25.0f);
		Test->TestTrue(TEXT("The former pawn cannot overwrite the current player's HUD"), FMath::IsNearlyEqual(Health->GetPercent(), 0.75f));
		Test->TestFalse(TEXT("Possession change releases the former pawn's HUD delegate"), PreviousPlayer->GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(UAesirAttributeSet::GetHealthAttribute()).IsBoundToObject(HUD));
		const int32 ObserverCount = ASC->GetGameplayAttributeValueChangeDelegate(UAesirAttributeSet::GetHealthAttribute()).RemoveAll(HUD);
		Test->TestEqual(TEXT("The current pawn has one HUD health observer after remove/readd and rebind"), ObserverCount, 1);
		return true;
	}
	if (Mode == TEXT("BossTarget"))
	{
		if (Stage == 0)
		{
			Test->TestFalse(TEXT("There is no boss panel without a real boss target"), BossPanel->IsVisible());
			Boss = Fixture->CreateBoss();
			if (!Test->TestNotNull(TEXT("A real enemy with its own GAS health is available"), Boss))
			{
				return true;
			}
			HUD->SetBossTarget(Boss, FText::FromString(TEXT("Test Warden")));
			Test->TestEqual(TEXT("Encounter pin selects the supplied boss"), HUD->GetDisplayedBoss(), static_cast<AActor*>(Boss));
			Test->TestTrue(TEXT("Pinning a living boss reveals the boss panel"), BossPanel->IsVisible());
			Test->TestTrue(TEXT("Boss display name is readable"), BossName->GetText().EqualTo(FText::FromString(TEXT("Test Warden"))));
			Boss->GetAbilitySystemComponent()->SetNumericAttributeBase(UAesirAttributeSet::GetHealthAttribute(), 250.0f);
			Test->TestTrue(TEXT("The boss health bar follows actual enemy health"), FMath::IsNearlyEqual(BossHealth->GetPercent(), 0.5f));
			HUD->ClearBossTarget();
			HUD->RefreshHUDSources();
			Test->TestNull(TEXT("Clearing the encounter pin clears the displayed boss"), HUD->GetDisplayedBoss());
			Test->TestFalse(TEXT("Clearing an untracked boss hides its panel"), BossPanel->IsVisible());
			HUD->SetBossTarget(Boss, FText::FromString(TEXT("Test Warden")));
			Boss->GetAbilitySystemComponent()->SetNumericAttributeBase(UAesirAttributeSet::GetHealthAttribute(), 0.0f);
			HUD->RefreshHUDSources();
			Test->TestFalse(TEXT("A dead boss does not leave an active boss panel"), BossPanel->IsVisible());
			Boss->Destroy();
			Boss = Fixture->CreateBoss();
			HUD->SetBossTarget(Boss, FText::FromString(TEXT("Replacement Warden")));
			Boss->Destroy();
			Stage = 1;
			return false;
		}
		Test->TestNull(TEXT("Automatic refresh drops a destroyed boss"), HUD->GetDisplayedBoss());
		Test->TestFalse(TEXT("A destroyed boss does not leave a stale health bar"), BossPanel->IsVisible());
		return true;
	}
	if (Mode == TEXT("ConfiguredAbility"))
	{
		UProgressBar* Cooldown = Widget<UProgressBar>(HUD, TEXT("Ability_0_Cooldown"));
		UTextBlock* Time = Widget<UTextBlock>(HUD, TEXT("Ability_0_Time"));
		if (!Test->TestNotNull(TEXT("A configured ability creates its actual cooldown overlay"), Cooldown) ||
			!Test->TestNotNull(TEXT("A configured ability creates a readable cooldown number"), Time))
		{
			return true;
		}
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Player.TestHeal"));
		if (Stage == 0)
		{
			Test->TestTrue(TEXT("A configured implemented ability reveals the ability panel"), Abilities->IsVisible());
			Test->TestEqual(TEXT("Only the configured ability receives a slot"), Abilities->GetChildrenCount(), 1);
			Test->TestTrue(TEXT("An available ability starts with no cooldown shade"), FMath::IsNearlyZero(Cooldown->GetPercent()));
			Test->TestTrue(TEXT("An available ability has no invented countdown"), Time->GetText().IsEmpty());
			UGameplayEffect* Effect = NewObject<UGameplayEffect>(ASC);
			Effect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
			Effect->DurationMagnitude = FScalableFloat(0.5f);
			FGameplayEffectSpec Spec(Effect, ASC->MakeEffectContext(), 1.0f);
			Spec.DynamicGrantedTags.AddTag(Tag);
			const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(Spec);
			Test->TestTrue(TEXT("The ability cooldown is backed by a real active duration effect"), Handle.IsValid());
			HUD->RefreshHUDSources();
			InitialCooldownRatio = Cooldown->GetPercent();
			Test->TestTrue(TEXT("Active cooldown produces a bounded visible shade"), InitialCooldownRatio > 0.0f && InitialCooldownRatio <= 1.0f);
			Test->TestFalse(TEXT("A real duration effect displays its remaining time"), Time->GetText().IsEmpty());
			Stage = 1;
			return false;
		}
		if (Stage == 1)
		{
			Test->TestTrue(TEXT("The countdown advances without reconstructing the widget"), Cooldown->GetPercent() < InitialCooldownRatio);
			Stage = 2;
		}
		if (ASC->HasMatchingGameplayTag(Tag))
		{
			return false;
		}
		HUD->RefreshHUDSources();
		Test->TestTrue(TEXT("Expiry clears the cooldown shade"), FMath::IsNearlyZero(Cooldown->GetPercent()));
		Test->TestTrue(TEXT("Expiry clears the cooldown number"), Time->GetText().IsEmpty());
		ASC->AddLooseGameplayTag(Tag);
		HUD->RefreshHUDSources();
		Test->TestTrue(TEXT("A blocking tag without a duration still shades the slot"), FMath::IsNearlyEqual(Cooldown->GetPercent(), 1.0f));
		Test->TestTrue(TEXT("An unknown expiry is shown honestly rather than as a fabricated number"), Time->GetText().EqualTo(FText::FromString(TEXT("…"))));
		ASC->RemoveLooseGameplayTag(Tag);
		HUD->RefreshHUDSources();
		Test->TestTrue(TEXT("Removing an indefinite blocking tag restores readiness"), FMath::IsNearlyZero(Cooldown->GetPercent()));
		return true;
	}
	return true;
}

} // namespace AesirCombatHUDTests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAesirCombatHUDSafeRatioTest, "Aesir.HUD.SafeRatio",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAesirCombatHUDSafeRatioTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Normal health ratio"), UAesirCombatHUDWidget::SafeRatio(25.0f, 100.0f), 0.25f);
	TestEqual(TEXT("Negative values stay empty"), UAesirCombatHUDWidget::SafeRatio(-25.0f, 100.0f), 0.0f);
	TestEqual(TEXT("Values over maximum stay full"), UAesirCombatHUDWidget::SafeRatio(125.0f, 100.0f), 1.0f);
	TestEqual(TEXT("Zero maximum is safe"), UAesirCombatHUDWidget::SafeRatio(25.0f, 0.0f), 0.0f);
	TestEqual(TEXT("Negative maximum is safe"), UAesirCombatHUDWidget::SafeRatio(25.0f, -100.0f), 0.0f);
	TestEqual(TEXT("A non-finite value cannot poison UMG"), UAesirCombatHUDWidget::SafeRatio(std::numeric_limits<float>::quiet_NaN(), 100.0f), 0.0f);
	TestEqual(TEXT("An infinite maximum cannot poison UMG"), UAesirCombatHUDWidget::SafeRatio(25.0f, std::numeric_limits<float>::infinity()), 0.0f);
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FAesirCombatHUDLiveDataTest, "Aesir.HUD.Live",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FAesirCombatHUDLiveDataTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	for (const TCHAR* Mode : { TEXT("PlayerVitals"), TEXT("HiddenAndRebind"), TEXT("BossTarget"), TEXT("ConfiguredAbility") })
	{
		OutBeautifiedNames.Add(Mode);
		OutTestCommands.Add(Mode);
	}
}

bool FAesirCombatHUDLiveDataTest::RunTest(const FString& Parameters)
{
	using namespace AesirCombatHUDTests;
	TSharedPtr<FHUDWorld> Fixture = MakeShared<FHUDWorld>();
	if (!TestNotNull(TEXT("A local controller is available"), Fixture->Controller) ||
		!TestNotNull(TEXT("A real native player with GAS attributes is available"), Fixture->Player) ||
		!Fixture->CreateHUD(this, Parameters == TEXT("ConfiguredAbility")))
	{
		return false;
	}
	AddCommand(new FExerciseHUD(this, Fixture, Parameters));
	return true;
}

#endif // WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
