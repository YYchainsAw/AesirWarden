// Copyright (c) 2026 YYchainsAw. All rights reserved.

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Flow/AesirCampfireStart.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Slate/SceneViewport.h"
#include "UI/AesirMainMenuWidget.h"
#include "UObject/UnrealType.h"
#include "Widgets/SViewport.h"

namespace AesirMenuTests
{

// A private game world and an offscreen viewport exercise real widget construction and
// controller input modes without opening Main, changing the editor map, or starting PIE.
class FMenuWorld
{
public:
	FMenuWorld()
	{
		GameInstance = NewObject<UGameInstance>(GEngine);
		GameInstance->AddToRoot();
		GameInstance->InitializeStandalone(MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("AesirMenuTest")));
		World = GameInstance->GetWorld();
		FWorldContext& Context = *GameInstance->GetWorldContext();
		Viewport = NewObject<UGameViewportClient>(GEngine);
		Context.GameViewport = Viewport;
		Viewport->Init(Context, GameInstance, false);
		SlateViewport = SNew(SViewport);
		SceneViewport = Viewport->CreateViewport(SlateViewport);
		World->InitializeActorsForPlay(FURL());

		FString PlayerError;
		ULocalPlayer* LocalPlayer = GameInstance->CreateLocalPlayer(0, PlayerError, false);
		if (LocalPlayer)
		{
			Controller = World->SpawnActor<APlayerController>();
			Pawn = World->SpawnActor<APawn>();
			if (Controller && Pawn)
			{
				Controller->SetPlayer(LocalPlayer);
				Controller->Possess(Pawn);
			}
		}
	}

	~FMenuWorld()
	{
		// Route EndPlay while the local controller and the viewport still exist.
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

	ACharacter* CreateAnimatedCharacter(FAutomationTestBase* Test)
	{
		UClass* PlayerClass = LoadClass<ACharacter>(nullptr,
			TEXT("/Game/Aesir/Characters/Player/BP_AesirPlayerCharacter.BP_AesirPlayerCharacter_C"));
		const ACharacter* PlayerDefaults = PlayerClass ? PlayerClass->GetDefaultObject<ACharacter>() : nullptr;
		if (!Test->TestNotNull(TEXT("The project player Blueprint is available"), PlayerDefaults))
		{
			return nullptr;
		}

		// Use the real player mesh/AnimBP on a plain Character. GAS initialization,
		// abilities and Blueprint input setup are outside this animation flow test.
		ACharacter* Character = World->SpawnActor<ACharacter>();
		if (!Test->TestNotNull(TEXT("An animated Character can be spawned"), Character))
		{
			return nullptr;
		}
		USkeletalMeshComponent* Mesh = Character->GetMesh();
		USkeletalMeshComponent* SourceMesh = PlayerDefaults->GetMesh();
		Mesh->SetSkeletalMeshAsset(SourceMesh->GetSkeletalMeshAsset());
		Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		Mesh->SetAnimInstanceClass(SourceMesh->GetAnimClass());
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		Character->GetCharacterMovement()->DisableMovement();
		Controller->Possess(Character);
		Pawn->Destroy();
		Pawn = Character;
		if (!Test->TestNotNull(TEXT("The real player AnimBP initializes on the Character"), Mesh->GetAnimInstance()))
		{
			return nullptr;
		}
		return Character;
	}

	void AdvanceAnimation(USkeletalMeshComponent* Mesh, const float DeltaSeconds)
	{
		// This standalone fixture has no rendered scene. Explicitly advance the real
		// montage and dispatch its queued completion delegate just as a mesh tick does.
		World->Tick(LEVELTICK_All, DeltaSeconds);
		Mesh->TickAnimation(DeltaSeconds, false);
		Mesh->RefreshBoneTransforms();
		if (UAnimInstance* Anim = Mesh->GetAnimInstance())
		{
			Anim->DispatchQueuedAnimEvents();
		}
	}

	UGameInstance* GameInstance = nullptr;
	UWorld* World = nullptr;
	UGameViewportClient* Viewport = nullptr;
	APlayerController* Controller = nullptr;
	APawn* Pawn = nullptr;
	TSharedPtr<SViewport> SlateViewport;
	TSharedPtr<FSceneViewport> SceneViewport;
};

template <typename TObject>
TObject* ReadObjectProperty(UObject* Object, const FName Name)
{
	const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Object->GetClass(), Name);
	return Property ? Cast<TObject>(Property->GetObjectPropertyValue_InContainer(Object)) : nullptr;
}

UClass* LoadMenuClass()
{
	return LoadClass<UAesirMainMenuWidget>(nullptr,
		TEXT("/Game/Aesir/UI/Menus/WBP_CampfireMainMenu.WBP_CampfireMainMenu_C"));
}

bool IsWaitingForStandUp(AAesirCampfireStart* Director)
{
	const FBoolProperty* Waiting = FindFProperty<FBoolProperty>(Director->GetClass(), TEXT("bWaitingForStandUpAnimation"));
	return Waiting && Waiting->GetPropertyValue_InContainer(Director);
}

// UWidget::RemoveFromParent is an existing parameterless UFUNCTION. Using it as a
// delegate observer makes the assertion about a delivered event, rather than IsBound.
FScriptDelegate RemovalObserver(UWidget* Widget)
{
	FScriptDelegate Observer;
	Observer.BindUFunction(Widget, TEXT("RemoveFromParent"));
	return Observer;
}

