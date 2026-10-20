// Copyright (c) 2026 YYchainsAw. All rights reserved. Third-party assets belong to their respective owners.

#include "Editor/AesirStartMenuEditorLibrary.h"
#include "AesirWarden.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/StaticMesh.h"
#include "Framework/Flow/AesirCampfireStart.h"
#include "HAL/FileManager.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UI/AesirMainMenuWidget.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace AesirStartMenuEditor
{
	UBlueprint* LoadOrCreateBlueprint(const FString& PackageName, UClass* ParentClass,
		const TSubclassOf<UBlueprint> BlueprintType, const TSubclassOf<UBlueprintGeneratedClass> GeneratedType,
		bool& bOutCreated)
	{
		bOutCreated = false;
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackageName);
		const FString ObjectPath = PackageName + TEXT(".") + AssetName;
		if (UObject* Existing = LoadObject<UObject>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn))
		{
			UBlueprint* ExistingBlueprint = Cast<UBlueprint>(Existing);
			if (!ExistingBlueprint || !ExistingBlueprint->IsA(BlueprintType.Get()) ||
				!ExistingBlueprint->ParentClass || !ExistingBlueprint->ParentClass->IsChildOf(ParentClass))
			{
				UE_LOG(LogAesirWarden, Error, TEXT("Start menu scaffold: %s already exists with an incompatible type or parent; preserving it."), *ObjectPath);
				return nullptr;
			}
			UE_LOG(LogAesirWarden, Log, TEXT("Start menu scaffold: preserving existing asset %s."), *ObjectPath);
			return ExistingBlueprint;
		}
		if (FPackageName::DoesPackageExist(PackageName))
		{
			UE_LOG(LogAesirWarden, Error, TEXT("Start menu scaffold: package %s exists but could not be loaded; preserving it."), *PackageName);
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackageName);
		UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(ParentClass, Package, FName(*AssetName),
			BPTYPE_Normal, BlueprintType, GeneratedType, TEXT("AesirStartMenuScaffold"));
		if (!Blueprint)
		{
			UE_LOG(LogAesirWarden, Error, TEXT("Start menu scaffold: failed to create %s."), *ObjectPath);
			return nullptr;
		}
		FAssetRegistryModule::AssetCreated(Blueprint);
		Blueprint->MarkPackageDirty();
		bOutCreated = true;
		return Blueprint;
	}

	bool CompileBlueprint(UBlueprint* Blueprint)
	{
		FCompilerResultsLog Results;
		FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Results);
		if (Results.NumErrors > 0 || Blueprint->Status == BS_Error || !Blueprint->GeneratedClass)
		{
			UE_LOG(LogAesirWarden, Error, TEXT("Start menu scaffold: %s failed Blueprint compilation (%d errors)."),
				*Blueprint->GetPathName(), Results.NumErrors);
			return false;
		}
		UE_LOG(LogAesirWarden, Log, TEXT("Start menu scaffold: compiled %s (status %d, %d errors)."),
			*Blueprint->GetPathName(), static_cast<int32>(Blueprint->Status), Results.NumErrors);
		return true;
	}

	bool SaveBlueprint(UBlueprint* Blueprint)
	{
		UPackage* Package = Blueprint->GetOutermost();
		const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		if (!UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs))
		{
			UE_LOG(LogAesirWarden, Error, TEXT("Start menu scaffold: could not save %s."), *Filename);
			return false;
		}
		UE_LOG(LogAesirWarden, Log, TEXT("Start menu scaffold: saved %s."), *Blueprint->GetPathName());
		return true;
	}
}
#endif

bool UAesirStartMenuEditorLibrary::CreateStartMenuAssets()
{
#if WITH_EDITOR
	using namespace AesirStartMenuEditor;

	bool bCreatedWidget = false;
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(LoadOrCreateBlueprint(
		TEXT("/Game/Aesir/UI/Menus/WBP_CampfireMainMenu"), UAesirMainMenuWidget::StaticClass(),
		UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass(), bCreatedWidget));
	if (!WidgetBlueprint)
	{
		return false;
	}
	if (bCreatedWidget)
	{
		if (!WidgetBlueprint->WidgetTree)
		{
			WidgetBlueprint->WidgetTree = NewObject<UWidgetTree>(WidgetBlueprint, TEXT("WidgetTree"), RF_Transactional);
		}
		UAesirMainMenuWidget::BuildDefaultMenuTree(WidgetBlueprint->WidgetTree);
	}
	// UE 5.8 tracks every source widget, including non-variable layout containers.
	// Repair metadata without replacing a user-authored Designer tree.
	bool bRepairedWidgetGuids = false;
	WidgetBlueprint->ForEachSourceWidget([WidgetBlueprint, &bRepairedWidgetGuids](UWidget* Widget)
	{
		if (!WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
		{
			WidgetBlueprint->OnVariableAdded(Widget->GetFName());
			bRepairedWidgetGuids = true;
		}
	});
	if (!CompileBlueprint(WidgetBlueprint) || ((bCreatedWidget || bRepairedWidgetGuids) && !SaveBlueprint(WidgetBlueprint)))
	{
		return false;
	}

	bool bCreatedActor = false;
	UBlueprint* CampfireBlueprint = LoadOrCreateBlueprint(
		TEXT("/Game/Aesir/Framework/Flow/BP_AesirCampfireStart"), AAesirCampfireStart::StaticClass(),
		UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), bCreatedActor);
	if (!CampfireBlueprint || !CompileBlueprint(CampfireBlueprint))
	{
		return false;
	}
	if (bCreatedActor)
	{
		AAesirCampfireStart* Defaults = Cast<AAesirCampfireStart>(CampfireBlueprint->GeneratedClass->GetDefaultObject());
		if (!Defaults)
		{
			UE_LOG(LogAesirWarden, Error, TEXT("Start menu scaffold: invalid campfire Blueprint default object."));
			return false;
		}
		Defaults->MainMenuWidgetClass = WidgetBlueprint->GeneratedClass;
		if (UClass* HUDClass = LoadClass<UUserWidget>(nullptr,
			TEXT("/Game/Aesir/UI/HealthBars/WBP_CombatHUD.WBP_CombatHUD_C"), nullptr, LOAD_NoWarn))
		{
			Defaults->GameplayHUDWidgetClasses.AddUnique(HUDClass);
		}
		UStaticMesh* FlameMesh = LoadObject<UStaticMesh>(nullptr,
			TEXT("/Game/CustomAssets/FX/Candles_FX/SM_FlameCard_01_N1.SM_FlameCard_01_N1"), nullptr, LOAD_NoWarn);
		if (FlameMesh)
		{
			Defaults->ConfigureFlameMesh(FlameMesh);
		}
		else
		{
			UE_LOG(LogAesirWarden, Warning, TEXT("Start menu scaffold: local flame card was not found; the campfire keeps its geometry placeholder."));
		}
		CampfireBlueprint->MarkPackageDirty();
		if (!SaveBlueprint(CampfireBlueprint))
		{
			return false;
		}
	}
	return true;
#else
	UE_LOG(LogAesirWarden, Warning, TEXT("Start menu asset creation is available only in the editor."));
	return false;
#endif
}
