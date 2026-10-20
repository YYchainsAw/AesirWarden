// Copyright (c) 2026 YYchainsAw. All rights reserved.

#include "Framework/Flow/AesirOpeningCameraComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

namespace AesirOpeningCamera
{
	constexpr float RouteMargin = 4.0f;
	constexpr float MaxStepAngle = PI / 90.0f;
	constexpr float MaxStepAlpha = 1.0f / 120.0f;

	float Smooth(float Alpha)
	{
		Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return Alpha * Alpha * (3.0f - 2.0f * Alpha);
	}

	bool IsFiniteView(const FMinimalViewInfo& View)
	{
		return !View.Location.ContainsNaN() && !View.Rotation.ContainsNaN() &&
			FMath::IsFinite(View.FOV) && View.FOV > 0.0f && View.FOV < 180.0f &&
			View.ProjectionMode == ECameraProjectionMode::Perspective;
	}
}

UAesirOpeningCameraComponent::UAesirOpeningCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

bool UAesirOpeningCameraComponent::ReadPlayerView(FMinimalViewInfo& OutView) const
{
	ACharacter* Character = PlayerCharacter.Get();
	if (!Character)
	{
		return false;
	}
	bool bHasActiveCamera = false;
	TInlineComponentArray<UCameraComponent*> Cameras(Character);
	for (const UCameraComponent* Camera : Cameras)
	{
		bHasActiveCamera |= IsValid(Camera) && Camera->IsActive();
	}
	if (!bHasActiveCamera)
	{
		return false;
	}
	// CalcCamera includes the active camera's additive offset, FOV and current
	// SpringArm collision/lag result, rather than reconstructing a nominal arm.
	Character->CalcCamera(0.0f, OutView);
	return AesirOpeningCamera::IsFiniteView(OutView);
}

void UAesirOpeningCameraComponent::UpdatePlayerEnvelope()
{
	ACharacter* Character = PlayerCharacter.Get();
	if (!Character)
	{
		return;
	}
	EnvelopeCenter = Character->GetActorLocation();
	EnvelopeRadius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	// A conservative vertical cylinder covers the capsule, animated mesh and
	// visible attached equipment. Keeping horizontal clearance also prevents an
	// apparently safe overhead shortcut from passing through a rising character.
	TArray<AActor*> Actors;
	Character->GetAttachedActors(Actors, true, true);
	Actors.Add(Character);
	for (AActor* Actor : Actors)
	{
		TInlineComponentArray<UPrimitiveComponent*> Components(Actor);
		for (UPrimitiveComponent* Component : Components)
		{
			if (!IsValid(Component) || !Component->IsVisible() || Component->bHiddenInGame ||
				(!Component->IsA<USkeletalMeshComponent>() && !Component->IsA<UStaticMeshComponent>()))
			{
				continue;
			}
			const FBoxSphereBounds& Bounds = Component->Bounds;
			const float Radius = FVector2D(Bounds.Origin - EnvelopeCenter).Size() + FVector2D(Bounds.BoxExtent).Size();
			if (FMath::IsFinite(Radius))
			{
				EnvelopeRadius = FMath::Max(EnvelopeRadius, Radius);
			}
		}
	}
}

FCollisionQueryParams UAesirOpeningCameraComponent::MakeQueryParams() const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AesirOpeningCamera), false);
	if (ACharacter* Character = PlayerCharacter.Get())
	{
		Params.AddIgnoredActor(Character);
		TArray<AActor*> Attached;
		Character->GetAttachedActors(Attached, true, true);
		Params.AddIgnoredActors(Attached);
	}
	if (TransitionCamera)
	{
		Params.AddIgnoredActor(TransitionCamera);
	}
	return Params;
}

float UAesirOpeningCameraComponent::GetClearance(const FVector& Location) const
{
	return FVector2D(Location - EnvelopeCenter).Size() - EnvelopeRadius - FMath::Max(5.0f, CameraProbeRadius);
}