class FExerciseMenuFlow final : public IAutomationLatentCommand
{
public:
	FExerciseMenuFlow(FAutomationTestBase* InTest, TSharedPtr<FMenuWorld> InFixture,
		AAesirCampfireStart* InDirector, const bool bInDestroyFromMenu)
		: Test(InTest), Fixture(MoveTemp(InFixture)), Director(InDirector), bDestroyFromMenu(bInDestroyFromMenu)
	{
	}

	virtual bool Update() override
	{
		// One world tick per engine frame allows the director's real timers to run.
		Fixture->World->Tick(LEVELTICK_All, 0.1f);
		if (++Frames > 60)
		{
			Test->AddError(TEXT("The transient campfire flow did not reach its expected state within 60 ticks."));
			return true;
		}

		if (!bStarted)
		{
			if (Director->GetFlowState() != EAesirLevelFlowState::MainMenu)
			{
				return false;
			}

			Menu = ReadObjectProperty<UAesirMainMenuWidget>(Director, TEXT("MainMenuRef"));
			if (!Test->TestNotNull(TEXT("The director created its configured menu"), Menu))
			{
				return true;
			}
			Test->TestTrue(TEXT("The menu is attached to the viewport"), Menu->IsInViewport());
			Test->TestTrue(TEXT("The menu suppresses gameplay input, including ability keys"), Fixture->Viewport->IgnoreInput());
			Test->TestTrue(TEXT("The menu displays the cursor"), Fixture->Controller->bShowMouseCursor);
			Test->TestFalse(TEXT("Exploration cannot bypass Start"), Director->EnterExploration());
			Test->TestFalse(TEXT("Repeated menu entry is rejected"), Director->EnterMainMenu());

			if (bDestroyFromMenu)
			{
				Director->Destroy();
				Test->TestFalse(TEXT("Destroy removes the menu"), Menu->IsInViewport());
				Test->TestFalse(TEXT("Destroy restores gameplay input routing"), Fixture->Viewport->IgnoreInput());
				Test->TestTrue(TEXT("Destroy preserves another system's move lock"), Fixture->Controller->IsMoveInputIgnored());
				Test->TestTrue(TEXT("Destroy preserves another system's look lock"), Fixture->Controller->IsLookInputIgnored());
				Fixture->Controller->SetIgnoreMoveInput(false);
				Fixture->Controller->SetIgnoreLookInput(false);
				Test->TestFalse(TEXT("Destroy leaked no menu move lock"), Fixture->Controller->IsMoveInputIgnored());
				Test->TestFalse(TEXT("Destroy leaked no menu look lock"), Fixture->Controller->IsLookInputIgnored());
				return true;
			}

			UButton* Start = Cast<UButton>(Menu->GetWidgetFromName(TEXT("Btn_Start")));
			if (!Test->TestNotNull(TEXT("The displayed menu has a Start button"), Start))
			{
				return true;
			}
			Menu->SetStartAvailability(false, FText::FromString(TEXT("Loading")));
			Start->OnClicked.Broadcast();
			Test->TestEqual(TEXT("An unavailable Start does not broadcast a start request"),
				Director->GetFlowState(), EAesirLevelFlowState::MainMenu);
			Menu->SetStartAvailability(true, FText::GetEmpty());
			Start->OnClicked.Broadcast();
			Test->TestEqual(TEXT("Clicking Start reaches the director through native dispatcher wiring"),
				Director->GetFlowState(), EAesirLevelFlowState::OpeningCinematic);
			Test->TestFalse(TEXT("Start removes the menu"), Menu->IsInViewport());
			Test->TestTrue(TEXT("Opening still blocks gameplay input"), Fixture->Viewport->IgnoreInput());
			Test->TestFalse(TEXT("Double Start is rejected"), Director->RequestStartGame());
			bStarted = true;
			return false;
		}

		if (Director->GetFlowState() != EAesirLevelFlowState::Exploration)
		{
			return false;
		}
		Test->TestFalse(TEXT("Exploration enables gameplay input"), Fixture->Viewport->IgnoreInput());
		Test->TestFalse(TEXT("Exploration hides the cursor"), Fixture->Controller->bShowMouseCursor);
		Test->TestEqual(TEXT("The camera returns to the possessed pawn"), Fixture->Controller->GetViewTarget(), static_cast<AActor*>(Fixture->Pawn));
		Test->TestFalse(TEXT("The startup streaming source is released"), Director->StartupStreamingSource->IsStreamingSourceEnabled());
		Test->TestFalse(TEXT("Duplicate exploration completion is rejected"), Director->EnterExploration());
		Test->TestFalse(TEXT("Exploration cannot reopen a stale startup menu"), Director->EnterMainMenu());
		Test->TestTrue(TEXT("Exploration preserves another system's move lock"), Fixture->Controller->IsMoveInputIgnored());
		Test->TestTrue(TEXT("Exploration preserves another system's look lock"), Fixture->Controller->IsLookInputIgnored());
		Fixture->Controller->SetIgnoreMoveInput(false);
		Fixture->Controller->SetIgnoreLookInput(false);
		Test->TestFalse(TEXT("No extra menu move lock remains"), Fixture->Controller->IsMoveInputIgnored());
		Test->TestFalse(TEXT("No extra menu look lock remains"), Fixture->Controller->IsLookInputIgnored());
		return true;
	}

private:
	FAutomationTestBase* Test;
	TSharedPtr<FMenuWorld> Fixture;
	AAesirCampfireStart* Director;
	UAesirMainMenuWidget* Menu = nullptr;
	bool bDestroyFromMenu;
	bool bStarted = false;
	int32 Frames = 0;
};

