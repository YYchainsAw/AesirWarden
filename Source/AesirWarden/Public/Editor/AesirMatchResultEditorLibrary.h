// Copyright (c) 2026 YYchainsAw. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AesirMatchResultEditorLibrary.generated.h"

/** Upgrade the existing result page without replacing its authored layout or graphs. */
UCLASS()
class AESIRWARDEN_API UAesirMatchResultEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Run setup_match_results.py first to retain a hashed package backup. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Editor", meta = (DevelopmentOnly))
	static bool UpgradeMatchResultWidget();

	/** Save only the existing external actor package after the setup script has backed it up. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Editor", meta = (DevelopmentOnly))
	static bool SaveMatchBossTag(AActor* Boss, const FString& ExpectedPackage);
};
