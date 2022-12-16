// Copyright 2022 PufStudio. All Rights Reserved.

 #include "TiledLevelStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr< FSlateStyleSet > FTiledLevelStyle::StyleInstance = NULL;

void FTiledLevelStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FTiledLevelStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FTiledLevelStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("TiledLevelStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon8x8(8.0f, 8.0f);
const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);
const FVector2D Icon128x128(128.0f, 128.0f);

TSharedRef< FSlateStyleSet > FTiledLevelStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("TiledLevelStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("TiledLevel")->GetBaseDir() / TEXT("Resources"));

	// Custom Asset Class Icons
	Style->Set("ClassIcon.TiledLevelAsset", new IMAGE_BRUSH(TEXT("Class_TiledAsset_128x"), Icon16x16));
	Style->Set("ClassThumbnail.TiledLevelAsset", new IMAGE_BRUSH(TEXT("Class_TiledAsset_128x"), Icon128x128));
	Style->Set("ClassIcon.TiledItemSet", new IMAGE_BRUSH(TEXT("Class_TiledItemSet_128x"), Icon16x16));
	Style->Set("ClassThumbnail.TiledItemSet", new IMAGE_BRUSH(TEXT("Class_TiledItemSet_128x"), Icon128x128));

	// Level viewport action
	Style->Set("TiledLevel.BreakAsset", new IMAGE_BRUSH(TEXT("Class_TiledAsset_128x"), Icon16x16));
	Style->Set("TiledLevel.BreakAsset.Small", new IMAGE_BRUSH(TEXT("Class_TiledAsset_128x"), Icon20x20));
	Style->Set("TiledLevel.MergeAsset", new IMAGE_BRUSH(TEXT("Merge_128x"), Icon16x16));
	Style->Set("TiledLevel.MergeAsset.Small", new IMAGE_BRUSH(TEXT("Merge_128x"), Icon20x20));
	
	// Floors Palette
	// To enable slate icon... .Small version is required!!
	Style->Set("TiledLevel.EyeOpened", new IMAGE_BRUSH(TEXT("EyeOpened_40x"), Icon20x20));
	Style->Set("TiledLevel.EyeOpened.Small", new IMAGE_BRUSH(TEXT("EyeOpened_40x"), Icon20x20));
	Style->Set("TiledLevel.EyeClosed", new IMAGE_BRUSH(TEXT("EyeClosed_40x"), Icon20x20));
	Style->Set("TiledLevel.EyeClosed.Small", new IMAGE_BRUSH(TEXT("EyeClosed_40x"), Icon20x20));
	Style->Set("TiledLevel.MoveFloorDown", new IMAGE_BRUSH(TEXT("MoveFloorDown_128x"), Icon20x20));
	Style->Set("TiledLevel.MoveFloorDown.Small", new IMAGE_BRUSH(TEXT("MoveFloorDown_128x"), Icon20x20));
	Style->Set("TiledLevel.MoveFloorUp", new IMAGE_BRUSH(TEXT("MoveFloorUp_128x"), Icon20x20));
	Style->Set("TiledLevel.MoveFloorUp.Small", new IMAGE_BRUSH(TEXT("MoveFloorUp_128x"), Icon20x20));
	Style->Set("TiledLevel.Eraser", new IMAGE_BRUSH(TEXT("Eraser_40x"), Icon20x20));
	Style->Set("TiledLevel.Eraser.Small", new IMAGE_BRUSH(TEXT("Eraser_40x"), Icon20x20));
	Style->Set("TiledLevel.AddNewFloorAbove", new IMAGE_BRUSH(TEXT("AddNewFloorAbove_128x"), Icon20x20));
	Style->Set("TiledLevel.AddNewFloorAbove.Small", new IMAGE_BRUSH(TEXT("AddNewFloorAbove_128x"), Icon20x20));
	Style->Set("TiledLevel.AddNewFloorBelow", new IMAGE_BRUSH(TEXT("AddNewFloorBelow_128x"), Icon20x20));
	Style->Set("TiledLevel.AddNewFloorBelow.Small", new IMAGE_BRUSH(TEXT("AddNewFloorBelow_128x"), Icon20x20));
	Style->Set("TiledLevel.MoveFloorToTop", new IMAGE_BRUSH(TEXT("MoveAllFloorsUp_128x"), Icon20x20));
	Style->Set("TiledLevel.MoveFloorToTop.Small", new IMAGE_BRUSH(TEXT("MoveAllFloorsUp_128x"), Icon20x20));
	Style->Set("TiledLevel.MoveFloorToBottom", new IMAGE_BRUSH(TEXT("MoveAllFloorsDown_128x"), Icon20x20));
	Style->Set("TiledLevel.MoveFloorToBottom.Small", new IMAGE_BRUSH(TEXT("MoveAllFloorsDown_128x"), Icon20x20));
	Style->Set("TiledLevel.MoveAllFloorsUp", new IMAGE_BRUSH(TEXT("MoveAllFloorsUp_128x"), Icon20x20));
	Style->Set("TiledLevel.MoveAllFloorsUp.Small", new IMAGE_BRUSH(TEXT("MoveAllFloorsUp_128x"), Icon20x20));
	Style->Set("TiledLevel.MoveAllFloorsDown", new IMAGE_BRUSH(TEXT("MoveAllFloorsDown_128x"), Icon20x20));
	Style->Set("TiledLevel.MoveAllFloorsDown.Small", new IMAGE_BRUSH(TEXT("MoveAllFloorsDown_128x"), Icon20x20));
	Style->Set("TiledLevel.DeleteFloor", new IMAGE_BRUSH(TEXT("DeleteFloor_128x"), Icon20x20));
	Style->Set("TiledLevel.DeleteFloor.Small", new IMAGE_BRUSH(TEXT("DeleteFloor_128x"), Icon20x20));
	Style->Set("TiledLevel.DuplicateFloor", new IMAGE_BRUSH(TEXT("DuplicateFloor_128x"), Icon20x20));
	Style->Set("TiledLevel.DuplicateFloor.Small", new IMAGE_BRUSH(TEXT("DuplicateFloor_128x"), Icon20x20));

	// EdMode Toolbar
	Style->Set("TiledLevel.SelectSelectTool", new IMAGE_BRUSH(TEXT("SelectTool_128x"), Icon20x20));
	Style->Set("TiledLevel.SelectSelectTool.Small", new IMAGE_BRUSH(TEXT("SelectTool_128x"), Icon20x20));
	Style->Set("TiledLevel.SelectPaintTool", new IMAGE_BRUSH(TEXT("PaintTool_128x"), Icon40x40));
	Style->Set("TiledLevel.SelectPaintTool.Small", new IMAGE_BRUSH(TEXT("PaintTool_128x"), Icon40x40));
	Style->Set("TiledLevel.SelectEraserTool", new IMAGE_BRUSH(TEXT("EraserTool_128x"), Icon40x40));
	Style->Set("TiledLevel.SelectEraserTool.Small", new IMAGE_BRUSH(TEXT("EraserTool_128x"), Icon40x40));
	Style->Set("TiledLevel.SelectEyedropperTool", new IMAGE_BRUSH(TEXT("Eyedropper_128x"), Icon40x40));
	Style->Set("TiledLevel.SelectEyedropperTool.Small", new IMAGE_BRUSH(TEXT("Eyedropper_128x"), Icon40x40));
	Style->Set("TiledLevel.SelectFillTool", new IMAGE_BRUSH(TEXT("FillTool_128x"), Icon40x40));
	Style->Set("TiledLevel.SelectFillTool.Small", new IMAGE_BRUSH(TEXT("FillTool_128x"), Icon40x40));
	Style->Set("TiledLevel.StepControl", new IMAGE_BRUSH(TEXT("Step_128x"), Icon40x40));
	Style->Set("TiledLevel.StepControl.Small", new IMAGE_BRUSH(TEXT("Step_128x"), Icon40x40));
	Style->Set("TiledLevel.ToggleGrid", new IMAGE_BRUSH(TEXT("ToggleGrid_128x"), Icon40x40));
	Style->Set("TiledLevel.ToggleGrid.Small", new IMAGE_BRUSH(TEXT("ToggleGrid_128x"), Icon40x40));
	Style->Set("TiledLevel.RotateCW", new IMAGE_BRUSH(TEXT("RotateCW_128x"), Icon40x40));
	Style->Set("TiledLevel.RotateCW.Small", new IMAGE_BRUSH(TEXT("RotateCW_128x"), Icon40x40));
	Style->Set("TiledLevel.RotateCCW", new IMAGE_BRUSH(TEXT("RotateCCW_128x"), Icon40x40));
	Style->Set("TiledLevel.RotateCCW.Small", new IMAGE_BRUSH(TEXT("RotateCCW_128x"), Icon40x40));
	Style->Set("TiledLevel.MirrorX", new IMAGE_BRUSH(TEXT("MirrorX_128x"), Icon40x40));
	Style->Set("TiledLevel.MirrorX.Small", new IMAGE_BRUSH(TEXT("MirrorX_128x"), Icon40x40));
	Style->Set("TiledLevel.MirrorY", new IMAGE_BRUSH(TEXT("MirrorY_128x"), Icon40x40));
	Style->Set("TiledLevel.MirrorY.Small", new IMAGE_BRUSH(TEXT("MirrorY_128x"), Icon40x40));
	Style->Set("TiledLevel.MirrorZ", new IMAGE_BRUSH(TEXT("MirrorZ_128x"), Icon40x40));
	Style->Set("TiledLevel.MirrorZ.Small", new IMAGE_BRUSH(TEXT("MirrorZ_128x"), Icon40x40));
	Style->Set("TiledLevel.EditUpperFloor", new IMAGE_BRUSH(TEXT("EditUpper_128x"), Icon40x40));
	Style->Set("TiledLevel.EditUpperFloor.Small", new IMAGE_BRUSH(TEXT("EditUpper_128x"), Icon40x40));
	Style->Set("TiledLevel.EditLowerFloor", new IMAGE_BRUSH(TEXT("EditLower_128x"), Icon40x40));
	Style->Set("TiledLevel.EditLowerFloor.Small", new IMAGE_BRUSH(TEXT("EditLower_128x"), Icon40x40));
	Style->Set("TiledLevel.Snap", new IMAGE_BRUSH(TEXT("Snap_128x"), Icon40x40));
	Style->Set("TiledLevel.Snap.Small", new IMAGE_BRUSH(TEXT("Snap_128x"), Icon40x40));
	Style->Set("TiledLevel.MultiMode", new IMAGE_BRUSH(TEXT("Multi_128x"), Icon40x40));
	Style->Set("TiledLevel.MultiMode.Small", new IMAGE_BRUSH(TEXT("Multi_128x"), Icon40x40));

	// Editor Toolbar
	Style->Set("TiledLevel.UpdateLevels", new IMAGE_BRUSH(TEXT("UpdateLevels_128x"), Icon40x40));
	Style->Set("TiledLevel.UpdateLevels.Small", new IMAGE_BRUSH(TEXT("UpdateLevels_128x"), Icon20x20));
	Style->Set("TiledLevel.FixItem", new IMAGE_BRUSH(TEXT("FixItem_128x"), Icon40x40));
	Style->Set("TiledLevel.FixItem.Small", new IMAGE_BRUSH(TEXT("FixItem_128x"), Icon20x20));
	Style->Set("TiledLevel.ClearFixedItem", new IMAGE_BRUSH(TEXT("UpdateLevels_128x"), Icon40x40));
	Style->Set("TiledLevel.ClearFixedItem.Small", new IMAGE_BRUSH(TEXT("UpdateLevels_128x"), Icon20x20));
	Style->Set("TiledLevel.MergeToStaticMesh", new IMAGE_BRUSH(TEXT("Merge_128x"), Icon40x40));
	Style->Set("TiledLevel.MergeToStaticMesh.Small", new IMAGE_BRUSH(TEXT("Merge_128x"), Icon20x20));

	const FTextBlockStyle& NormalText = FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");
	Style->Set("TiledLevel.FixedItemButtonText", FTextBlockStyle(FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText"))
		.SetFont(DEFAULT_FONT("BoldCondensed", 18))
		.SetColorAndOpacity(FLinearColor(0.8, 0.0f, 0.0f, 0.8f))
		.SetShadowOffset(FVector2D(1.0f, 1.0f))
		.SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f))
		);

	const FLinearColor LayerSelectionColor = FLinearColor(0.13f, 0.70f, 1.00f);
	const FTableRowStyle& NormalTableRowStyle = FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row");
	Style->Set("TiledLevel.LayerBrowser.TableViewRow",
		 FTableRowStyle(NormalTableRowStyle)
		 .SetActiveBrush(IMAGE_BRUSH("Selection", Icon8x8, LayerSelectionColor))
		 .SetActiveHoveredBrush(IMAGE_BRUSH("Selection", Icon8x8, LayerSelectionColor))
		 .SetInactiveBrush(IMAGE_BRUSH("Selection", Icon8x8, LayerSelectionColor))
		 .SetInactiveHoveredBrush(IMAGE_BRUSH("Selection", Icon8x8, LayerSelectionColor))
		 );

	Style->Set("TiledLevel.LayerBrowser.SelectionColor", LayerSelectionColor);


	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef DEFAULT_FONT
#undef TTF_FONT
#undef OTF_FONT

void FTiledLevelStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FTiledLevelStyle::Get()
{
	return *StyleInstance;
}