class FExerciseSitFlow final : public IAutomationLatentCommand
{
public:
	FExerciseSitFlow(FAutomationTestBase* InTest, TSharedPtr<FMenuWorld> InFixture,
		AAesirCampfireStart* InDirector, ACharacter* InCharacter, const FString& InCompletion)
		: Test(InTest), Fixture(MoveTemp(InFixture)), Director(InDirector), Character(InCharacter), Completion(InCompletion)
	{
		InitialTransform = Character->GetActorTransform();
	}

	virtual bool Update() override
	{
		USkeletalMeshComponent* Mesh = Character->GetMesh();
		UAnimInstance* Anim = Mesh->GetAnimInstance();
		Fixture->AdvanceAnimation(Mesh, 0.1f);
		if (++Frames > 160)
		{
			Test->AddError(TEXT("The sit/stand-up flow did not finish within 160 animation ticks."));
			return true;
		}

		if (Stage == 0)
		{
			if (Director->GetFlowState() != EAesirLevelFlowState::MainMenu)
			{
				return false;
			}
			UAnimMontage* Loop = Anim->GetCurrentActiveMontage();
			if (!Test->TestNotNull(TEXT("The menu plays a real player montage"), Loop))
			{
				return true;
			}
			Test->TestEqual(TEXT("The menu montage uses AS_SSH_Sit_Loop"),
				Loop->GetFirstAnimReference(), static_cast<UAnimSequenceBase*>(Director->MenuSitLoopAnimation.Get()));
			Test->TestEqual(TEXT("The existing Animation Blueprint remains active"),
				Mesh->GetAnimationMode(), EAnimationMode::AnimationBlueprint);
			Test->TestEqual(TEXT("Menu animation consumes root motion without moving the player"),
				Anim->RootMotionMode.GetValue(), ERootMotionMode::IgnoreRootMotion);
			Test->TestEqual(TEXT("The menu keeps animation ticking outside the camera view"),
				Mesh->VisibilityBasedAnimTickOption, EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones);
			LoopMontage = Loop;
			// Cross a complete source-animation boundary to catch a non-looping menu.
			Anim->Montage_SetPosition(Loop, FMath::Max(0.0f, Director->MenuSitLoopAnimation->GetPlayLength() - 0.05f));
			Stage = 1;
			return false;
		}

		if (Stage == 1)
		{
			Test->TestTrue(TEXT("The seated montage keeps playing after its first loop"), Anim->Montage_IsPlaying(LoopMontage.Get()));
			Test->TestTrue(TEXT("The seated loop leaves the player capsule in place"),
				Character->GetActorTransform().Equals(InitialTransform, 0.01f));
			if (!Test->TestTrue(TEXT("Start is accepted after the menu loop"), Director->RequestStartGame()))
			{
				return true;
			}
			Test->TestTrue(TEXT("Start waits for the stand-up montage"), IsWaitingForStandUp(Director));
			Test->TestEqual(TEXT("Start remains in the opening while the player rises"),
				Director->GetFlowState(), EAesirLevelFlowState::OpeningCinematic);
			Test->TestTrue(TEXT("Gameplay input stays blocked while the player rises"), Fixture->Viewport->IgnoreInput());
			Test->TestFalse(TEXT("A direct exploration request cannot bypass stand-up"), Director->EnterExploration());
			Test->TestFalse(TEXT("Repeated Start cannot restart stand-up"), Director->RequestStartGame());
			UAnimMontage* StandUp = Anim->GetCurrentActiveMontage();
			if (!Test->TestNotNull(TEXT("Start plays a stand-up montage"), StandUp))
			{
				return true;
			}
			Test->TestEqual(TEXT("The stand-up montage uses AS_SSH_Sit_End"),
				StandUp->GetFirstAnimReference(), static_cast<UAnimSequenceBase*>(Director->StandUpAnimation.Get()));
			StandUpMontage = StandUp;

			if (Completion == TEXT("Destroy"))
			{
				Director->Destroy();
				Test->TestFalse(TEXT("Destroy cancels a pending stand-up montage"), Anim->Montage_IsPlaying(StandUp));
				Test->TestEqual(TEXT("Destroy restores the previous root-motion mode"),
					Anim->RootMotionMode.GetValue(), ERootMotionMode::RootMotionFromEverything);
				Test->TestEqual(TEXT("Destroy restores the previous mesh animation tick policy"),
					Mesh->VisibilityBasedAnimTickOption, EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered);
				Test->TestFalse(TEXT("Destroy releases the startup input block"), Fixture->Viewport->IgnoreInput());
				return true;
			}
			if (Completion == TEXT("Skip"))
			{
				Test->TestTrue(TEXT("Skip is accepted while stand-up is pending"), Director->SkipOpeningCinematic());
				Test->TestFalse(TEXT("Skip clears the stand-up gate"), IsWaitingForStandUp(Director));
				Test->TestFalse(TEXT("Skip stops its owned stand-up montage"), Anim->Montage_IsPlaying(StandUp));
			}
			Stage = 2;
			return false;
		}

		if (Director->GetFlowState() != EAesirLevelFlowState::Exploration)
		{
			if (IsWaitingForStandUp(Director))
			{
				Test->TestTrue(TEXT("The player remains input locked while the stand-up gate is pending"), Fixture->Viewport->IgnoreInput());
				Test->TestFalse(TEXT("Exploration remains guarded throughout stand-up"), Director->EnterExploration());
			}
			return false;
		}
		Test->TestFalse(TEXT("Exploration has no remaining stand-up wait"), IsWaitingForStandUp(Director));
		Test->TestFalse(TEXT("Stand-up no longer plays after gameplay begins"), Anim->Montage_IsPlaying(StandUpMontage.Get()));
		Test->TestEqual(TEXT("Gameplay restores the AnimBP's previous root-motion mode"),
			Anim->RootMotionMode.GetValue(), ERootMotionMode::RootMotionFromEverything);
		Test->TestEqual(TEXT("Gameplay restores the previous mesh animation tick policy"),
			Mesh->VisibilityBasedAnimTickOption, EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered);
		Test->TestTrue(TEXT("Menu and stand-up do not displace the player capsule"),
			Character->GetActorTransform().Equals(InitialTransform, 0.01f));
		Test->TestFalse(TEXT("Gameplay input is enabled after camera handoff"), Fixture->Viewport->IgnoreInput());
		Test->TestEqual(TEXT("Gameplay camera returns to the same animated player"),
			Fixture->Controller->GetViewTarget(), static_cast<AActor*>(Character));
		return true;
	}

private:
	FAutomationTestBase* Test;
	TSharedPtr<FMenuWorld> Fixture;
	AAesirCampfireStart* Director;
	ACharacter* Character;
	FString Completion;
	FTransform InitialTransform;
	TWeakObjectPtr<UAnimMontage> LoopMontage;
	TWeakObjectPtr<UAnimMontage> StandUpMontage;
	int32 Stage = 0;
	int32 Frames = 0;
};

} // namespace AesirMenuTests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAesirMenuWidgetActionsTest, "Aesir.Menu.WidgetActions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAesirMenuWidgetActionsTest::RunTest(const FString& Parameters)
{
	using namespace AesirMenuTests;
	FMenuWorld Fixture;
	if (!TestNotNull(TEXT("A local controller is available"), Fixture.Controller))
	{
		return false;
	}
	UClass* BlueprintClass = LoadMenuClass();
	if (!TestNotNull(TEXT("The editable menu Blueprint is present"), BlueprintClass))
	{
		return false;
	}
	for (UClass* MenuClass : { UAesirMainMenuWidget::StaticClass(), BlueprintClass })
	{
		AddInfo(FString::Printf(TEXT("Checking menu class %s"), *MenuClass->GetPathName()));
		UAesirMainMenuWidget* Menu = CreateWidget<UAesirMainMenuWidget>(Fixture.Controller, MenuClass);
		if (!TestNotNull(TEXT("The menu can be instantiated"), Menu))
		{
			continue;
		}
		TSharedRef<SWidget> SlateMenu = Menu->TakeWidget();
		(void)SlateMenu; // Retain SObjectWidget until this iteration finishes.
		UButton* Start = Cast<UButton>(Menu->GetWidgetFromName(TEXT("Btn_Start")));
		UButton* Quit = Cast<UButton>(Menu->GetWidgetFromName(TEXT("Btn_Quit")));
		UTextBlock* Status = Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("Txt_Status")));
		if (!TestNotNull(TEXT("Start binding target exists"), Start) ||
			!TestNotNull(TEXT("Quit binding target exists"), Quit) ||
			!TestNotNull(TEXT("Status binding target exists"), Status))
		{
			continue;
		}
		TestFalse(TEXT("Start is disabled before preparation"), Start->GetIsEnabled());
		if (UTextBlock* Title = Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("Txt_Title"))))
		{
			TestNotNull(TEXT("Menu text uses a serializable font asset"), Title->GetFont().FontObject.Get());
		}
		UWidgetTree* Tree = ReadObjectProperty<UWidgetTree>(Menu, TEXT("WidgetTree"));
		if (TestNotNull(TEXT("The menu owns a widget tree"), Tree))
		{
			UWidget* AuthoredRoot = Tree->RootWidget;
			UAesirMainMenuWidget::BuildDefaultMenuTree(Tree);
			TestEqual(TEXT("Fallback construction preserves an existing Designer tree"), Tree->RootWidget.Get(), AuthoredRoot);
		}

		UCanvasPanel* ObserverParent = NewObject<UCanvasPanel>(Menu);
		UTextBlock* StartObserver = NewObject<UTextBlock>(Menu);
		UTextBlock* QuitObserver = NewObject<UTextBlock>(Menu);
		ObserverParent->AddChildToCanvas(StartObserver);
		ObserverParent->AddChildToCanvas(QuitObserver);
		Menu->OnStartRequested.Add(RemovalObserver(StartObserver));
		Menu->OnQuitRequested.Add(RemovalObserver(QuitObserver));
		Start->OnClicked.Broadcast();
		TestNotNull(TEXT("A disabled Start delivers no event"), StartObserver->GetParent());
		Menu->SetStartAvailability(true, FText::GetEmpty());
		TestTrue(TEXT("Readiness enables the Start button"), Start->GetIsEnabled());
		TestEqual(TEXT("An empty status frees the status area"), Status->GetVisibility(), ESlateVisibility::Collapsed);
		Start->OnClicked.Broadcast();
		TestNull(TEXT("Start automatically broadcasts its action"), StartObserver->GetParent());
		Quit->OnClicked.Broadcast();
		TestNull(TEXT("Quit automatically broadcasts its action"), QuitObserver->GetParent());
		const FText LoadingText = FText::FromString(TEXT("Preparing the starting area"));
		Menu->SetStartAvailability(false, LoadingText);
		TestFalse(TEXT("Preparation can disable Start again"), Start->GetIsEnabled());
		TestTrue(TEXT("Preparation displays its actual status"), Status->GetText().EqualTo(LoadingText));
		TestEqual(TEXT("A nonempty status is visible"), Status->GetVisibility(), ESlateVisibility::HitTestInvisible);
	}
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FAesirCampfireMenuFlowTest, "Aesir.Menu.Flow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FAesirCampfireMenuFlowTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TEXT("NativeFallback"));
	OutTestCommands.Add(TEXT("Native"));
	OutBeautifiedNames.Add(TEXT("BlueprintScaffold"));
	OutTestCommands.Add(TEXT("Blueprint"));
	OutBeautifiedNames.Add(TEXT("DestroyDuringMenu"));
	OutTestCommands.Add(TEXT("Destroy"));
}

