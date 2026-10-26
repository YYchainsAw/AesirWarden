// Copyright (c) 2026 YYchainsAw. All rights reserved.

#include "Framework/Camera/AesirCameraGuideTrigger.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Characters/Player/AesirPlayerCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "TimerManager.h"

namespace AesirCameraGuide
{
	float Smooth(float Value)
	{
		Value = FMath::Clamp(Value, 0.0f, 1.0f);
		return Value * Value * (3.0f - 2.0f * Value);
	}
}

AAesirCameraGuideTrigger::AAesirCameraGuideTrigger()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	GuideRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GuideRoot"));
	SetRootComponent(GuideRoot);
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(GuideRoot);
	TriggerBox->SetBoxExtent(FVector(200.0f, 200.0f, 150.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->SetCanEverAffectNavigation(false);
	DirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
	DirectionArrow->SetupAttachment(GuideRoot);
	DirectionArrow->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	DirectionArrow->ArrowColor = FColor(240, 190, 70);
	DirectionArrow->ArrowSize = 2.0f;
	DirectionArrow->SetHiddenInGame(true);
}

void AAesirCameraGuideTrigger::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &AAesirCameraGuideTrigger::HandleTriggerOverlap);
	// An initially overlapping player may have entered during component registration,
	// before BeginPlay bound the event. This is one initial check, not a retry loop.
	GetWorldTimerManager().SetTimerForNextTick(this, &AAesirCameraGuideTrigger::TryStartForOverlappingPlayer);
}

void AAesirCameraGuideTrigger::HandleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AAesirPlayerCharacter* Player = Cast<AAesirPlayerCharacter>(OtherActor);
	if (Player && OtherComponent == Player->GetCapsuleComponent())
	{
		TryStartGuide(Player);
	}
}

void AAesirCameraGuideTrigger::TryStartForOverlappingPlayer()
{
	if (!IsValid(TriggerBox) || bEndingPlay || IsGuiding())
	{
		return;
	}
	TArray<AActor*> Overlapping;
	TriggerBox->GetOverlappingActors(Overlapping, AAesirPlayerCharacter::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		if (TryStartGuide(Cast<APawn>(Actor)))
		{
			return;
		}
	}
}

bool AAesirCameraGuideTrigger::IsExplorationAllowed() const
{
	if (!GetWorld())
	{
		return false;
	}
	for (TActorIterator<AAesirLevelFlowDirector> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && It->GetFlowState() != EAesirLevelFlowState::Exploration)
		{
			return false;
		}
	}
	return true;
}

FRotator AAesirCameraGuideTrigger::ClampCameraRotation(const FRotator& Rotation, const APlayerController* Controller) const
{
	float MinPitch = -89.0f;
	float MaxPitch = 89.0f;
	if (Controller && Controller->PlayerCameraManager)
	{
		MinPitch = Controller->PlayerCameraManager->ViewPitchMin;
		MaxPitch = Controller->PlayerCameraManager->ViewPitchMax;
	}
	if (!FMath::IsFinite(MinPitch) || !FMath::IsFinite(MaxPitch) || MinPitch > MaxPitch)
	{
		MinPitch = -89.0f;
		MaxPitch = 89.0f;
	}
	return FRotator(FMath::Clamp(FRotator::NormalizeAxis(Rotation.Pitch), MinPitch, MaxPitch),
		FRotator::NormalizeAxis(Rotation.Yaw), 0.0f);
}

bool AAesirCameraGuideTrigger::GetDesiredRotation(const AAesirPlayerCharacter* Player,
	const APlayerController* Controller, FRotator& OutRotation) const
{
	if (!Player || !Controller || !IsValid(Player->GetFollowCamera()) || !Player->GetFollowCamera()->IsActive())
	{
		return false;
	}
	FRotator Desired;
	if (AimMode == EAesirCameraGuideAimMode::FixedDirection)
	{
		if (!IsValid(DirectionArrow))
		{
			return false;
		}
		Desired = DirectionArrow->GetComponentRotation();
	}
	else
	{
		// Do not synchronously load a selected actor's World Partition cell.
		AActor* Target = TargetActor.Get();
		if (!IsValid(Target) || Target->IsActorBeingDestroyed() || Target->GetWorld() != GetWorld() || TargetOffset.ContainsNaN())
		{
			return false;
		}
		const FVector Direction = Target->GetActorLocation() + TargetOffset - Player->GetFollowCamera()->GetComponentLocation();
		if (Direction.ContainsNaN() || Direction.IsNearlyZero())
		{
			return false;
		}
		Desired = Direction.Rotation();
	}
	if (Desired.ContainsNaN())
	{
		return false;
	}
	OutRotation = ClampCameraRotation(Desired, Controller);
	return true;
}

