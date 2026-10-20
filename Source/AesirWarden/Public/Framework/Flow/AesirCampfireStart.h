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
class UAnimInstance;
class UAnimMontage;
class UAnimSequence;
class USkeletalMeshComponent;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campfire|Player Animation")
	bool bUsePlayerSitAnimations = true;

	/** Soft references keep third-party animation data out of the native class package. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campfire|Player Animation", meta = (EditCondition = "bUsePlayerSitAnimations"))
	TSoftObjectPtr<UAnimSequence> MenuSitLoopAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campfire|Player Animation", meta = (EditCondition = "bUsePlayerSitAnimations"))
	TSoftObjectPtr<UAnimSequence> StandUpAnimation;

	/** Must be a full-body Slot connected to the player's AnimGraph output. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campfire|Player Animation")
	FName PlayerAnimationSlot = TEXT("DefaultSlot");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campfire|Player Animation", meta = (ClampMin = "0.0", Units = "s"))
	float SitBlendTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campfire|Player Animation", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float StandUpPlayRate = 1.0f;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Campfire|Player Animation")
	bool bWaitingForStandUpAnimation = false;

protected:
	virtual void BeginPlay() override;
	virtual void PrepareMainMenuPresentation() override;
	virtual bool BeginOpeningPresentation() override;
	virtual void FinishOpeningPresentation(bool bSkipped) override;

private:
	void UpdateFlameScale();
	UAnimMontage* PlayPlayerPresentationAnimation(UAnimSequence* Animation, float PlayRate, float BlendInTime);
	void HandleStandUpEnded(UAnimMontage* Montage, bool bInterrupted);
	void MonitorStandUp();
	void ClearPlayerPresentation();

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> MenuLoopMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> StandUpMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> LoadedStandUpAnimation;

	TWeakObjectPtr<UAnimInstance> PresentationAnimInstance;
	TWeakObjectPtr<USkeletalMeshComponent> PresentationMesh;
	FTimerHandle StandUpMonitorTimer;
	double StandUpDeadline = 0.0;
	uint8 SavedRootMotionMode = 0;
	uint8 SavedVisibilityTickOption = 0;
};
