// Copyright (c) 2026 YYchainsAw. All rights reserved.

#pragma once

#include "CoreMinimal.h"

namespace AesirBossRuntime
{
struct FIdentity
{
	FString ModelId;
	FString ModelSha256;
	FString ReleaseId;
	FString SimulationRevision;
	FString RewardRevision;
};

bool ValidateBundle(
	const FString& Executable,
	const FString& ExpectedExeSHA256,
	const FString& ExpectedManifestSHA256,
	FIdentity& Out,
	FString& Error);
}
