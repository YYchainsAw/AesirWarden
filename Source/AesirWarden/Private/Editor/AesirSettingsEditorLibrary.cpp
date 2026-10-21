// Copyright (c) 2026 YYchainsAw. All rights reserved.

#include "Editor/AesirSettingsEditorLibrary.h"
#include "AesirWarden.h"

#if WITH_EDITOR
#include "Animation/WidgetAnimation.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Editor.h"
#include "HAL/FileManager.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UI/AesirSettingsMenuWidget.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace AesirSettingsEditor
{
	bool Validate(UWidgetBlueprint* Blueprint)
	{
		if (!Blueprint->WidgetTree || !Blueprint->WidgetTree->RootWidget) return false;
		auto Require = [Blueprint](const TCHAR* Name, UClass* Type)
		{
			UWidget* Widget = Blueprint->WidgetTree->FindWidget(FName(Name));
			if (Widget && Widget->IsA(Type)) return true;
			UE_LOG(LogAesirWarden, Error, TEXT("Settings scaffold: missing or incompatible %s (%s). Existing layout was preserved."), Name, *Type->GetName());
			return false;
		};
		bool bValid = true;
		for (const TCHAR* Name : {TEXT("Btn_TabDisplay"), TEXT("Btn_TabQuality"), TEXT("Btn_TabAudio"), TEXT("Btn_TabControls"),
			TEXT("Btn_Apply"), TEXT("Btn_Back"), TEXT("Btn_Defaults"), TEXT("Btn_ConfirmYes"), TEXT("Btn_ConfirmNo")})
			bValid &= Require(Name, UButton::StaticClass());
		for (const TCHAR* Name : {TEXT("Txt_TabDisplay"), TEXT("Txt_TabQuality"), TEXT("Txt_TabAudio"), TEXT("Txt_TabControls"),
			TEXT("Txt_VolumeValue"), TEXT("Txt_SensitivityValue"), TEXT("Txt_SettingsStatus"), TEXT("Txt_Confirmation"),
			TEXT("Txt_ConfirmYes"), TEXT("Txt_ConfirmNo")})
			bValid &= Require(Name, UTextBlock::StaticClass());
		for (const TCHAR* Name : {TEXT("Cmb_WindowMode"), TEXT("Cmb_Resolution"), TEXT("Cmb_FrameLimit"), TEXT("Cmb_OverallQuality"),
			TEXT("Cmb_ShadowQuality"), TEXT("Cmb_TextureQuality"), TEXT("Cmb_EffectsQuality")})
			bValid &= Require(Name, UComboBoxString::StaticClass());
		for (const TCHAR* Name : {TEXT("Chk_VSync"), TEXT("Chk_InvertY")})
			bValid &= Require(Name, UCheckBox::StaticClass());
		for (const TCHAR* Name : {TEXT("Sld_MasterVolume"), TEXT("Sld_MouseSensitivity")})
			bValid &= Require(Name, USlider::StaticClass());
		bValid &= Require(TEXT("SettingsContent"), UWidget::StaticClass());
		bValid &= Require(TEXT("ConfirmPanel"), UWidget::StaticClass());
		bValid &= Require(TEXT("SettingsPages"), UWidgetSwitcher::StaticClass());
		bValid &= Require(TEXT("Blur_SettingsBackdrop"), UBackgroundBlur::StaticClass());
		bValid &= Require(TEXT("Img_SettingsBackdropDim"), UImage::StaticClass());
		if (UWidgetSwitcher* Pages = Cast<UWidgetSwitcher>(Blueprint->WidgetTree->FindWidget(TEXT("SettingsPages"))))
		{
			if (Pages->GetChildrenCount() != 4)
			{
				UE_LOG(LogAesirWarden, Error, TEXT("Settings scaffold: SettingsPages must contain Display, Quality, Audio and Controls in that order."));
				bValid = false;
			}
		}
		return bValid;
	}
}
#endif

