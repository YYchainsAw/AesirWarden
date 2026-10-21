// Copyright (c) 2026 YYchainsAw. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "AesirCombatHUDWidget.generated.h"

class UAbilitySystemComponent;
class UHorizontalBox;
class UProgressBar;
class UTextBlock;
class UTexture2D;
class UWidgetTree;
struct FOnAttributeChangeData;

/** Only configured, implemented abilities get a visible slot. No placeholder mana/stamina. */
USTRUCT(BlueprintType)
struct FAesirHUDAbilityEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	FText InputHint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD", meta = (Categories = "Cooldown"))
	FGameplayTag CooldownTag;
};

/** Transparent combat HUD. The persisted Widget Blueprint owns the editable Designer layout. */
UCLASS(Blueprintable)
class AESIRWARDEN_API UAesirCombatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Pin a boss independently of lock-on, e.g. when an encounter starts. Null clears the pin. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|HUD")
	void SetBossTarget(AActor* Target, FText DisplayName);

	/** Remove the encounter pin; automatic lock-on tracking may take over on the next refresh. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|HUD")
	void ClearBossTarget();

	/** Rebind after possession changes. Also called automatically at 10 Hz. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|HUD")
	void RefreshHUDSources();

	UFUNCTION(BlueprintPure, Category = "Aesir|HUD")
	AActor* GetDisplayedBoss() const { return DisplayedBoss.Get(); }

	/** Builds only an empty tree; never overwrites an authored Blueprint layout. */
	static void BuildDefaultHUDTree(UWidgetTree* Tree);

	/** Health/max ratios used by both the live HUD and Designer preview. */
	static float SafeRatio(float Value, float Maximum);

	/** Slots stay hidden until real abilities are configured in Class Defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aesir|HUD|Abilities")
	TArray<FAesirHUDAbilityEntry> AbilityEntries;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aesir|HUD|Boss")
	bool bFollowLockedBoss = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aesir|HUD|Style")
	FLinearColor HealthColor = FLinearColor(0.53f, 0.055f, 0.065f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aesir|HUD|Style")
	FLinearColor GuardColor = FLinearColor(0.83f, 0.52f, 0.16f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aesir|HUD|Style")
	FLinearColor BrokenGuardColor = FLinearColor(1.0f, 0.18f, 0.10f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aesir|HUD|Preview", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PreviewHealthRatio = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aesir|HUD|Preview", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PreviewGuardRatio = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aesir|HUD|Preview")
	bool bPreviewBoss = true;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaSeconds) override;

private:
	void ResolveWidgets();
	void BindPlayer(APawn* Pawn);
	void BindBoss(AActor* Boss);
	void UnbindPlayer();
	void UnbindBoss();
	void RefreshPlayerVitals();
	void RefreshBossVitals();
	void RebuildAbilitySlots();
	void RefreshAbilitySlots();
	void HandlePlayerAttributeChanged(const FOnAttributeChangeData& Data);
	void HandleBossAttributeChanged(const FOnAttributeChangeData& Data);

	TWeakObjectPtr<APawn> BoundPlayer;
	TWeakObjectPtr<UAbilitySystemComponent> PlayerASC;
	TWeakObjectPtr<UAbilitySystemComponent> BossASC;
	TWeakObjectPtr<AActor> PinnedBoss;
	TWeakObjectPtr<AActor> DisplayedBoss;
	FText PinnedBossName;
	bool bBossPinned = false;
	bool bHealthInitialized = false;
	bool bAbilitySlotsBuilt = false;
	float LastHealthRatio = 0.0f;
	float HealthTrailRatio = 0.0f;
	float HealthTrailDelay = 0.0f;
	TArray<FDelegateHandle> PlayerAttributeHandles;
	TArray<FDelegateHandle> BossAttributeHandles;
	FTimerHandle RefreshTimer;

	// Resolved by name instead of generated variables so existing BP health graphs remain compatible.
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> PlayerHealthBar;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> PlayerHealthTrail;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> GuardPressureBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HealthValueText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GuardStateText;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> CachedBossPanel;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> BossHealthBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BossNameText;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> AbilityPanel;
};
