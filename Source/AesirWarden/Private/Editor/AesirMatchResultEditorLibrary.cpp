// Copyright (c) 2026 YYchainsAw. All rights reserved.

#include "Editor/AesirMatchResultEditorLibrary.h"
#include "AesirWarden.h"

#if WITH_EDITOR
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Editor.h"
#include "Characters/Enemy/AesirEnemyCharacter.h"
#include "Engine/Blueprint.h"
#include "Engine/Font.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UI/AesirCombatResultWidget.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "WidgetBlueprint.h"

namespace AesirMatchResultEditor
{
bool Compile(UBlueprint* Blueprint)
{
	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Results);
	return Results.NumErrors == 0 && Blueprint->Status != BS_Error && Blueprint->GeneratedClass;
}
}
#endif

bool UAesirMatchResultEditorLibrary::UpgradeMatchResultWidget()
{
#if WITH_EDITOR
	using namespace AesirMatchResultEditor;
	if (GEditor && GEditor->PlayWorld)
	{
		UE_LOG(LogAesirWarden, Error, TEXT("Result upgrade: stop Play/Simulate before migrating the result widget."));
		return false;
	}
	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/Aesir/UI/Results/WBP_CombatResult.WBP_CombatResult"));
	UBlueprint* GameMode = LoadObject<UBlueprint>(nullptr,
		TEXT("/Game/Aesir/Framework/BP_AesirGameMode.BP_AesirGameMode"));
	UClass* NativeParent = UAesirCombatResultWidget::StaticClass();
	if (!Blueprint || !GameMode || !Blueprint->WidgetTree || !Blueprint->WidgetTree->RootWidget ||
		!Blueprint->ParentClass || (Blueprint->ParentClass != UUserWidget::StaticClass() && Blueprint->ParentClass != NativeParent))
	{
		UE_LOG(LogAesirWarden, Error, TEXT("Result upgrade: expected the existing result page with UserWidget/native parent and an authored layout."));
		return false;
	}
	UPackage* Package = Blueprint->GetOutermost();
	if (Package->IsDirty() || GameMode->GetOutermost()->IsDirty())
	{
		UE_LOG(LogAesirWarden, Error, TEXT("Result upgrade: save existing result/GameMode edits before taking the on-disk backup."));
		return false;
	}
	if (!Compile(Blueprint) || !Compile(GameMode))
	{
		UE_LOG(LogAesirWarden, Error, TEXT("Result upgrade: pre-existing Blueprint compile errors; no package saved."));
		return false;
	}
	if (!Cast<UTextBlock>(Blueprint->WidgetTree->FindWidget(TEXT("TXT_Result"))) ||
		!Cast<UButton>(Blueprint->WidgetTree->FindWidget(TEXT("BTN_Restart"))) ||
		!Cast<UVerticalBox>(Blueprint->WidgetTree->FindWidget(TEXT("VerticalBox_Result"))))
	{
		UE_LOG(LogAesirWarden, Error, TEXT("Result upgrade: expected TXT_Result, BTN_Restart and VerticalBox_Result. No replacement layout created."));
		return false;
	}
	for (const TCHAR* Name : {TEXT("BTN_ReturnToMenu"), TEXT("BTN_Quit")})
	{
		if (UWidget* Existing = Blueprint->WidgetTree->FindWidget(Name); Existing && !Existing->IsA<UButton>())
		{
			UE_LOG(LogAesirWarden, Error, TEXT("Result upgrade: %s is already used by a non-button control."), Name);
			return false;
		}
	}

	UClass* OriginalParent = Blueprint->ParentClass;
	TStrongObjectPtr<UWidgetTree> OriginalTree(Blueprint->WidgetTree);
	const TMap<FName, FGuid> OriginalGuids = Blueprint->WidgetVariableNameToGuidMap;
	const TArray<FDelegateEditorBinding> OriginalBindings = Blueprint->Bindings;
	Blueprint->Modify();
	Blueprint->WidgetTree = DuplicateObject<UWidgetTree>(OriginalTree.Get(), Blueprint,
		MakeUniqueObjectName(Blueprint, UWidgetTree::StaticClass(), TEXT("MatchResultWidgetTree")));
	Blueprint->ParentClass = NativeParent;
	auto Restore = [&]()
	{
		Blueprint->ParentClass = OriginalParent;
		Blueprint->WidgetTree = OriginalTree.Get();
		Blueprint->WidgetVariableNameToGuidMap = OriginalGuids;
		Blueprint->Bindings = OriginalBindings;
		FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		Compile(Blueprint);
		Compile(GameMode);
		Package->SetDirtyFlag(false);
		UE_LOG(LogAesirWarden, Error, TEXT("Result upgrade: original parent/tree restored in memory. Retain the setup script's disk backup for recovery."));
	};
	UVerticalBox* Actions = CastChecked<UVerticalBox>(Blueprint->WidgetTree->FindWidget(TEXT("VerticalBox_Result")));
	UButton* Restart = CastChecked<UButton>(Blueprint->WidgetTree->FindWidget(TEXT("BTN_Restart")));
	UTextBlock* Result = CastChecked<UTextBlock>(Blueprint->WidgetTree->FindWidget(TEXT("TXT_Result")));
	if (UWidget* Background = Blueprint->WidgetTree->FindWidget(TEXT("Border_Background"));
		Background && Background->GetParent() == Actions->GetParent())
	{
		Background->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (auto* Slot = Cast<UCanvasPanelSlot>(Background->Slot))
		{
			Slot->SetZOrder(0);
		}
		if (auto* Slot = Cast<UCanvasPanelSlot>(Actions->Slot))
		{
			Slot->SetZOrder(1);
		}
	}
	UFont* Font = LoadObject<UFont>(nullptr, TEXT("/Game/Aesir/UI/Fonts/F_MenuSerif.F_MenuSerif"), nullptr, LOAD_NoWarn);
	auto ConfigureAction = [&](FName Name, FText Text, int32 Index)
	{
		UButton* Button = Cast<UButton>(Blueprint->WidgetTree->FindWidget(Name));
		if (!Button)
		{
			Button = Blueprint->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
			Button->SetStyle(Restart->GetStyle());
			Button->SetBackgroundColor(Restart->GetBackgroundColor());
		}
		UTextBlock* Label = Cast<UTextBlock>(Button->GetContent());
		if (!Label)
		{
			Label = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
				FName(*(Name.ToString() + TEXT("_Label"))));
			Button->SetContent(Label);
		}
		Label->SetText(Text);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.88f, 0.75f)));
		FSlateFontInfo Style = Label->GetFont();
		Style.Size = 32.0f;
		if (Font)
		{
			Style.FontObject = Font;
			Style.TypefaceFontName = TEXT("Regular");
		}
		Label->SetFont(Style);
		Label->SetJustification(ETextJustify::Center);
		Button->RemoveFromParent();
		auto* Slot = CastChecked<UVerticalBoxSlot>(Actions->InsertChildAt(Index, Button));
		Slot->SetPadding(FMargin(0.0f, 6.0f));
	};
	const int32 FirstAction = Actions->GetChildIndex(Result) + 1;
	ConfigureAction(TEXT("BTN_Restart"), NSLOCTEXT("AesirResult", "Retry", "重试"), FirstAction);
	ConfigureAction(TEXT("BTN_ReturnToMenu"), NSLOCTEXT("AesirResult", "ReturnToMenu", "返回菜单"), FirstAction + 1);
	ConfigureAction(TEXT("BTN_Quit"), NSLOCTEXT("AesirResult", "Quit", "退出游戏"), FirstAction + 2);
	Blueprint->ForEachSourceWidget([Blueprint](UWidget* Widget)
	{
		if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName())) { Blueprint->OnVariableAdded(Widget->GetFName()); }
	});
	FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	if (!Compile(Blueprint) || !Compile(GameMode) || !Blueprint->GeneratedClass->IsChildOf(NativeParent))
	{
		Restore();
		return false;
	}
	Blueprint->GeneratedClass->GetDefaultObject<UUserWidget>()->SetIsFocusable(true);
	Blueprint->MarkPackageDirty();
	const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	if (!UPackage::SavePackage(Package, Blueprint, *Filename, Args))
	{
		Restore();
		return false;
	}
	UE_LOG(LogAesirWarden, Display, TEXT("Result upgrade: saved %s; original layout/graphs and GameMode asset retained, native result actions installed."), *Blueprint->GetPathName());
	return true;
#else
	return false;
#endif
}

bool UAesirMatchResultEditorLibrary::SaveMatchBossTag(AActor* Boss, const FString& ExpectedPackage)
{
#if WITH_EDITOR
	if (!IsValid(Cast<AAesirEnemyCharacter>(Boss)) || !Boss->GetWorld() ||
		Boss->GetWorld()->WorldType != EWorldType::Editor)
	{
		return false;
	}
	UPackage* Package = Boss->GetExternalPackage();
	if (!Package || Package->GetName() != ExpectedPackage || Package->IsDirty())
	{
		return false;
	}
	const FName MatchTag(TEXT("Aesir.MatchBoss"));
	if (Boss->ActorHasTag(MatchTag))
	{
		return true;
	}
	Boss->Modify();
	Boss->Tags.AddUnique(MatchTag);
	Package->MarkPackageDirty();
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Standalone;
	const FString Filename = FPackageName::LongPackageNameToFilename(ExpectedPackage, FPackageName::GetAssetPackageExtension());
	if (UPackage::SavePackage(Package, Boss, *Filename, Args))
	{
		return true;
	}
	Boss->Tags.Remove(MatchTag);
	Package->SetDirtyFlag(false);
#endif
	return false;
}
