// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelEditorLog.h"
#include "TiledLevelTypes.generated.h"


UENUM()
enum class EPlacedType :uint8
{
    Block = 0,
	Floor = 1,
	Wall = 2,
	Edge = 3,
	Pillar = 4,
	Point = 5,
	Any = 6 UMETA(Hidden) 
};

enum EPlacedShapeType : uint8
{
	Shape3D = 0,
	Shape2D = 1,
	Shape1D = 2
};

/*
 * Block, Floor -> Shape3D
 * Wall, Edge -> Shape2D
 * Pillar, Point -> Shpae1D
 */

UENUM()
enum class ETLStructureType :uint8
{
	Structure = 0,
	Prop = 1
};

UENUM()
enum class ETLSourceType :uint8
{
	Actor = 0,
	Mesh = 1
};

// TODO: for future updates...
UENUM()
enum class ETiledLevelStyle: uint8
{
	// The most common way...
	TopDown,
	// For metroidvania game
	Vertical,
};

UENUM()
enum class EPivotPosition: uint8
{
	Bottom,
	Center,
	Corner,
	Side,
	Fit
};

/*
 * Add "UPROPERTY" is a MUST in so many case
 * Otherwise, value of variables will become default...
 * not adding the dll output in a struct will cause unresolved external symbol... a case i have never seen...
 */


USTRUCT()
struct TILEDLEVELRUNTIME_API FItemPlacement
{
	GENERATED_BODY()

	virtual ~FItemPlacement() {}
	
	UPROPERTY(EditAnywhere, Category="Data")
	class UTiledItemSet* ItemSet = nullptr;

	UPROPERTY(VisibleAnywhere, Category="Data")
	FGuid ItemID;

	class UTiledLevelItem* GetItem() const;

	UPROPERTY(EditAnywhere, Category="Data")
	FTransform TileObjectTransform;

	UPROPERTY(EditAnywhere, Category="Data")
	bool IsMirrored = false;

	virtual void OnMoveFloor(float TileSizeZ, bool IsUp = true, int Times = 1)
	{
		int D = IsUp? 1 : -1;
		TileObjectTransform.AddToTranslation(FVector(0, 0, TileSizeZ * Times * D));
	};
};

// includes block, floor, pillar, and point
/*
 * Should move pillar & point to another Placement?
 */
USTRUCT()
struct FTilePlacement : public FItemPlacement
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, Category="Item")
	FIntVector GridPosition;
	
	UPROPERTY(VisibleAnywhere, Category="Item")
	FIntVector Extent;

	FBox2D GetBox2D(FVector InTileSize = FVector(1)) const
	{
		// offset minor distance to make sure no overlap
		return FBox2D(FVector2D(FVector(GridPosition) * InTileSize) + FVector2D(5),
			FVector2D(FVector(GridPosition + Extent) * InTileSize) - FVector2D(5));
	}

	FBox GetBox(FVector InTileSize = FVector(1))
	{
		return FBox(FVector(GridPosition) * InTileSize, FVector(GridPosition + Extent) * InTileSize);
	}
	
	virtual void OnMoveFloor(float TileSizeZ, bool IsUp, int Times = 1) override
	{
		Super::OnMoveFloor(TileSizeZ, IsUp, Times);
		const int D = IsUp? 1 : -1;
		GridPosition.Z += D * Times;
	}

	void OffsetGridPosition(FIntVector DeltaGridPosition, FVector TileSize)
	{
		GridPosition += DeltaGridPosition;
		TileObjectTransform.AddToTranslation(FVector(DeltaGridPosition) * TileSize);
	}

	// get separate tile positions ex, 2*2*2 tile placement will get 8 tile positions
	TArray<FIntVector> GetOccupiedTilePositions() const
	{
		TArray<FIntVector> OutPositions;
		for (int X = 0; X < Extent.X; X++)
		{
			for (int Y = 0; Y < Extent.Y; Y++)
			{
				for (int Z = 0; Z < Extent.Z; Z++)
				{
					OutPositions.Add(GridPosition + FIntVector(X, Y, Z));
				}
			}
		}
		return OutPositions;
	}

	bool operator== (const FTilePlacement& Other) const
	{
		return GridPosition == Other.GridPosition && Extent == Other.Extent && ItemID == Other.ItemID;
	}
};

UENUM(BlueprintType)
enum class EEdgeType: uint8
{
	Horizontal,
	Vertical,
};

USTRUCT(BlueprintType)
struct FTiledLevelEdge
{
	GENERATED_BODY()

	UPROPERTY()
	int X;
	
	UPROPERTY()
	int Y;

	UPROPERTY()
	int Z;

	UPROPERTY()
	EEdgeType EdgeType;

