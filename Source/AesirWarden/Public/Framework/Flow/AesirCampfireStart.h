// Copyright (c) 2026 YYchainsAw. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Flow/AesirLevelFlowDirector.h"
#include "AesirCampfireStart.generated.h"

class UChildActorComponent;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;

/** Place one at the campfire center. Camera, menu flow and editable campfire dressing travel together. */
UCLASS(Blueprintable)
class AESIRWARDEN_API AAesirCampfireStart : public AAesirLevelFlowDirector
{
	GENERATED_BODY()

public:
	AAesirCampfireStart();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** The editor asset builder assigns an existing local flame card; no third-party asset is required by C++. */
	void ConfigureFlameMesh(UStaticMesh* Mesh);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire|Components")
	TObjectPtr<USceneComponent> CampfireRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire|Components")
	TObjectPtr<UChildActorComponent> MenuCameraChild;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire|Components")
	TObjectPtr<UInstancedStaticMeshComponent> Firewood;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire|Components")
	TObjectPtr<UInstancedStaticMeshComponent> StoneRing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire|Components")
	TObjectPtr<UStaticMeshComponent> FlameCard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire|Components")
	TObjectPtr<UStaticMeshComponent> FlameCardCross;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire|Components")
	TObjectPtr<UPointLightComponent> Firelight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campfire|Appearance", meta = (ClampMin = "1.0", Units = "cm"))
	float FlameHeight = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campfire|Appearance", meta = (ClampMin = "0.0"))
	float FirelightIntensity = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campfire|Appearance")
	bool bAnimateFirelight = true;

protected:
	virtual void BeginPlay() override;

private:
	void UpdateFlameScale();
};
