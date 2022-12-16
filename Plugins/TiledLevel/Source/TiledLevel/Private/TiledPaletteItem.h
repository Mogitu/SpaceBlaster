// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelTypes.h"

class FTiledLevelEdMode;
class FTiledItemSetEditor;
class STiledPalette;
class UTiledLevelItem;

// Could be in two editor, use either InAssetEditor or InEdMode to differ
class FTiledItemViewData : public TSharedFromThis<FTiledItemViewData>
{
	
public:
	FTiledItemViewData(UTiledLevelItem* InItem, FTiledLevelEdMode* InEdMode, 
	                      TSharedPtr<FAssetThumbnailPool> InThumbnailPool);
	TSharedRef<class SToolTip> CreateTooltipWidget();
	TSharedRef<class SCheckBox> CreateAllowEraserCheckBox();
	TSharedRef<SWidget> GetThumbnailWidget();
	UTiledLevelItem* Item;
	FText GetInstanceCountText(bool bCurrentLayerOnly) const;
	FName GetDisplayName() const;
    
private:
	void HandleCheckStateChanged(const ECheckBoxState NewCheckedState);
	EVisibility GetEraserCheckBoxVisibility() const;
	ECheckBoxState GetEraserCheckBoxState() const;
	EVisibility GetVisibilityBasedOnIsMesh() const;
	FText GetPaletteSearchText() const;
	FText GetSourceAssetTypeText() const;
	FText GetMeshApproxSize() const;
	FText GetPlacedTypeText() const;
	FText GetExtentText() const;
	FText GetStructureTypeText() const;
    FText GetRestrictionRuleText() const;
    FText GetRestrictionTargetNumText() const;
    FText GetTemplateAssetInfoText(int WhichInfo) const;
	
	TSharedPtr<class FAssetThumbnailPool> ThumbnailPool;

	TSharedPtr<SWidget> ThumbnailWidget;
	FTiledLevelEdMode* EdMode;
    TSharedRef<SWidget> MakeItemTooltipContent();
    TSharedRef<SWidget> MakeRestrictionItemTooltipContent();
    TSharedRef<SWidget> MakeTemplateItemTooltipContent();
    
    
};


class TILEDLEVEL_API STiledPaletteItem : public STableRow<TSharedPtr<FTiledItemViewData>>
{
public:
	SLATE_BEGIN_ARGS(STiledPaletteItem) {}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, TSharedPtr<FTiledItemViewData> InData, const TSharedRef<STableViewBase>& InOwnerTableView, FTiledLevelEdMode* InEdMode);
	FLinearColor GetTileColorAndOpacity() const;
	EVisibility GetCheckBoxVisibility() const;
	EVisibility GetInstanceCountVisibility() const;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	FTiledLevelEdMode* EdMode = nullptr; // if is null, means its in SetEditor
	TSharedPtr<FTiledItemViewData> Data;
	TSharedPtr<SCheckBox> MyCheckBox;
	const float MinScaleForShowInstanceCount = 0.2;
};


class SNewItemDropConfirmItemRow : public SMultiColumnTableRow<TSharedPtr<FString>>
{
public:
	SLATE_BEGIN_ARGS(SNewItemDropConfirmItemRow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FString> InAssetName, const TSharedRef<STableViewBase>& InOwnerTableView, EPlacedType* InTargetPlacedType, ETLStructureType* InTargetStructure, FVector* InTargetExtent);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;
private:
	TSharedPtr<FString> AssetName;
	TArray<TSharedPtr<FString>> PlacedTypeOptions;
	EPlacedType* TargetPlacedType = nullptr;
	TArray<TSharedPtr<FString>> StructureTypeOptions;
	ETLStructureType* TargetStructureType = nullptr;
	FVector* TargetExtent = nullptr;

	EVisibility GetExtentInputVisibility(EAxis::Type Axis) const;
	TOptional<int> GetExtentValue(EAxis::Type Axis) const;
	void OnExtentChanged(int NewValue, EAxis::Type Axis);
	void OnExtentCommitted(int NewValue, ETextCommit::Type CommitType, EAxis::Type Axis);
	
};

class STiledPaletteFillDetailRow : public SMultiColumnTableRow<TSharedPtr<FTiledItemViewData>>
{
public:
	SLATE_BEGIN_ARGS(STiledPaletteFillDetailRow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, TSharedPtr<FTiledItemViewData> InData, FTiledLevelEdMode* InEdMode);
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;
private:
	TSharedPtr<FTiledItemViewData> MyData;
	FTiledLevelEdMode* EdMode = nullptr;
	
	
	TOptional<float> GetCoefficient() const;
	void OnCoefficientChanged(float NewValue);
	void OnCoefficientCommitted(float NewValue, ETextCommit::Type CommitType);
	FText GetActualRatioText() const;
	ECheckBoxState GetAllowRandomRotation() const;
	void OnAllowRandomRotationChanged(ECheckBoxState NewCheckBoxState);
	
	// functionality of overlay only open for prop-type
	ECheckBoxState GetAllowOverlay() const;
	void OnAllowOverlayChanged(ECheckBoxState NewCheckBoxState);
	
};
