// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelAssetTypeActions.h"
#include "TiledLevelAsset.h"
#include "TiledLevelEditor.h"
#include "TiledLevelStyle.h"
#include "ToolMenuSection.h"
#include "TiledLevel.h"
#include "TiledLevelEditorUtility.h"
#include "EditorFramework/ThumbnailInfo.h"
#include "ThumbnailRendering/SceneThumbnailInfo.h"

#define LOCTEXT_NAMESPACE "TiledLevelAssetTypeActions"


FTiledLevelAssetTypeActions::FTiledLevelAssetTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: MyAssetCategory(InAssetCategory)
{
}

FText FTiledLevelAssetTypeActions::GetName() const
{
	return FText(LOCTEXT("AssetName", "Tiled Level"));
}

UClass* FTiledLevelAssetTypeActions::GetSupportedClass() const
{
	return UTiledLevelAsset::StaticClass();
}

FColor FTiledLevelAssetTypeActions::GetTypeColor() const
{
	return FColorList::Goldenrod;
}

uint32 FTiledLevelAssetTypeActions::GetCategories()
{
	return MyAssetCategory;
}

void FTiledLevelAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects,
	TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
 	for (UObject* obj : InObjects)
 	{
 		if (UTiledLevelAsset* TileLevel = Cast<UTiledLevelAsset>(obj))
 		{
			TSharedRef<FTiledLevelEditor> NewTiledLevelEditor(new FTiledLevelEditor());
			NewTiledLevelEditor->InitTileLevelEditor(Mode, EditWithinLevelEditor, TileLevel);
 		}		
 	}	
}

void FTiledLevelAssetTypeActions::GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section)
{
	TArray<TWeakObjectPtr<UTiledLevelAsset>> TiledLevelAssets = GetTypedWeakObjectPtrs<UTiledLevelAsset>(InObjects);

	if (TiledLevelAssets.Num() == 1)
	{
		Section.AddMenuEntry(
			"TiledLevelAsset_MergeTiledLevel",
			LOCTEXT("TiledLevelAsset_MergeTiledLevel", "Merge Tiled Level"),
			LOCTEXT("TiledLevelAsset_MergeTiledLevelTooltip", "Merge tiled level into single static mesh asset"),
			FSlateIcon(FTiledLevelStyle::GetStyleSetName(), "TiledLevel.MergeAsset"),
			FUIAction(FExecuteAction::CreateSP(this, &FTiledLevelAssetTypeActions::ExecuteMergeTiledAsset, TiledLevelAssets[0]))
			);
	}
}

UThumbnailInfo* FTiledLevelAssetTypeActions::GetThumbnailInfo(UObject* Asset) const
{
    UTiledLevelAsset* TiledLevelAsset = CastChecked<UTiledLevelAsset>(Asset);
    UThumbnailInfo* ThumbnailInfo = TiledLevelAsset->ThumbnailInfo;
    if (ThumbnailInfo == NULL)
    {
        ThumbnailInfo = NewObject<USceneThumbnailInfo>(TiledLevelAsset, NAME_None, RF_Transactional);
        TiledLevelAsset->ThumbnailInfo = ThumbnailInfo;
    }
    return ThumbnailInfo;
}

void FTiledLevelAssetTypeActions::ExecuteMergeTiledAsset(TWeakObjectPtr<UTiledLevelAsset> TiledLevelAssetPtr)
{
	if (UTiledLevelAsset* TiledLevelAsset = TiledLevelAssetPtr.Get())
	{
		// spawn a tiled level actor using this asset to world
		ATiledLevel* NTL = Cast<ATiledLevel>(GEditor->GetEditorWorldContext().World()->SpawnActor(ATiledLevel::StaticClass()));
		NTL->SetActiveAsset(TiledLevelAsset);
		TiledLevelAsset->HostLevel = NTL;
		NTL->ResetAllInstance();
		// implement merge
		FTiledLevelEditorUtility::MergeTiledLevelAsset(TiledLevelAsset);
		// delete that actor
		TiledLevelAsset->HostLevel = nullptr;
		NTL->Destroy();
	}
}

#undef LOCTEXT_NAMESPACE
