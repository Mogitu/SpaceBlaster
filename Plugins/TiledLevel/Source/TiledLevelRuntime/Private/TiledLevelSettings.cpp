// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelSettings.h"

void UTiledLevelSettings::ResetTiledPaletteSettings()
{
	ThumbnailScale = 0.5f;
	PaletteSearchText = FText::GetEmpty();
	IsItemsVisible_Block = true;
	IsItemsVisible_Floor = true;
	IsItemsVisible_Wall = true;
	IsItemsVisible_Structure = true;
	IsItemsVisible_Prop = true;
	IsItemsVisible_Actor = true;
	IsItemsVisible_Mesh = true;
	DefaultPlacedTypePivotPositionMap =
    {
		{EPlacedType::Block, EPivotPosition::Bottom},
		{EPlacedType::Floor, EPivotPosition::Bottom},
		{EPlacedType::Wall, EPivotPosition::Bottom},
		{EPlacedType::Edge, EPivotPosition::Center},
		{EPlacedType::Pillar, EPivotPosition::Bottom},
		{EPlacedType::Point, EPivotPosition::Center},
		
    };
}
