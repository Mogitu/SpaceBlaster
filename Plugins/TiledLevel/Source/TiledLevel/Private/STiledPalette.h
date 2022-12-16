// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Misc/TextFilter.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STileView.h"
#include "TiledLevelTypes.h"

class FTiledItemSetEditor;
class FTiledLevelEdMode;
class UTiledItemSet;
class FTiledItemViewData;

namespace ETiledPaletteDetailViewMode 
{
	enum Type
	{
		ItemDetail,
		FillDetail
	};
}

namespace FillDetailTreeColumns
{
	static const FName ColumnID_Thumbnail("Thumbnail");
	static const FName ColumnID_Coefficient("Coefficient");
	static const FName ColumnID_RandomRotation("RandomRotation");
	static const FName ColumnID_AllowOverlay("AllowOverlay");
}

namespace ItemSelectionColumns
{
	static const FName ColumnID_Selection("Selection");	
	static const FName ColumnID_Name("Name");	
	static const FName ColumnID_Thumbnail("Thumbnail");	
}

namespace ItemSelectedColumns
{
	static const FName ColumnID_Name("Name");	
	static const FName ColumnID_Thumbnail("Thumbnail");	
	static const FName ColumnID_Remove("Remove");	
}


class TILEDLEVEL_API STiledPalette : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STiledPalette) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FTiledItemSetEditor> InSetEditor, FTiledLevelEdMode* InEdMode, UTiledItemSet* InItemSet);
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	void ChangeSet(UTiledItemSet* NewSet);
	void UpdatePalette(bool bRebuildItem = false);
	void SelectItem(UTiledLevelItem* NewItem);
	void ClearItemSelection();
	void SetThumbnailScale(float NewScale);
	float GetThumbnailScale() const;
	float GetThumbnailSize() const;
	bool AnyTileHovered() const;
	void DisplayFillDetail(bool bShouldDisplay) const;

	int32 TempSetVersionNumber = 0;
	
private:
	// Core vars
	// Use whether edmode / seteditor is nullptr to distinguish in set editor or level editor
	TWeakPtr<FTiledItemSetEditor> SetEditor; // This must be weakptr, otherwise will trigger severe issue..., maybe due to reference to something important is affected!!
	FTiledLevelEdMode* EdMode = nullptr;
	TWeakObjectPtr<UTiledItemSet> ItemSetPtr;
	TSharedPtr<FUICommandList> CommandList;
	TArray<TSharedPtr<FTiledItemViewData>> PaletteItems;
	TArray<TSharedPtr<FTiledItemViewData>> FilteredItems; 
	typedef TTextFilter<TSharedPtr<FTiledItemViewData>> ItemTextFilter;
	TSharedPtr<ItemTextFilter> ItemFilter;
	TSharedPtr<class FAssetThumbnailPool> ThumbnailPool;
	TSharedPtr<FStructOnScope> ItemCustomDataRow;
	
	// Core widgets
	TSharedPtr<class SSearchBox> SearchBox;
	TSharedPtr<STileView<TSharedPtr<FTiledItemViewData>>> TileViewWidget;
	TSharedPtr<class SWidgetSwitcher> PaletteDetailSwitcher;
	TSharedPtr<class IDetailsView> DetailsWidget;
	TSharedPtr<class IStructureDetailsView> CustomDataDetailsWidget;
	
	void GetPaletteFilterString(TSharedPtr<FTiledItemViewData> Item, TArray<FString>& OutArray) const;
	void OnSearchTextChanged(const FText& InFilterText);
	bool PassesOptionFilter(UTiledLevelItem* InItem);
	FText GetCurrentItemName() const;
	TSharedRef<SWidget> GetViewOptionMenuContent();
	TSharedPtr<SWidget> ConstructItemContextMenu();
	FText GetShowHideDetailsTooltipText() const;
	FReply OnShowHideDetailsClicked() const;
	const FSlateBrush* GetShowHideDetailsImage() const;

	// Core Palette functions
	void RemoveTiledItem();
	
	// Tile view functions
	ESelectionMode::Type GetSelectionMode() const;
	TSharedRef<ITableRow> GenerateTile(TSharedPtr<FTiledItemViewData> InData, const TSharedRef<STableViewBase>& OwnerTable);
	void OnSelectionChanged(TSharedPtr<FTiledItemViewData> InData, ESelectInfo::Type SelectInfo);
	void OnItemDoubleClicked(TSharedPtr<FTiledItemViewData> InData) const;
	