bool FAesirCampfireMenuFlowTest::RunTest(const FString& Parameters)
{
	using namespace AesirMenuTests;
	if (!TestTrue(TEXT("The editor initialized Slate"), FSlateApplication::IsInitialized()))
	{
		return false;
	}
	TSharedPtr<FMenuWorld> Fixture = MakeShared<FMenuWorld>();
	if (!TestNotNull(TEXT("The test has a local controller"), Fixture->Controller) ||
		!TestNotNull(TEXT("The test has a possessed pawn"), Fixture->Pawn))
	{
		return false;
	}
	UClass* CampfireClass = AAesirCampfireStart::StaticClass();
	if (Parameters == TEXT("Blueprint"))
	{
		CampfireClass = LoadClass<AAesirCampfireStart>(nullptr,
			TEXT("/Game/Aesir/Framework/Flow/BP_AesirCampfireStart.BP_AesirCampfireStart_C"));
		if (!TestNotNull(TEXT("The editable campfire Blueprint is present"), CampfireClass))
		{
			return false;
		}
	}
	AAesirCampfireStart* Director = Fixture->World->SpawnActor<AAesirCampfireStart>(CampfireClass);
	if (!TestNotNull(TEXT("The campfire can be spawned"), Director))
	{
		return false;
	}
	// These existing tests deliberately use a plain APawn and isolate menu/input
	// behavior. The animated Character path is covered by Aesir.Menu.SitFlow.
	Director->bUsePlayerSitAnimations = false;
	if (Parameters == TEXT("Blueprint"))
	{
		TestEqual(TEXT("The campfire is configured to use the editable UI"),
			ReadObjectProperty<UClass>(Director, TEXT("MainMenuWidgetClass")), LoadMenuClass());
	}
	FFloatProperty* BlendTime = FindFProperty<FFloatProperty>(Director->GetClass(), TEXT("OpeningBlendTime"));
	if (!TestNotNull(TEXT("The camera blend is configurable"), BlendTime))
	{
		return false;
	}
	BlendTime->SetPropertyValue_InContainer(Director, 0.05f);
	TestFalse(TEXT("Start is rejected before initialization"), Director->RequestStartGame());
	TestFalse(TEXT("Exploration is rejected before initialization"), Director->EnterExploration());
	// An unrelated system already owns one lock. Menu entry and exit must preserve it.
	Fixture->Controller->SetIgnoreMoveInput(true);
	Fixture->Controller->SetIgnoreLookInput(true);
	Director->DispatchBeginPlay();
	AddCommand(new FExerciseMenuFlow(this, Fixture, Director, Parameters == TEXT("Destroy")));
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FAesirCampfireSitFlowTest, "Aesir.Menu.SitFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FAesirCampfireSitFlowTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	for (const FString& Completion : { FString(TEXT("Natural")), FString(TEXT("Skip")), FString(TEXT("Destroy")) })
	{
		OutBeautifiedNames.Add(Completion);
		OutTestCommands.Add(Completion);
	}
}

