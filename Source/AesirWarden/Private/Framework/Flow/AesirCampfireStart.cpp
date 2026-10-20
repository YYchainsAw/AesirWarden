// Copyright (c) 2026 YYchainsAw. All rights reserved.

#include "Framework/Flow/AesirCampfireStart.h"

#include "AesirWarden.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AAesirCampfireStart::AAesirCampfireStart()
{
	MenuSitLoopAnimation = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
		TEXT("/Game/Aesir/Animations/Player/Actions/AS_SSH_Sit_Loop.AS_SSH_Sit_Loop")));
	StandUpAnimation = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
		TEXT("/Game/Aesir/Animations/Player/Actions/AS_SSH_Sit_End.AS_SSH_Sit_End")));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.08f;

	CampfireRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CampfireRoot"));
	SetRootComponent(CampfireRoot);

	MenuCameraChild = CreateDefaultSubobject<UChildActorComponent>(TEXT("MenuCamera"));
	MenuCameraChild->SetupAttachment(CampfireRoot);
	MenuCameraChild->SetChildActorClass(ACameraActor::StaticClass());
	MenuCameraChild->SetRelativeLocation(FVector(-480.0f, -300.0f, 190.0f));
	MenuCameraChild->SetRelativeRotation(FRotator(-12.0f, 32.0f, 0.0f));

	Firewood = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Firewood"));
	Firewood->SetupAttachment(CampfireRoot);
	Firewood->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Firewood->SetCanEverAffectNavigation(false);
	StoneRing = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("StoneRing"));
	StoneRing->SetupAttachment(CampfireRoot);
	StoneRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StoneRing->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Firewood->SetStaticMesh(Cylinder.Object);
	StoneRing->SetStaticMesh(Sphere.Object);

	FlameCard = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlameCard"));
	FlameCard->SetupAttachment(CampfireRoot);
	FlameCard->SetRelativeLocation(FVector(0.0f, 0.0f, 25.0f));
	FlameCard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlameCard->SetCanEverAffectNavigation(false);
	FlameCard->SetCastShadow(false);
	FlameCardCross = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlameCardCross"));
	FlameCardCross->SetupAttachment(FlameCard);
	FlameCardCross->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	FlameCardCross->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlameCardCross->SetCanEverAffectNavigation(false);
	FlameCardCross->SetCastShadow(false);

	Firelight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Firelight"));
	Firelight->SetupAttachment(CampfireRoot);
	Firelight->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
	Firelight->SetIntensityUnits(ELightUnits::Lumens);
	Firelight->SetIntensity(FirelightIntensity);
	Firelight->SetAttenuationRadius(550.0f);
	Firelight->SetLightColor(FLinearColor(1.0f, 0.35f, 0.09f));
	Firelight->SetCastShadows(false);
}

void AAesirCampfireStart::ConfigureFlameMesh(UStaticMesh* Mesh)
{
	FlameCard->SetStaticMesh(Mesh);
	FlameCardCross->SetStaticMesh(Mesh);
	UpdateFlameScale();
}

void AAesirCampfireStart::UpdateFlameScale()
{
	if (const UStaticMesh* Mesh = FlameCard->GetStaticMesh())
	{
		const float MeshHeight = FMath::Max(Mesh->GetBounds().BoxExtent.Z * 2.0f, 1.0f);
		FlameCard->SetRelativeScale3D(FVector(FlameHeight / MeshHeight));
	}
}

void AAesirCampfireStart::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	MenuCamera = Cast<ACameraActor>(MenuCameraChild->GetChildActor());
	if (IsValid(MenuCamera))
	{
		MenuCamera->GetCameraComponent()->SetFieldOfView(50.0f);
	}

	// Engine primitives keep the editable scaffold usable even without the private art assets.
	Firewood->ClearInstances();
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const float Angle = 45.0f + Index * 90.0f;
		Firewood->AddInstance(FTransform(FRotator(90.0f, Angle, 0.0f),
			FVector(0.0f, 0.0f, 12.0f + (Index % 2) * 9.0f), FVector(0.17f, 0.17f, 0.95f)));
	}
	StoneRing->ClearInstances();
	for (int32 Index = 0; Index < 10; ++Index)
	{
		const float Angle = Index * 2.0f * PI / 10.0f;
		StoneRing->AddInstance(FTransform(FRotator(0.0f, FMath::RadiansToDegrees(Angle), 0.0f),
			FVector(FMath::Cos(Angle) * 62.0f, FMath::Sin(Angle) * 62.0f, 9.0f), FVector(0.30f, 0.24f, 0.18f)));
	}
	if (UMaterialInstanceDynamic* WoodMaterial = Firewood->CreateAndSetMaterialInstanceDynamic(0))
	{
		WoodMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.09f, 0.035f, 0.012f));
	}
	if (UMaterialInstanceDynamic* RockMaterial = StoneRing->CreateAndSetMaterialInstanceDynamic(0))
	{
		RockMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.15f, 0.14f, 0.13f));
	}
	UpdateFlameScale();
	Firelight->SetIntensity(FirelightIntensity);
}

