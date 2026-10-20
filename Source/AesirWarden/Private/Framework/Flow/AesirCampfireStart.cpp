// Copyright (c) 2026 YYchainsAw. All rights reserved.

#include "Framework/Flow/AesirCampfireStart.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AAesirCampfireStart::AAesirCampfireStart()
{
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
