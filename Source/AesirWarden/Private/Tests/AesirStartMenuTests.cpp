// Copyright (c) 2026 YYchainsAw. All rights reserved.

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/ChildActorComponent.h"
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
#include "Framework/Flow/AesirOpeningCameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Misc/ScopeExit.h"
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

	ACharacter* CreateCameraCharacter(FAutomationTestBase* Test)
	{
		ACharacter* Character = CreateAnimatedCharacter(Test);
		if (!Character)
		{
			return nullptr;
		}
		// The real mesh/AnimBP is paired with the same 400 cm SpringArm camera
		// arrangement as the player, without unrelated GAS/AI initialization.
		USpringArmComponent* Arm = NewObject<USpringArmComponent>(Character, TEXT("TestCameraBoom"));
		Character->AddInstanceComponent(Arm);
		Arm->SetupAttachment(Character->GetRootComponent());
		Arm->TargetArmLength = 400.0f;
		Arm->bUsePawnControlRotation = true;
		Arm->bDoCollisionTest = false; // The transition itself owns path collision tests.
		Arm->RegisterComponent();
		UCameraComponent* Camera = NewObject<UCameraComponent>(Character, TEXT("TestFollowCamera"));
		Character->AddInstanceComponent(Camera);
		Camera->SetupAttachment(Arm, USpringArmComponent::SocketName);
		Camera->RegisterComponent();
		Camera->Activate(true);
		Character->DispatchBeginPlay();
		Arm->TickComponent(0.1f, LEVELTICK_All, &Arm->PrimaryComponentTick);
		return Character;
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

class FExerciseOpeningCamera final : public IAutomationLatentCommand
{
public:
	FExerciseOpeningCamera(FAutomationTestBase* InTest, TSharedPtr<FMenuWorld> InFixture,
		AAesirCampfireStart* InDirector, ACharacter* InCharacter, const FString& InMode)
		: Test(InTest), Fixture(MoveTemp(InFixture)), Director(InDirector), Character(InCharacter), Mode(InMode)
	{
	}

	virtual bool Update() override
	{
		Fixture->AdvanceAnimation(Character->GetMesh(), 0.05f);
		if (++Frames > 320)
		{
			Test->AddError(TEXT("The synchronized opening camera did not finish within 320 ticks."));
			return true;
		}
		UAesirOpeningCameraComponent* OpeningCamera = Director->OpeningCamera;
		if (!bStarted)
		{
			if (Director->GetFlowState() != EAesirLevelFlowState::MainMenu)
			{
				return false;
			}
			if (Fixture->Controller->PlayerCameraManager)
			{
				Fixture->Controller->PlayerCameraManager->UpdateCamera(0.05f);
			}
			if (Mode == TEXT("Blocked"))
			{
				// A fully enclosed source/target cannot satisfy the path constraint.
				// Rejection must leave the existing menu and its input/AI gate intact.
				BlockCameraRoute();
				Test->TestFalse(TEXT("Start rejects a camera route enclosed by blocking geometry"), Director->RequestStartGame());
				Test->TestEqual(TEXT("A rejected camera route restores MainMenu"), Director->GetFlowState(), EAesirLevelFlowState::MainMenu);
				Test->TestFalse(TEXT("A rejected route leaves no active transition"), OpeningCamera->IsTransitionActive());
				Test->TestFalse(TEXT("A rejected route does not report camera completion"), OpeningCamera->WasTransitionCompleted());
				Test->TestTrue(TEXT("A rejected route keeps gameplay input blocked"), Fixture->Viewport->IgnoreInput());
				Test->TestTrue(TEXT("The rejected route provides a useful failure reason"), !OpeningCamera->GetLastFailure().IsEmpty());
				UAesirMainMenuWidget* Menu = ReadObjectProperty<UAesirMainMenuWidget>(Director, TEXT("MainMenuRef"));
				Test->TestTrue(TEXT("A rejected route keeps the menu available"), Menu && Menu->IsInViewport());
				return true;
			}
			if (!Test->TestTrue(TEXT("Start accepts a clear orbit camera path"), Director->RequestStartGame()))
			{
				return true;
			}
			Test->TestTrue(TEXT("The transition camera starts in the same call as stand-up"), OpeningCamera->IsTransitionActive());
			Test->TestTrue(TEXT("The real stand-up gate is active with the camera"), IsWaitingForStandUp(Director));
			ACameraActor* Camera = OpeningCamera->GetTransitionCamera();
			if (!Test->TestNotNull(TEXT("The opening owns a temporary camera"), Camera))
			{
				return true;
			}
			Test->TestEqual(TEXT("The opening immediately uses its temporary view target"), Fixture->Controller->GetViewTarget(), static_cast<AActor*>(Camera));
			StartLocation = Camera->GetActorLocation();
			StandUpMontage = ReadObjectProperty<UAnimMontage>(Director, TEXT("StandUpMontage"));
			bStarted = true;
			if (Mode == TEXT("Destroy"))
			{
				TWeakObjectPtr<ACameraActor> OwnedCamera(Camera);
				Director->Destroy();
				Test->TestFalse(TEXT("Destroy removes the temporary opening camera"), OwnedCamera.IsValid());
				Test->TestEqual(TEXT("Destroy returns the view to the possessed player"), Fixture->Controller->GetViewTarget(), static_cast<AActor*>(Character));
				Test->TestFalse(TEXT("Destroy releases its gameplay input block"), Fixture->Viewport->IgnoreInput());
				return true;
			}
			if (Mode == TEXT("Skip"))
			{
				Test->TestTrue(TEXT("Skip accepts an active simultaneous opening"), Director->SkipOpeningCinematic());
			}
			if (Mode == TEXT("Interrupted"))
			{
				// Stop the owned montage through the actual animation API so its
				// interruption delegate, rather than a direct flow call, recovers.
				UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance();
				Anim->Montage_Stop(0.0f, StandUpMontage.Get());
				Anim->DispatchQueuedAnimEvents();
				FailureInjectedFrame = Frames;
			}
			if (Mode == TEXT("DynamicBlocked"))
			{
				// The clear path was already accepted and is active. Insert a new
				// obstacle now to exercise runtime sweep failure independently.
				BlockCameraRoute();
				FailureInjectedFrame = Frames;
			}
			return false;
		}

		if (FailureInjectedFrame != INDEX_NONE)
		{
			if (Director->GetFlowState() != EAesirLevelFlowState::MainMenu)
			{
				if (Director->GetFlowState() == EAesirLevelFlowState::Exploration || Frames - FailureInjectedFrame > 2)
				{
					Test->AddError(TEXT("An interrupted or obstructed opening did not promptly recover to MainMenu."));
					return true;
				}
				return false;
			}
			Test->TestFalse(TEXT("Recovery does not leave an active transition"), OpeningCamera->IsTransitionActive());
			Test->TestFalse(TEXT("Recovery never reports a successful camera handoff"), OpeningCamera->WasTransitionCompleted());
			Test->TestNull(TEXT("Recovery destroys the temporary transition camera"), OpeningCamera->GetTransitionCamera());
			Test->TestFalse(TEXT("Recovery clears the interrupted stand-up gate"), IsWaitingForStandUp(Director));
			Test->TestTrue(TEXT("Recovery keeps gameplay input blocked"), Fixture->Viewport->IgnoreInput());
			Test->TestTrue(TEXT("Recovery keeps player movement and look locked"),
				Fixture->Controller->IsMoveInputIgnored() && Fixture->Controller->IsLookInputIgnored());
			UAesirMainMenuWidget* Menu = ReadObjectProperty<UAesirMainMenuWidget>(Director, TEXT("MainMenuRef"));
			Test->TestTrue(TEXT("Recovery restores the interactive menu"), Menu && Menu->IsInViewport());
			Test->TestEqual(TEXT("Recovery restores the authored menu camera"), Fixture->Controller->GetViewTarget(),
				static_cast<AActor*>(ReadObjectProperty<ACameraActor>(Director, TEXT("MenuCamera"))));
			UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance();
			UAnimMontage* Loop = ReadObjectProperty<UAnimMontage>(Director, TEXT("MenuLoopMontage"));
			Test->TestTrue(TEXT("Recovery resumes the owned seated loop"), Loop && Anim->Montage_IsPlaying(Loop));
			return true;
		}

		if (ACameraActor* Camera = OpeningCamera->GetTransitionCamera())
		{
			MaxTravel = FMath::Max(MaxTravel, FVector::Distance(StartLocation, Camera->GetActorLocation()));
		}
		if (OpeningCamera->IsTransitionActive())
		{
			Test->TestTrue(TEXT("Every observed camera sample preserves the configured player clearance"),
				OpeningCamera->GetMinimumObservedClearance() + 0.1f >= OpeningCamera->MinPlayerClearance);
			Test->TestTrue(TEXT("The transition holds gameplay input until its safe endpoint"), Fixture->Viewport->IgnoreInput());
		}
		UAnimInstance* Anim = Character->GetMesh()->GetAnimInstance();
		const FAnimMontageInstance* Instance = Anim && StandUpMontage.IsValid() ? Anim->GetInstanceForMontage(StandUpMontage.Get()) : nullptr;
		const bool bAnimationFinished = !Instance || (Instance->IsStopped() && Instance->GetBlend().IsComplete());
		if (bAnimationFinished && AnimationFinishedFrame == INDEX_NONE)
		{
			AnimationFinishedFrame = Frames;
		}
		if (Director->GetFlowState() != EAesirLevelFlowState::Exploration)
		{
			if (AnimationFinishedFrame != INDEX_NONE && Frames - AnimationFinishedFrame > 2)
			{
				Test->AddError(TEXT("The camera added a post-animation delay instead of completing with stand-up."));
				return true;
			}
			return false;
		}
		Test->TestTrue(TEXT("The opening camera reached and handed off its safe endpoint"), OpeningCamera->WasTransitionCompleted());
		Test->TestFalse(TEXT("The completed opening has no active transition"), OpeningCamera->IsTransitionActive());
		Test->TestTrue(TEXT("The endpoint preserves at least 100 cm from the player safety bounds"),
			OpeningCamera->GetMinimumObservedClearance() + 0.1f >= OpeningCamera->MinPlayerClearance);
		Test->TestTrue(TEXT("The final camera position matches the actual follow camera"), OpeningCamera->GetEndPositionError() <= 0.1f);
		Test->TestTrue(TEXT("The final camera rotation matches the actual follow camera"), OpeningCamera->GetEndRotationError() <= 0.1f);
		Test->TestTrue(TEXT("The final field of view matches the actual follow camera"), OpeningCamera->GetEndFOVError() <= 0.1f);
		Test->TestEqual(TEXT("The gameplay view returns to the same player"), Fixture->Controller->GetViewTarget(), static_cast<AActor*>(Character));
		Test->TestFalse(TEXT("The synchronized opening releases gameplay input"), Fixture->Viewport->IgnoreInput());
		if (Mode != TEXT("Skip"))
		{
			Test->TestTrue(TEXT("The camera actually moves while the player is rising"), MaxTravel > 5.0f);
		}
		return true;
	}

private:
	void BlockCameraRoute()
	{
		AActor* Obstacle = Fixture->World->SpawnActor<AActor>();
		UBoxComponent* Blocker = NewObject<UBoxComponent>(Obstacle);
		Obstacle->AddInstanceComponent(Blocker);
		Obstacle->SetRootComponent(Blocker);
		Blocker->SetBoxExtent(FVector(2000.0f));
		Blocker->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Blocker->SetCollisionResponseToAllChannels(ECR_Block);
		Blocker->RegisterComponent();
	}

	FAutomationTestBase* Test;
	TSharedPtr<FMenuWorld> Fixture;
	AAesirCampfireStart* Director;
	ACharacter* Character;
	FString Mode;
	FVector StartLocation = FVector::ZeroVector;
	TWeakObjectPtr<UAnimMontage> StandUpMontage;
	float MaxTravel = 0.0f;
	int32 Frames = 0;
	int32 AnimationFinishedFrame = INDEX_NONE;
	int32 FailureInjectedFrame = INDEX_NONE;
	bool bStarted = false;
};

} // namespace AesirMenuTests

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FAesirCampfireConstructionRecoveryTest, "Aesir.Menu.ConstructionRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FAesirCampfireConstructionRecoveryTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TEXT("Native"));
	OutTestCommands.Add(TEXT("Native"));
	OutBeautifiedNames.Add(TEXT("Blueprint"));
	OutTestCommands.Add(TEXT("Blueprint"));
}