bool AAesirCameraGuideTrigger::TryStartGuide(APawn* PlayerPawn)
{
	if (bEndingPlay || IsActorBeingDestroyed() || !bEnabled || IsGuiding() || (bTriggerOnce && bHasTriggered))
	{
		return false;
	}
	AAesirPlayerCharacter* Player = Cast<AAesirPlayerCharacter>(PlayerPawn);
	APlayerController* Controller = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	if (!IsValid(Player) || !IsValid(Controller) || !Controller->IsLocalController() || Controller->GetPawn() != Player ||
		Controller->GetViewTarget() != Player || Controller->IsLookInputIgnored() || !Player->CanAcceptCameraGuide() ||
		!IsExplorationAllowed())
	{
		LastFailure = TEXT("A local living player in exploration must own its gameplay camera and look input.");
		return false;
	}
	FRotator Desired;
	if (!GetDesiredRotation(Player, Controller, Desired))
	{
		LastFailure = TEXT("Select a loaded Target Actor, or choose Fixed Direction and rotate DirectionArrow.");
		return false;
	}
	if (!FMath::IsFinite(TurnDuration) || !FMath::IsFinite(HoldDuration) || !FMath::IsFinite(ReturnDuration))
	{
		LastFailure = TEXT("Camera guide durations must be finite.");
		return false;
	}
	if (!Player->TryAcquireCameraGuide(this))
	{
		LastFailure = TEXT("Another guide already owns this player's camera.");
		return false;
	}
	GuidedPlayer = Player;
	GuidedController = Controller;
	OriginalRotation = ClampCameraRotation(Controller->GetControlRotation(), Controller);
	TurnSeconds = FMath::Max(0.0f, TurnDuration);
	HoldSeconds = FMath::Max(0.0f, HoldDuration);
	bShouldReturn = bReturnToOriginalView;
	ReturnSeconds = bShouldReturn ? FMath::Max(0.0f, ReturnDuration) : 0.0f;
	PhaseElapsed = TotalElapsed = Progress = 0.0f;
	LastPlayerHealth = Player->GetGASHealth();
	Stage = EAesirCameraGuideStage::Turning;
	bHasTriggered = true;
	LastFailure.Reset();
	bOwnsLookLock = true;
	Controller->SetIgnoreLookInput(true);
	bOwnsMoveLock = bLockMovement;
	if (bOwnsMoveLock)
	{
		Controller->SetIgnoreMoveInput(true);
	}
	AddTickPrerequisiteActor(Controller);
	AddTickPrerequisiteActor(Player);
	if (USpringArmComponent* Arm = Player->GetCameraBoom())
	{
		Arm->AddTickPrerequisiteActor(this);
	}
	SetActorTickEnabled(true);
	// All state is established before Blueprint listeners can cancel/destroy/re-enter.
	OnGuideStarted.Broadcast();
	return true;
}

bool AAesirCameraGuideTrigger::IsRuntimeValid() const
{
	const AAesirPlayerCharacter* Player = GuidedPlayer.Get();
	const APlayerController* Controller = GuidedController.Get();
	return bEnabled && IsValid(Player) && IsValid(Controller) && Controller->GetPawn() == Player &&
		Controller->IsLocalController() && Controller->GetViewTarget() == Player &&
		Player->CanAcceptCameraGuide() && Player->IsCameraGuideActive() && IsExplorationAllowed();
}

