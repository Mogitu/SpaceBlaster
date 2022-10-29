// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/*
 * TODO: not generic enough now... how to make it easy to use for V3.0
 * TODO: Add scroll view
 */

class TILEDLEVEL_API STiledItemPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STiledItemPicker) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
    void SetContent(TArray<FGuid>* InPickedItemIDs, const TArray<class UTiledLevelItem*>& InItemOptions,
        const TSharedPtr<FAssetThumbnailPool>& InThumbnailPool);
    
private:
    TSharedPtr<class SVerticalBox> Content;
    TSharedPtr<class SVerticalBox> OptionRows;
    TSharedPtr<class SButton> ExpanderArrow;
    TArray<FGuid>* PickedItemIDs = nullptr;
    TArray<class UTiledLevelItem*> ItemOptions;
    TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
    const FSlateBrush* GetExpanderImage() const;
    FReply OnExpanderArrowClicked();
    bool IsCollapsed = false;
    EVisibility GetChildVisibility() const;
    FText GetTargetItemName(int Nth) const;
    TSharedRef<SWidget> MakeAddItemMenu(int Nth);
    void RefreshContent();
};
