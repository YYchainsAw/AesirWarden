// Copyright (c) 2026 YYchainsAw. All rights reserved. Third-party assets belong to their respective owners.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AesirStartMenuEditorLibrary.generated.h"

/** Editor scaffolding only; generated assets remain editable in Blueprint and UMG Designer. */
UCLASS()
class AESIRWARDEN_API UAesirStartMenuEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Create the campfire actor and menu assets once, preserving existing user-authored assets. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Editor", meta = (DevelopmentOnly))
	static bool CreateStartMenuAssets();

	/** Explicitly replace only the menu Designer tree with the transparent golden-section layout. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Editor", meta = (DevelopmentOnly))
	static bool RebuildCampfireMenuLayout();
};