#define DECLARE_FILTER_FUNCTIONS(Target) \
	ECheckBoxState GetFilterOptionState_##Target() const;\
	void SetFilterOptionState_##Target(ECheckBoxState NewState);

	DECLARE_FILTER_FUNCTIONS(Block)
	DECLARE_FILTER_FUNCTIONS(Floor)
	DECLARE_FILTER_FUNCTIONS(Wall)
	DECLARE_FILTER_FUNCTIONS(Edge)
	DECLARE_FILTER_FUNCTIONS(Pillar)
	DECLARE_FILTER_FUNCTIONS(Point)
	DECLARE_FILTER_FUNCTIONS(Structure)
	DECLARE_FILTER_FUNCTIONS(Prop)
	DECLARE_FILTER_FUNCTIONS(Actor)
	DECLARE_FILTER_FUNCTIONS(Mesh)
	
	//Drag Drop
	EVisibility GetDragItemHintVisibility() const;
	TSharedRef<SWidget> CreateDropConfirmDialogContent();
	EVisibility GetItemDropTargetVisibility() const;
	FReply HandleItemDropped(const FGeometry& DropZoneGeometry, const FDragDropEvent& DragDropEvent);
	TSharedPtr<class SHeaderRow> DropConfirmHeaderRow;
	
	TArray<TSharedPtr<FString>> NewItemData_SourceName;
	TArray<EPlacedType> NewItemData_PlacedType;
	TArray<ETLStructureType> NewItemData_StructureType;
	TArray<FVector> NewItemData_Extent;

	// Fill Widgets
	TSharedPtr<class SBorder> FillDetailWidget;
	TSharedPtr<STreeView<TSharedPtr<FTiledItemViewData>>> FillDetailTreeWidget;
	TArray<TSharedPtr<FTiledItemViewData>> SelectedFillItems;
	TSharedRef<ITableRow> GenerateFillItemRow(TSharedPtr<FTiledItemViewData> InData, const TSharedRef<STableViewBase>& OwnerTable);
	void GetFillItemChildren(TSharedPtr<FTiledItemViewData> InData, TArray<TSharedPtr<FTiledItemViewData>>& OutChildren);
	EVisibility GetFillWidgetVisibility() const;
	EVisibility GetFillWidgetTooltipVisibility() const;
	void OnEnableFillGapChanged(ECheckBoxState NewState);
	ECheckBoxState GetEnableInsertGap() const;
	bool CanSetGapCoefficient() const;
	void OnGapCoefficientChanged(float NewValue);
	void OnGapCoefficientCommitted(float NewValue, ETextCommit::Type CommitType);
	TOptional<float> GetGapCoefficient() const;
	FText GetGapActualRatioText() const;

	// Custom data
	EVisibility GetCreateCustomDataRowButtonVisibility() const;
	FReply OnCreateNewEmptyCustomDataRowClicked();
	void OnCustomDataEdited(const FPropertyChangedEvent& PropertyChangedEvent);

	// Special items
	EVisibility GetAddSpecialItemsVisibility() const;
	TSharedRef<SWidget> GetAddSpecialItemsMenuContent();
	// special item: restriction
	void AddRestrictionItem();
    void AddTemplateItem();
	TSharedPtr<class STiledItemPicker> ItemPicker;
	EVisibility GetItemPickerVisibility() const;
	
};
