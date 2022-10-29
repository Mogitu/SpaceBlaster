// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelTypes.h"
#include "Engine/EngineTypes.h"

class ATiledLevel;

class TILEDLEVELRUNTIME_API FTiledLevelUtility
{
public:
	
	static bool IsTilePlacementOverlapping(const FTilePlacement& Tile1, const FTilePlacement& Tile2);
	static bool IsEdgePlacementOverlapping(const FEdgePlacement& Wall1, const FEdgePlacement& Wall2);
	static bool IsEdgeOverlapping(const FTiledLevelEdge& Edge1, const FVector& Extent1, const FTiledLevelEdge& Edge2, const FVector& Extent2);
	static bool IsEdgeInsideTile(const FTiledLevelEdge& Edge, const FIntVector& EdgeExtent, const FIntVector& TilePosition, const FIntVector& TileExtent);
	static bool IsPointPlacementOverlapping(const FPointPlacement& Point1, int ZExtent1, const FPointPlacement& Point2, int ZExtent2);
	static bool IsPointOverlapping(const FIntVector& Point1, int ZExtent1, const FIntVector& Point2, int ZExtent2);
	static bool IsPointInsideTile(FIntVector PointPosition, int PointZExtent, const FIntVector TilePosition, const FIntVector& TileExtent);
	static bool IsEdgeInsideArea(const FTiledLevelEdge& TestEdge, int32 Length, FIntPoint AreaSize);

	// from placement data to get instance indices
	static void FindInstanceIndexByPlacement(TArray<int32>& FoundIndex,const TArray<float>& CustomData,const TArray<float>& SearchData);
	// make HISM custom data from placement
	static TArray<float> ConvertPlacementToHISM_CustomData(const FTilePlacement& P);
	static TArray<float> ConvertPlacementToHISM_CustomData(const FEdgePlacement& P);
	static TArray<float> ConvertPlacementToHISM_CustomData(const FPointPlacement& P);
	static void SetSpawnedActorTag(const FTilePlacement& P, AActor* TargetActor);
	static void SetSpawnedActorTag(const FEdgePlacement& P, AActor* TargetActor);
	static void SetSpawnedActorTag(const FPointPlacement& P, AActor* TargetActor);

	static TArray<FIntVector> GetOccupiedPositions(class UTiledLevelItem* Item, FIntVector StartPosition, bool ShouldRotate);
	static TArray<FIntVector> GetOccupiedPositions(class UTiledLevelItem* Item, FTiledLevelEdge StartEdge);
	
	// used in both set editor and level helper..., will return init mesh location
	static void ApplyPlacementSettings(const FVector& TileSize, UTiledLevelItem* Item,
		class UStaticMeshComponent* TargetMeshComponent,
		class UTiledLevelGrid* TargetBrushGrid,
		class USceneComponent* Center);

	static void ApplyPlacementSettings_TiledActor(const FVector& TileSize, UTiledLevelItem* Item,
		class UTiledLevelGrid* TargetBrushGrid,
		class USceneComponent* Center);
	// returns movement distance
	static float TrySnapPropToFloor(const FVector& InitLocation, uint8 RotationIndex, const FVector& TileSize ,UTiledLevelItem* Item, UStaticMeshComponent* TargetMeshComponent);
	static void TrySnapPropToWall(const FVector& InitLocation, uint8 RotationIndex, const FVector& TileSize, UTiledLevelItem* Item, UStaticMeshComponent* TargetMeshComponent, float Z_Offset);

	// for fixing duplicate multiple tiled level, the attached actors will not spawn...
	static void RequestToResetInstances(const class ATiledLevel* RequestedLevel);

	static FString GetFloorNameFromPosition(int FloorPositionIndex);

	// Fill tool algorithms
	static void FloodFill(TArray<TArray<int>>& InBoard, const FIntPoint& InBoardSize, int X , int Y, TArray<FIntPoint>& FilledTarget);
	static void FloodFillByEdges(const TArray<FTiledLevelEdge>& BlockingEdges, TArray<TArray<int>>& InBoard, const FIntPoint& InBoardSize, int X, int Y, TArray<FIntPoint>& FilledTarget);
	static void GetConsecutiveTiles(TArray<TArray<int>>& InBoard, FIntPoint InBoardSize, int X, int Y, TArray<FIntPoint>& OutTiles);
	static TArray<float> GetWeightedCoefficient(TArray<float>& RawCoefficientArray);
	static bool GetFeasibleFillTile(UTiledLevelItem* InItem, int& InRotationIndex, TArray<FIntPoint>& InCandidatePoints, FIntPoint& OutPoint);
	static TArray<FTiledLevelEdge> GetAreaEdges(const TSet<FIntPoint>& Region, int Z = 1, bool IsOuter = true);
	static TArray<FTiledLevelEdge> GetAreaEdges(const TSet<FIntVector>& Region, bool IsOuter = true); // create an easier overload
	static TSet<FIntVector> GetAreaPoints(const TSet<FIntVector>& Region, bool IsOuter = true);
	static bool GetFeasibleFillEdge(UTiledLevelItem* InItem, TArray<FTiledLevelEdge>& InCandidateEdges, FTiledLevelEdge& OutEdge);

	// enum conversion
	static EPlacedShapeType PlacedTypeToShape(const EPlacedType& PlacedType);

	// gametime utilities
	static void ApplyGametimeRequiredTransform(ATiledLevel* TargetTiledLevel, FVector TargetTileSize);
	static bool CheckOverlappedTiledLevels(ATiledLevel* FirstLevel, ATiledLevel* SecondLevel);

	// Copy from ProceduralMeshConversion::BuildMeshDescription
    static struct FMeshDescription BuildMeshDescription(class UProceduralMeshComponent* ProcMeshComp);
	// Copy from FProceduralMeshComponentDetails::ClickedOnConvertToStaticMesh and with minor modifications...
	static class UProceduralMeshComponent* ConvertTiledLevelAssetToProcMesh(class UTiledLevelAsset* TargetAsset, int TargetLOD = 0, UObject* Outer = nullptr, EObjectFlags Flags = RF_NoFlags);


	

private:
	static bool IsRequested;
	static FTimerHandle TimerHandle;

};


/** TODO: IntVector, IntPoint, or vector ...  
 * Tile or Grid??
 * The naming convention should be the same, the type to use should be the same?
 */
