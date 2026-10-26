// Copyright (c) 2026 YYchainsAw. All rights reserved.

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Abilities/AesirAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Characters/Player/AesirPlayerCharacter.h"
#include "Combat/AesirCombatStateComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Camera/AesirCameraGuideTrigger.h"
#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameplayEffect.h"
#include "Slate/SceneViewport.h"
#include "UObject/UnrealType.h"
#include "Widgets/SViewport.h"

namespace AesirCameraGuideTests
{

// Keep input counters, possession, overlaps and the actual player camera in the
// fixture. Nothing is loaded into Main or saved back to the editor's world.
class FGuideWorld
{
public:
	FGuideWorld()
	{
		GameInstance = NewObject<UGameInstance>(GEngine);
		GameInstance->AddToRoot();
		GameInstance->InitializeStandalone(MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("AesirCameraGuideTest")));
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
		Player = World->SpawnActor<AAesirPlayerCharacter>();
		if (!Controller || !Player)
		{
			return;
		}
		// An empty instant initialization effect keeps the native player's real
		// attribute components alive without requiring Blueprint combat content.
		if (FClassProperty* Effect = FindFProperty<FClassProperty>(Player->GetClass(), TEXT("DefaultAttributesEffect")))
		{
			Effect->SetObjectPropertyValue_InContainer(Player, UGameplayEffect::StaticClass());
		}
		Player->GetCharacterMovement()->DisableMovement();
		Controller->SetPlayer(LocalPlayer);
		Controller->Possess(Player);
		Player->DispatchBeginPlay();
		Controller->SetViewTarget(Player);
		Controller->SetControlRotation(FRotator::ZeroRotator);
		Player->GetCameraBoom()->bEnableCameraLag = false;
		Player->GetCameraBoom()->bEnableCameraRotationLag = false;
		Target = World->SpawnActor<AActor>();
		USceneComponent* TargetRoot = NewObject<USceneComponent>(Target);
		Target->AddInstanceComponent(TargetRoot);
		Target->SetRootComponent(TargetRoot);
		TargetRoot->RegisterComponent();
		Target->SetActorLocation(FVector(0.0f, 10000.0f, 96.0f));
	}