bool FAesirCampfireConstructionRecoveryTest::RunTest(const FString& Parameters)
{
	using namespace AesirMenuTests;
	UClass* CampfireClass = AAesirCampfireStart::StaticClass();
	if (Parameters == TEXT("Blueprint"))
	{
		CampfireClass = LoadClass<AAesirCampfireStart>(nullptr,
			TEXT("/Game/Aesir/Framework/Flow/BP_AesirCampfireStart.BP_AesirCampfireStart_C"));
	}
	if (!TestNotNull(TEXT("The campfire class is available"), CampfireClass))
	{
		return false;
	}

	// Keep reconstruction away from Main and the editor's current world/undo stack.
	const UWorld::InitializationValues Initialization = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false)
		.ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
		MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("AesirConstructionTest")),
		GetTransientPackage(), true, ERHIFeatureLevel::Num, &Initialization);
	if (!TestNotNull(TEXT("A private editor world can be created"), World))
	{
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	};
	AAesirCampfireStart* Director = World->SpawnActor<AAesirCampfireStart>(CampfireClass);
	if (!TestNotNull(TEXT("The campfire can be constructed"), Director) ||
		!TestNotNull(TEXT("The campfire owns its camera child component"), Director->MenuCameraChild.Get()))
	{
		return false;
	}
	UChildActorComponent* CameraChild = Director->MenuCameraChild;
	ACameraActor* Camera = Cast<ACameraActor>(CameraChild->GetChildActor());
	UCameraComponent* CameraComponent = Camera ? Camera->GetCameraComponent() : nullptr;
	FObjectPropertyBase* CameraProperty = FindFProperty<FObjectPropertyBase>(ACameraActor::StaticClass(), TEXT("CameraComponent"));
	if (!TestNotNull(TEXT("The child camera is available"), Camera) ||
		!TestNotNull(TEXT("The child camera component is available"), CameraComponent) ||
		!TestNotNull(TEXT("The camera's serialized component reference can be inspected"), CameraProperty))
	{
		return false;
	}

	TestTrue(TEXT("The non-root camera component participates in editor Delete/Undo"),
		CameraComponent->HasAnyFlags(RF_Transactional));

	{
		// Delete/Undo can restore the child actor before its serialized component
		// reference. Reproduce that exact intermediate state deterministically.
		CameraProperty->SetObjectPropertyValue_InContainer(Camera, nullptr);
		ON_SCOPE_EXIT { CameraProperty->SetObjectPropertyValue_InContainer(Camera, CameraComponent); };
		TestTrue(TEXT("The restored child actor is valid while its camera component is absent"),
			IsValid(Camera) && Camera->GetCameraComponent() == nullptr);
		Director->OnConstruction(Director->GetActorTransform());
		TestNull(TEXT("An incomplete child camera is not published to menu flow"),
			ReadObjectProperty<ACameraActor>(Director, TEXT("MenuCamera")));
	}
	CameraComponent->SetFieldOfView(73.0f);
	Director->OnConstruction(Director->GetActorTransform());
	TestEqual(TEXT("Construction recovers the menu camera after its component is restored"),
		ReadObjectProperty<ACameraActor>(Director, TEXT("MenuCamera")), Camera);
	TestEqual(TEXT("The recovered camera receives the menu field of view"), CameraComponent->FieldOfView, 50.0f);

	{
		TGuardValue<TObjectPtr<UChildActorComponent>> MissingCameraChild(Director->MenuCameraChild, nullptr);
		Director->OnConstruction(Director->GetActorTransform());
		TestNull(TEXT("A temporarily absent child component clears the menu camera"),
			ReadObjectProperty<ACameraActor>(Director, TEXT("MenuCamera")));
	}
	Director->OnConstruction(Director->GetActorTransform());
	TestEqual(TEXT("Construction recovers after the child component reference is restored"),
		ReadObjectProperty<ACameraActor>(Director, TEXT("MenuCamera")), Camera);
	TestNotNull(TEXT("The recovered child camera still owns a usable component"), Camera->GetCameraComponent());
	return true;
}

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
		UButton* Settings = Cast<UButton>(Menu->GetWidgetFromName(TEXT("Btn_Settings")));
		UButton* Credits = Cast<UButton>(Menu->GetWidgetFromName(TEXT("Btn_Credits")));
		UButton* Back = Cast<UButton>(Menu->GetWidgetFromName(TEXT("Btn_Back")));
		UWidget* Actions = Menu->GetWidgetFromName(TEXT("MenuActions"));
		UWidget* Details = Menu->GetWidgetFromName(TEXT("MenuDetails"));
		UTextBlock* DetailsTitle = Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("Txt_DetailsTitle")));
		UTextBlock* DetailsBody = Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("Txt_DetailsBody")));
		UTextBlock* Status = Cast<UTextBlock>(Menu->GetWidgetFromName(TEXT("Txt_Status")));
		if (!TestNotNull(TEXT("Start binding target exists"), Start) ||
			!TestNotNull(TEXT("Quit binding target exists"), Quit) ||
			!TestNotNull(TEXT("Settings binding target exists"), Settings) ||
			!TestNotNull(TEXT("Credits binding target exists"), Credits) ||
			!TestNotNull(TEXT("Back binding target exists"), Back) ||
			!TestNotNull(TEXT("The main actions page exists"), Actions) ||
			!TestNotNull(TEXT("The details page exists"), Details) ||
			!TestNotNull(TEXT("The details page has a title"), DetailsTitle) ||
			!TestNotNull(TEXT("The details page has body text"), DetailsBody) ||
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
		UTextBlock* SettingsObserver = NewObject<UTextBlock>(Menu);
		UTextBlock* CreditsObserver = NewObject<UTextBlock>(Menu);
		ObserverParent->AddChildToCanvas(StartObserver);
		ObserverParent->AddChildToCanvas(QuitObserver);
		ObserverParent->AddChildToCanvas(SettingsObserver);
		ObserverParent->AddChildToCanvas(CreditsObserver);
		Menu->OnStartRequested.Add(RemovalObserver(StartObserver));
		Menu->OnQuitRequested.Add(RemovalObserver(QuitObserver));
		Menu->OnSettingsRequested.Add(RemovalObserver(SettingsObserver));
		Menu->OnCreditsRequested.Add(RemovalObserver(CreditsObserver));
		Start->OnClicked.Broadcast();
		TestNotNull(TEXT("A disabled Start delivers no event"), StartObserver->GetParent());
		Menu->SetStartAvailability(true, FText::GetEmpty());
		TestTrue(TEXT("Readiness enables the Start button"), Start->GetIsEnabled());
		TestEqual(TEXT("An empty status frees the status area"), Status->GetVisibility(), ESlateVisibility::Collapsed);
		Settings->OnClicked.Broadcast();
		TestNull(TEXT("Settings automatically broadcasts its action"), SettingsObserver->GetParent());
		TestEqual(TEXT("Settings hides the main actions"), Actions->GetVisibility(), ESlateVisibility::Collapsed);
		TestTrue(TEXT("Settings displays the details page"), Details->IsVisible());
		TestTrue(TEXT("Settings displays its own title"), DetailsTitle->GetText().EqualTo(FText::FromString(TEXT("设置"))));
		TestFalse(TEXT("Settings has explanatory content"), DetailsBody->GetText().IsEmpty());
		Start->OnClicked.Broadcast();
		TestNotNull(TEXT("A hidden Start cannot bypass the details page"), StartObserver->GetParent());
		Back->OnClicked.Broadcast();
		TestEqual(TEXT("Back closes the settings page"), Details->GetVisibility(), ESlateVisibility::Collapsed);
		TestTrue(TEXT("Back restores the main actions"), Actions->IsVisible());
		TestTrue(TEXT("Returning from settings preserves readiness"), Start->GetIsEnabled());
		Credits->OnClicked.Broadcast();
		TestNull(TEXT("Credits automatically broadcasts its action"), CreditsObserver->GetParent());
		TestTrue(TEXT("Credits displays its own title"), DetailsTitle->GetText().EqualTo(FText::FromString(TEXT("制作人员"))));
		TestTrue(TEXT("Credits displays the details page"), Details->IsVisible());
		Menu->SetStartAvailability(false, FText::GetEmpty());
		Back->OnClicked.Broadcast();
		TestEqual(TEXT("Back closes the credits page"), Details->GetVisibility(), ESlateVisibility::Collapsed);
		TestTrue(TEXT("Back from credits restores the main actions"), Actions->IsVisible());
		TestFalse(TEXT("Returning from credits respects updated readiness"), Start->GetIsEnabled());
		Menu->SetStartAvailability(true, FText::GetEmpty());
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
	// These fixtures deliberately isolate the sit/stand-up contract from cameras.
	Director->OpeningCamera->bEnabled = false;
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

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FAesirOpeningCameraFlowTest, "Aesir.Menu.CameraFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FAesirOpeningCameraFlowTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	for (const FString& Mode : { FString(TEXT("NaturalSlow")), FString(TEXT("NaturalFast")), FString(TEXT("Skip")), FString(TEXT("Destroy")),
		FString(TEXT("Blocked")), FString(TEXT("Interrupted")), FString(TEXT("DynamicBlocked")) })
	{
		OutBeautifiedNames.Add(Mode);
		OutTestCommands.Add(Mode);
	}
}

