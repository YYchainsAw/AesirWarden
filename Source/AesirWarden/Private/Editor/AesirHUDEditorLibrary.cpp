// Copyright (c) 2026 YYchainsAw. All rights reserved.

#include "Editor/AesirHUDEditorLibrary.h"
#include "AesirWarden.h"

#if WITH_EDITOR
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "ScopedTransaction.h"
#include "UI/AesirCombatHUDWidget.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "WidgetBlueprint.h"

namespace AesirHUDEditor
{
	bool Compile(UBlueprint* Blueprint)
	{
		FCompilerResultsLog Results;
		FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Results);
		if (Results.NumErrors > 0 || Blueprint->Status == BS_Error || !Blueprint->GeneratedClass)
		{
			UE_LOG(LogAesirWarden, Error, TEXT("HUD upgrade: %s failed compilation (%d errors); no asset will be saved."),
				*Blueprint->GetPathName(), Results.NumErrors);
			return false;
		}
		return true;
	}

	bool ValidateLayout(UWidgetBlueprint* Blueprint)
	{
		if (!Blueprint->WidgetTree || !Blueprint->WidgetTree->RootWidget)
		{
			return false;
		}
		auto Require = [Blueprint](const TCHAR* Name, UClass* ExpectedClass)
		{
			UWidget* Widget = Blueprint->WidgetTree->FindWidget(FName(Name));
			if (!Widget || !Widget->IsA(ExpectedClass))
			{
				UE_LOG(LogAesirWarden, Error, TEXT("HUD upgrade: missing/wrong-type Designer control %s (%s). Existing layout is preserved; review it or explicitly request a rebuild."),
					Name, *ExpectedClass->GetName());
				return false;
			}
			return true;
		};
		bool bValid = true;
		for (const TCHAR* Name : {TEXT("PB_PlayerHealth"), TEXT("PB_PlayerHealthTrail"), TEXT("PB_GuardPressure"), TEXT("PB_BossHealth")})
		{
			bValid &= Require(Name, UProgressBar::StaticClass());
		}
		for (const TCHAR* Name : {TEXT("Txt_PlayerHealth"), TEXT("Txt_GuardState"), TEXT("Txt_BossName")})
		{
			bValid &= Require(Name, UTextBlock::StaticClass());
		}
		bValid &= Require(TEXT("BossPanel"), UWidget::StaticClass());
		bValid &= Require(TEXT("HB_Abilities"), UHorizontalBox::StaticClass());
		return bValid;
	}

	void RegisterWidgetGuids(UWidgetBlueprint* Blueprint, bool bRetireRemovedWidgets)
	{
		TSet<FName> Names;
		Blueprint->ForEachSourceWidget([Blueprint, &Names](UWidget* Widget)
		{
			Names.Add(Widget->GetFName());
			// UE 5.8 expects GUIDs even for non-variable layout containers. Keep the
			// original PB_PlayerHealth GUID so old Get-variable graph nodes survive.
			if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
			{
				Blueprint->OnVariableAdded(Widget->GetFName());
			}
		});
		for (UWidgetAnimation* Animation : Blueprint->Animations)
		{
			if (Animation)
			{
				Names.Add(Animation->GetFName());
				if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Animation->GetFName()))
				{
					Blueprint->OnVariableAdded(Animation->GetFName());
				}
			}
		}
		if (bRetireRemovedWidgets)
		{
			for (auto It = Blueprint->WidgetVariableNameToGuidMap.CreateIterator(); It; ++It)
			{
				if (!Names.Contains(It.Key()))
				{
					It.RemoveCurrent();
				}
			}
		}
	}
}
#endif