bool UAesirOpeningCameraComponent::IsPointSafe(const FVector& Location, FString* OutReason) const
{
	const float Clearance = GetClearance(Location);
	if (Location.ContainsNaN() || Clearance + 0.1f < FMath::Max(100.0f, MinPlayerClearance))
	{
		if (OutReason)
		{
			*OutReason = FString::Printf(TEXT("Camera endpoint/path has %.1f cm player clearance; %.1f cm is required (equipment envelope radius %.1f cm)."),
				Clearance, FMath::Max(100.0f, MinPlayerClearance), EnvelopeRadius);
		}
		return false;
	}
	if (GetWorld()->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_Camera,
		FCollisionShape::MakeSphere(FMath::Max(5.0f, CameraProbeRadius)), MakeQueryParams()))
	{
		if (OutReason)
		{
			*OutReason = TEXT("Camera endpoint/path overlaps blocking world geometry.");
		}
		return false;
	}
	return true;
}

bool UAesirOpeningCameraComponent::IsSegmentSafe(const FVector& From, const FVector& To, FString* OutReason) const
{
	if (!IsPointSafe(To, OutReason))
	{
		return false;
	}
	const FVector2D Start(From - EnvelopeCenter);
	const FVector2D Delta(To - From);
	const float Fraction = Delta.SizeSquared() > SMALL_NUMBER ? FMath::Clamp(-FVector2D::DotProduct(Start, Delta) / Delta.SizeSquared(), 0.0f, 1.0f) : 0.0f;
	const float SegmentClearance = (Start + Delta * Fraction).Size() - EnvelopeRadius - FMath::Max(5.0f, CameraProbeRadius);
	if (SegmentClearance + 0.1f < FMath::Max(100.0f, MinPlayerClearance))
	{
		if (OutReason)
		{
			*OutReason = TEXT("Camera movement would cross the player's protected volume.");
		}
		return false;
	}
	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, From, To, FQuat::Identity, ECC_Camera,
		FCollisionShape::MakeSphere(FMath::Max(5.0f, CameraProbeRadius)), MakeQueryParams()))
	{
		if (OutReason)
		{
			*OutReason = FString::Printf(TEXT("Camera route is blocked by %s."), *GetNameSafe(Hit.GetActor()));
		}
		return false;
	}
	return true;
}

FVector UAesirOpeningCameraComponent::EvaluateLocation(float Alpha) const
{
	const FVector EndOffset = EndView.Location - EnvelopeCenter;
	const float EndYaw = FMath::Atan2(EndOffset.Y, EndOffset.X);
	const float AdjustedSweep = SweepAngle + FMath::FindDeltaAngleRadians(InitialEndYaw, EndYaw);
	const float Yaw = StartYaw + AdjustedSweep * Alpha;
	const float Radius = FMath::Lerp(StartRadius, static_cast<float>(FVector2D(EndOffset).Size()), Alpha) +
		AesirOpeningCamera::RouteMargin * FMath::Sin(PI * Alpha);
	const float Height = FMath::Lerp(StartZOffset, static_cast<float>(EndOffset.Z), Alpha) +
		SelectedArcHeight * FMath::Sin(PI * Alpha);
	return EnvelopeCenter + FVector(FMath::Cos(Yaw) * Radius, FMath::Sin(Yaw) * Radius, Height);
}

FMinimalViewInfo UAesirOpeningCameraComponent::EvaluateView(float Alpha) const
{
	FMinimalViewInfo View = Alpha < 0.5f ? StartView : EndView;
	View.Location = EvaluateLocation(Alpha);
	const FVector Focus = EnvelopeCenter + FVector(0.0f, 0.0f, FMath::Lerp(-30.0f, 35.0f, Alpha));
	const FQuat LookAt = (Focus - View.Location).Rotation().Quaternion();
	FQuat Rotation = FQuat::Slerp(StartView.Rotation.Quaternion(), LookAt, AesirOpeningCamera::Smooth(Alpha / 0.25f));
	Rotation = FQuat::Slerp(Rotation, EndView.Rotation.Quaternion(), AesirOpeningCamera::Smooth((Alpha - 0.7f) / 0.3f));
	View.Rotation = Rotation.Rotator();
	View.FOV = FMath::Lerp(StartView.FOV, EndView.FOV, Alpha);
	View.AspectRatio = FMath::Lerp(StartView.AspectRatio, EndView.AspectRatio, Alpha);
	View.PostProcessBlendWeight = FMath::Lerp(StartView.PostProcessBlendWeight, EndView.PostProcessBlendWeight, Alpha);
	if (Alpha <= 0.0f)
	{
		View = StartView;
	}
	else if (Alpha >= 1.0f)
	{
		View = EndView;
	}
	return View;
}

