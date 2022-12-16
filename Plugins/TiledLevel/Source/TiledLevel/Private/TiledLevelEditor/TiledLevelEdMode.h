// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelTypes.h"
#include "EdMode.h"

class UTiledLevelItem;
struct FTiledLevelEdge;
class ATiledLevelEditorHelper;
class ATiledLevelSelectHelper;
class ATiledItemThumbnail;
class ATiledLevel;
class UTiledLevelLayer;
class UTiledLevelAsset;

enum ETiledLevelEditTool
{
	Select,
	Paint,
	Eraser,
	Eyedropper,
	Fill
};

enum ETiledLevelBrushAction
{
	None,
	FreePaint,
	QuickErase,
	Erase,
	BoxSelect,
	PaintSelected
};

class FTiledLevelEdMode : public FEdMode
{
public:
	const static FEditorModeID EM_TiledLevelEdModeId;
	FTiledLevelEdMode();
	virtual ~FTiledLevelEdMode();

	// FEdMode interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return "FTiledLevelEdMode"; }
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;
	virtual void PostUndo() override;
	virtual bool MouseEnter(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;
	virtual bool MouseLeave(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	virtual bool MouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;
	virtual bool CapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX,
		int32 InMouseY) override;
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key,
		EInputEvent Event) override;
	virtual void DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View,
		FCanvas* Canvas) override;
	virtual bool Select(AActor* InActor, bool bInSelected) override;
	virtual bool IsSelectionAllowed(AActor* InActor, bool bInSelection) const override;
	virtual void ActorSelectionChangeNotify() override;
	bool UsesToolkits() const override;
	virtual bool AllowWidgetMove() override { return  false; }
	virtual bool ShouldDrawWidget() const override { return false; }
	virtual bool UsesTransformWidget() const override { return false; }
	// End of FEdMode interface

	virtual bool DisallowMouseDeltaTracking() const override;

	void SetupData(UTiledLevelAsset* InData, ATiledLevel* InLevel, ATiledLevelEditorHelper* InHelper, const UStaticMeshComponent* InPreviewSceneFloor);
	void ClearActiveItemSet();
	void ClearItemInstances(UTiledLevelItem* TargetItem);
	void ClearSelectedItemInstances();
	bool CanClearSelectedItemInstancesInActiveFloor() const;
	void ClearSelectedItemInstancesInActiveFloor();
	ATiledLevelEditorHelper* GetEditorHelper() const { return Helper; }
	// void SetupThumbnailMesh(UStaticMesh* NewMesh); // future update or remove it?
	bool IsReadyToEdit() const;
	
	TSharedPtr<class STiledFloorList> FloorListWidget;
	UTiledLevelItem* ActiveItem = nullptr;
	TArray<UTiledLevelItem*> SelectedItems;
	UTiledLevelAsset* ActiveAsset = nullptr;
	ATiledLevel* GetActiveLevelActor() const { return ActiveLevel; }
	EPlacedType EraserTarget = EPlacedType::Any;
	ETiledLevelEditTool ActiveEditTool = ETiledLevelEditTool::Paint;
	FIntVector EraserExtent{1, 1, 1};

    // selection tool params
    int FloorSelectCount = 1;
    bool bSelectAllFloors = false;
    
	// fill tool params
	bool IsFillTiles = true;
	bool IsTileAsFillBoundary = true;
	bool NeedGround = false;
	bool EnableFillGap = false;
	float GapCoefficient = 1.0f;
	
	TSharedPtr<class STiledPalette> GetPalettePtr();
	
	// statics
	TMap<int, uint32> PerFloorInstanceCount;
	TMap<FGuid, uint32> PerItemInstanceCount;
	uint32 TotalInstanceCount = 0;