bool FAesirOpeningCameraFlowTest::RunTest(const FString& Parameters)
{
	using namespace AesirMenuTests;
	TSharedPtr<FMenuWorld> Fixture = MakeShared<FMenuWorld>();
	ACharacter* Character = Fixture->Controller ? Fixture->CreateCameraCharacter(this) : nullptr;
	if (!TestNotNull(TEXT("An animated character with a real follow camera is available"), Character))
	{
		return false;
	}
	AAesirCampfireStart* Director = Fixture->World->SpawnActor<AAesirCampfireStart>();
	if (!TestNotNull(TEXT("The campfire owns an opening camera component"), Director ? Director->OpeningCamera.Get() : nullptr))
	{
		return false;
	}
	Director->StandUpPlayRate = Parameters == TEXT("NaturalSlow") ? 0.5f : 1.5f;
	if (Parameters == TEXT("Blocked") || Parameters == TEXT("Interrupted") || Parameters == TEXT("DynamicBlocked"))
	{
		AddExpectedError(TEXT("Campfire: synchronized camera aborted:"), EAutomationExpectedErrorFlags::Contains, 1);
	}
	Director->DispatchBeginPlay();
	AddCommand(new FExerciseOpeningCamera(this, Fixture, Director, Character, Parameters));
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
#include "Camera/CameraActor.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Combat/AesirHealthComponent.h"
#include "Components/Button.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextBlock.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Flow/AesirCampfireStart.h"
#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "Framework/Flow/AesirOpeningCameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UI/AesirMainMenuWidget.h"
#include "UnrealClient.h"
#include "UObject/UnrealType.h"

namespace AesirMainMenuSmoke
{

class FRun final : public TSharedFromThis<FRun>
{
public:
	explicit FRun(UWorld* InWorld, const bool bInQuitOnly, const float InStandUpRate)
		: World(InWorld), StartedAt(FPlatformTime::Seconds()), RequestedStandUpRate(InStandUpRate), bQuitOnly(bInQuitOnly)
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
		UAnimMontage* Montage = ReadObject<UAnimMontage>(Campfire, bStandUp ? TEXT("StandUpMontage") : TEXT("MenuLoopMontage"));
		const FAnimMontageInstance* Instance = Anim && Montage ? Anim->GetInstanceForMontage(Montage) : nullptr;
		UAnimSequence* ExpectedAnimation = bStandUp ? Campfire->StandUpAnimation.Get() : Campfire->MenuSitLoopAnimation.Get();
		// Active-montage queries exclude blend-out, before the stand-up end delegate releases gameplay.
		const bool bPlaying = Instance && Instance->IsValid() && Instance->IsActive() && Instance->IsPlaying();
		const bool bFinishingStandUp = bStandUp && Instance && Instance->IsValid() &&
			Instance->IsStopped() && !Instance->GetBlend().IsComplete();
		if (!Anim || !Montage || !ExpectedAnimation || Montage->GetFirstAnimReference() != ExpectedAnimation ||
			(!bPlaying && !bFinishingStandUp) ||
			Anim->RootMotionMode != ERootMotionMode::IgnoreRootMotion)
		{
			Fail(FString::Printf(TEXT("The real player is not playing or finishing the expected %s montage with root motion suppressed: montage=%s, animation=%s, expected=%s, instance=%d, playing=%d, stopped=%d, blendComplete=%d, rootMotionMode=%d."),
				bStandUp ? TEXT("stand-up") : TEXT("seated loop"), *GetNameSafe(Montage),
				*GetNameSafe(Montage ? Montage->GetFirstAnimReference() : nullptr), *GetNameSafe(ExpectedAnimation),
				Instance ? Instance->GetInstanceID() : INDEX_NONE, Instance && Instance->IsPlaying(),
				Instance && Instance->IsStopped(), Instance && Instance->GetBlend().IsComplete(),
				Anim ? static_cast<int32>(Anim->RootMotionMode.GetValue()) : INDEX_NONE));
			return false;
		}
		return true;
	}

	bool IsStandUpPending(AAesirCampfireStart* Campfire) const
	{
		const FBoolProperty* Waiting = FindFProperty<FBoolProperty>(Campfire->GetClass(), TEXT("bWaitingForStandUpAnimation"));
		return Waiting && Waiting->GetPropertyValue_InContainer(Campfire);
	}

	bool CheckMenuPresentation()
	{
		// Inspect geometry after the real viewport has painted, so DPI and the saved
		// Designer tree participate in these user-facing layout checks.
		UWidget* Root = Menu->GetWidgetFromName(TEXT("MenuRoot"));
		UWidget* Logo = Menu->GetWidgetFromName(TEXT("Img_Logo"));
		if (!Root || !Logo)
		{
			Fail(TEXT("The displayed menu is missing its canvas or title logo."));
			return false;
		}
		const FGeometry& RootGeometry = Root->GetCachedGeometry();
		const FVector2D CanvasSize = RootGeometry.GetLocalSize();
		const FGeometry& LogoGeometry = Logo->GetCachedGeometry();
		const FVector2D LogoCenter = RootGeometry.AbsoluteToLocal(
			LogoGeometry.LocalToAbsolute(LogoGeometry.GetLocalSize() * 0.5f));
		if (CanvasSize.X <= 0.0f || CanvasSize.Y <= 0.0f ||
			!FMath::IsNearlyEqual(LogoCenter.X / CanvasSize.X, 0.618034, 0.01) ||
			!FMath::IsNearlyEqual(LogoCenter.Y / CanvasSize.Y, 0.381966, 0.01))
		{
			Fail(FString::Printf(TEXT("The rendered logo is not centered on the requested golden-section intersection: canvas=(%.1f, %.1f), logo=(%.1f, %.1f)."),
				CanvasSize.X, CanvasSize.Y, LogoCenter.X, LogoCenter.Y));
			return false;
		}
		float PreviousBottom = RootGeometry.AbsoluteToLocal(
			LogoGeometry.LocalToAbsolute(FVector2D(0.0f, LogoGeometry.GetLocalSize().Y))).Y;
		const TCHAR* ButtonNames[] = { TEXT("Btn_Start"), TEXT("Btn_Settings"), TEXT("Btn_Credits"), TEXT("Btn_Quit") };
		const TCHAR* LabelNames[] = { TEXT("Txt_Start"), TEXT("Txt_Settings"), TEXT("Txt_Credits"), TEXT("Txt_Quit") };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(ButtonNames); ++Index)
		{
			UButton* Button = Cast<UButton>(Menu->GetWidgetFromName(ButtonNames[Index]));
			UTextBlock* Label = Cast<UTextBlock>(Menu->GetWidgetFromName(LabelNames[Index]));
			if (!Button || !Label || Label->GetFont().Size != 55)
			{
				Fail(FString::Printf(TEXT("Action %s is missing or its text is not size 55."), ButtonNames[Index]));
				return false;
			}
			const FGeometry& ButtonGeometry = Button->GetCachedGeometry();
			const FVector2D TopLeft = RootGeometry.AbsoluteToLocal(ButtonGeometry.LocalToAbsolute(FVector2D::ZeroVector));
			const FVector2D BottomRight = RootGeometry.AbsoluteToLocal(ButtonGeometry.LocalToAbsolute(ButtonGeometry.GetLocalSize()));
			if (!Button->IsVisible() || !FMath::IsNearlyEqual((TopLeft.X + BottomRight.X) / (2.0 * CanvasSize.X), 0.618034, 0.01) ||
				TopLeft.X < 0.0f || BottomRight.X > CanvasSize.X || BottomRight.Y > CanvasSize.Y ||
				TopLeft.Y < PreviousBottom || BottomRight.Y <= TopLeft.Y)
			{
				Fail(FString::Printf(TEXT("Action %s is not fully visible below the logo on the shared menu axis."), ButtonNames[Index]));
				return false;
			}
			PreviousBottom = BottomRight.Y;
			const FButtonStyle& Style = Button->GetStyle();
			for (const FSlateBrush* Brush : { &Style.Normal, &Style.Hovered, &Style.Pressed, &Style.Disabled })
			{
				const UTexture2D* Texture = Cast<UTexture2D>(Brush->GetResourceObject());
				if (Brush->DrawAs != ESlateBrushDrawType::NoDrawType &&
					Brush->TintColor.GetSpecifiedColor().A > KINDA_SMALL_NUMBER &&
					Button->GetBackgroundColor().A > KINDA_SMALL_NUMBER && (!Texture || !Texture->HasAlphaChannel()))
				{
					Fail(FString::Printf(TEXT("Action %s has an opaque color or texture brush instead of a transparent border."), ButtonNames[Index]));
					return false;
				}
			}
		}
		UE_LOG(LogAesirWarden, Display, TEXT("START_MENU_PRESENTATION_PASS: rendered logo center=(%.4f, %.4f); four ordered actions share x=0.618, remain on screen, use size 55 text and alpha-capable border brushes."),
			LogoCenter.X / CanvasSize.X, LogoCenter.Y / CanvasSize.Y);
		return true;
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
		if (!bRateApplied)
		{
			if (RequestedStandUpRate > 0.0f)
			{
				Campfire->StandUpPlayRate = FMath::Clamp(RequestedStandUpRate, 0.1f, 3.0f);
			}
			bRateApplied = true;
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
			if (!CheckMenuPresentation())
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
			UAesirOpeningCameraComponent* Camera = Campfire->OpeningCamera;
			if (!Camera || !Camera->bEnabled || !Camera->IsTransitionActive() ||
				!Camera->GetTransitionCamera() || Controller->GetViewTarget() != Camera->GetTransitionCamera())
			{
				Fail(TEXT("Start did not immediately activate the synchronized opening camera."));
				return;
			}
			CameraStartLocation = Camera->GetTransitionCamera()->GetActorLocation();
			bStartClicked = true;
			StartClickedAt = Now;
			UE_LOG(LogAesirWarden, Display, TEXT("START_MENU_MAIN_SMOKE_STAND_UP: real player entered Sit_End; direct exploration is blocked."));
			return;
		}
		UAesirOpeningCameraComponent* OpeningCamera = Campfire->OpeningCamera;
		if (!OpeningCamera)
		{
			Fail(TEXT("The campfire lost its opening camera component."));
			return;
		}
		if (ACameraActor* Camera = OpeningCamera->GetTransitionCamera())
		{
			MaxCameraTravel = FMath::Max(MaxCameraTravel, FVector::Distance(CameraStartLocation, Camera->GetActorLocation()));
		}
		if (OpeningCamera->IsTransitionActive() &&
			OpeningCamera->GetMinimumObservedClearance() + 0.1f < OpeningCamera->MinPlayerClearance)
		{
			Fail(TEXT("The transition camera entered the player's one-metre safety clearance."));
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
				if (!bStandUpScreenshotRequested && OpeningCamera->GetProgress() >= 0.3f)
				{
					FScreenshotRequest::RequestScreenshot(StandUpScreenshot, true, false);
					bStandUpScreenshotRequested = true;
				}
			}
			return;
		}
		if (!bStartedScreenshotRequested)
		{
			if (!OpeningCamera->WasTransitionCompleted() || OpeningCamera->IsTransitionActive() ||
				OpeningCamera->GetMinimumObservedClearance() + 0.1f < OpeningCamera->MinPlayerClearance ||
				OpeningCamera->GetEndPositionError() > 0.1f || OpeningCamera->GetEndRotationError() > 0.1f ||
				OpeningCamera->GetEndFOVError() > 0.1f || MaxCameraTravel < 5.0f)
			{
				Fail(FString::Printf(TEXT("The opening camera did not complete a safe, moving, matched handoff: completed=%d, active=%d, clearance=%.2f, travel=%.2f, endpointError=%.3f cm/%.3f deg, FOV error=%.3f deg."),
					OpeningCamera->WasTransitionCompleted(), OpeningCamera->IsTransitionActive(), OpeningCamera->GetMinimumObservedClearance(),
					MaxCameraTravel, OpeningCamera->GetEndPositionError(), OpeningCamera->GetEndRotationError(), OpeningCamera->GetEndFOVError()));
				return;
			}
			UE_LOG(LogAesirWarden, Display, TEXT("START_MENU_CAMERA_PASS: rate=%.2f; minimum body clearance=%.2f cm; camera travel=%.2f cm; endpoint error=%.3f cm/%.3f degrees; FOV error=%.3f degrees; simultaneous stand-up handoff in %.3f seconds."),
				Campfire->StandUpPlayRate, OpeningCamera->GetMinimumObservedClearance(), MaxCameraTravel,
				OpeningCamera->GetEndPositionError(), OpeningCamera->GetEndRotationError(), OpeningCamera->GetEndFOVError(), Now - StartClickedAt);
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
	float RequestedStandUpRate = 0.0f;
	FVector CameraStartLocation = FVector::ZeroVector;
	float MaxCameraTravel = 0.0f;
	bool bRateApplied = false;
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
	TEXT("Verify menu, 10s idle combat gate, synchronized camera/stand-up, HUD and input. Save screenshots and exit. Optional 'quit' checks Quit; 'rate=0.5' overrides stand-up speed for this run. Editor builds only."),
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
		float StandUpRate = 0.0f;
		for (const FString& Arg : Args)
		{
			FParse::Value(*Arg, TEXT("rate="), StandUpRate);
		}
		ActiveRun = MakeShared<FRun>(World, bQuitOnly, StandUpRate);
		ActiveRun->Begin();
	}));

} // namespace AesirMainMenuSmoke

#endif // WITH_EDITOR && !UE_BUILD_SHIPPING