bool UAesirHUDEditorLibrary::UpgradeCombatHUD(bool bRebuildLayout)
{
#if WITH_EDITOR
	using namespace AesirHUDEditor;
	if (GEditor && GEditor->PlayWorld)
	{
		UE_LOG(LogAesirWarden, Error, TEXT("HUD upgrade: stop Play/Simulate before changing a Widget Blueprint parent."));
		return false;
	}
	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/Aesir/UI/HealthBars/WBP_CombatHUD.WBP_CombatHUD"), nullptr, LOAD_NoWarn);
	UBlueprint* PlayerBlueprint = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/Aesir/Characters/Player/BP_AesirPlayerCharacter.BP_AesirPlayerCharacter"), nullptr, LOAD_NoWarn);
	if (!Blueprint || !PlayerBlueprint || !Blueprint->ParentClass)
	{
		UE_LOG(LogAesirWarden, Error, TEXT("HUD upgrade: expected the existing WBP_CombatHUD and BP_AesirPlayerCharacter. No replacement asset was created."));
		return false;
	}
	UClass* NativeParent = UAesirCombatHUDWidget::StaticClass();
	if (Blueprint->ParentClass != UUserWidget::StaticClass() && Blueprint->ParentClass != NativeParent)
	{
		UE_LOG(LogAesirWarden, Error, TEXT("HUD upgrade: unexpected parent %s; refusing to remove a custom parent."), *Blueprint->ParentClass->GetPathName());
		return false;
	}
	UPackage* Package = Blueprint->GetOutermost();
	if (Package->IsDirty() || PlayerBlueprint->GetOutermost()->IsDirty())
	{
		UE_LOG(LogAesirWarden, Error, TEXT("HUD upgrade: save the HUD and Player Blueprint before migration so the Python backup includes current edits."));
		return false;
	}
	if (!Compile(Blueprint) || !Compile(PlayerBlueprint))
	{
		UE_LOG(LogAesirWarden, Error, TEXT("HUD upgrade: fix pre-existing Blueprint compile errors before migrating."));
		return false;
	}

	UClass* OriginalParent = Blueprint->ParentClass;
	TStrongObjectPtr<UWidgetTree> OriginalTree(Blueprint->WidgetTree);
	const TMap<FName, FGuid> OriginalGuids = Blueprint->WidgetVariableNameToGuidMap;
	const TArray<FDelegateEditorBinding> OriginalBindings = Blueprint->Bindings;
	const bool bInstallLayout = bRebuildLayout || OriginalParent != NativeParent ||
		!Blueprint->WidgetTree || !Blueprint->WidgetTree->RootWidget;
	const FScopedTransaction Transaction(NSLOCTEXT("AesirHUDEditor", "UpgradeHUD", "Upgrade Aesir combat HUD"));
	Blueprint->Modify();

	auto RestoreOriginal = [&]()
	{
		Blueprint->ParentClass = OriginalParent;
		Blueprint->WidgetTree = OriginalTree.Get();
		Blueprint->WidgetVariableNameToGuidMap = OriginalGuids;
		Blueprint->Bindings = OriginalBindings;
		FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		const bool bRestored = Compile(Blueprint) && Compile(PlayerBlueprint);
		Package->SetDirtyFlag(false);
		UE_LOG(LogAesirWarden, Error, TEXT("HUD upgrade: restored the original parent/tree in memory (%s). Nothing was intentionally saved; the Python disk backup remains the recovery source."),
			bRestored ? TEXT("compile OK") : TEXT("inspect Blueprint errors before continuing"));
	};

	Blueprint->ParentClass = NativeParent;
	if (bInstallLayout)
	{
		Blueprint->WidgetTree = NewObject<UWidgetTree>(Blueprint,
			MakeUniqueObjectName(Blueprint, UWidgetTree::StaticClass(), TEXT("CombatHUDWidgetTree")), RF_Transactional);
		UAesirCombatHUDWidget::BuildDefaultHUDTree(Blueprint->WidgetTree);
	}
	if (!ValidateLayout(Blueprint))
	{
		RestoreOriginal();
		return false;
	}
	// This is the only old generated widget variable that the original health graph requires.
	Blueprint->WidgetTree->FindWidget(TEXT("PB_PlayerHealth"))->bIsVariable = true;
	RegisterWidgetGuids(Blueprint, bInstallLayout);
	FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	if (!Compile(Blueprint) || !Compile(PlayerBlueprint) || !Blueprint->GeneratedClass->IsChildOf(NativeParent))
	{
		RestoreOriginal();
		return false;
	}
	UUserWidget* Defaults = Blueprint->GeneratedClass->GetDefaultObject<UUserWidget>();
	Defaults->DesignTimeSize = FVector2D(1920.0f, 1080.0f);
	Defaults->DesignSizeMode = EDesignPreviewSizeMode::FillScreen;
	Blueprint->MarkPackageDirty();
	const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	if (!UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs))
	{
		UE_LOG(LogAesirWarden, Error, TEXT("HUD upgrade: could not save %s. Use Saved/HUD/BeforeInstall if disk recovery is needed."), *Filename);
		RestoreOriginal();
		return false;
	}
	UE_LOG(LogAesirWarden, Log, TEXT("HUD upgrade: saved %s, %s Designer layout. Existing graphs and Player Blueprint asset reference are retained; no extra HUD is created."),
		*Blueprint->GetPathName(), bInstallLayout ? TEXT("installed editable") : TEXT("preserved user"));
	return true;
#else
	return false;
#endif
}
