// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelPropertyCustomizations.h"
#include "DetailWidgetRow.h"
#include "TiledLevelAsset.h"
#include "PropertyCustomizationHelpers.h"
#include "TiledLevelItem.h"


void FTemplateAssetDetails::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
                                            IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    TSharedPtr<IPropertyHandle> MyProperty = PropertyHandle->GetChildHandle(TEXT("TemplateAsset"));
    TArray<UObject*> Owners;
    PropertyHandle->GetOuterObjects(Owners);
    if (Owners.IsValidIndex(0))
    {
        Owner = Cast<UTiledLevelTemplateItem>(Owners[0]);
    }
    
    HeaderRow
    .NameContent()
    [
        MyProperty->CreatePropertyNameWidget()
    ]
    .ValueContent()
    .MinDesiredWidth(300.f)
    [
        SNew(SObjectPropertyEntryBox)
        .AllowedClass(UTiledLevelAsset::StaticClass())
        .PropertyHandle(MyProperty)
        .ThumbnailPool(CustomizationUtils.GetThumbnailPool())
        .AllowClear(false)
        .DisplayUseSelected(true)
        .DisplayBrowse(true) // enable this could open weird tab..., its not a big deal compares to its merit?
        .NewAssetFactories(TArray<UFactory*>{}) // inconvenient but the most safe way, not allowed to create new asset here
        .OnShouldFilterAsset(this, &FTemplateAssetDetails::OnShouldFilterTemplateAsset)
    ];
    
}

void FTemplateAssetDetails::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle,
    IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

bool FTemplateAssetDetails::OnShouldFilterTemplateAsset(const FAssetData& AssetData) const
{
    if (UTiledLevelAsset* TLA = Cast<UTiledLevelAsset>(AssetData.GetAsset()))
    {
        if (Owner)
        {
            if (UTiledItemSet* OwnerItemSet = Cast<UTiledItemSet>(Owner->GetOuter()))
            {
                return TLA->GetTileSize() != OwnerItemSet->GetTileSize();
            }
        }
    }
    return true;
}