void AAesirCampfireStart::BeginPlay()
{
	MenuCamera = Cast<ACameraActor>(MenuCameraChild->GetChildActor());
	Super::BeginPlay();
	SetActorTickEnabled(bAnimateFirelight);
}

void AAesirCampfireStart::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Time = GetWorld()->GetTimeSeconds();
	const float Flicker = 1.0f + 0.06f * FMath::Sin(Time * 9.7f) + 0.035f * FMath::Sin(Time * 17.3f);
	Firelight->SetIntensity(FirelightIntensity * Flicker);
}

void AAesirCampfireStart::PrepareMainMenuPresentation()
{
	ClearPlayerPresentation();
	if (!bUsePlayerSitAnimations)
	{
		return;
	}
	ACharacter* Character = Cast<ACharacter>(PlayerPawnRef);
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		// A non-character pawn can still use the menu and its original immediate handoff.
		return;
	}
	UAnimSequence* SitLoop = MenuSitLoopAnimation.LoadSynchronous();
	LoadedStandUpAnimation = StandUpAnimation.LoadSynchronous();
	if (!SitLoop)
	{
		UE_LOG(LogAesirWarden, Warning, TEXT("Campfire: Menu Sit Loop Animation is missing; using the player's normal idle pose."));
		return;
	}

	PresentationMesh = Mesh;
	PresentationAnimInstance = AnimInstance;
	SavedRootMotionMode = static_cast<uint8>(AnimInstance->RootMotionMode.GetValue());
	SavedVisibilityTickOption = static_cast<uint8>(Mesh->VisibilityBasedAnimTickOption);
	// Extract and discard root motion: repeated sit cycles must not move the capsule.
	AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	// End delegates must advance even if an authored menu camera cannot see the player.
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	MenuLoopMontage = PlayPlayerPresentationAnimation(SitLoop, 1.0f, 0.0f);
	if (!MenuLoopMontage)
	{
		ClearPlayerPresentation();
		return;
	}
	// Dynamic montages have one Default section. Link it to itself for an indefinite menu loop.
	AnimInstance->Montage_SetNextSection(TEXT("Default"), TEXT("Default"), MenuLoopMontage);
	UE_LOG(LogAesirWarden, Display, TEXT("Campfire: looping %s on the possessed player via %s."),
		*SitLoop->GetName(), *PlayerAnimationSlot.ToString());
}

UAnimMontage* AAesirCampfireStart::PlayPlayerPresentationAnimation(
	UAnimSequence* Animation, float PlayRate, float BlendInTime)
{
	UAnimInstance* AnimInstance = PresentationAnimInstance.Get();
	USkeletalMeshComponent* Mesh = PresentationMesh.Get();
	if (!AnimInstance || !Mesh || !Animation || PlayerAnimationSlot.IsNone() ||
		!Animation->GetSkeleton() || !Animation->GetSkeleton()->IsCompatibleMesh(Mesh->GetSkeletalMeshAsset()))
	{
		UE_LOG(LogAesirWarden, Warning, TEXT("Campfire: animation %s cannot play on the player's skeleton/slot; falling back to normal animation."),
			*GetNameSafe(Animation));
		return nullptr;
	}
	return AnimInstance->PlaySlotAnimationAsDynamicMontage(Animation, PlayerAnimationSlot,
		FMath::Max(0.0f, BlendInTime), FMath::Max(0.0f, SitBlendTime), PlayRate, 1);
}