private:

	void BindCommands();
	bool ValidCheck(); // Check core ptrs are correctly setup
	bool IsInTiledLevelEditor = false;

	// Settings 
	// Save preference vars to EditorPerProjectIni, empty for now??
	void SavePreference();
	void LoadPreference();
	
	//Toolbar functions
	void SwitchToNewTool(ETiledLevelEditTool NewTool);
	bool IsTargetToolChecked(ETiledLevelEditTool TargetTool) const;
	void MirrorItem(EAxis::Type MirrorAxis);
	bool CanMirrorItem() const;
	void ToggleGrid();
	void RotateItem(bool IsClockWise = true);
	bool CanRotateItem() const;
	void EditAnotherFloor(bool IsUpper = true);
	void ToggleMultiMode();
	void ToggleSnap();
	bool CanToggleSnap() const;
	bool IsEnableSnapChecked() const;
	

	//Brush Functions
	void SetupBrush();
	void SetupBrushCursorOnly();
	bool ShouldUpdateBrushLocation();
	FVector2D GetMouse2DLocation(FEditorViewportClient* ViewportClient, int32 InMouseX, int32 InMouseY);
	void UpdateGirdLocation(FEditorViewportClient* ViewportClient, int32 InMouseX, int32 InMouseY);
	void UpdateEyedropperTarget(FEditorViewportClient* ViewportClient, int32 InMouseX, int32 InMouseY);
	void EnterNewGrid(FIntVector NewTilePosition); // also include point
	void EnterNewEdge(FTiledLevelEdge NewEdge);
	void SelectionStart(FEditorViewportClient* ViewportClient);
	void SelectionEnd(FEditorViewportClient* ViewportClient);
	void SelectionCanceled();
	void EvaluateBoxSelection(FVector2D StartPos, FVector2D EndPos);
	void PaintCopiedStart(bool bForTemplate = false); 
	void PaintCopiedEnd();
	void PaintCopiedInstances(bool bForTemplate = false);
	void PaintStart();
	void PaintEnd();
	void PaintItemInstance();
	bool PaintItemPreparation();
	void QuickErase();
	void EraserStart();
	void EraserEnd();
	void EraseItems();
	void PerformEyedropper();
	void PerformFillTile();
	void PerformFillEdge();
	void SetupFillBoardFromTiles();
	void UpdateFillBoardFromGround();
	template <typename T>
	void CreateEmptyGaps(TArray<T>& InCandidatePoints, float Ratio); // randomly remove candidate points 
	void UpdateStatics();
	int32 GetNumEraserActiveItems() const;
	UTiledLevelItem* FindItemFromMeshPtr(UStaticMesh* MeshPtr);

	// Core params
	ATiledLevelEditorHelper* Helper = nullptr;
	ATiledLevel* ActiveLevel = nullptr;
	ATiledLevelSelectHelper* SelectionHelper = nullptr;
	TSharedPtr<FUICommandList> UICommandList;
	const UStaticMeshComponent* PreviewSceneFloor = nullptr; // for fixing line trace hit on it!
	// TODO: can't not handle scale issue when edit on level, I just cache and force it be FVector(1) as a temporary solution
	FVector CachedActiveLevelScale = FVector(1); 
	
	// Brush control params
	bool BrushIsSetup = false;
	EPlacedShapeType CurrentEditShape = EPlacedShapeType::Shape3D; // either editing tile or edge (Tile VS Edge VS Point placement)
	EPlacedType ActivePlacedType = EPlacedType::Block;
	bool bStraightEdit = false; // make brush only move vertical or horizontal, active when ctl is pressed 
	bool ShouldRotateBrushExtent = false;
	ETiledLevelBrushAction ActiveBrushAction = ETiledLevelBrushAction::None;
	FIntVector CurrentTilePosition{0, 0, 0};
	FIntVector FixedTiledPosition{0, 0, 0}; // use for directional paint
	TArray<FTilePlacement> LastPlacedTiles;
	TArray<FEdgePlacement> LastPlacedEdges;
	TArray<FPointPlacement> LastPlacedPoints;
	FTiledLevelEdge CurrentEdge;
	FTiledLevelEdge FixedEdge;
	bool IsMouseInsideCanvas = false;
	bool IsInsideValidArea = false;
	bool IsHelperVisible = true;
	bool IsMultiMode = false;
	bool bSnapEnabled = false;
	bool IsMirrored_X = false;
	bool IsMirrored_Y = false;
	bool IsMirrored_Z = false;
	UTiledLevelItem* EyedropperTarget = nullptr;
	TSet<UTiledLevelItem*> ExistingItems;
	void* CursorResource_Select = nullptr; 
	void* CursorResource_Pen = nullptr;
	void* CursorResource_Eraser = nullptr;
	void* CursorResource_Eyedropper = nullptr;
	void* CursorResource_Fill = nullptr;
	uint8 CachedBrushRotationIndex = 0;
	
	// selection related params
	bool IsInstancesSelected = false;
	FIntPoint SelectionBeginMousePos; // for draw hud
	FVector2D SelectionBeginWorldPos; // for actual implement copy...
	TArray<bool> CachedFloorsVisibility;

	// Fill tool params
	TArray<TArray<int>> Board;
	TArray<FIntPoint> CandidateFillTiles;
	TArray<FTiledLevelEdge> CandidateFillEdges;
	int MaxZInFillItems = 1;

	
	// Input Params
	bool bIsAltDown = false;
	bool bIsShiftDown = false;
	bool bIsControlDown = false;

};
