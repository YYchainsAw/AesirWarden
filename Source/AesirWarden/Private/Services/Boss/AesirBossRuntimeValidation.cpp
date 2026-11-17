// Copyright (c) 2026 YYchainsAw. All rights reserved.

#include "Services/Boss/AesirBossRuntimeValidation.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <bcrypt.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace AesirBossRuntime
{
namespace
{
bool IsSHA256(const FString& Value)
{
	if (Value.Len() != 64) return false;
	for (TCHAR Character : Value)
	{
		if (!FChar::IsHexDigit(Character)) return false;
	}
	return true;
}

#if PLATFORM_WINDOWS
bool HashFile(const FString& Path, FString& Hash, FString& Error, TArray<uint8>* Contents = nullptr)
{
	TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*Path, FILEREAD_Silent));
	if (!Reader || Reader->TotalSize() < 0)
	{
		Error = FString::Printf(TEXT("Cannot read runtime file: %s"), *Path);
		return false;
	}

	BCRYPT_ALG_HANDLE Algorithm = nullptr;
	NTSTATUS Status = BCryptOpenAlgorithmProvider(&Algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
	if (Status < 0)
	{
		Error = FString::Printf(TEXT("Cannot initialize runtime SHA256 provider (0x%08X)."), static_cast<uint32>(Status));
		return false;
	}
	ON_SCOPE_EXIT { BCryptCloseAlgorithmProvider(Algorithm, 0); };

	BCRYPT_HASH_HANDLE Hasher = nullptr;
	Status = BCryptCreateHash(Algorithm, &Hasher, nullptr, 0, nullptr, 0, 0);
	if (Status < 0)
	{
		Error = FString::Printf(TEXT("Cannot initialize runtime SHA256 hash (0x%08X)."), static_cast<uint32>(Status));
		return false;
	}
	ON_SCOPE_EXIT { BCryptDestroyHash(Hasher); };

	TArray<uint8> Buffer;
	Buffer.SetNumUninitialized(1024 * 1024);
	const int64 FileSize = Reader->TotalSize();
	if (Contents)
	{
		if (FileSize > MAX_int32)
		{
			Error = TEXT("Runtime manifest is too large to parse.");
			return false;
		}
		Contents->Reset();
	}
	for (int64 Remaining = FileSize; Remaining > 0;)
	{
		const int32 Bytes = static_cast<int32>(FMath::Min<int64>(Remaining, Buffer.Num()));
		Reader->Serialize(Buffer.GetData(), Bytes);
		if (Reader->IsError())
		{
			Error = FString::Printf(TEXT("Cannot finish reading runtime file: %s"), *Path);
			return false;
		}
		Status = BCryptHashData(Hasher, Buffer.GetData(), static_cast<ULONG>(Bytes), 0);
		if (Status < 0)
		{
			Error = FString::Printf(TEXT("Cannot hash runtime file (0x%08X): %s"), static_cast<uint32>(Status), *Path);
			return false;
		}
		if (Contents) Contents->Append(Buffer.GetData(), Bytes);
		Remaining -= Bytes;
	}

	uint8 Digest[32];
	Status = BCryptFinishHash(Hasher, Digest, sizeof(Digest), 0);
	if (Status < 0)
	{
		Error = FString::Printf(TEXT("Cannot finalize runtime SHA256 hash (0x%08X)."), static_cast<uint32>(Status));
		return false;
	}
	Hash = BytesToHex(Digest, UE_ARRAY_COUNT(Digest));
	return true;
}
#endif
}

bool ValidateBundle(
	const FString& Executable,
	const FString& ExpectedExeSHA256,
	const FString& ExpectedManifestSHA256,
	FIdentity& Out,
	FString& Error)
{
	Out = FIdentity();
	Error.Reset();
#if PLATFORM_WINDOWS
	if (!IsSHA256(ExpectedExeSHA256) || !IsSHA256(ExpectedManifestSHA256))
	{
		Error = TEXT("Executable and manifest SHA256 settings must each contain 64 hexadecimal characters.");
		return false;
	}

	FString ActualHash;
	if (!HashFile(Executable, ActualHash, Error)) return false;
	if (!ActualHash.Equals(ExpectedExeSHA256, ESearchCase::IgnoreCase))
	{
		Error = TEXT("Boss runtime executable SHA256 does not match the configured release.");
		return false;
	}

	const FString ManifestPath = FPaths::Combine(FPaths::GetPath(Executable), TEXT("release-manifest.json"));
	TArray<uint8> ManifestBytes;
	if (!HashFile(ManifestPath, ActualHash, Error, &ManifestBytes)) return false;
	if (!ActualHash.Equals(ExpectedManifestSHA256, ESearchCase::IgnoreCase))
	{
		Error = TEXT("Boss runtime manifest SHA256 does not match the configured release.");
		return false;
	}

	// Parse the same bytes that were hashed, even if the file is replaced later.
	FString ManifestText;
	FFileHelper::BufferToString(ManifestText, ManifestBytes.GetData(), ManifestBytes.Num());
	TSharedPtr<FJsonObject> Manifest;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ManifestText), Manifest) || !Manifest.IsValid())
	{
		Error = TEXT("Boss runtime release manifest is not valid JSON.");
		return false;
	}
	FString Format;
	if (!Manifest->TryGetStringField(TEXT("format"), Format) || Format != TEXT("aesir-boss-windows-onedir-v1"))
	{
		Error = TEXT("Boss runtime release manifest format must be aesir-boss-windows-onedir-v1.");
		return false;
	}
	const TSharedPtr<FJsonObject>* Model = nullptr;
	if (!Manifest->TryGetObjectField(TEXT("model"), Model) || !Model || !Model->IsValid())
	{
		Error = TEXT("Boss runtime release manifest has no model object.");
		return false;
	}

	double Schema = 0.0;
	double ObservationDimension = 0.0;
	double ActionCount = 0.0;
	if (!(*Model)->TryGetNumberField(TEXT("schema_version"), Schema) || Schema != 4.0 ||
		!(*Model)->TryGetNumberField(TEXT("observation_dimension"), ObservationDimension) || ObservationDimension != 26.0 ||
		!(*Model)->TryGetNumberField(TEXT("action_count"), ActionCount) || ActionCount != 9.0)
	{
		Error = TEXT("Boss runtime model must use schema 4, 26 observation features and 9 actions.");
		return false;
	}

	FIdentity Identity;
	auto ReadIdentity = [&Model, &Error](const TCHAR* Field, FString& Value)
	{
		if (!(*Model)->TryGetStringField(Field, Value) || Value.TrimStartAndEnd().IsEmpty())
		{
			Error = FString::Printf(TEXT("Boss runtime manifest model field %s must be a non-empty string."), Field);
			return false;
		}
		return true;
	};
	if (!ReadIdentity(TEXT("model_id"), Identity.ModelId) ||
		!ReadIdentity(TEXT("model_sha256"), Identity.ModelSha256) ||
		!ReadIdentity(TEXT("release_id"), Identity.ReleaseId) ||
		!ReadIdentity(TEXT("simulation_revision"), Identity.SimulationRevision) ||
		!ReadIdentity(TEXT("reward_revision"), Identity.RewardRevision))
	{
		return false;
	}
	if (!IsSHA256(Identity.ModelSha256))
	{
		Error = TEXT("Boss runtime manifest model_sha256 must contain 64 hexadecimal characters.");
		return false;
	}
	Identity.ModelSha256.ToLowerInline();
	Out = MoveTemp(Identity);
	return true;
#else
	Error = TEXT("Managed Boss runtime bundles require 64-bit Windows.");
	return false;
#endif
}
}
