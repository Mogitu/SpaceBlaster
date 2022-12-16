// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class UTiledLevelAsset;

class TILEDLEVEL_API FTiledLevelAssetTypeActions : public FAssetTypeActions_Base
{
public:
	FTiledLevelAssetTypeActions(EAssetTypeCategories::Type InAssetCategory);
	
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual uint32 GetCategories() override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects,
		TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return true; }
	virtual void GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section) override;
    virtual UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const override;
private:
	EAssetTypeCategories::Type MyAssetCategory;

	void ExecuteMergeTiledAsset(TWeakObjectPtr<UTiledLevelAsset> TiledLevelAssetPtr);
};