bool UAesirSettingsEditorLibrary::CreateSettingsMenuAsset()
{
#if WITH_EDITOR
	if (GEditor && GEditor->PlayWorld)
	{
		UE_LOG(LogAesirWarden, Error, TEXT("Settings scaffold: stop Play/Simulate before editing a Widget Blueprint."));
		return false;
	}
	const FString PackageName(TEXT("/Game/Aesir/UI/Menus/WBP_SettingsMenu"));
	const FString ObjectPath = PackageName + TEXT(".WBP_SettingsMenu");
	UWidgetBlueprint* Blueprint = nullptr;
	bool bCreated = false;
	if (UObject* Existing = LoadObject<UObject>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn))
	{
		Blueprint = Cast<UWidgetBlueprint>(Existing);
		if (!Blueprint || !Blueprint->ParentClass || !Blueprint->ParentClass->IsChildOf(UAesirSettingsMenuWidget::StaticClass()))
		{
			UE_LOG(LogAesirWarden, Error, TEXT("Settings scaffold: incompatible asset at %s; preserving it."), *ObjectPath);
			return false;
		}
		if (Blueprint->GetOutermost()->IsDirty())
		{
			UE_LOG(LogAesirWarden, Error, TEXT("Settings scaffold: save your settings Blueprint edits before running the script again."));
			return false;
		}
	}
	else
	{
		if (FPackageName::DoesPackageExist(PackageName))
		{
			UE_LOG(LogAesirWarden, Error, TEXT("Settings scaffold: existing package could not be loaded; preserving it."));
			return false;
		}
		UPackage* Package = CreatePackage(*PackageName);
		Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UAesirSettingsMenuWidget::StaticClass(), Package,
			TEXT("WBP_SettingsMenu"), BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass(), TEXT("AesirSettingsScaffold")));
		if (!Blueprint) return false;
		FAssetRegistryModule::AssetCreated(Blueprint);
		bCreated = true;
		if (!Blueprint->WidgetTree) Blueprint->WidgetTree = NewObject<UWidgetTree>(Blueprint, TEXT("WidgetTree"), RF_Transactional);
		UAesirSettingsMenuWidget::BuildDefaultSettingsTree(Blueprint->WidgetTree);
	}
	const bool bBackdropAdded = UAesirSettingsMenuWidget::EnsureSettingsBackdrop(Blueprint->WidgetTree);
	if (!AesirSettingsEditor::Validate(Blueprint)) return false;

	// UE 5.8 requires widget GUID metadata for non-variable Designer containers too.
	bool bRepaired = bBackdropAdded;
	Blueprint->ForEachSourceWidget([Blueprint, &bRepaired](UWidget* Widget)
	{
		if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
		{
			Blueprint->OnVariableAdded(Widget->GetFName());
			bRepaired = true;
		}
	});
	for (UWidgetAnimation* Animation : Blueprint->Animations)
	{
		if (Animation && !Blueprint->WidgetVariableNameToGuidMap.Contains(Animation->GetFName()))
		{
			Blueprint->OnVariableAdded(Animation->GetFName());
			bRepaired = true;
		}
	}
	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Results);
	if (Results.NumErrors || Blueprint->Status == BS_Error || !Blueprint->GeneratedClass)
	{
		UE_LOG(LogAesirWarden, Error, TEXT("Settings scaffold: compilation failed (%d errors); no package saved."), Results.NumErrors);
		return false;
	}
	if (bCreated)
	{
		UUserWidget* Defaults = Blueprint->GeneratedClass->GetDefaultObject<UUserWidget>();
		Defaults->SetIsFocusable(true);
		Defaults->DesignTimeSize = FVector2D(1920.0f, 1080.0f);
		Defaults->DesignSizeMode = EDesignPreviewSizeMode::FillScreen;
	}
	if (bCreated || bRepaired)
	{
		Blueprint->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		if (!UPackage::SavePackage(Blueprint->GetOutermost(), Blueprint, *Filename, SaveArgs)) return false;
	}
	UE_LOG(LogAesirWarden, Log, TEXT("Settings scaffold: %s %s, 0 Blueprint errors; existing layouts and main menu preserved."),
		bCreated ? TEXT("created") : TEXT("verified"), *ObjectPath);
	return true;
#else
	return false;
#endif
}