bool UAesirOpeningCameraComponent::ValidateRoute(float FromAlpha, float ToAlpha, bool bObserve, FString* OutReason)
{
	const float Range = FMath::Max(0.0f, ToAlpha - FromAlpha);
	const int32 Steps = FMath::Max(1, FMath::CeilToInt(FMath::Max(Range / AesirOpeningCamera::MaxStepAlpha,
		FMath::Abs(SweepAngle) * Range / AesirOpeningCamera::MaxStepAngle)));
	FVector Previous = bObserve ? LastView.Location : EvaluateLocation(FromAlpha);
	for (int32 Index = 1; Index <= Steps; ++Index)
	{
		const FVector Next = EvaluateLocation(FMath::Lerp(FromAlpha, ToAlpha, static_cast<float>(Index) / Steps));
		if (!IsSegmentSafe(Previous, Next, OutReason))
		{
			return false;
		}
		if (bObserve)
		{
			MinimumObservedClearance = FMath::Min(MinimumObservedClearance, GetClearance(Next));
		}
		Previous = Next;
	}
	return true;
}

bool UAesirOpeningCameraComponent::BeginTransition(APlayerController* Controller, ACharacter* Character, UAnimInstance* AnimInstance, UAnimMontage* Montage)
{
	CancelTransition(false);
	bCompleted = false;
	bSkipRequested = false;
	Progress = RouteAlpha = 0.0f;
	BlendOutStartProgress = -1.0f;
	MinimumObservedClearance = TNumericLimits<float>::Max();
	EndPositionError = EndRotationError = EndFOVError = 0.0f;
	LastFailure.Reset();
	if (!bEnabled || !IsValid(Controller) || !IsValid(Character) || Controller->GetPawn() != Character ||
		!IsValid(AnimInstance) || !IsValid(Montage) || !GetWorld())
	{
		LastFailure = TEXT("Opening camera requires its possessed character and a playing stand-up montage.");
		return false;
	}
	PlayerController = Controller;
	PlayerCharacter = Character;
	PlayerAnimInstance = AnimInstance;
	StandUpMontage = Montage;
	if (!ReadPlayerView(EndView))
	{
		LastFailure = TEXT("The possessed character has no active perspective gameplay camera.");
		return false;
	}
	if (AActor* Target = Controller->GetViewTarget())
	{
		Target->CalcCamera(0.0f, StartView);
	}
	else
	{
		LastFailure = TEXT("There is no current menu view to preserve.");
		return false;
	}
	if (Controller->PlayerCameraManager && Controller->PlayerCameraManager->GetCameraCacheTime() > 0.0f)
	{
		const FMinimalViewInfo& Cached = Controller->PlayerCameraManager->GetCameraCacheView();
		if (AesirOpeningCamera::IsFiniteView(Cached))
		{
			StartView = Cached;
		}
	}
	if (!AesirOpeningCamera::IsFiniteView(StartView))
	{
		LastFailure = TEXT("The menu camera view is not a valid perspective view.");
		return false;
	}
	UpdatePlayerEnvelope();
	if (!IsPointSafe(StartView.Location, &LastFailure) || !IsPointSafe(EndView.Location, &LastFailure))
	{
		return false;
	}
	const FVector StartOffset = StartView.Location - EnvelopeCenter;
	const FVector EndOffset = EndView.Location - EnvelopeCenter;
	StartRadius = FVector2D(StartOffset).Size();
	StartYaw = FMath::Atan2(StartOffset.Y, StartOffset.X);
	InitialEndYaw = FMath::Atan2(EndOffset.Y, EndOffset.X);
	StartZOffset = StartOffset.Z;
	const float ShortSweep = FMath::FindDeltaAngleRadians(StartYaw, InitialEndYaw);
	const float LeftSweep = ShortSweep > 0.0f ? ShortSweep - 2.0f * PI : ShortSweep;
	const float RightSweep = ShortSweep < 0.0f ? ShortSweep + 2.0f * PI : ShortSweep;
	TArray<float> Sweeps;
	if (OrbitDirection == EAesirOpeningCameraOrbitDirection::Auto)
	{
		Sweeps = { ShortSweep, ShortSweep >= 0.0f ? ShortSweep - 2.0f * PI : ShortSweep + 2.0f * PI };
	}
	else
	{
		Sweeps.Add(OrbitDirection == EAesirOpeningCameraOrbitDirection::Left ? LeftSweep : RightSweep);
	}
	bool bFoundRoute = false;
	const float Heights[] = { FMath::Max(0.0f, ArcHeight), FMath::Max(0.0f, ArcHeight) + 80.0f, FMath::Max(0.0f, ArcHeight) + 180.0f };
	for (float CandidateSweep : Sweeps)
	{
		SweepAngle = CandidateSweep;
		for (float Height : Heights)
		{
			SelectedArcHeight = Height;
			if (ValidateRoute(0.0f, 1.0f, false, &LastFailure))
			{
				bFoundRoute = true;
				break;
			}
		}
		if (bFoundRoute)
		{
			break;
		}
	}
	if (!bFoundRoute)
	{
		LastFailure = TEXT("No safe opening camera orbit; adjust the menu/gameplay camera or campfire placement. ") + LastFailure;
		return false;
	}
	LastFailure.Reset();
	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TransitionCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), StartView.Location, StartView.Rotation, Params);
	if (!TransitionCamera)
	{
		LastFailure = TEXT("Could not create the temporary opening camera.");
		return false;
	}
	LastView = StartView;
	ApplyView(StartView);
	MinimumObservedClearance = GetClearance(StartView.Location);
	bAnimationFinished = false;
	bActive = true;
	if (Character->GetMesh())
	{
		AddTickPrerequisiteComponent(Character->GetMesh());
	}
	TInlineComponentArray<USpringArmComponent*> Arms(Character);
	for (USpringArmComponent* Arm : Arms)
	{
		AddTickPrerequisiteComponent(Arm);
	}
	SetComponentTickEnabled(true);
	Controller->SetViewTargetWithBlend(TransitionCamera, 0.0f);
	return true;
}