	FTiledLevelEdge()
		: X(0), Y(0), Z(0), EdgeType(EEdgeType::Horizontal)
	{}

	FTiledLevelEdge(const int& InX, const int& InY, const int& InZ,const EEdgeType& InEdgeType)
		: X(InX), Y(InY), Z(InZ), EdgeType(InEdgeType)
	{}

	FTiledLevelEdge(const FIntVector& InPosition ,const EEdgeType& InEdgeType)
		: X(InPosition.X), Y(InPosition.Y), Z(InPosition.Z), EdgeType(InEdgeType)
	{}

	FIntVector GetEdgePosition() const
	{
		return FIntVector(X, Y, Z);
	}
	
	FString ToString() const
	{
		if (EdgeType == EEdgeType::Horizontal)
			return FString::Printf(TEXT("%d,%d,%d-H"), X,Y,Z);
		return FString::Printf(TEXT("%d,%d,%d-V"), X,Y,Z);
	}
	
	bool operator== (const FTiledLevelEdge& OtherEdge) const
	{
		return X == OtherEdge.X && Y == OtherEdge.Y && Z == OtherEdge.Z && EdgeType == OtherEdge.EdgeType;
	}

	bool operator!= (const FTiledLevelEdge& OtherEdge) const
	{
		return X != OtherEdge.X || Y != OtherEdge.Y || Z != OtherEdge.Z || EdgeType != OtherEdge.EdgeType;
	}

	bool operator< (const FTiledLevelEdge& OtherEdge) const
	{
		if (EdgeType != OtherEdge.EdgeType)
			return EdgeType == EEdgeType::Vertical;
		if (Z != OtherEdge.Z)
			return Z < OtherEdge.Z;
		if (EdgeType == EEdgeType::Horizontal)
		{
			if (Y != OtherEdge.Y)
				 return Y < OtherEdge.Y;
			return X < OtherEdge.X;
		}
		if (X != OtherEdge.X)
			 return X < OtherEdge.X;
		return Y < OtherEdge.Y;
	}
};

// includes both wall and Edge, have "vertical" and "horizontal"
USTRUCT()
struct FEdgePlacement : public FItemPlacement
{
	GENERATED_BODY()

	UPROPERTY()
	FTiledLevelEdge Edge;
	
	FVector GetEdgePosition() const { return FVector(Edge.X, Edge.Y, Edge.Z); }

	// Add 1/4 tile size as thickness of this box in each side
	FBox2D GetBox2D(FVector InTileSize = FVector(1), FVector MyExtent = FVector(1)) const
	{
		FVector2D Min = FVector2D((GetEdgePosition() - FVector(0.25)));
		FVector2D Max = Edge.EdgeType == EEdgeType::Horizontal?
			Min + FVector2D(MyExtent.X + 0.5, 0.5) : Min + FVector2D(0.5, MyExtent.X + 0.5);
		Min *= FVector2D(InTileSize);
		Max *= FVector2D(InTileSize);
		// Minor offset edge to prevent overlap...
		Min += FVector2D(InTileSize) * 0.1;
		Max -= FVector2D(InTileSize) * 0.1;
		return FBox2D(Min, Max);
	}

	FBox GetBox(FVector InTileSize = FVector(1), FVector MyExtent = FVector(1)) const
	{
		FVector Min = FVector(GetEdgePosition() - FVector(0.25));
		FVector Max = Edge.EdgeType == EEdgeType::Horizontal?
			Min + FVector(MyExtent.X + 0.5, 0.5, MyExtent.Z) : Min + FVector(0.5, MyExtent.X + 0.5, MyExtent.Z);
		Min *= InTileSize;
		Max *= InTileSize;
		return FBox(Min,Max);
	}

	void SetEdgePosition(FIntVector NewEdgePosition)
	{
		Edge.X = NewEdgePosition.X;
		Edge.Y = NewEdgePosition.Y;
		Edge.Z = NewEdgePosition.Z;
	}

	void ToggleEdgeType()
	{
		if (Edge.EdgeType == EEdgeType::Horizontal)
			Edge.EdgeType = EEdgeType::Vertical;
		else
			Edge.EdgeType = EEdgeType::Horizontal;
	}

	void OffsetEdgePosition(FIntVector DeltaPosition, FVector InTileSize)
	{
		Edge.X += DeltaPosition.X;
		Edge.Y += DeltaPosition.Y;
		Edge.Z += DeltaPosition.Z;
		TileObjectTransform.AddToTranslation(FVector(DeltaPosition) * InTileSize);
	}

