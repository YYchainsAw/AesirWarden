// Copyright (c) 2026 YYchainsAw. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "AesirGameUserSettings.generated.h"

class UWorld;

/** Persistent player preferences. Setters stage values; applying/saving belongs to the settings UI. */
UCLASS(Config = GameUserSettings, BlueprintType)
class AESIRWARDEN_API UAesirGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UAesirGameUserSettings(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "Aesir|Settings")
	static UAesirGameUserSettings* Get();

	UFUNCTION(BlueprintPure, Category = "Aesir|Settings|Audio")
	float GetMasterVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Aesir|Settings|Audio")
	void SetMasterVolume(float Value);

	UFUNCTION(BlueprintPure, Category = "Aesir|Settings|Controls")
	float GetMouseSensitivity() const;

	UFUNCTION(BlueprintCallable, Category = "Aesir|Settings|Controls")
	void SetMouseSensitivity(float Value);

	UFUNCTION(BlueprintPure, Category = "Aesir|Settings|Controls")
	bool GetInvertMouseY() const { return bInvertMouseY; }

	UFUNCTION(BlueprintCallable, Category = "Aesir|Settings|Controls")
	void SetInvertMouseY(bool bValue) { bInvertMouseY = bValue; }

	/** Apply the current volume to this game world's audio device without saving any settings. */
	UFUNCTION(BlueprintCallable, Category = "Aesir|Settings", meta = (WorldContext = "World"))
	void ApplyRuntimeSettings(UWorld* World);

	virtual void SetToDefaults() override;
	virtual void ValidateSettings() override;
	virtual void ApplyNonResolutionSettings() override;

private:
	UPROPERTY(Config)
	float MasterVolume = 1.0f;

	UPROPERTY(Config)
	float MouseSensitivity = 1.0f;

	UPROPERTY(Config)
	bool bInvertMouseY = false;
};
