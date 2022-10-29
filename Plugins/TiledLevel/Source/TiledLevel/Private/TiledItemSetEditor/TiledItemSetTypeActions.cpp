// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledItemSetTypeActions.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "TiledItemSet.h"
#include "TiledItemSetEditor.h"
#include "TiledLevelAsset.h"
#include "TiledLevelModule.h"
#include "TiledLevelStyle.h"
#include "ToolMenuSection.h"
#include "TiledLevelEditor/TiledLevelFactory.h"

#define LOCTEXT_NAMESPACE "TiledItemSetTypeActions"

FTiledItemSetTypeActions::FTiledItemSetTypeActions(EAssetTypeCategories::Type InAssetCategory)
	:MyAssetCategory(InAssetCategory)
{
}

FText FTiledItemSetTypeActions::GetName() const
{
	return LOCTEXT("FTiledItemSetAssetTypeActionsName", "Tiled Item Set");
}

UClass* FTiledItemSetTypeActions::GetSupportedClass() const
{
	return UTiledItemSet::StaticClass();
}

FColor FTiledItemSetTypeActions::GetTypeColor() const
{
	return FColorList::Gold;
}

uint32 FTiledItemSetTypeActions::GetCategories()
{
	return MyAssetCategory;
}

void FTiledItemSetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	for (UObject* Obj : InObjects)
 	{
 		if (UTiledItemSet* TiledElementSet = Cast<UTiledItemSet>(Obj))
 		{
 			TSharedRef<FTiledItemSetEditor> NewTiledElementSetEditor(new FTiledItemSetEditor());
 			NewTiledElementSetEditor->InitTiledItemSetEditor(Mode, EditWithinLevelEditor, TiledElementSet);
 		}		
 	}	
}

void FTiledItemSetTypeActions::GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section)
{
	TArray<TWeakObjectPtr<UTiledItemSet>> TiledItemSets = GetTypedWeakObjectPtrs<UTiledItemSet>(InObjects);

	if (TiledItemSets.Num() == 1)
	{
		Section.AddMenuEntry(
			"TiledItemSet_CreateTiledLevel",
			LOCTEXT("TiledItemSet_CreateTiledLevel", "Create Tiled Level"),
			LOCTEXT("TiledItemSet_CreateTiledLevelTooltip", "Creates a tiled level using the selected tiled item set"),
			FSlateIcon(FTiledLevelStyle::GetStyleSetName(), "ClassIcon.TiledLevelAsset"),
			FUIAction(FExecuteAction::CreateSP(this, &FTiledItemSetTypeActions::ExecuteCreateTiledLevel, TiledItemSets[0]))
			);
	}
}

void FTiledItemSetTypeActions::ExecuteCreateTiledLevel(TWeakObjectPtr<UTiledItemSet> TiledItemSetPtr)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	const FString SuffixToIgnore(TEXT("Set"));
	const FString TileMapSuffix(TEXT("Level"));

	if (UTiledItemSet* TiledItemSet = TiledItemSetPtr.Get())
	{
		// Figure out what to call the new tiled level
		FString EffectiveTiledItemSetName = TiledItemSet->GetName();
		EffectiveTiledItemSetName.RemoveFromEnd(SuffixToIgnore);

		const FString TileSetPathName = TiledItemSet->GetOutermost()->GetPathName();
		const FString LongPackagePath = FPackageName::GetLongPackagePath(TileSetPathName);

		const FString NewTiledLevelDefaultPath = LongPackagePath / EffectiveTiledItemSetName;

		// Make sure the name is unique
		FString AssetName;
		FString PackageName;
		AssetToolsModule.Get().CreateUniqueAssetName(NewTiledLevelDefaultPath, TileMapSuffix, /*out*/ PackageName, /*out*/ AssetName);
		const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);

		// Create the new tiled level
		UTiledLevelFactory* TiledLevelFactory = NewObject<UTiledLevelFactory>();
		TiledLevelFactory->InitialItemSet = TiledItemSet;
		ContentBrowserModule.Get().CreateNewAsset(AssetName, PackagePath, UTiledLevelAsset::StaticClass(), TiledLevelFactory);
	}
}

#undef LOCTEXT_NAMESPACE