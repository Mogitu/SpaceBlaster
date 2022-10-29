// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FTiledLevelCommands : public TCommands<FTiledLevelCommands>
{
public:
    FTiledLevelCommands();

	virtual void RegisterCommands() override;

//	Level Editing
	TSharedPtr<FUICommandInfo> SelectSelectTool;
	TSharedPtr<FUICommandInfo> SelectPaintTool;
	TSharedPtr<FUICommandInfo> SelectEraserTool;
	TSharedPtr<FUICommandInfo> SelectEyedropperTool;
	TSharedPtr<FUICommandInfo> SelectFillTool;
	TSharedPtr<FUICommandInfo> ToggleGrid;
	TSharedPtr<FUICommandInfo> RotateCW;
	TSharedPtr<FUICommandInfo> RotateCCW;
	TSharedPtr<FUICommandInfo> MirrorX;
	TSharedPtr<FUICommandInfo> MirrorY;
	TSharedPtr<FUICommandInfo> MirrorZ;	
	TSharedPtr<FUICommandInfo> EditUpperFloor;
	TSharedPtr<FUICommandInfo> EditLowerFloor;
	TSharedPtr<FUICommandInfo> MultiMode;
	TSharedPtr<FUICommandInfo> Snap;

// Floor Editing
	TSharedPtr<FUICommandInfo> AddNewFloorAbove;
	TSharedPtr<FUICommandInfo> AddNewFloorBelow;
	TSharedPtr<FUICommandInfo> MoveFloorUp;
	TSharedPtr<FUICommandInfo> MoveFloorDown;
	TSharedPtr<FUICommandInfo> MoveFloorToTop;
	TSharedPtr<FUICommandInfo> MoveFloorToBottom;
	TSharedPtr<FUICommandInfo> MoveAllFloorsUp;
	TSharedPtr<FUICommandInfo> MoveAllFloorsDown;
	TSharedPtr<FUICommandInfo> SelectFloorAbove;
	TSharedPtr<FUICommandInfo> SelectFloorBelow;
	TSharedPtr<FUICommandInfo> HideAllFloors;
	TSharedPtr<FUICommandInfo> UnhideAllFloors;
	TSharedPtr<FUICommandInfo> HideOtherFloors;
	TSharedPtr<FUICommandInfo> UnhideOtherFloors;
	
// Tiled Item Set Editor Commands
	// TSharedPtr<FUICommandInfo> FixItem;
	TSharedPtr<FUICommandInfo> ClearFixedItem;
	
// Tiled Level Editor Commands
	TSharedPtr<FUICommandInfo> UpdateLevels;
	TSharedPtr<FUICommandInfo> MergeToStaticMesh;

};
