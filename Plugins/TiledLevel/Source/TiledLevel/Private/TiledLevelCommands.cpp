// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelCommands.h"
#include "TiledLevelStyle.h"

#define LOCTEXT_NAMESPACE "TiledLevelCommands"

FTiledLevelCommands::FTiledLevelCommands()
	: TCommands<FTiledLevelCommands>(TEXT("TiledLevel"),
		NSLOCTEXT("Contexts", "TiledLevel", "TiledLevel"),
		NAME_None,
		FTiledLevelStyle::GetStyleSetName()
	)
{	
}

void FTiledLevelCommands::RegisterCommands()
{
// Level Editing	
	UI_COMMAND(SelectSelectTool, "Select", "Select (Shift + RMB to start select)", EUserInterfaceActionType::RadioButton, FInputChord(EKeys::G));
	UI_COMMAND(SelectPaintTool, "Paint", "Paint selected item in palette (Holding 'Shift' to enter quick erase | Holding 'Ctrl' to straint mode | Shift + RMB to start select)", EUserInterfaceActionType::RadioButton, FInputChord(EKeys::B));
	UI_COMMAND(SelectEraserTool, "Eraser", "Eraser", EUserInterfaceActionType::RadioButton, FInputChord(EKeys::E));
	UI_COMMAND(SelectFillTool, "Fill", "Fill", EUserInterfaceActionType::RadioButton, FInputChord());
	UI_COMMAND(SelectEyedropperTool, "Eyedropper", "Eyedropper", EUserInterfaceActionType::RadioButton, FInputChord(EKeys::X));
	UI_COMMAND(ToggleGrid, "Grid", "Toggle grid visibility", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::Tab));
	UI_COMMAND(RotateCW, "Rot. CW", "Rotate item clockwise", EUserInterfaceActionType::Button, FInputChord(EKeys::R));
	UI_COMMAND(RotateCCW, "Rot. CCW", "Rotate item counter-clockwise", EUserInterfaceActionType::Button, FInputChord(EKeys::Q));
	UI_COMMAND(MirrorX, "MirrorX", "Mirror item along X axis", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::One));
	UI_COMMAND(MirrorY, "MirrorY", "Mirror item along Y axis", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::Two));
	UI_COMMAND(MirrorZ, "MirrorZ", "Mirror element along Z axis", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::Three));
	UI_COMMAND(EditUpperFloor, "Upper", "Edit Upper Floor if exist", EUserInterfaceActionType::Button, FInputChord(EKeys::C));
	UI_COMMAND(EditLowerFloor, "Lower", "Edit Lower Floor if exist", EUserInterfaceActionType::Button, FInputChord(EKeys::Z));
	UI_COMMAND(MultiMode, "Multi", "Allow multiple items in same tile unit", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::M));
	UI_COMMAND(Snap, "Snap", "Enale Snap if possible", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::N));
	// UI_COMMAND(Random, "Random", "Random", EUserInterfaceActionType::RadioButton, FInputChord());
	
// Floor Editing
	UI_COMMAND(AddNewFloorAbove, "Add new floor above", "Add a new floor above selected floor", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::N));
	UI_COMMAND(AddNewFloorBelow, "Add new floor below", "Add a new floor below selected floor", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(MoveFloorUp, "Move floor up", "Move selected floor up", EUserInterfaceActionType::Button, FInputChord(EKeys::RightBracket));
	UI_COMMAND(MoveFloorDown, "Move floor down", "Move selected floor down", EUserInterfaceActionType::Button, FInputChord(EKeys::LeftBracket));
	UI_COMMAND(MoveFloorToTop, "Move floor top", "Move selected floor top", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::RightBracket));
	UI_COMMAND(MoveFloorToBottom, "Move floor bottom", "Move selected floor bottom", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::LeftBracket));
	UI_COMMAND(MoveAllFloorsUp, "Move all floors up", "Move all floors up", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(MoveAllFloorsDown, "Move all floors down", "Move all floors down", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SelectFloorAbove, "Select floor above", "Selected floor above", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Alt , EKeys::RightBracket));
	UI_COMMAND(SelectFloorBelow, "Select floor below", "Select floor below", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Alt, EKeys::LeftBracket));
	UI_COMMAND(HideAllFloors, "Hide all floors", "Hide all floors", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(UnhideAllFloors, "Unhide all floors", "Unhide all floors", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(HideOtherFloors, "Hide other floors", "Hide all floors except this", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(UnhideOtherFloors, "Unhide other floors", "Unhide all floors except this", EUserInterfaceActionType::Button, FInputChord());

// Tiled Item Set Editor Commands
	// UI_COMMAND(FixItem, "FixItem", "Fix this item in viewport", EUserInterfaceActionType::CollapsedButton, FInputChord());
	UI_COMMAND(ClearFixedItem, "ClearFixed", "Clear all fixed items", EUserInterfaceActionType::Button, FInputChord());

// Tiled Level Editor Commands
	UI_COMMAND(UpdateLevels, "Update Levels", "Update all tiled level using this asset to newest version", EUserInterfaceActionType::Button, FInputChord())
	UI_COMMAND(MergeToStaticMesh, "Merge", "Merge this tiled level asset to static mesh asset", EUserInterfaceActionType::Button, FInputChord())
}

#undef LOCTEXT_NAMESPACE