bool AAesirCampfireStart::BeginOpeningPresentation()
{
	UAnimInstance* AnimInstance = PresentationAnimInstance.Get();
	USkeletalMeshComponent* Mesh = PresentationMesh.Get();
	if (!bUsePlayerSitAnimations || !MenuLoopMontage || !AnimInstance || !Mesh ||
		Mesh->GetAnimInstance() != AnimInstance || Mesh->GetOwner() != PlayerPawnRef)
	{
		return false;
	}
	if (!LoadedStandUpAnimation)
	{
		UE_LOG(LogAesirWarden, Warning, TEXT("Campfire: Stand Up Animation is missing; restoring locomotion before the camera handoff."));
		return false;
	}
	const float PlayRate = FMath::Clamp(StandUpPlayRate, 0.1f, 3.0f);
	StandUpMontage = PlayPlayerPresentationAnimation(LoadedStandUpAnimation, PlayRate, SitBlendTime);
	if (!StandUpMontage)
	{
		return false;
	}
	bWaitingForStandUpAnimation = true;
	FOnMontageEnded Finished;
	Finished.BindUObject(this, &AAesirCampfireStart::HandleStandUpEnded);
	AnimInstance->Montage_SetEndDelegate(Finished, StandUpMontage);

	// This is a recovery watchdog, not the normal animation completion signal.
	const float EffectiveRate = PlayRate * FMath::Max(LoadedStandUpAnimation->RateScale, 0.01f);
	StandUpDeadline = GetWorld()->GetTimeSeconds() + LoadedStandUpAnimation->GetPlayLength() / EffectiveRate +
		FMath::Max(0.0f, SitBlendTime) + 5.0f;
	GetWorldTimerManager().SetTimer(StandUpMonitorTimer, this, &AAesirCampfireStart::MonitorStandUp, 0.1f, true);
	UE_LOG(LogAesirWarden, Display, TEXT("Campfire: playing %s; waiting for its montage end before gameplay."),
		*LoadedStandUpAnimation->GetName());
	return true;
}

void AAesirCampfireStart::HandleStandUpEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bWaitingForStandUpAnimation || Montage != StandUpMontage)
	{
		return;
	}
	if (bInterrupted)
	{
		UE_LOG(LogAesirWarden, Warning, TEXT("Campfire: stand-up montage was interrupted; restoring locomotion and completing the opening."));
	}
	else
	{
		UE_LOG(LogAesirWarden, Display, TEXT("Campfire: stand-up animation completed."));
	}
	CompleteOpeningPresentation();
}

void AAesirCampfireStart::MonitorStandUp()
{
	if (!bWaitingForStandUpAnimation)
	{
		GetWorldTimerManager().ClearTimer(StandUpMonitorTimer);
		return;
	}
	USkeletalMeshComponent* Mesh = PresentationMesh.Get();
	if (!Mesh || Mesh->GetAnimInstance() != PresentationAnimInstance.Get() ||
		!IsValid(PlayerControllerRef) || PlayerControllerRef->GetPawn() != Mesh->GetOwner() ||
		GetWorld()->GetTimeSeconds() > StandUpDeadline)
	{
		UE_LOG(LogAesirWarden, Warning, TEXT("Campfire: stand-up lost its player/AnimInstance or timed out; restoring gameplay safely."));
		CompleteOpeningPresentation();
	}
}

void AAesirCampfireStart::FinishOpeningPresentation(bool bSkipped)
{
	ClearPlayerPresentation();
}

void AAesirCampfireStart::ClearPlayerPresentation()
{
	bWaitingForStandUpAnimation = false;
	GetWorldTimerManager().ClearTimer(StandUpMonitorTimer);
	if (UAnimInstance* AnimInstance = PresentationAnimInstance.Get())
	{
		// Touch only this actor's montage delegates. Player evade/attack bindings remain intact.
		if (StandUpMontage)
		{
			FOnMontageEnded Empty;
			AnimInstance->Montage_SetEndDelegate(Empty, StandUpMontage);
			AnimInstance->Montage_Stop(0.0f, StandUpMontage);
		}
		if (MenuLoopMontage)
		{
			AnimInstance->Montage_Stop(0.0f, MenuLoopMontage);
		}
		AnimInstance->SetRootMotionMode(static_cast<ERootMotionMode::Type>(SavedRootMotionMode));
	}
	if (USkeletalMeshComponent* Mesh = PresentationMesh.Get())
	{
		Mesh->VisibilityBasedAnimTickOption = static_cast<EVisibilityBasedAnimTickOption>(SavedVisibilityTickOption);
	}
	MenuLoopMontage = nullptr;
	StandUpMontage = nullptr;
	LoadedStandUpAnimation = nullptr;
	PresentationAnimInstance.Reset();
	PresentationMesh.Reset();
}
