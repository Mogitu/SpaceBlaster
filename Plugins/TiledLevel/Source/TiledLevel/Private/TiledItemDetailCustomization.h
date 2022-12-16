// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelTypes.h"
#include "IDetailCustomization.h"


class TILEDLEVEL_API FTiledItemDetailCustomization : public IDetailCustomization
{
public:
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    static TSharedRef<IDetailCustomization> MakeInstance();

private:
	void CustomizeRestrictionItem(IDetailLayoutBuilder& DetailBuilder, class UTiledLevelRestrictionItem* RestrictionItem );
    void CustomizeTemplateItem(IDetailLayoutBuilder& DetailBuilder, class UTiledLevelTemplateItem* TemplateItem);
	// TSharedRef<SWidget> GetTargetItemsSelectionMenu();
	
	IDetailLayoutBuilder* MyDetailLayout = nullptr;
	TArray<FName> UnavailableSourceNames;
	
	TSharedPtr<IPropertyHandle> Property_PlacedType;
	TSharedPtr<IPropertyHandle> Property_StructureType;
	TSharedPtr<IPropertyHandle> Property_Extent;

	// set these two static to prevent weird bugs...
	static class UTiledLevelItem* CustomizedItem;
	static TArray<class UTiledLevelItem*> CustomizedItems;

	EVisibility GetPivotOptionVisibility(EPivotPosition TargetValue) const;
	ECheckBoxState GetPivotOptionCheckState(EPivotPosition TargetValue) const;
	void OnPivotOptionCheckStateChanged(ECheckBoxState NewState, EPivotPosition TargetValue);
	
	bool OnShouldFilterActorAsset(const FAssetData& AssetData) const;
	bool OnShouldFilterMeshAsset(const FAssetData& AssetData) const;
	TOptional<int> GetExtentValue(EAxis::Type Axis) const;
	void OnExtentChanged(int InValue, EAxis::Type Axis);
	void OnExtentCommitted(int InValue, ETextCommit::Type CommitType, EAxis::Type Axis);

};
