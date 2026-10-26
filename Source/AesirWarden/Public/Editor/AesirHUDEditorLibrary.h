// Copyright (c) 2026 YYchainsAw. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AesirHUDEditorLibrary.generated.h"

/** One-time migration of the existing HUD; does not create a second viewport widget. */
UCLASS()
class AESIRWARDEN_API UAesirHUDEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Upgrade WBP_CombatHUD in place, retaining its graphs and original health widget GUID.
	 * The first upgrade installs an editable Designer layout. Later calls preserve it unless
	 * bRebuildLayout is explicitly true. Run setup_combat_hud.py to make a disk backup first.
	 */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Editor", meta = (DevelopmentOnly))
	static bool UpgradeCombatHUD(bool bRebuildLayout = false);

	/** Move only the two existing HUD containers; retain their size, contents and all authored graphs. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Editor", meta = (DevelopmentOnly))
	static bool RepositionCombatHUD();
};
