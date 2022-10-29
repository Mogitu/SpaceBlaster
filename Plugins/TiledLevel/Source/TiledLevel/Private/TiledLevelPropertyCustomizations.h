// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once

#include "IPropertyTypeCustomization.h"

/**
 * Inside template item
 */
class FTemplateAssetDetails : public IPropertyTypeCustomization
{
public:

    static TSharedRef<IPropertyTypeCustomization> MakeInstance()
    {
        return MakeShareable(new FTemplateAssetDetails());
    }
    
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    
private:
    bool OnShouldFilterTemplateAsset(const FAssetData& AssetData) const;
    class UTiledLevelTemplateItem* Owner = nullptr;
};
