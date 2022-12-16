// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelTypes.h"
#include "GameFramework/Actor.h"
#include "TiledLevelEditorHelper.generated.h"

class UTiledLevelItem;
class UTiledLevelAsset;

UCLASS(NotPlaceable, NotBlueprintable)
class TILEDLEVELRUNTIME_API ATiledLevelEditorHelper : public AActor
{
	GENERATED_UCLASS_BODY()

protected:
	// ATiledLevelEditorHelper();
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	
public:
	UPROPERTY()
	class USceneComponent* Root;
	
	UPROPERTY()
	class UTiledLevelGrid* AreaHint;

	UPROPERTY()
	class UProceduralMeshComponent* FloorGrids;

	UPROPERTY()
	class UProceduralMeshComponent* FillPreviewGrids;
	
	UPROPERTY()
	class UTiledLevelGrid* CustomGizmo;
	
	UPROPERTY()
	class UTiledLevelGrid* Brush;
	
	UPROPERTY()
	class USceneComponent* Center;
	
	UPROPERTY()
	UStaticMeshComponent* PreviewMesh;

	// for debug
	UPROPERTY()
	class UTiledLevelGrid* CenterHint;

	void SetupPaintBrush(UTiledLevelItem* Item);
	void SetupEraserBrush(FIntVector EraserExtent, EPlacedType SnapTarget);
	void ResetBrush();
	void ToggleQuickEraserBrush(bool bIsSet);
	void ToggleValidBrush(bool IsValid);
	void MoveBrush(FIntVector TilePosition, bool IsSnapEnabled = true);
	void MoveBrush(FTiledLevelEdge TileEdge, bool IsSnapEnable = true ); //this move to edge will not handle vertical or horizontal
	void RotateBrush(bool IsCW, EPlacedType PlacedType, bool& ShouldRotateExtent);
	void RotateEraserBrush(bool CW, bool& ShouldEraseVertical);
	bool IsBrushExtentChanged();	
	FTransform GetPreviewPlacementWorldTransform();
	void ResizeArea(float X, float Y, float Z, int Num_X, int Num_Y, int Num_Floors, int LowestFloor);
	void MoveFloorGrids(int TargetFloorIndex);
	void LoadAsset(UTiledLevelAsset* InAsset) { LevelAsset = InAsset; }
	void SetHelperGridsVisibility(bool IsVisible);

	void UpdateFillPreviewGrids(TArray<FIntPoint> InFillBoard, int InFillFloorPosition = 0, int InFillHeight = 1);
	void UpdateFillPreviewEdges(TArray<FTiledLevelEdge> InEdgePoints, int InFillHeight = 1);
	
	uint8 BrushRotationIndex;

	// For gameplay support
	bool IsEditorHelper = false; 
	void SetupPreviewBrushInGame(UTiledLevelItem* Item);
	void SetShouldUsePreviewMaterial(bool ShouldUse) { ShouldUsePreviewMaterial = ShouldUse; }
	void SetCanBuildPreviewMaterial(UMaterialInterface* NewMaterial) { CanBuildPreviewMaterial = NewMaterial; }
	void SetCanNotBuildPreviewMaterial(UMaterialInterface* NewMaterial) { CanNotBuildPreviewMaterial = NewMaterial; }
	void UpdatePreviewHint(bool CanBuild);
	void SetTileSize(const FVector& NewTileSize) { TileSize = NewTileSize; }

	// TODO: leave for next update...
	/*UFUNCTION(NetMulticast, Reliable)
	void SpawnGametimeLevel();*/

	// prevent to trigger nav update?
	// INavRelevantInterface begin
	// virtual ENavDataGatheringMode GetGeometryGatheringMode() const { return ENavDataGatheringMode::Lazy; }
	// INavRelevantInterface end
	
private:
	UPROPERTY()
	class UTiledLevelAsset* LevelAsset;

	UPROPERTY()
	AActor* PreviewActor;

	UPROPERTY()
	UTiledLevelItem* ActiveItem;
	
	UPROPERTY()
	UMaterialInterface* M_FloorGrids;

	UPROPERTY()
	UMaterialInterface* M_HelperFloor;

	UPROPERTY()
	UMaterialInterface* M_FillPreview;

	FVector TileSize;
	FVector TileExtent;
	FVector PreviewMeshInitLocation;
	FVector PreviewActorRelativeLocation;
	FVector CachedCenterScale{1, 1, 1};
	FVector CachedBrushLocation;
	bool IsBrushValid;
	TArray<FIntPoint> CacheFillTiles;
	TArray<FTiledLevelEdge> CacheFillEdges;
	
	// If not, use original material for valid, and hidden actors for not valid 
	bool ShouldUsePreviewMaterial = true;
	
	UPROPERTY()
	UMaterialInterface* CanBuildPreviewMaterial;
	UPROPERTY()
	UMaterialInterface* CanNotBuildPreviewMaterial;
	
};
