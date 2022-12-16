// Copyright 2022 PufStudio. All Rights Reserved.

#include "StaticTiledLevelDetailCustomization.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

void FStaticTiledLevelDetailCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	
	IDetailCategoryBuilder& TiledLevelCategory = DetailBuilder.EditCategory(
		"Work flow", FText::GetEmpty(), ECategoryPriority::Important);
}

TSharedRef<IDetailCustomization> FStaticTiledLevelDetailCustomization::MakeInstance()
{
	return MakeShareable(new FStaticTiledLevelDetailCustomization);
}