bool FAesirCampfireSitFlowTest::RunTest(const FString& Parameters)
{
	using namespace AesirMenuTests;
	TSharedPtr<FMenuWorld> Fixture = MakeShared<FMenuWorld>();
	if (!TestNotNull(TEXT("The test has a local controller"), Fixture->Controller))
	{
		return false;
	}
	ACharacter* Character = Fixture->CreateAnimatedCharacter(this);
	if (!Character)
	{
		return false;
	}
	AAesirCampfireStart* Director = Fixture->World->SpawnActor<AAesirCampfireStart>();
	if (!TestNotNull(TEXT("The animated campfire can be spawned"), Director))
	{
		return false;
	}
	TestTrue(TEXT("Menu sit animations are enabled by default"), Director->bUsePlayerSitAnimations);
	if (!TestNotNull(TEXT("The default seated loop asset exists"), Director->MenuSitLoopAnimation.LoadSynchronous()) ||
		!TestNotNull(TEXT("The default stand-up asset exists"), Director->StandUpAnimation.LoadSynchronous()))
	{
		return false;
	}
	FFloatProperty* BlendTime = FindFProperty<FFloatProperty>(Director->GetClass(), TEXT("OpeningBlendTime"));
	if (!TestNotNull(TEXT("The camera blend is configurable"), BlendTime))
	{
		return false;
	}
	BlendTime->SetPropertyValue_InContainer(Director, 0.05f);
	Character->GetMesh()->GetAnimInstance()->SetRootMotionMode(ERootMotionMode::RootMotionFromEverything);
	Director->DispatchBeginPlay();
	AddCommand(new FExerciseSitFlow(this, Fixture, Director, Character, Parameters));
	return true;
}