void AAesirCameraGuideTrigger::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsGuiding())
	{
		return;
	}
	if (!IsRuntimeValid())
	{
		CompleteGuide(true, TEXT("The player, gameplay phase, or camera ownership changed during the guide."));
		return;
	}
	AAesirPlayerCharacter* Player = GuidedPlayer.Get();
	APlayerController* Controller = GuidedController.Get();
	const float Health = Player->GetGASHealth();
	if (FMath::IsFinite(Health) && Health + KINDA_SMALL_NUMBER < LastPlayerHealth)
	{
		CompleteGuide(true, TEXT("The player took damage during the camera guide."));
		return;
	}
	LastPlayerHealth = Health;
	FRotator Desired;
	if (!GetDesiredRotation(Player, Controller, Desired))
	{
		CompleteGuide(true, TEXT("The guide target or active gameplay camera is unavailable."));
		return;
	}
	float Remaining = FMath::IsFinite(DeltaSeconds) ? FMath::Max(0.0f, DeltaSeconds) : 0.0f;
	// A bounded state machine also handles zero-length stages and large frames.
	for (int32 Iteration = 0; Iteration < 4 && IsGuiding(); ++Iteration)
	{
		const float Duration = Stage == EAesirCameraGuideStage::Turning ? TurnSeconds
			: (Stage == EAesirCameraGuideStage::Holding ? HoldSeconds : ReturnSeconds);
		const float Advance = FMath::Min(Remaining, FMath::Max(0.0f, Duration - PhaseElapsed));
		PhaseElapsed += Advance;
		TotalElapsed += Advance;
		Remaining -= Advance;
		const float Alpha = Duration > SMALL_NUMBER ? FMath::Clamp(PhaseElapsed / Duration, 0.0f, 1.0f) : 1.0f;
		if (Stage == EAesirCameraGuideStage::Turning)
		{
			const FQuat Rotation = FQuat::Slerp(OriginalRotation.Quaternion(), Desired.Quaternion(), AesirCameraGuide::Smooth(Alpha));
			Controller->SetControlRotation(ClampCameraRotation(Rotation.Rotator(), Controller));
		}
		else if (Stage == EAesirCameraGuideStage::Holding)
		{
			Controller->SetControlRotation(Desired);
		}
		else
		{
			const FQuat Rotation = FQuat::Slerp(ReturnStartRotation.Quaternion(), OriginalRotation.Quaternion(), AesirCameraGuide::Smooth(Alpha));
			Controller->SetControlRotation(ClampCameraRotation(Rotation.Rotator(), Controller));
		}
		const float TotalDuration = TurnSeconds + HoldSeconds + ReturnSeconds;
		Progress = TotalDuration > SMALL_NUMBER ? FMath::Clamp(TotalElapsed / TotalDuration, 0.0f, 1.0f) : 1.0f;
		if (Alpha < 1.0f)
		{
			break;
		}
		if (Stage == EAesirCameraGuideStage::Turning)
		{
			Stage = EAesirCameraGuideStage::Holding;
			PhaseElapsed = 0.0f;
		}
		else if (Stage == EAesirCameraGuideStage::Holding && bShouldReturn)
		{
			ReturnStartRotation = Controller->GetControlRotation();
			Stage = EAesirCameraGuideStage::Returning;
			PhaseElapsed = 0.0f;
		}
		else
		{
			Progress = 1.0f;
			CompleteGuide(false);
			return;
		}
	}
}

void AAesirCameraGuideTrigger::CompleteGuide(bool bCancelled, const FString& Failure)
{
	if (!IsGuiding())
	{
		return;
	}
	AAesirPlayerCharacter* Player = GuidedPlayer.Get();
	APlayerController* Controller = GuidedController.Get();
	const bool bReleaseLook = bOwnsLookLock;
	const bool bReleaseMove = bOwnsMoveLock;
	// Retire first: Blueprint callbacks and input/ownership release may be re-entrant.
	Stage = EAesirCameraGuideStage::Inactive;
	bOwnsLookLock = bOwnsMoveLock = false;
	GuidedPlayer.Reset();
	GuidedController.Reset();
	SetActorTickEnabled(false);
	LastFailure = Failure;
	if (Player)
	{
		RemoveTickPrerequisiteActor(Player);
		if (USpringArmComponent* Arm = Player->GetCameraBoom())
		{
			Arm->RemoveTickPrerequisiteActor(this);
		}
		Player->ReleaseCameraGuide(this);
	}
	if (Controller)
	{
		RemoveTickPrerequisiteActor(Controller);
		if (bReleaseLook)
		{
			Controller->SetIgnoreLookInput(false);
		}
		if (bReleaseMove)
		{
			Controller->SetIgnoreMoveInput(false);
		}
	}
	if (bCancelled)
	{
		OnGuideCancelled.Broadcast();
	}
	else
	{
		OnGuideFinished.Broadcast();
	}
}

void AAesirCameraGuideTrigger::CancelGuide()
{
	CompleteGuide(true, TEXT("The camera guide was cancelled."));
}

void AAesirCameraGuideTrigger::ResetTrigger()
{
	CancelGuide();
	if (!IsGuiding())
	{
		bHasTriggered = false;
		Progress = 0.0f;
		LastFailure.Reset();
	}
}

void AAesirCameraGuideTrigger::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.RemoveDynamic(this, &AAesirCameraGuideTrigger::HandleTriggerOverlap);
	}
	CancelGuide();
	Super::EndPlay(EndPlayReason);
}