float UAesirOpeningCameraComponent::GetAnimationProgress()
{
	UAnimInstance* Anim = PlayerAnimInstance.Get();
	UAnimMontage* Montage = StandUpMontage.Get();
	const FAnimMontageInstance* Instance = Anim && Montage ? Anim->GetInstanceForMontage(Montage) : nullptr;
	if (!Instance || !Instance->IsValid())
	{
		return Progress;
	}
	const float PositionAlpha = FMath::Clamp(Instance->GetPosition() / FMath::Max(Montage->GetPlayLength(), 0.001f), 0.0f, 1.0f);
	float Next = PositionAlpha * 0.97f;
	if (Instance->IsStopped())
	{
		if (BlendOutStartProgress < 0.0f)
		{
			BlendOutStartProgress = FMath::Max(Progress, Next);
		}
		Next = FMath::Lerp(BlendOutStartProgress, 1.0f, Instance->GetBlend().GetAlpha());
	}
	// The completion delegate owns the exact endpoint, including final blend-out.
	return FMath::Clamp(FMath::Max(Progress, Next), 0.0f, 0.9999f);
}

float UAesirOpeningCameraComponent::Ease(float InProgress) const
{
	if (InProgress <= 0.0f || InProgress >= 1.0f)
	{
		return FMath::Clamp(InProgress, 0.0f, 1.0f);
	}
	return EaseCurve ? FMath::Clamp(EaseCurve->GetFloatValue(InProgress), 0.0f, 1.0f) : AesirOpeningCamera::Smooth(InProgress);
}

void UAesirOpeningCameraComponent::ApplyView(const FMinimalViewInfo& View)
{
	if (!TransitionCamera)
	{
		return;
	}
	TransitionCamera->SetActorLocationAndRotation(View.Location, View.Rotation);
	UCameraComponent* Camera = TransitionCamera->GetCameraComponent();
	Camera->SetFieldOfView(View.FOV);
	Camera->SetAspectRatio(View.AspectRatio);
	Camera->SetConstraintAspectRatio(View.bConstrainAspectRatio);
	Camera->bOverrideAspectRatioAxisConstraint = View.AspectRatioAxisConstraint.IsSet();
	if (View.AspectRatioAxisConstraint.IsSet())
	{
		Camera->SetAspectRatioAxisConstraint(View.AspectRatioAxisConstraint.GetValue());
	}
	Camera->SetProjectionMode(View.ProjectionMode);
	Camera->PostProcessBlendWeight = View.PostProcessBlendWeight;
	Camera->PostProcessSettings = View.PostProcessSettings;
}

void UAesirOpeningCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bActive)
	{
		return;
	}
	APlayerController* Controller = PlayerController.Get();
	ACharacter* Character = PlayerCharacter.Get();
	if (!Controller || !Character || Controller->GetPawn() != Character || !IsValid(TransitionCamera) ||
		Controller->GetViewTarget() != TransitionCamera || !ReadPlayerView(EndView))
	{
		Fail(TEXT("The opening lost its player, gameplay camera or temporary camera view target."));
		return;
	}
	UpdatePlayerEnvelope();
	const float NextProgress = bAnimationFinished ? 1.0f : GetAnimationProgress();
	const float NextAlpha = FMath::Max(RouteAlpha, Ease(NextProgress));
	const FMinimalViewInfo NextView = EvaluateView(NextAlpha);
	FString Reason;
	// A long frame writes only the next rendered pose. Its straight chord must
	// also be safe; clear samples along the intended arc alone are insufficient.
	// Explicit skipping cuts to the endpoint after validating the remaining arc.
	if (!IsPointSafe(LastView.Location, &Reason) ||
		(!bSkipRequested && !IsSegmentSafe(LastView.Location, NextView.Location, &Reason)) ||
		!ValidateRoute(RouteAlpha, NextAlpha, true, &Reason))
	{
		Fail(Reason);
		return;
	}
	Progress = NextProgress;
	RouteAlpha = NextAlpha;
	LastView = NextView;
	ApplyView(LastView);
	if (bAnimationFinished)
	{
		// World camera evaluation occurs after TG_PostPhysics. SpringArm and mesh
		// prerequisites have run, so both targets produce the same POV this frame.
		EndPositionError = FVector::Distance(LastView.Location, EndView.Location);
		EndRotationError = FMath::RadiansToDegrees(LastView.Rotation.Quaternion().AngularDistance(EndView.Rotation.Quaternion()));
		EndFOVError = FMath::Abs(LastView.FOV - EndView.FOV);
		bActive = false;
		bCompleted = true;
		SetComponentTickEnabled(false);
		Controller->SetViewTargetWithBlend(Character, 0.0f);
		DestroyTransitionCamera();
		OnTransitionFinished.Broadcast();
	}
}

void UAesirOpeningCameraComponent::NotifyAnimationFinished(bool bInterrupted)
{
	if (!bActive)
	{
		return;
	}
	if (bInterrupted)
	{
		Fail(TEXT("The stand-up animation was interrupted before the camera handoff."));
		return;
	}
	bAnimationFinished = true;
}

void UAesirOpeningCameraComponent::RequestFinishTransition()
{
	if (bActive)
	{
		bSkipRequested = true;
		bAnimationFinished = true;
	}
}

void UAesirOpeningCameraComponent::Fail(const FString& Reason)
{
	LastFailure = Reason;
	bActive = false;
	bCompleted = false;
	SetComponentTickEnabled(false);
	// Keep the last safe view alive until the director restores the menu.
	OnTransitionFailed.Broadcast();
}

void UAesirOpeningCameraComponent::DestroyTransitionCamera()
{
	if (IsValid(TransitionCamera))
	{
		TransitionCamera->Destroy();
	}
	TransitionCamera = nullptr;
}

void UAesirOpeningCameraComponent::CancelTransition(bool bRestorePlayerView)
{
	bActive = false;
	bAnimationFinished = false;
	bSkipRequested = false;
	SetComponentTickEnabled(false);
	if (bRestorePlayerView && PlayerController.IsValid() && PlayerCharacter.IsValid() &&
		PlayerController->GetViewTarget() == TransitionCamera)
	{
		PlayerController->SetViewTargetWithBlend(PlayerCharacter.Get(), 0.0f);
	}
	if (ACharacter* Character = PlayerCharacter.Get())
	{
		if (Character->GetMesh())
		{
			RemoveTickPrerequisiteComponent(Character->GetMesh());
		}
		TInlineComponentArray<USpringArmComponent*> Arms(Character);
		for (USpringArmComponent* Arm : Arms)
		{
			RemoveTickPrerequisiteComponent(Arm);
		}
	}
	DestroyTransitionCamera();
	PlayerController.Reset();
	PlayerCharacter.Reset();
	PlayerAnimInstance.Reset();
	StandUpMontage.Reset();
}

void UAesirOpeningCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelTransition(true);
	Super::EndPlay(EndPlayReason);
}