#endif // WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#if WITH_EDITOR && !UE_BUILD_SHIPPING

#include "AesirWarden.h"
#include "AI/Boss/AesirBossAIController.h"
#include "AI/Boss/AesirBossTelemetryComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Combat/AesirHealthComponent.h"
#include "Components/Button.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Flow/AesirCampfireStart.h"
#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UI/AesirMainMenuWidget.h"
#include "UnrealClient.h"
#include "UObject/UnrealType.h"

namespace AesirMainMenuSmoke
{

class FRun final : public TSharedFromThis<FRun>
{
public:
	explicit FRun(UWorld* InWorld, const bool bInQuitOnly)
		: World(InWorld), StartedAt(FPlatformTime::Seconds()), bQuitOnly(bInQuitOnly)
	{
		OutputDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("StartMenu"));
		MenuScreenshot = OutputDirectory / TEXT("MainMenu.png");
		StandUpScreenshot = OutputDirectory / TEXT("MainStandingUp.png");
		StartedScreenshot = OutputDirectory / TEXT("MainStarted.png");
	}

	void Begin()
	{
		IFileManager::Get().MakeDirectory(*OutputDirectory, true);
		IFileManager::Get().Delete(*MenuScreenshot, false, true);
		IFileManager::Get().Delete(*StandUpScreenshot, false, true);
		IFileManager::Get().Delete(*StartedScreenshot, false, true);
		World->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateSP(AsShared(), &FRun::Tick), 0.25f, true);
		UE_LOG(LogAesirWarden, Display, TEXT("START_MENU_MAIN_SMOKE_BEGIN: waiting for a ready campfire menu, then observing 10 seconds of idle gameplay."));
	}

	bool IsFinished() const { return bFinished; }