	~FGuideWorld()
	{
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

	AAesirCameraGuideTrigger* CreateGuide(UClass* GuideClass = AAesirCameraGuideTrigger::StaticClass())
	{
		AAesirCameraGuideTrigger* Guide = World->SpawnActor<AAesirCameraGuideTrigger>(
			GuideClass, FVector(1000.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
		if (Guide)
		{
			Guide->TargetActor = Target;
			Guide->TurnDuration = 0.2f;
			Guide->HoldDuration = 0.15f;
			Guide->ReturnDuration = 0.2f;
			Guide->DispatchBeginPlay();
		}
		return Guide;
	}

	UGameInstance* GameInstance = nullptr;
	UWorld* World = nullptr;
	UGameViewportClient* Viewport = nullptr;
	APlayerController* Controller = nullptr;
	AAesirPlayerCharacter* Player = nullptr;
	AActor* Target = nullptr;
	TSharedPtr<SViewport> SlateViewport;
	TSharedPtr<FSceneViewport> SceneViewport;
};

class FExerciseGuide final : public IAutomationLatentCommand
{
public:
	FExerciseGuide(FAutomationTestBase* InTest, TSharedPtr<FGuideWorld> InFixture, const FString& InMode)
		: Test(InTest), Fixture(MoveTemp(InFixture)), Mode(InMode)
	{
	}

	virtual bool Update() override;

private:
	bool VerifyReleased(const TCHAR* Context)
	{
		Test->TestFalse(FString::Printf(TEXT("%s releases its player camera ownership"), Context), Fixture->Player->IsCameraGuideActive());
		Test->TestFalse(FString::Printf(TEXT("%s releases its look-input counter"), Context), Fixture->Controller->IsLookInputIgnored());
		Test->TestFalse(FString::Printf(TEXT("%s leaves movement available"), Context), Fixture->Controller->IsMoveInputIgnored());
		Test->TestEqual(FString::Printf(TEXT("%s preserves the player view target"), Context), Fixture->Controller->GetViewTarget(), static_cast<AActor*>(Fixture->Player));
		return true;
	}

	FAutomationTestBase* Test;
	TSharedPtr<FGuideWorld> Fixture;
	FString Mode;
	AAesirCameraGuideTrigger* Guide = nullptr;
	FRotator InitialRotation = FRotator::ZeroRotator;
	float InitialArmLength = 0.0f;
	float InitialFOV = 0.0f;
	int32 Frames = 0;
	int32 Stage = 0;
	bool bSawHolding = false;
};

bool FExerciseGuide::Update()
{
	Fixture->World->Tick(LEVELTICK_All, 0.05f);
	if (++Frames > 100)
	{
		Test->AddError(FString::Printf(TEXT("Camera guide case %s did not finish within 100 ticks."), *Mode));
		return true;
	}
	if (Stage == 0)
	{
		UClass* GuideClass = AAesirCameraGuideTrigger::StaticClass();
		if (Mode == TEXT("BlueprintAsset"))
		{
			GuideClass = LoadClass<AAesirCameraGuideTrigger>(nullptr,
				TEXT("/Game/Aesir/Framework/Camera/BP_CameraGuideTrigger.BP_CameraGuideTrigger_C"));
			if (!Test->TestNotNull(TEXT("The saved guide Blueprint loads with its native parent"), GuideClass))
			{
				return true;
			}
		}
		Guide = Fixture->CreateGuide(GuideClass);
		if (!Test->TestNotNull(TEXT("The guide can be constructed"), Guide))
		{
			return true;
		}
		InitialRotation = Fixture->Controller->GetControlRotation();
		InitialArmLength = Fixture->Player->GetCameraBoom()->TargetArmLength;
		InitialFOV = Fixture->Player->GetFollowCamera()->FieldOfView;
		if (Mode == TEXT("Rejection"))
		{
			Guide->TargetActor.Reset();
			Test->TestFalse(TEXT("An unconfigured target does not start a guide"), Guide->TryStartGuide(Fixture->Player));
			Test->TestFalse(TEXT("A rejected target does not consume Trigger Once"), Guide->HasTriggered());
			Guide->TargetActor = Fixture->Target;
			ACameraActor* ExternalCamera = Fixture->World->SpawnActor<ACameraActor>();
			Fixture->Controller->SetViewTarget(ExternalCamera);
			Test->TestFalse(TEXT("A guide cannot take over an external cinematic camera"), Guide->TryStartGuide(Fixture->Player));
			Test->TestEqual(TEXT("A rejected guide preserves the external camera"), Fixture->Controller->GetViewTarget(), static_cast<AActor*>(ExternalCamera));
			Fixture->Controller->SetViewTarget(Fixture->Player);
			AAesirLevelFlowDirector* Flow = Fixture->World->SpawnActor<AAesirLevelFlowDirector>();
			Test->TestFalse(TEXT("A guide cannot bypass an uninitialized/menu level flow"), Guide->TryStartGuide(Fixture->Player));
			Test->TestFalse(TEXT("Flow rejection still does not consume Trigger Once"), Guide->HasTriggered());
			Flow->Destroy();
			Fixture->Controller->SetIgnoreLookInput(true);
			Test->TestFalse(TEXT("A guide refuses a look-input lock owned by another system"), Guide->TryStartGuide(Fixture->Player));
			Test->TestTrue(TEXT("Rejection preserves the other system's lock"), Fixture->Controller->IsLookInputIgnored());
			Fixture->Controller->SetIgnoreLookInput(false);
			return VerifyReleased(TEXT("Rejected guide"));
		}
		if (Mode == TEXT("Overlap"))
		{
			AActor* NonPlayer = Fixture->World->SpawnActor<AActor>();
			UBoxComponent* OtherBox = NewObject<UBoxComponent>(NonPlayer);
			NonPlayer->AddInstanceComponent(OtherBox);
			NonPlayer->SetRootComponent(OtherBox);
			OtherBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			OtherBox->SetCollisionObjectType(ECC_Pawn);
			OtherBox->SetCollisionResponseToAllChannels(ECR_Overlap);
			OtherBox->SetGenerateOverlapEvents(true);
			OtherBox->RegisterComponent();
			NonPlayer->SetActorLocation(Guide->GetActorLocation());
			OtherBox->UpdateOverlaps();
			Test->TestFalse(TEXT("A Pawn-channel prop cannot consume the player trigger"), Guide->HasTriggered());
			Test->TestFalse(TEXT("The guide remains idle for a non-player overlap"), Guide->IsGuiding());
			Fixture->Player->SetActorLocation(Guide->GetActorLocation());
			Fixture->Player->GetCapsuleComponent()->UpdateOverlaps();
			Stage = 1;
			return false;
		}
		if (Mode == TEXT("Return"))
		{
			Guide->bReturnToOriginalView = true;
			Guide->AimMode = EAesirCameraGuideAimMode::FixedDirection;
			Guide->DirectionArrow->SetWorldRotation(FRotator(-15.0f, 90.0f, 0.0f));
			Guide->TargetActor.Reset();
		}
		if (Mode == TEXT("BlueprintAsset"))
		{
			Guide->AimMode = EAesirCameraGuideAimMode::FixedDirection;
			Guide->DirectionArrow->SetWorldRotation(FRotator(-10.0f, 90.0f, 0.0f));
			Guide->TargetActor.Reset();
		}
		if (Mode == TEXT("Ownership"))
		{
			Guide->bLockMovement = true;
		}
		if (Mode == TEXT("ZeroDuration"))
		{
			Guide->TurnDuration = Guide->HoldDuration = Guide->ReturnDuration = 0.0f;
			Guide->bReturnToOriginalView = true;
		}
		if (!Test->TestTrue(TEXT("A configured guide starts for the local living player"), Guide->TryStartGuide(Fixture->Player)))
		{
			return true;
		}
		Test->TestTrue(TEXT("Starting immediately reserves camera ownership"), Fixture->Player->IsCameraGuideActive());
		Test->TestTrue(TEXT("Starting suppresses manual look input"), Fixture->Controller->IsLookInputIgnored());
		Test->TestEqual(TEXT("Starting retains the player's follow camera"), Fixture->Controller->GetViewTarget(), static_cast<AActor*>(Fixture->Player));
		if (Mode == TEXT("Ownership"))
		{
			AAesirCameraGuideTrigger* OtherGuide = Fixture->CreateGuide();
			Test->TestFalse(TEXT("Two overlapping guides cannot own one player"), OtherGuide->TryStartGuide(Fixture->Player));
			Test->TestFalse(TEXT("A rejected second guide remains available for later"), OtherGuide->HasTriggered());
			Fixture->Controller->SetIgnoreLookInput(true);
			Fixture->Controller->SetIgnoreMoveInput(true);
			Guide->CancelGuide();
			Test->TestFalse(TEXT("Cancellation retires the owner"), Fixture->Player->IsCameraGuideActive());
			Test->TestTrue(TEXT("Cancellation preserves another system's added look lock"), Fixture->Controller->IsLookInputIgnored());
			Test->TestTrue(TEXT("Cancellation preserves another system's added move lock"), Fixture->Controller->IsMoveInputIgnored());
			Fixture->Controller->SetIgnoreLookInput(false);
			Fixture->Controller->SetIgnoreMoveInput(false);
			Test->TestTrue(TEXT("The other guide can acquire the camera once locks are released"), OtherGuide->TryStartGuide(Fixture->Player));
			OtherGuide->CancelGuide();
			return VerifyReleased(TEXT("Mutually exclusive guides"));
		}
		Stage = 1;
		return false;
	}

	if (Mode == TEXT("TargetLost") && Stage == 1)
	{
		Fixture->Target->Destroy();
		Stage = 2;
		return false;
	}
	if (Mode == TEXT("Interrupted") && Stage == 1)
	{
		UAesirCombatStateComponent* Combat = Fixture->Player->FindComponentByClass<UAesirCombatStateComponent>();
		if (!Test->TestNotNull(TEXT("The real player has its combat state component"), Combat))
		{
			return true;
		}
		Combat->SetCombatState(EAesirCombatState::HitReact);
		Stage = 2;
		return false;
	}
	if (Mode == TEXT("Destroyed"))
	{
		Guide->Destroy();
		return VerifyReleased(TEXT("Destroyed trigger"));
	}
	if (Mode == TEXT("TargetLost") || Mode == TEXT("Interrupted"))
	{
		Test->TestFalse(TEXT("Losing a target or receiving a hit cancels the active guide"), Guide->IsGuiding());
		Test->TestTrue(TEXT("Cancellation records a diagnostic"), !Guide->GetLastFailure().IsEmpty());
		return VerifyReleased(TEXT("Interrupted guide"));
	}
	if (Mode == TEXT("Overlap") && Stage == 1)
	{
		Test->TestTrue(TEXT("Entering with the local player's capsule automatically starts the guide"), Guide->HasTriggered());
		Test->TestTrue(TEXT("The overlap guide is actively turning or holding"), Guide->IsGuiding());
		Stage = 2;
	}
	bSawHolding |= Guide->GetStage() == EAesirCameraGuideStage::Holding;
	if (Guide->IsGuiding())
	{
		Test->TestTrue(TEXT("Manual look stays locked until the guide finishes"), Fixture->Controller->IsLookInputIgnored());
		return false;
	}
	VerifyReleased(TEXT("Completed guide"));
	Test->TestEqual(TEXT("Guidance preserves the gameplay spring arm length"), Fixture->Player->GetCameraBoom()->TargetArmLength, InitialArmLength);
	Test->TestEqual(TEXT("Guidance preserves the gameplay FOV"), Fixture->Player->GetFollowCamera()->FieldOfView, InitialFOV);
	if (Mode == TEXT("Return") || Mode == TEXT("ZeroDuration"))
	{
		Test->TestTrue(TEXT("The optional return restores the original control rotation"), Fixture->Controller->GetControlRotation().Equals(InitialRotation, 0.1f));
	}
	else
	{
		Test->TestTrue(TEXT("Default completion leaves the player looking toward the revealed target"), FMath::Abs(FRotator::NormalizeAxis(Fixture->Controller->GetControlRotation().Yaw - InitialRotation.Yaw)) > 30.0f);
		Test->TestTrue(TEXT("The guide visits its timed hold stage"), bSawHolding);
	}
	if (Mode == TEXT("OnceAndReset"))
	{
		Test->TestFalse(TEXT("A completed Trigger Once cannot restart"), Guide->TryStartGuide(Fixture->Player));
		Guide->ResetTrigger();
		Test->TestFalse(TEXT("Reset makes the trigger available again"), Guide->HasTriggered());
		Test->TestTrue(TEXT("Reset permits another guide"), Guide->TryStartGuide(Fixture->Player));
		Guide->CancelGuide();
		Guide->bTriggerOnce = false;
		Test->TestTrue(TEXT("Repeatable guides can start after a previous guide was consumed"), Guide->TryStartGuide(Fixture->Player));
		Guide->CancelGuide();
		return VerifyReleased(TEXT("Reset and repeatable guide"));
	}
	return true;
}

} // namespace AesirCameraGuideTests

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FAesirCameraGuideBehaviorTest, "Aesir.CameraGuide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FAesirCameraGuideBehaviorTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	for (const TCHAR* Mode : { TEXT("BlueprintAsset"), TEXT("TurnAndHold"), TEXT("Return"), TEXT("Overlap"), TEXT("OnceAndReset"),
		TEXT("Ownership"), TEXT("TargetLost"), TEXT("Interrupted"), TEXT("Destroyed"), TEXT("Rejection"), TEXT("ZeroDuration") })
	{
		OutBeautifiedNames.Add(Mode);
		OutTestCommands.Add(Mode);
	}
}

bool FAesirCameraGuideBehaviorTest::RunTest(const FString& Parameters)
{
	using namespace AesirCameraGuideTests;
	TSharedPtr<FGuideWorld> Fixture = MakeShared<FGuideWorld>();
	if (!TestNotNull(TEXT("A local controller is available"), Fixture->Controller) ||
		!TestNotNull(TEXT("A living native player with its camera is available"), Fixture->Player))
	{
		return false;
	}
	AddCommand(new FExerciseGuide(this, Fixture, Parameters));
	return true;
}

#endif // WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
