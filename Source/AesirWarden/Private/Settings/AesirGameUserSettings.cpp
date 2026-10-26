// Copyright (c) 2026 YYchainsAw. All rights reserved.

#include "Settings/AesirGameUserSettings.h"

#include "AudioDevice.h"
#include "AudioDeviceHandle.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UAesirGameUserSettings::UAesirGameUserSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The base constructor only dispatches its own SetToDefaults implementation.
	SetToDefaults();
}

UAesirGameUserSettings* UAesirGameUserSettings::Get()
{
	return GEngine ? Cast<UAesirGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

float UAesirGameUserSettings::GetMasterVolume() const
{
	return FMath::IsFinite(MasterVolume) ? FMath::Clamp(MasterVolume, 0.0f, 1.0f) : 1.0f;
}

void UAesirGameUserSettings::SetMasterVolume(float Value)
{
	MasterVolume = FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.0f, 1.0f) : 1.0f;
}

float UAesirGameUserSettings::GetMouseSensitivity() const
{
	return FMath::IsFinite(MouseSensitivity) ? FMath::Clamp(MouseSensitivity, 0.1f, 3.0f) : 1.0f;
}

void UAesirGameUserSettings::SetMouseSensitivity(float Value)
{
	MouseSensitivity = FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.1f, 3.0f) : 1.0f;
}

void UAesirGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();
	MasterVolume = 1.0f;
	MouseSensitivity = 1.0f;
	bInvertMouseY = false;
	SetFrameRateLimit(60.0f);
}

void UAesirGameUserSettings::ValidateSettings()
{
	Super::ValidateSettings();
	SetMasterVolume(MasterVolume);
	SetMouseSensitivity(MouseSensitivity);
}

void UAesirGameUserSettings::ApplyRuntimeSettings(UWorld* World)
{
	if (!IsValid(World) || !World->IsGameWorld())
	{
		return;
	}
	// Keep the device alive while queuing the thread-safe audio command. No
	// SoundClass routing or editor-global volume is changed by a game-world call.
	FAudioDeviceHandle AudioDevice = World->GetAudioDevice();
	if (AudioDevice)
	{
		AudioDevice->SetTransientPrimaryVolume(GetMasterVolume());
	}
}

void UAesirGameUserSettings::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();
	if (!GEngine)
	{
		return;
	}
	bool bHasGameWorld = false;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* World = Context.World();
		if (IsValid(World) && World->IsGameWorld())
		{
			bHasGameWorld = true;
			ApplyRuntimeSettings(World);
		}
	}
	// UGameEngine loads non-resolution settings before constructing its first
	// game world. The player's BeginPlay reapplies them to each new world/device.
	if (!bHasGameWorld && !GIsEditor)
	{
		FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice();
		if (AudioDevice)
		{
			AudioDevice->SetTransientPrimaryVolume(GetMasterVolume());
		}
	}
}