	// separate into small edges according to its width and height 
	TArray<FTiledLevelEdge> GetOccupiedEdges(FVector MyExtent) const
	{
		TArray<FTiledLevelEdge> OutEdges;
		for (int x = 0; x < MyExtent.X; x++)
		{
			for (int z = 0; z < MyExtent.Z; z++)
			{
				if (Edge.EdgeType==EEdgeType::Horizontal)
				{
					OutEdges.Add(FTiledLevelEdge(Edge.X + x, Edge.Y, Edge.Z + z, Edge.EdgeType));
				}
				else
				{
					OutEdges.Add(FTiledLevelEdge(Edge.X, Edge.Y + x, Edge.Z + z, Edge.EdgeType));
				}
			}
		}
		return OutEdges;
	}

	virtual void OnMoveFloor(float TileSizeZ, bool IsUp, int Times = 1) override
	{
		Super::OnMoveFloor(TileSizeZ, IsUp, Times);
		int D = IsUp? 1 : -1;
		Edge.Z += Times * D;
	}
	
	bool operator== (const FEdgePlacement& Other) const
	{
		return Edge == Other.Edge && ItemID == Other.ItemID;
	}
};


USTRUCT()
struct FPointPlacement : public FItemPlacement
{
	GENERATED_BODY()

	UPROPERTY()
	FIntVector GridPosition;

	FVector2D GetPoint(FVector InTileSize) const
	{
		return FVector2D(GridPosition.X * InTileSize.X, GridPosition.Y * InTileSize.Y);
	}

	void OffsetPointPosition( FIntVector DeltaPosition, FVector InTileSize)
	{
		GridPosition += DeltaPosition;
		TileObjectTransform.AddToTranslation(FVector(DeltaPosition) * InTileSize);
	}
	
	virtual void OnMoveFloor(float TileSizeZ, bool IsUp, int Times) override
	{
		Super::OnMoveFloor(TileSizeZ, IsUp, Times);
		int D = IsUp? 1 : -1;
		GridPosition.Z += Times * D;
	}
	
	bool operator== (const FPointPlacement& Other) const
	{
		return GridPosition == Other.GridPosition && ItemID == Other.ItemID;
	}
};




/*
 * Just copy all placement data from Tiled Level Asset to this game data... let it handle all the rest...
 */
USTRUCT(BlueprintType)
struct FTiledLevelGameData
{
	GENERATED_BODY()

	// use this to control which floor should hide
	UPROPERTY(VisibleAnywhere, Category="Data")
	TArray<int> HiddenFloors;

	UPROPERTY(VisibleAnywhere, Category="Data")
	TArray<FTilePlacement> BlockPlacements;
	
	UPROPERTY(VisibleAnywhere, Category="Data")
	TArray<FTilePlacement> FloorPlacements;
	
	UPROPERTY(VisibleAnywhere, Category="Data")
	TArray<FEdgePlacement> WallPlacements;
	
	UPROPERTY(VisibleAnywhere, Category="Data")
	TArray<FEdgePlacement> EdgePlacements;
	
	UPROPERTY(VisibleAnywhere, Category="Data")
	TArray<FPointPlacement> PillarPlacements;
	
	UPROPERTY(VisibleAnywhere, Category="Data")
	TArray<FPointPlacement> PointPlacements;

	UPROPERTY(VisibleAnywhere, Category="Data")
	TArray<FBox> Boundaries;

	void Empty();
	
	bool RemovePlacement(FTransform CompareTransform, FGuid ItemID);
	void RemovePlacements(const TArray<FTilePlacement>& ToDelete);
	void RemovePlacements(const TArray<FEdgePlacement>& ToDelete);
	void RemovePlacements(const TArray<FPointPlacement>& ToDelete);

	void operator+=(const FTiledLevelGameData& Other)
	{
		BlockPlacements.Append(Other.BlockPlacements);
		FloorPlacements.Append(Other.FloorPlacements);
		WallPlacements.Append(Other.WallPlacements);
		EdgePlacements.Append(Other.EdgePlacements);
		PillarPlacements.Append(Other.PillarPlacements);
		PointPlacements.Append(Other.PointPlacements);
		Boundaries.Append(Other.Boundaries);
	}

	void SetFocusFloor(int FloorPosition);
};

UENUM()
enum class ERestrictionType : uint8
{
	AllowBuilding   UMETA(ToolTip = "Allow player to build in defined area"),
	DisallowBuilding  UMETA(ToolTip = "Disallow player to build in defined area"),
	AllowRemoving   UMETA(ToolTip = "Allow player to remove in defined area"),
	DisallowRemoving  UMETA(ToolTip = "Disallow player to remove in defined area"),
	AllowBuildingAndRemoving UMETA(ToolTip = "Allow both build and remove in defined area"),
	DisallowBuildingAndRemoving UMETA(ToolTip = "Disallow both build and remove in defined area")
};