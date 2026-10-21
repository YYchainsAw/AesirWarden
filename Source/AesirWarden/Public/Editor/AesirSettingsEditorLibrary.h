// Copyright (c) 2026 YYchainsAw. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AesirSettingsEditorLibrary.generated.h"

/** Creates the separate editable settings menu without touching the authored main menu. */
UCLASS()
class AESIRWARDEN_API UAesirSettingsEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Create once, then validate and preserve the existing Designer tree and defaults. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Editor", meta = (DevelopmentOnly))
	static bool CreateSettingsMenuAsset();
};
