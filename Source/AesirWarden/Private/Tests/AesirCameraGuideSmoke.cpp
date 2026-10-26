// Copyright (c) 2026 YYchainsAw. All rights reserved.

#if WITH_EDITOR && !UE_BUILD_SHIPPING

#include "AesirWarden.h"
#include "Camera/CameraComponent.h"
#include "Characters/Player/AesirPlayerCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/Button.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Camera/AesirCameraGuideTrigger.h"
#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HighResScreenshot.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UI/AesirMainMenuWidget.h"
#include "UnrealClient.h"

namespace AesirCameraGuideSmoke
{
class FRun : public TSharedFromThis<FRun>
{
public:
	explicit FRun(UWorld* InWorld) : World(InWorld), StartedAt(FPlatformTime::Seconds()) {}
	void Begin()
	{
		Output = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("CameraGuide/Preview"));
		IFileManager::Get().MakeDirectory(*Output, true);
		IFileManager::Get().Delete(*(Output / TEXT("GuidedTarget.png")), false, true);
		IFileManager::Get().Delete(*(Output / TEXT("ControlReturned.png")), false, true);
		World->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateSP(AsShared(), &FRun::Tick), 0.1f, true);
	}
	bool IsFinished() const { return bFinished; }

private:
	void Finish(const FString& Failure = FString())
	{
		if (bFinished) { return; }
		bFinished = true;
		if (World.IsValid()) { World->GetTimerManager().ClearTimer(Timer); }
		if (!Failure.IsEmpty())
		{
			UE_LOG(LogAesirWarden, Error, TEXT("CAMERA_GUIDE_MAIN_SMOKE_FAIL: %s"), *Failure);
		}
		else
		{
			UE_LOG(LogAesirWarden, Display, TEXT("CAMERA_GUIDE_MAIN_SMOKE_PASS: saved BP triggered by real player overlap; actor target and fixed direction/return completed; look/move input and camera ownership restored. Temporary test actors were never saved. Screenshots: %s"), *Output);
		}
		if (Guide.IsValid()) { Guide->Destroy(); }
		if (Target.IsValid()) { Target->Destroy(); }
		FPlatformMisc::RequestExit(false);
	}
	void Tick()
	{
		if (bFinished) { return; }
		if (!World.IsValid() || FPlatformTime::Seconds() - StartedAt > 60.0)
		{
			Finish(TEXT("Timed out waiting for the real Main player and guide.")); return;
		}
		if (!Flow.IsValid())
		{
			for (TActorIterator<AAesirLevelFlowDirector> It(World.Get()); It; ++It) { Flow = *It; break; }
			if (!Flow.IsValid()) { Finish(TEXT("Main has no level flow director.")); return; }
		}
		if (Stage == 0)
		{
			if (Flow->GetFlowState() == EAesirLevelFlowState::MainMenu)
			{
				// The production entry checks World Partition readiness itself.
				Flow->RequestStartGame();
				return;
			}
			if (Flow->GetFlowState() != EAesirLevelFlowState::Exploration) { return; }
			APlayerController* PC = World->GetFirstPlayerController();
			Player = PC ? Cast<AAesirPlayerCharacter>(PC->GetPawn()) : nullptr;
			if (!Player.IsValid() || !PC || PC->IsLookInputIgnored() || PC->IsMoveInputIgnored())
			{
				Finish(TEXT("Exploration did not provide an unlocked player.")); return;
			}
			Controller = PC;
			StartRotation = PC->GetControlRotation();
			ArmLength = Player->GetCameraBoom()->TargetArmLength;
			FOV = Player->GetFollowCamera()->FieldOfView;
			UClass* Blueprint = LoadClass<AAesirCameraGuideTrigger>(nullptr,
				TEXT("/Game/Aesir/Framework/Camera/BP_CameraGuideTrigger.BP_CameraGuideTrigger_C"));
			if (!Blueprint) { Finish(TEXT("The saved camera guide Blueprint is unavailable.")); return; }
			FActorSpawnParameters Params;
			Params.ObjectFlags |= RF_Transient;
			Target = World->SpawnActor<ATargetPoint>(ATargetPoint::StaticClass(),
				Player->GetActorLocation() + FRotationMatrix(StartRotation).GetUnitAxis(EAxis::Y) * 5000.0f + FVector(0, 0, 100),
				FRotator::ZeroRotator, Params);
			const FTransform InitialTransform(Player->GetActorLocation() + FVector(2000, 0, 0));
			Guide = World->SpawnActorDeferred<AAesirCameraGuideTrigger>(Blueprint, InitialTransform,
				nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Guide.IsValid() || !Target.IsValid()) { Finish(TEXT("Could not spawn transient test actors.")); return; }
			Guide->SetFlags(RF_Transient);
			Guide->TargetActor = Target.Get();
			Guide->TurnDuration = 1.2f;
			Guide->HoldDuration = 1.0f;
			Guide->bLockMovement = true;
			Guide->FinishSpawning(InitialTransform);
			// Move the box into the capsule: the real native overlap path must start it.
			Guide->SetActorLocation(Player->GetActorLocation());
			Guide->TriggerBox->UpdateOverlaps();
			if (!Guide->IsGuiding()) { Finish(TEXT("Moving the saved BP trigger over the player did not start guidance.")); return; }
			Stage = 1;
			return;
		}
		if (!Guide.IsValid() || !Player.IsValid() || !Controller.IsValid())
		{
			Finish(TEXT("A test actor was unexpectedly lost.")); return;
		}
		if (Guide->IsGuiding())
		{
			if (!Player->IsCameraGuideActive() || !Controller->IsLookInputIgnored() || !Controller->IsMoveInputIgnored())
			{
				Finish(TEXT("An active guide lost its ownership or input locks.")); return;
			}
			if (Stage == 1 && !bTargetScreenshot && Guide->GetStage() == EAesirCameraGuideStage::Holding)
			{
				FScreenshotRequest::RequestScreenshot(Output / TEXT("GuidedTarget.png"), true, false);
				bTargetScreenshot = true;
			}
			return;
		}
		if (!Guide->GetLastFailure().IsEmpty() || Player->IsCameraGuideActive() || Controller->IsLookInputIgnored() ||
			Controller->IsMoveInputIgnored() || Controller->GetViewTarget() != Player.Get() ||
			!FMath::IsNearlyEqual(Player->GetCameraBoom()->TargetArmLength, ArmLength) ||
			!FMath::IsNearlyEqual(Player->GetFollowCamera()->FieldOfView, FOV))
		{
			Finish(TEXT("Guidance failed or did not restore the existing gameplay camera/input.")); return;
		}
		if (Stage == 1)
		{
			if (!bTargetScreenshot || FMath::Abs(FMath::FindDeltaAngleDegrees(StartRotation.Yaw, Controller->GetControlRotation().Yaw)) < 20.0f)
			{
				Finish(TEXT("Actor-target guidance did not visibly rotate the actual gameplay camera.")); return;
			}
			ReturnRotation = Controller->GetControlRotation();
			Guide->ResetTrigger();
			Guide->AimMode = EAesirCameraGuideAimMode::FixedDirection;
			Guide->DirectionArrow->SetWorldRotation(ReturnRotation + FRotator(0, 45, 0));
			Guide->bReturnToOriginalView = true;
			Guide->ReturnDuration = 1.0f;
			if (!Guide->TryStartGuide(Player.Get())) { Finish(TEXT("Fixed direction could not start after reset.")); return; }
			Stage = 2;
			return;
		}
		if (Stage == 2)
		{
			if (!Controller->GetControlRotation().Equals(ReturnRotation, 0.05f))
			{
				Finish(TEXT("The optional return did not restore the captured control rotation.")); return;
			}
			FScreenshotRequest::RequestScreenshot(Output / TEXT("ControlReturned.png"), true, false);
			Stage = 3;
			return;
		}
		if (FPaths::FileExists(Output / TEXT("ControlReturned.png"))) { Finish(); }
	}
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AAesirLevelFlowDirector> Flow;
	TWeakObjectPtr<AAesirPlayerCharacter> Player;
	TWeakObjectPtr<APlayerController> Controller;
	TWeakObjectPtr<AAesirCameraGuideTrigger> Guide;
	TWeakObjectPtr<ATargetPoint> Target;
	FTimerHandle Timer;
	FString Output;
	FRotator StartRotation;
	FRotator ReturnRotation;
	double StartedAt;
	float ArmLength = 0;
	float FOV = 0;
	int32 Stage = 0;
	bool bTargetScreenshot = false;
	bool bFinished = false;
};

TSharedPtr<FRun> ActiveRun;
FAutoConsoleCommandWithWorld RunCommand(TEXT("Aesir.CameraGuide.Smoke"),
	TEXT("Verify saved camera guide BP in Main using transient overlap/target actors. Does not save maps; screenshots and exit."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (!World || !World->IsGameWorld() || (ActiveRun.IsValid() && !ActiveRun->IsFinished())) { return; }
		ActiveRun = MakeShared<FRun>(World);
		ActiveRun->Begin();
	}));
} // namespace AesirCameraGuideSmoke

#endif