private:
	template <typename TObject>
	TObject* ReadObject(UObject* Object, const FName Name) const
	{
		const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Object->GetClass(), Name);
		return Property ? Cast<TObject>(Property->GetObjectPropertyValue_InContainer(Object)) : nullptr;
	}

	void Fail(const FString& Reason)
	{
		UE_LOG(LogAesirWarden, Error, TEXT("START_MENU_MAIN_SMOKE_FAIL: %s"), *Reason);
		Finish();
	}

	void Finish()
	{
		bFinished = true;
		if (UWorld* TestWorld = World.Get())
		{
			TestWorld->GetTimerManager().ClearTimer(Timer);
		}
		FPlatformMisc::RequestExit(false);
	}

	bool CheckIdleCombat()
	{
		for (TActorIterator<AAesirBossAIController> It(World.Get()); It; ++It)
		{
			BossesObserved.Add(*It);
			const UAesirBossTelemetryComponent* Telemetry = It->GetBossTelemetryComponent();
			if (!Telemetry || Telemetry->IsEpisodeActive())
			{
				Fail(FString::Printf(TEXT("Boss %s started telemetry before Start was clicked."), *It->GetName()));
				return false;
			}
		}
		if (Health.IsValid() && !FMath::IsNearlyEqual(Health->GetCurrentHealth(), InitialHealth))
		{
			Fail(FString::Printf(TEXT("Player health changed while the menu was open: %.2f -> %.2f."),
				InitialHealth, Health->GetCurrentHealth()));
			return false;
		}
		return true;
	}

	bool CollectHiddenHUD(AAesirLevelFlowDirector* Flow)
	{
		const FArrayProperty* Classes = FindFProperty<FArrayProperty>(Flow->GetClass(), TEXT("GameplayHUDWidgetClasses"));
		const FObjectPropertyBase* ClassProperty = Classes ? CastField<FObjectPropertyBase>(Classes->Inner) : nullptr;
		if (!Classes || !ClassProperty)
		{
			Fail(TEXT("The configured HUD classes could not be inspected."));
			return false;
		}
		FScriptArrayHelper ClassArray(Classes, Classes->ContainerPtrToValuePtr<void>(Flow));
		for (int32 Index = 0; Index < ClassArray.Num(); ++Index)
		{
			UClass* HUDClass = Cast<UClass>(ClassProperty->GetObjectPropertyValue(ClassArray.GetRawPtr(Index)));
			if (!HUDClass)
			{
				continue;
			}
			TArray<UUserWidget*> Widgets;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World.Get(), Widgets, HUDClass, true);
			for (UUserWidget* Widget : Widgets)
			{
				if (Widget->GetVisibility() != ESlateVisibility::Collapsed)
				{
					Fail(FString::Printf(TEXT("Gameplay HUD %s is visible over the main menu."), *Widget->GetName()));
					return false;
				}
				HiddenHUD.AddUnique(Widget);
			}
		}
		if (HiddenHUD.IsEmpty())
		{
			Fail(TEXT("No configured gameplay HUD was found; HUD restoration cannot be verified."));
			return false;
		}
		return true;
	}

	bool CheckPlayerAnimation(AAesirCampfireStart* Campfire, APlayerController* Controller, const bool bStandUp)
	{
		ACharacter* Character = Cast<ACharacter>(Controller->GetPawn());
		UAnimInstance* Anim = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
		UAnimMontage* Montage = Anim ? Anim->GetCurrentActiveMontage() : nullptr;
		UAnimSequence* ExpectedAnimation = bStandUp ? Campfire->StandUpAnimation.Get() : Campfire->MenuSitLoopAnimation.Get();
		if (!Montage || !ExpectedAnimation || Montage->GetFirstAnimReference() != ExpectedAnimation ||
			Anim->RootMotionMode != ERootMotionMode::IgnoreRootMotion)
		{
			Fail(FString::Printf(TEXT("The real player is not playing the expected %s montage with root motion suppressed."),
				bStandUp ? TEXT("stand-up") : TEXT("seated loop")));
			return false;
		}
		return true;
	}

	bool IsStandUpPending(AAesirCampfireStart* Campfire) const
	{
		const FBoolProperty* Waiting = FindFProperty<FBoolProperty>(Campfire->GetClass(), TEXT("bWaitingForStandUpAnimation"));
		return Waiting && Waiting->GetPropertyValue_InContainer(Campfire);
	}

	void Tick()
	{
		if (bFinished)
		{
			return;
		}
		const double Now = FPlatformTime::Seconds();
		if (!World.IsValid() || Now - StartedAt > 60.0)
		{
			Fail(TEXT("The menu smoke test timed out after 60 seconds or lost its game world."));
			return;
		}
		if (bQuitRequested)
		{
			if (Now - QuitRequestedAt > 5.0)
			{
				Fail(TEXT("Clicking Quit did not close the game within five seconds."));
			}
			return;
		}
		if (!Director.IsValid())
		{
			int32 DirectorCount = 0;
			for (TActorIterator<AAesirLevelFlowDirector> It(World.Get()); It; ++It)
			{
				Director = *It;
				++DirectorCount;
			}
			if (DirectorCount == 0)
			{
				return;
			}
			if (DirectorCount != 1)
			{
				Fail(FString::Printf(TEXT("Expected one startup flow director, found %d."), DirectorCount));
				return;
			}
		}

		AAesirLevelFlowDirector* Flow = Director.Get();
		APlayerController* Controller = UGameplayStatics::GetPlayerController(World.Get(), 0);
		if (!Controller || !Controller->GetPawn())
		{
			return;
		}
		UGameViewportClient* Viewport = World->GetGameViewport();
		if (!Viewport)
		{
			return;
		}
		AAesirCampfireStart* Campfire = Cast<AAesirCampfireStart>(Flow);
		if (!Campfire || !Campfire->bUsePlayerSitAnimations)
		{
			Fail(TEXT("The Main menu smoke test requires an animated campfire startup actor."));
			return;
		}

		if (!bStartClicked)
		{
			if (!CheckIdleCombat())
			{
				return;
			}
			if (Flow->GetFlowState() == EAesirLevelFlowState::Uninitialized)
			{
				return;
			}
			if (Flow->GetFlowState() != EAesirLevelFlowState::MainMenu)
			{
				Fail(TEXT("The level left MainMenu before the smoke test clicked Start."));
				return;
			}
			Menu = ReadObject<UAesirMainMenuWidget>(Flow, TEXT("MainMenuRef"));
			const FBoolProperty* StreamingReady = FindFProperty<FBoolProperty>(Flow->GetClass(), TEXT("bStartupStreamingReady"));
			if (!Menu.IsValid() || !StreamingReady || !StreamingReady->GetPropertyValue_InContainer(Flow))
			{
				return;
			}
			UButton* Start = Cast<UButton>(Menu->GetWidgetFromName(TEXT("Btn_Start")));
			if (!Start || !Start->GetIsEnabled())
			{
				return;
			}
			if (!Menu->IsInViewport() || !Viewport->IgnoreInput() ||
				!Controller->IsMoveInputIgnored() || !Controller->IsLookInputIgnored() || !Controller->bShowMouseCursor)
			{
				Fail(TEXT("A ready menu has inconsistent viewport, cursor, or gameplay input locks."));
				return;
			}
			if (!CheckPlayerAnimation(Campfire, Controller, false))
			{
				return;
			}
			if (bQuitOnly)
			{
				UButton* Quit = Cast<UButton>(Menu->GetWidgetFromName(TEXT("Btn_Quit")));
				if (!Quit || !Quit->GetIsEnabled())
				{
					Fail(TEXT("The prepared menu has no enabled Quit button."));
					return;
				}
				bQuitRequested = true;
				QuitRequestedAt = Now;
				UE_LOG(LogAesirWarden, Display, TEXT("START_MENU_QUIT_REQUESTED: dispatching the configured Quit button."));
				Quit->OnClicked.Broadcast();
				return;
			}
			if (MenuReadyAt == 0.0)
			{
				Health = Controller->GetPawn()->FindComponentByClass<UAesirHealthComponent>();
				if (!Health.IsValid() || Health->GetCurrentHealth() <= 0.0f)
				{
					Fail(TEXT("A living player health component is required for the menu damage check."));
					return;
				}
				InitialHealth = Health->GetCurrentHealth();
				if (!CollectHiddenHUD(Flow))
				{
					return;
				}
				MenuReadyAt = Now;
				UE_LOG(LogAesirWarden, Display, TEXT("START_MENU_MAIN_SMOKE_MENU_READY: player health %.2f; observing the menu for 10 seconds."), InitialHealth);
			}
			if (Now - MenuReadyAt < 10.0)
			{
				return;
			}
			if (BossesObserved.IsEmpty())
			{
				Fail(TEXT("No loaded BossController was observed; the menu combat gate cannot be verified."));
				return;
			}
			if (!bMenuScreenshotRequested)
			{
				FScreenshotRequest::RequestScreenshot(MenuScreenshot, true, false);
				bMenuScreenshotRequested = true;
				return;
			}
			// Let the renderer save the menu image before dispatching the real button event.
			if (!FPaths::FileExists(MenuScreenshot))
			{
				return;
			}
			Start->OnClicked.Broadcast();
			if (Flow->GetFlowState() != EAesirLevelFlowState::OpeningCinematic || !IsStandUpPending(Campfire) ||
				!CheckPlayerAnimation(Campfire, Controller, true) || Flow->EnterExploration())
			{
				Fail(TEXT("Start did not enter the guarded stand-up opening before exploration."));
				return;
			}
			bStartClicked = true;
			StartClickedAt = Now;
			UE_LOG(LogAesirWarden, Display, TEXT("START_MENU_MAIN_SMOKE_STAND_UP: real player entered Sit_End; direct exploration is blocked."));
			return;
		}

		if (Flow->GetFlowState() != EAesirLevelFlowState::Exploration)
		{
			if (IsStandUpPending(Campfire))
			{
				if (!CheckPlayerAnimation(Campfire, Controller, true) || !CheckIdleCombat())
				{
					return;
				}
				if (!Viewport->IgnoreInput() || Flow->EnterExploration())
				{
					Fail(TEXT("Stand-up released gameplay before its montage completed."));
					return;
				}
				if (!bStandUpScreenshotRequested && Now - StartClickedAt >= 0.5)
				{
					FScreenshotRequest::RequestScreenshot(StandUpScreenshot, true, false);
					bStandUpScreenshotRequested = true;
				}
			}
			return;
		}
		if (!bStartedScreenshotRequested)
		{
			if (Controller->IsMoveInputIgnored() || Controller->IsLookInputIgnored() ||
				Viewport->IgnoreInput() || Controller->bShowMouseCursor ||
				(Menu.IsValid() && Menu->IsInViewport()) || Controller->GetViewTarget() != Controller->GetPawn())
			{
				Fail(TEXT("Exploration did not restore the pawn camera, gameplay input, cursor, and menu visibility."));
				return;
			}
			for (const TWeakObjectPtr<UUserWidget>& Widget : HiddenHUD)
			{
				if (!Widget.IsValid() || !Widget->IsVisible())
				{
					Fail(TEXT("A gameplay HUD hidden by the menu was not restored for exploration."));
					return;
				}
			}
			FScreenshotRequest::RequestScreenshot(StartedScreenshot, true, false);
			bStartedScreenshotRequested = true;
			ExplorationAt = Now;
			return;
		}
		if (Now - ExplorationAt >= 2.0 && FPaths::FileExists(StartedScreenshot))
		{
			UE_LOG(LogAesirWarden, Display, TEXT("START_MENU_MAIN_SMOKE_PASS: %d BossController(s) stayed idle; health %.2f unchanged for 10 seconds; seated loop and guarded stand-up verified; gameplay input/HUD restored; screenshots: %s, %s and %s."),
				BossesObserved.Num(), InitialHealth, *MenuScreenshot, *StandUpScreenshot, *StartedScreenshot);
			Finish();
		}
	}

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AAesirLevelFlowDirector> Director;
	TWeakObjectPtr<UAesirMainMenuWidget> Menu;
	TWeakObjectPtr<UAesirHealthComponent> Health;
	TSet<TWeakObjectPtr<AAesirBossAIController>> BossesObserved;
	TArray<TWeakObjectPtr<UUserWidget>> HiddenHUD;
	FTimerHandle Timer;
	FString OutputDirectory;
	FString MenuScreenshot;
	FString StandUpScreenshot;
	FString StartedScreenshot;
	double StartedAt;
	double MenuReadyAt = 0.0;
	double StartClickedAt = 0.0;
	double ExplorationAt = 0.0;
	double QuitRequestedAt = 0.0;
	float InitialHealth = 0.0f;
	bool bQuitOnly = false;
	bool bQuitRequested = false;
	bool bStartClicked = false;
	bool bMenuScreenshotRequested = false;
	bool bStandUpScreenshotRequested = false;
	bool bStartedScreenshotRequested = false;
	bool bFinished = false;
};

TSharedPtr<FRun> ActiveRun;

FAutoConsoleCommandWithWorldAndArgs RunCommand(
	TEXT("Aesir.Menu.Smoke"),
	TEXT("Verify the startup menu, 10s idle combat gate, Start handoff, HUD and input. Save screenshots and exit. Optional argument 'quit' checks only the Quit button. Editor builds only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World || !World->IsGameWorld())
		{
			UE_LOG(LogAesirWarden, Error, TEXT("START_MENU_MAIN_SMOKE_FAIL: run this command in a game world, e.g. UnrealEditor project /Game/Main -game."));
			return;
		}
		if (ActiveRun.IsValid() && !ActiveRun->IsFinished())
		{
			UE_LOG(LogAesirWarden, Warning, TEXT("A startup menu smoke test is already running."));
			return;
		}
		const bool bQuitOnly = !Args.IsEmpty() && Args[0].Equals(TEXT("quit"), ESearchCase::IgnoreCase);
		ActiveRun = MakeShared<FRun>(World, bQuitOnly);
		ActiveRun->Begin();
	}));

} // namespace AesirMainMenuSmoke

#endif // WITH_EDITOR && !UE_BUILD_SHIPPING
