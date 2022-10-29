// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelUtility.h"

#include "KismetProceduralMeshLibrary.h"
#include "ProceduralMeshComponent.h"
#include "Materials/Material.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"
#include "TiledLevel.h"
#include "TiledLevelAsset.h"
#include "TiledLevelEditorLog.h"
#include "TiledLevelGrid.h"
#include "TiledLevelItem.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"

bool FTiledLevelUtility::IsTilePlacementOverlapping(const FTilePlacement& Tile1, const FTilePlacement& Tile2)
{
	FBox Box1 = FBox(FVector(Tile1.GridPosition) + FVector(0.05), FVector(Tile1.GridPosition) + FVector(Tile1.Extent) - FVector(0.05));
	FBox Box2 = FBox(FVector(Tile2.GridPosition) + FVector(0.05), FVector(Tile2.GridPosition) + FVector(Tile2.Extent) - FVector(0.05));
	return Box1.Intersect(Box2);
}

bool FTiledLevelUtility::IsEdgePlacementOverlapping(const FEdgePlacement& Wall1, const FEdgePlacement& Wall2)
{
	return IsEdgeOverlapping(Wall1.Edge, Wall1.GetItem()->Extent, Wall2.Edge, Wall2.GetItem()->Extent);
}

bool FTiledLevelUtility::IsEdgeOverlapping(const FTiledLevelEdge& Edge1, const FVector& Extent1,
	const FTiledLevelEdge& Edge2, const FVector& Extent2)
{
	if (Edge1.EdgeType != Edge2.EdgeType) return false;
	float Padding = 0.05f;
	if (Edge1.EdgeType == EEdgeType::Horizontal)
	{
		if (Edge1.Y != Edge2.Y) return false;
		FBox2D B1 = FBox2D(FVector2D(Edge1.X+Padding, Edge1.Z+Padding), FVector2D(Edge1.X+Extent1.X-Padding, Edge1.Z+Extent1.Z-Padding));
		FBox2D B2 = FBox2D(FVector2D(Edge2.X+Padding, Edge2.Z+Padding), FVector2D(Edge2.X+Extent2.X-Padding, Edge2.Z+Extent2.Z-Padding));
		return B1.Intersect(B2);
	}
	if (Edge1.X != Edge2.X) return false;
	FBox2D B1 = FBox2D(FVector2D(Edge1.Y+Padding, Edge1.Z+Padding), FVector2D(Edge1.Y+Extent1.X-Padding, Edge1.Z+Extent1.Z-Padding));
	FBox2D B2 = FBox2D(FVector2D(Edge2.Y+Padding, Edge2.Z+Padding), FVector2D(Edge2.Y+Extent2.X-Padding, Edge2.Z+Extent2.Z-Padding));
	return B1.Intersect(B2);
}

bool FTiledLevelUtility::IsEdgeInsideTile(const FTiledLevelEdge& Edge, const FIntVector& EdgeExtent,
	const FIntVector& TilePosition, const FIntVector& TileExtent)
{
	for (int x = 0 ; x <= TileExtent.X ; x++)
	{
		FTiledLevelEdge V{TilePosition + FIntVector(x, 0, 0), EEdgeType::Vertical};
		if (IsEdgeOverlapping(Edge, FVector(EdgeExtent), V, FVector(TileExtent.Y, TileExtent.Y, TileExtent.Z)))
		{
			return true;
		}
	}
	for (int y = 0 ; y <= TileExtent.Y ; y++)
	{
		FTiledLevelEdge H{TilePosition + FIntVector(0, y, 0), EEdgeType::Horizontal};
		if (IsEdgeOverlapping(Edge, FVector(EdgeExtent), H, FVector(TileExtent.X, TileExtent.X, TileExtent.Z)))
		{
			return true;
		}
	}
	return false;
}

bool FTiledLevelUtility::IsPointInsideTile(FIntVector PointPosition, int PointZExtent,
	const FIntVector TilePosition, const FIntVector& TileExtent)
{
	for (int x = 0 ; x <= TileExtent.X ; x++)
	{
		for (int y = 0 ; y <= TileExtent.Y ; y++)
		{
			if (IsPointOverlapping(PointPosition, PointZExtent, TilePosition + FIntVector(x, y, 0), TileExtent.Z))
				return true;
		}
	}
	return false;
}

bool FTiledLevelUtility::IsPointPlacementOverlapping(const FPointPlacement& Point1, int ZExtent1,
	const FPointPlacement& Point2, int ZExtent2)
{
	return IsPointOverlapping(Point1.GridPosition, ZExtent1, Point2.GridPosition, ZExtent2);
}

bool FTiledLevelUtility::IsPointOverlapping(const FIntVector& Point1, int ZExtent1, const FIntVector& Point2,
	int ZExtent2)
{
	if (Point1.X != Point2.X || Point1.Y != Point2.Y) return false;
	return (Point1.Z >= Point2.Z && Point1.Z + ZExtent1 <= Point2.Z + ZExtent2) ||
		(Point2.Z >= Point1.Z && Point2.Z + ZExtent2 <= Point1.Z + ZExtent1); 
}

// TODO: Z is ignored... but its OK?
bool FTiledLevelUtility::IsEdgeInsideArea(const FTiledLevelEdge& TestEdge, int32 Length, FIntPoint AreaSize)
{
    return TestEdge.EdgeType == EEdgeType::Horizontal?
        TestEdge.X >= 0 && TestEdge.Y >=0 && TestEdge.X < AreaSize.X - Length + 1 && TestEdge.Y <= AreaSize.Y:
        TestEdge.X >= 0 && TestEdge.Y >=0 && TestEdge.X <= AreaSize.X && TestEdge.Y < AreaSize.Y - Length + 1;
}

void FTiledLevelUtility::FindInstanceIndexByPlacement(TArray<int32>& FoundIndex, const TArray<float>& CustomData,
	const TArray<float>& SearchData)
{
	for (int32 i = 0; i < CustomData.Num()/6; i++)
	{
		if (FoundIndex.Contains(i)) continue;
		for (int j = 0; j < 6;j++)
		{
			if (CustomData[6*i + j] != SearchData[j]) break;
			if (j == 5)
			{
				FoundIndex.Add(i);
				return;
			}
		}
	}
}

TArray<float> FTiledLevelUtility::ConvertPlacementToHISM_CustomData(const FTilePlacement& P)
{
	return{
		float(P.GridPosition.X),
		float(P.GridPosition.Y),
		float(P.GridPosition.Z),
		float(P.Extent.X),
		float(P.Extent.Y),
		float(P.Extent.Z)
	};
}

TArray<float> FTiledLevelUtility::ConvertPlacementToHISM_CustomData(const FEdgePlacement& P)
{
	return{
		float(P.Edge.X),
		float(P.Edge.Y),
		float(P.Edge.Z),
		float(P.GetItem()->Extent.X),
		float(P.GetItem()->Extent.Z),
		P.Edge.EdgeType == EEdgeType::Horizontal? -1.0f : 0.f // use -1 and 0 , this can help distinguish floor and wall.
	};
}

TArray<float> FTiledLevelUtility::ConvertPlacementToHISM_CustomData(const FPointPlacement& P)
{
	return{
		float(P.GridPosition.X),
		float(P.GridPosition.Y),
		float(P.GridPosition.Z),
		float(1),
		float(1),
		float(P.GetItem()->Extent.Z)
	};
}

void FTiledLevelUtility::SetSpawnedActorTag(const FTilePlacement& P, AActor* TargetActor)
{
	TargetActor->Tags.Add(FName(FString::Printf(TEXT("X=%d,Y=%d,Z=%d"),P.GridPosition.X,P.GridPosition.Y,P.GridPosition.Z)));
	TargetActor->Tags.Add(FName(FString::Printf(TEXT("X=%d,Y=%d,Z=%d"),P.Extent.X,P.Extent.Y,P.Extent.Z)));
	TargetActor->Tags.Add(FName(P.ItemID.ToString()));
}

void FTiledLevelUtility::SetSpawnedActorTag(const FEdgePlacement& P, AActor* TargetActor)
{
	TargetActor->Tags.Add(FName(FString::Printf(TEXT("X=%d,Y=%d,Z=%d"),P.Edge.X,P.Edge.Y,P.Edge.Z)));
	TargetActor->Tags.Add(FName(FString::Printf(TEXT("X=%f,Y=%f,Z=%f"),P.GetItem()->Extent.X,P.GetItem()->Extent.Z,P.Edge.EdgeType == EEdgeType::Horizontal? -1.0f : 0.f)));
	TargetActor->Tags.Add(FName(P.ItemID.ToString()));
}

void FTiledLevelUtility::SetSpawnedActorTag(const FPointPlacement& P, AActor* TargetActor)
{
	TargetActor->Tags.Add(FName(FString::Printf(TEXT("X=%d,Y=%d,Z=%d"),P.GridPosition.X,P.GridPosition.Y,P.GridPosition.Z)));
	TargetActor->Tags.Add(FName(P.GetItem()->Extent.ToString()));
	TargetActor->Tags.Add(FName(P.ItemID.ToString()));
}

TArray<FIntVector> FTiledLevelUtility::GetOccupiedPositions(UTiledLevelItem* Item, FIntVector StartPosition,
	bool ShouldRotate)
{
	TArray<FIntVector> OutPoints;
	int X, Y, Z;
	if (ShouldRotate)
	{
		X = Item->Extent.Y;
		Y = Item->Extent.X;
	}
	else
	{
		X = Item->Extent.X;
		Y = Item->Extent.Y;
	}
	Z = Item->Extent.Z;
	
	EPlacedShapeType PlacedShape = PlacedTypeToShape(Item->PlacedType);
	if (PlacedShape == EPlacedShapeType::Shape3D)
	{
		for (int x=0; x<X; x++)
		{
			for (int y=0; y<Y; y++)
			{
				for (int z=0; z<Z; z++)
				{
					OutPoints.Add(FIntVector(StartPosition.X+x, StartPosition.Y+y, StartPosition.Z+z ));
				}
			}
		}
	}
	else if (PlacedShape == EPlacedShapeType::Shape1D)
	{
		for (int z = 0 ; z < Z; z++)
		{
			OutPoints.Add(FIntVector(StartPosition.X, StartPosition.Y, StartPosition.Z + z));
			OutPoints.Add(FIntVector(StartPosition.X + 1, StartPosition.Y, StartPosition.Z + z));
			OutPoints.Add(FIntVector(StartPosition.X, StartPosition.Y + 1, StartPosition.Z + z));
			OutPoints.Add(FIntVector(StartPosition.X + 1, StartPosition.Y + 1, StartPosition.Z + z));
		}
	}
	return OutPoints;
}

TArray<FIntVector> FTiledLevelUtility::GetOccupiedPositions(UTiledLevelItem* Item, FTiledLevelEdge StartEdge)
{
	TArray<FIntVector> OutPoints;
	int EdgeLength = Item->Extent.X;
	int EdgeHeight = Item->Extent.Z;
	if (StartEdge.EdgeType == EEdgeType::Horizontal)
	{
		for (int L=0; L<EdgeLength; L++)
		{
			for (int H=0; H<EdgeHeight; H++)
			{
				OutPoints.Add(FIntVector(StartEdge.X + L, StartEdge.Y, StartEdge.Z + H));
				OutPoints.Add(FIntVector(StartEdge.X + L, StartEdge.Y - 1, StartEdge.Z + H));
			}
		}
	}
	else
	{
		for (int L=0; L<EdgeLength; L++)
		{
			for (int H=0; H<EdgeHeight; H++)
			{
				OutPoints.Add(FIntVector(StartEdge.X, StartEdge.Y + L, StartEdge.Z + H));
				OutPoints.Add(FIntVector(StartEdge.X - 1, StartEdge.Y + L, StartEdge.Z + H));
			}
		}
	}
	return OutPoints;
}

void FTiledLevelUtility::ApplyPlacementSettings(const FVector& TileSize, UTiledLevelItem* Item, UStaticMeshComponent* TargetMeshComponent,
                                                UTiledLevelGrid* TargetBrushGrid, USceneComponent* Center)
{
	if (!TargetMeshComponent->GetStaticMesh()) return;
	FVector BrushExtent;
	FBoxSphereBounds MeshBound = TargetMeshComponent->GetStaticMesh()->GetBounds();
	FVector TileExtent = Item->Extent;
	TargetMeshComponent->ResetRelativeTransform();
	Center->ResetRelativeTransform();
	bool ShouldRotateCenter = MeshBound.BoxExtent.X < MeshBound.BoxExtent.Y;// initial wall structure could be vertical aligned, rather than horizontal

	switch (Item->PlacedType) {
		case EPlacedType::Block:
			BrushExtent = TileExtent * TileSize * 0.5;
			TargetBrushGrid->SetRelativeLocation(BrushExtent);
			break;
		case EPlacedType::Floor:
			BrushExtent = TileExtent * TileSize * 0.5 * FVector(1, 1, 0.25);
			TargetBrushGrid->SetRelativeLocation(BrushExtent);
			break;
		case EPlacedType::Wall:
			BrushExtent = FVector(TileSize.X * TileExtent.X, TileSize.Y * 0.25, TileSize.Z * TileExtent.Z) * 0.5;
			TargetBrushGrid->SetRelativeLocation(BrushExtent * FVector(1, 0, 1));
			if (ShouldRotateCenter)
				Center->SetRelativeRotation(FRotator(0, 90, 0));
			break;
		case EPlacedType::Edge:
			BrushExtent = FVector(TileSize.X * TileExtent.X, TileSize.X * 0.25, TileSize.Z * 0.25) * 0.5;
			TargetBrushGrid->SetRelativeLocation(BrushExtent * FVector(1, 0, 0));
			if (ShouldRotateCenter)
				Center->SetRelativeRotation(FRotator(0,  90, 0));
			break;
		case EPlacedType::Pillar:
			BrushExtent = FVector(TileSize.X * 0.25, TileSize.X * 0.25, TileSize.Z * TileExtent.Z) * 0.5;
			TargetBrushGrid->SetRelativeLocation(BrushExtent * FVector(0, 0, 1));
			break;
		case EPlacedType::Point:
			BrushExtent = FVector(TileSize.X * 0.25, TileSize.X * 0.25, TileSize.Z * 0.25) * 0.5;
			TargetBrushGrid->SetRelativeLocation(FVector(0));
			break;
		default: ;
	}
	TargetBrushGrid->SetBoxExtent(BrushExtent);

	FVector SideMod;
	if (ShouldRotateCenter)
		SideMod = Item->PlacedType==EPlacedType::Wall? FVector(0, 1, 1) : FVector(0, 1, 0);
	else
		SideMod = Item->PlacedType==EPlacedType::Wall? FVector(1, 0, 1) : FVector(1, 0, 0);
	FVector SideDirectionMod;
	if (ShouldRotateCenter)
		SideDirectionMod = MeshBound.Origin.Y > 0? FVector(0) : FVector(0,1,0);
	else
		SideDirectionMod = MeshBound.Origin.X > 0? FVector(0) : FVector(1,0,0);
	
	float FloorMod = Item->PlacedType==EPlacedType::Floor? 0.25 : 1;
	FVector CornerDirectionMod = FVector(
		MeshBound.Origin.X >= 0? 1 : -1,
		MeshBound.Origin.Y >= 0? 1 : -1,
		MeshBound.Origin.Z >= 0? 1 : -1
	);
	if (Item->PlacedType == EPlacedType::Floor) CornerDirectionMod.Z = 1; // stop floor go to upper corner
	switch (Item->PivotPosition)
	{
		case EPivotPosition::Bottom:
			if (Item->bAutoPlacement) TargetMeshComponent->SetRelativeLocation(-MeshBound.Origin + FVector(0, 0, MeshBound.BoxExtent.Z));
			Center->AddLocalOffset(FVector(0, 0, -(TileSize.Z * TileExtent.Z) * 0.5 * FloorMod)); // move the center
			break;
		case EPivotPosition::Center:
			if (Item->bAutoPlacement) TargetMeshComponent->SetRelativeLocation(-MeshBound.Origin);
			break;
		case EPivotPosition::Corner:
			if (Item->bAutoPlacement)
			{
				TargetMeshComponent->SetRelativeLocation(-MeshBound.Origin + MeshBound.BoxExtent);
				CornerDirectionMod = FVector(1);
			}
			if (Item->PlacedType == EPlacedType::Floor)
				Center->AddLocalOffset(-TileSize * TileExtent * FVector(0.5, 0.5, 0.125) * CornerDirectionMod);
			else
				Center->AddLocalOffset(-TileSize * TileExtent * 0.5 * CornerDirectionMod);
			break;
		case EPivotPosition::Side:
			if (Item->bAutoPlacement)
			{
				TargetMeshComponent->SetRelativeLocation(-MeshBound.Origin + MeshBound.BoxExtent * SideMod);
				SideDirectionMod = FVector(0);
			}
			Center->AddLocalOffset(-TileSize * TileExtent * 0.5 * SideMod);
			Center->AddLocalOffset(TileSize * SideDirectionMod * TileExtent);
			break;
		case EPivotPosition::Fit:
			if (Item->PlacedType == EPlacedType::Edge)
			{
				TargetMeshComponent->SetRelativeLocation(-MeshBound.Origin);
			} else
			{
				TargetMeshComponent->SetRelativeLocation(-MeshBound.Origin + FVector(0, 0, MeshBound.BoxExtent.Z));
				Center->AddLocalOffset(FVector(0, 0, -(TileSize.Z * TileExtent.Z) * 0.5));
			}

			float SX, SY, SZ, SW, ST, Thickness;
			switch (Item->PlacedType)
			{
				case EPlacedType::Block:
					SX = TileSize.X * TileExtent.X / (MeshBound.BoxExtent.X * 2);
					SY = TileSize.Y * TileExtent.Y / (MeshBound.BoxExtent.Y * 2);
					SZ = TileSize.Z * TileExtent.Z / (MeshBound.BoxExtent.Z * 2);
					Center->SetRelativeScale3D(FVector(SX, SY, SZ));
					break;
				case EPlacedType::Floor:
					SX = TileSize.X * TileExtent.X / (MeshBound.BoxExtent.X * 2);
					SY = TileSize.Y * TileExtent.Y / (MeshBound.BoxExtent.Y * 2);
					SZ = 1;
					if (Item->bHeightOverride)
					{
						SZ = Item->NewHeight / (MeshBound.BoxExtent.Z * 2);
					}
					Center->SetRelativeLocation	(FVector(0, 0, TileSize.Z * -0.125) );
					Center->SetRelativeScale3D(FVector(SX, SY, SZ));
					break;
				case EPlacedType::Wall:
					ST = 1; // scale of thickness
					Thickness = MeshBound.BoxExtent.X >= MeshBound.BoxExtent.Y ? MeshBound.BoxExtent.Y: MeshBound.BoxExtent.X;
					if (Item->bThicknessOverride)
					{
						ST = Item->NewThickness / (Thickness * 2);
					}
					SZ = TileSize.Z * TileExtent.Z / (MeshBound.BoxExtent.Z * 2);
					if (ShouldRotateCenter)
					{
						SW = TileSize.Y * TileExtent.Y / (MeshBound.BoxExtent.Y * 2);
						Center->SetRelativeScale3D(FVector(ST, SW, SZ));
					}
					else
					{
						SW = TileSize.X * TileExtent.X / (MeshBound.BoxExtent.X * 2);
						Center->SetRelativeScale3D(FVector(SW, ST, SZ));
					}
					break;
				case EPlacedType::Pillar:
					SZ = TileSize.Z * TileExtent.Z / (MeshBound.BoxExtent.Z * 2);
					Center->SetRelativeScale3D(FVector(1, 1, SZ));
					break;
				case EPlacedType::Edge:
					if (ShouldRotateCenter)
					{
						SW = TileSize.Y * TileExtent.Y / (MeshBound.BoxExtent.Y * 2);
						Center->SetRelativeScale3D(FVector(1, SW, 1));
					}
					else
					{
						SW = TileSize.X * TileExtent.X / (MeshBound.BoxExtent.X * 2);
						Center->SetRelativeScale3D(FVector(SW, 1, 1));
					}
					break;
				default: ;
			}
		
		default: ;
	}
	
	FTransform ModTransform = Item->TransformAdjustment;
	TargetMeshComponent->AddRelativeLocation(ModTransform.GetTranslation());
	Center->AddRelativeRotation(ModTransform.GetRotation());
	Center->SetRelativeScale3D(Center->GetComponentScale() * ModTransform.GetScale3D());
}

void FTiledLevelUtility::ApplyPlacementSettings_TiledActor(const FVector& TileSize, UTiledLevelItem* Item,
	 UTiledLevelGrid* TargetBrushGrid, USceneComponent* Center)
{
	FVector BrushExtent;
	switch (Item->PlacedType) {
		case EPlacedType::Block:
			BrushExtent = Item->Extent * TileSize * 0.5;
			TargetBrushGrid->SetBoxExtent(BrushExtent);
			TargetBrushGrid->SetRelativeLocation(BrushExtent);
			break;
		case EPlacedType::Floor:
			BrushExtent = Item->Extent * TileSize * 0.5 * FVector(1, 1, 0.25);
			TargetBrushGrid->SetBoxExtent(BrushExtent);
			TargetBrushGrid->SetRelativeLocation(BrushExtent);
			break;
		case EPlacedType::Wall:
			BrushExtent = FVector(TileSize.X * Item->Extent.X, TileSize.Y * 0.25, TileSize.Z * Item->Extent.Z) * 0.5;
			TargetBrushGrid->SetBoxExtent(BrushExtent);
			TargetBrushGrid->SetRelativeLocation(BrushExtent * FVector(1, 0, 1));
			break;
		case EPlacedType::Pillar:
			BrushExtent = FVector(TileSize.X * 0.25, TileSize.Y * 0.25, TileSize.Z * Item->Extent.Z) * 0.5;
			TargetBrushGrid->SetBoxExtent(BrushExtent);
			TargetBrushGrid->SetRelativeLocation(BrushExtent * FVector(0, 0, 1));
			break;
		case EPlacedType::Edge:
			BrushExtent = FVector(TileSize.X * Item->Extent.X, TileSize.Y * 0.25, TileSize.Z * 0.25) * 0.5;
			TargetBrushGrid->SetBoxExtent(BrushExtent);
			TargetBrushGrid->SetRelativeLocation(BrushExtent * FVector(1, 0, 0));
			break;
	default: ;
	}
	
	Center->ResetRelativeTransform();
	FVector SideMode = Item->PlacedType == EPlacedType::Wall? FVector(1, 0, 1) : FVector(1, 0, 0);
	switch (Item->PivotPosition) {
		case EPivotPosition::Bottom:
			if (Item->PlacedType == EPlacedType::Floor)
				Center->AddLocalOffset(FVector(0, 0, -(TileSize.Z * Item->Extent.Z) * 0.125));
			else
				Center->AddLocalOffset(FVector(0, 0, -(TileSize.Z * Item->Extent.Z) * 0.5));
			break;
		case EPivotPosition::Corner:
			if (Item->PlacedType == EPlacedType::Floor)
				Center->AddLocalOffset(FVector(TileSize * Item->Extent * -FVector(0.5, 0.5, 0.125)));
			else
				Center->AddLocalOffset(FVector(TileSize * Item->Extent * -0.5));
			break;
		case EPivotPosition::Side:
			Center->AddLocalOffset(-TileSize * Item->Extent * 0.5 * SideMode);
			break;
	default: ;
	}
}

float FTiledLevelUtility::TrySnapPropToFloor(const FVector& InitLocation, uint8 RotationIndex, const FVector& TileSize, UTiledLevelItem* Item,
                                             UStaticMeshComponent* TargetMeshComponent)
{
	TargetMeshComponent->SetRelativeLocation(InitLocation);
	if (!Item) return 0;
	if (!Item->bSnapToFloor) return 0;
	if (!TargetMeshComponent->GetStaticMesh()) return 0;

	FBoxSphereBounds MeshBounds = TargetMeshComponent->GetStaticMesh()->GetBounds();
	FHitResult HitResult;

	// Move up a little
	TargetMeshComponent->AddRelativeLocation(FVector(0, 0, TileSize.Z * 0.5));
	FVector Start = TargetMeshComponent->GetComponentLocation() + MeshBounds.Origin - FVector(0, 0, MeshBounds.BoxExtent.Z);
	
	// move a away from wall if it's wall
	if (Item->PlacedType == EPlacedType::Wall)
	{
		switch (RotationIndex)
		{
		case 0:
			Start += FVector(0, TileSize.Y * 0.5, 0);
			break;
		case 1:
			Start += FVector(TileSize.X * -0.5, 0, 0);
			break;
		case 2:
			Start += FVector(0, TileSize.Y * -0.5, 0);
			break;
		case 3:
			Start += FVector(TileSize.X * 0.5, 0, 0);
			break;
		default: ;
		}
	}
	
	FVector End = Start - FVector(0, 0, TileSize.Z * 1.5);
	if (TargetMeshComponent->GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility))
	{
		/*
		 * there is no need for further check if it hits HISM, and this is a feature!
		 */
		TargetMeshComponent->AddRelativeLocation(FVector(0, 0, -HitResult.Distance));
		return HitResult.Distance;
	}
	
	TargetMeshComponent->AddRelativeLocation(FVector(0, 0, -TileSize.Z * 0.5));
	return 0;
}

void FTiledLevelUtility::TrySnapPropToWall(const FVector& InitLocation, uint8 RotationIndex, const FVector& TileSize,
	UTiledLevelItem* Item, UStaticMeshComponent* TargetMeshComponent, float Z_Offset)
{
	if (!Item) return;
	if (Item->StructureType == ETLStructureType::Structure || Item->PlacedType != EPlacedType::Wall || !Item->bSnapToWall) return;
	if (!TargetMeshComponent->GetStaticMesh()) return;
	TargetMeshComponent->SetRelativeLocation(InitLocation);
	// Make sure away from wall
	FBoxSphereBounds MeshBound = TargetMeshComponent->GetStaticMesh()->GetBounds();
	FHitResult HitResult;
	FVector Start, End;
	switch (RotationIndex)
	{
		case 0 : // front: Y+
			TargetMeshComponent->AddWorldOffset(FVector(0, TileSize.Y * 0.5, 0));
			Start = TargetMeshComponent->GetComponentLocation() + MeshBound.Origin - FVector(0, MeshBound.BoxExtent.Y, 0);
			End = Start - FVector(0, TileSize.Y, 0);
			break;
		case 1: // front: X-
			TargetMeshComponent->AddWorldOffset(FVector(-TileSize.X * 0.5, 0, 0));
			Start = TargetMeshComponent->GetComponentLocation() + FRotator(0, 90, 0).RotateVector(MeshBound.Origin) + FVector(MeshBound.BoxExtent.Y, 0,0);
			End = Start + FVector(TileSize.X, 0, 0);
			break;
		case 2: // front: Y-
			TargetMeshComponent->AddWorldOffset(FVector(0, TileSize.Y * -0.5, 0));
			Start = TargetMeshComponent->GetComponentLocation() + FRotator(0, 180, 0).RotateVector(MeshBound.Origin) + FVector(0, MeshBound.BoxExtent.Y, 0);
			End = Start + FVector(0, TileSize.Y, 0);
			break;
		case 3: // front X+
			TargetMeshComponent->AddWorldOffset(FVector(TileSize.X * 0.5, 0, 0));
			Start = TargetMeshComponent->GetComponentLocation() + FRotator(0, 270, 0).RotateVector(MeshBound.Origin) - FVector(MeshBound.BoxExtent.Y, 0,0);
			End = Start - FVector(TileSize.X, 0, 0);
			break;
		default: ;
	}
	if (TargetMeshComponent->GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility))
	{
		switch (RotationIndex)
		{
			case 0:
				TargetMeshComponent->AddWorldOffset(FVector(0, -HitResult.Distance, 0));
				break;
			case 1:
				TargetMeshComponent->AddWorldOffset(FVector(HitResult.Distance, 0, 0));
				break;
			case 2:
				TargetMeshComponent->AddWorldOffset(FVector(0, HitResult.Distance, 0));
				break;
			case 3:
				TargetMeshComponent->AddWorldOffset(FVector(-HitResult.Distance, 0, 0));
				break;
			default: ;
		}
	}
	else
	{
		TargetMeshComponent->SetRelativeLocation(InitLocation);
	}
	if (Z_Offset > 0)
	{
		// Add back the z offset since it could be snapped to ground (need push up!!)
		TargetMeshComponent->AddWorldOffset(FVector(0, 0 , -Z_Offset + TileSize.Z * 0.5));
	}
}

bool FTiledLevelUtility::IsRequested = false;
FTimerHandle FTiledLevelUtility::TimerHandle = FTimerHandle();

void FTiledLevelUtility::RequestToResetInstances(const ATiledLevel* RequestedLevel)
{
	if (!IsRequested)
	{
		IsRequested = true;
		RequestedLevel->GetWorld()->GetTimerManager().SetTimer(TimerHandle, [=]()
		{
			TArray<AActor*> FoundActors;
			UGameplayStatics::GetAllActorsOfClass(RequestedLevel->GetWorld(), ATiledLevel::StaticClass(), FoundActors);
			for (AActor* Actor : FoundActors)
			{
				ATiledLevel* ATL = Cast<ATiledLevel>(Actor);
				if (ATL->GetAsset()->GetItemSetAsset() == RequestedLevel->GetAsset()->GetItemSetAsset())
				{
					 ATL->ResetAllInstance(true);
				}
			}
			IsRequested = false;
		}, 0.5, false );
	}

}

FString FTiledLevelUtility::GetFloorNameFromPosition(int FloorPositionIndex)
{
	if (FloorPositionIndex < 0)
		 return  FString::Printf(TEXT("B%d"), abs(FloorPositionIndex));
	return FString::Printf(TEXT("%dF"), FloorPositionIndex + 1);
}

void FTiledLevelUtility::FloodFill(TArray<TArray<int>>& InBoard, const FIntPoint& InBoardSize, int X, int Y,
	TArray<FIntPoint>& FilledTarget)
{
	// copy from https://www.educative.io/edpresso/flood-fill-algorithm-in-cpp
	// works like a charm!!
	if (X < 0 || X >= InBoardSize.X || Y < 0 || Y >= InBoardSize.Y)
		return;
	// 0 will be empty place, 1 will be occupied region
	if (InBoard[X][Y] == 1)
		return;
	InBoard[X][Y] = 1;
	FilledTarget.Add(FIntPoint(X, Y));
	
	FloodFill(InBoard, InBoardSize, X+1, Y, FilledTarget);
	FloodFill(InBoard, InBoardSize, X-1, Y, FilledTarget);
	FloodFill(InBoard, InBoardSize, X, Y+1, FilledTarget);
	FloodFill(InBoard, InBoardSize, X, Y-1, FilledTarget);
}

void FTiledLevelUtility::FloodFillByEdges(const TArray<FTiledLevelEdge>& BlockingEdges, TArray<TArray<int>>& InBoard, const FIntPoint& InBoardSize, int X,
	int Y, TArray<FIntPoint>& FilledTarget)
{
	if (X < 0 || X >= InBoardSize.X || Y < 0 || Y >= InBoardSize.Y)
		return;
	if (InBoard[X][Y] == 1)
		return;
	InBoard[X][Y] = 1;
	FilledTarget.Add(FIntPoint(X, Y));

	// test right
	if (!BlockingEdges.Contains(FTiledLevelEdge(X + 1, Y, 1, EEdgeType::Vertical)))
		FloodFillByEdges(BlockingEdges, InBoard, InBoardSize, X + 1, Y, FilledTarget);
	// test left
	if (!BlockingEdges.Contains(FTiledLevelEdge(X, Y, 1, EEdgeType::Vertical)))
		FloodFillByEdges(BlockingEdges, InBoard, InBoardSize, X - 1, Y, FilledTarget);
	//test up
	if (!BlockingEdges.Contains(FTiledLevelEdge(X, Y, 1, EEdgeType::Horizontal)))
		FloodFillByEdges(BlockingEdges, InBoard, InBoardSize, X, Y - 1, FilledTarget);
	// test down
	if (!BlockingEdges.Contains(FTiledLevelEdge(X, Y + 1, 1, EEdgeType::Horizontal)))
		FloodFillByEdges(BlockingEdges, InBoard, InBoardSize, X, Y + 1, FilledTarget);
}

void FTiledLevelUtility::GetConsecutiveTiles(TArray<TArray<int>>& InBoard, FIntPoint InBoardSize, int X, int Y,
                                             TArray<FIntPoint>& OutTiles)
{
	if (X < 0 || X >= InBoardSize.X || Y < 0 || Y >= InBoardSize.Y)
		return;
	// 0 will be empty place, 1 will be occupied region
	if (InBoard[X][Y] == 0)
		return;
	InBoard[X][Y] = 0;
	OutTiles.Add(FIntPoint(X, Y));
	
	GetConsecutiveTiles(InBoard, InBoardSize, X+1, Y,OutTiles);
	GetConsecutiveTiles(InBoard, InBoardSize, X-1, Y,OutTiles);
	GetConsecutiveTiles(InBoard, InBoardSize, X, Y+1,OutTiles);
	GetConsecutiveTiles(InBoard, InBoardSize, X, Y-1,OutTiles);
	
}

TArray<float> FTiledLevelUtility::GetWeightedCoefficient(TArray<float>& RawCoefficientArray)
{
	TArray<float> OutArray;
	float RawSum = 0.f;
	for (float& Value : RawCoefficientArray)
		RawSum += Value;
	float v = 0.f;
	for (float& Value : RawCoefficientArray)
	{
		v += Value;
		OutArray.Add(v / RawSum);
	}
	return OutArray;
}

/*
 * should also remove points in candidate points...
 */
bool FTiledLevelUtility::GetFeasibleFillTile(UTiledLevelItem* InItem, int& InRotationIndex,
	TArray<FIntPoint>& InCandidatePoints, FIntPoint& OutPoint)
{
	FIntPoint CheckExtent = (InRotationIndex == 1 || InRotationIndex == 3)?
		                              FIntPoint(int(InItem->Extent.Y), int(InItem->Extent.X)) : FIntPoint(int(InItem->Extent.X), int(InItem->Extent.Y));
	bool Pass = false;
	TArray<FIntPoint> PointsToRemove;
	for (FIntPoint& P : InCandidatePoints)
	{
		PointsToRemove.Empty();
		for (int x = 0; x < CheckExtent.X; x++)
		{
			for (int y = 0; y < CheckExtent.Y; y++)
			{
				Pass = InCandidatePoints.Contains(FIntPoint(P.X + x , P.Y + y));
				if (!Pass) break;
				PointsToRemove.Add(FIntPoint(P.X + x , P.Y + y));
			}
			if (!Pass) break;
		}
		if (!Pass) continue;
		OutPoint = P;
		break;
	}
	// in case the remaining space is just enough for rotated shape
	if (!Pass && InItem->bAllowRandomRotation)
	{
		InRotationIndex = InRotationIndex == 3? 0 : InRotationIndex + 1;
		CheckExtent = (InRotationIndex == 1 || InRotationIndex == 3)?
										  FIntPoint(int(InItem->Extent.Y), int(InItem->Extent.X)) : FIntPoint(int(InItem->Extent.X), int(InItem->Extent.Y));
		Pass = false;
		for (FIntPoint& P : InCandidatePoints)
		{
			PointsToRemove.Empty();
			for (int x = 0; x < CheckExtent.X; x++)
			{
				for (int y = 0; y < CheckExtent.Y; y++)
				{
					Pass = InCandidatePoints.Contains(FIntPoint(P.X + x , P.Y + y));
					if (!Pass) break;
					PointsToRemove.Add(FIntPoint(P.X + x , P.Y + y));
				}
				if (!Pass) break;
			}
			if (!Pass) continue;
			OutPoint = P;
			break;
		}
	}
	if (Pass)
	{
		for (FIntPoint P : PointsToRemove)
			InCandidatePoints.Remove(P);
	}
	return Pass;
}

TArray<FTiledLevelEdge> FTiledLevelUtility::GetAreaEdges(const TSet<FIntPoint>& Region, int Z, bool IsOuter)
{
	TArray<FTiledLevelEdge> OutEdges;
	for (FIntPoint Tile : Region)
	{
		if (IsOuter)
		{
			if (!Region.Contains(FIntPoint(Tile.X + 1, Tile.Y)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Tile.X + 1, Tile.Y, Z, EEdgeType::Vertical));
			}
			if (!Region.Contains(FIntPoint(Tile.X - 1, Tile.Y)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Tile.X, Tile.Y, Z, EEdgeType::Vertical));
			}
			if (!Region.Contains(FIntPoint(Tile.X, Tile.Y + 1)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Tile.X, Tile.Y + 1, Z, EEdgeType::Horizontal));
			}
			if (!Region.Contains(FIntPoint(Tile.X, Tile.Y - 1)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Tile.X, Tile.Y, Z, EEdgeType::Horizontal));
			}
		}
		else
		{
			if (Region.Contains(FIntPoint(Tile.X + 1, Tile.Y)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Tile.X + 1, Tile.Y, Z, EEdgeType::Vertical));
			}
			if (Region.Contains(FIntPoint(Tile.X - 1, Tile.Y)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Tile.X, Tile.Y, Z, EEdgeType::Vertical));
			}
			if (Region.Contains(FIntPoint(Tile.X, Tile.Y + 1)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Tile.X, Tile.Y + 1, Z, EEdgeType::Horizontal));
			}
			if (Region.Contains(FIntPoint(Tile.X, Tile.Y - 1)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Tile.X, Tile.Y, Z, EEdgeType::Horizontal));
			}
		}
	}
	return OutEdges;
}

TArray<FTiledLevelEdge> FTiledLevelUtility::GetAreaEdges(const TSet<FIntVector>& Region, bool IsOuter)
{
	TArray<FTiledLevelEdge> OutEdges;
	for (FIntVector Grid : Region)
	{
		if (IsOuter)
		{
			if (!Region.Contains(FIntVector(Grid.X + 1, Grid.Y, Grid.Z)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Grid.X + 1, Grid.Y, Grid.Z, EEdgeType::Vertical));
			}
			if (!Region.Contains(FIntVector(Grid.X - 1, Grid.Y, Grid.Z)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Grid.X, Grid.Y, Grid.Z, EEdgeType::Vertical));
			}
			if (!Region.Contains(FIntVector(Grid.X, Grid.Y + 1, Grid.Z)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Grid.X, Grid.Y + 1, Grid.Z, EEdgeType::Horizontal));
			}
			if (!Region.Contains(FIntVector(Grid.X, Grid.Y - 1, Grid.Z)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Grid.X, Grid.Y, Grid.Z, EEdgeType::Horizontal));
			}
		}
		else
		{
			if (Region.Contains(FIntVector(Grid.X + 1, Grid.Y, Grid.Z)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Grid.X + 1, Grid.Y, Grid.Z, EEdgeType::Vertical));
			}
			if (Region.Contains(FIntVector(Grid.X - 1, Grid.Y, Grid.Z)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Grid.X, Grid.Y, Grid.Z, EEdgeType::Vertical));
			}
			if (Region.Contains(FIntVector(Grid.X, Grid.Y + 1, Grid.Z)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Grid.X, Grid.Y + 1, Grid.Z, EEdgeType::Horizontal));
			}
			if (Region.Contains(FIntVector(Grid.X, Grid.Y - 1, Grid.Z)))
			{
				OutEdges.AddUnique(FTiledLevelEdge(Grid.X, Grid.Y, Grid.Z, EEdgeType::Horizontal));
			}
			
		}
	}
	return OutEdges;
}

TSet<FIntVector> FTiledLevelUtility::GetAreaPoints(const TSet<FIntVector>& Region, bool IsOuter)
{
	TSet<FIntVector> AllPoints;
	TSet<FIntVector> InnerPoints;
	
	for (FIntVector Grid : Region)
	{
		AllPoints.Add(Grid);	
		AllPoints.Add(FIntVector(Grid.X + 1, Grid.Y, Grid.Z));	
		AllPoints.Add(FIntVector(Grid.X, Grid.Y + 1, Grid.Z));	
		AllPoints.Add(FIntVector(Grid.X + 1, Grid.Y + 1, Grid.Z));	
		
		TSet<FIntVector> UpperLeftTestSet = {
			FIntVector(Grid.X - 1, Grid.Y , Grid.Z),
			FIntVector(Grid.X - 1, Grid.Y - 1, Grid.Z ),
			FIntVector(Grid.X, Grid.Y - 1, Grid.Z )
		};
		TSet<FIntVector> UpperRightTestSet = {
			FIntVector(Grid.X, Grid.Y - 1, Grid.Z),
			FIntVector(Grid.X + 1, Grid.Y - 1, Grid.Z ),
			FIntVector(Grid.X + 1, Grid.Y, Grid.Z )
		};
		TSet<FIntVector> LowerLeftTestSet = {
			FIntVector(Grid.X - 1, Grid.Y , Grid.Z),
			FIntVector(Grid.X - 1, Grid.Y + 1, Grid.Z ),
			FIntVector(Grid.X, Grid.Y + 1, Grid.Z )
		};
		TSet<FIntVector> LowerRightTestSet = {
			FIntVector(Grid.X, Grid.Y + 1, Grid.Z),
			FIntVector(Grid.X + 1, Grid.Y + 1, Grid.Z ),
			FIntVector(Grid.X + 1, Grid.Y, Grid.Z )
		};

		if (Region.Includes(UpperLeftTestSet))
		{
			InnerPoints.Add(Grid);	
		}
		if (Region.Includes(UpperRightTestSet))
		{
			InnerPoints.Add(FIntVector(Grid.X + 1, Grid.Y, Grid.Z));	
		}
		if (Region.Includes(LowerLeftTestSet))
		{
			InnerPoints.Add(FIntVector(Grid.X, Grid.Y + 1, Grid.Z));	
		}
		if (Region.Includes(LowerRightTestSet))
		{
			InnerPoints.Add(FIntVector(Grid.X + 1, Grid.Y + 1, Grid.Z));	
		}
	}
	if (IsOuter)
	{
		return AllPoints.Difference(InnerPoints);
	}
	return InnerPoints;
}

bool FTiledLevelUtility::GetFeasibleFillEdge(UTiledLevelItem* InItem, TArray<FTiledLevelEdge>& InCandidateEdges,
	FTiledLevelEdge& OutEdge)
{
	bool Pass = false;
	TArray<FTiledLevelEdge> EdgesToRemove;
	int Extent = InItem->Extent.X;
	
	// if edge span is 1 just return first one
	if (Extent == 1)
	{
		EdgesToRemove.Add(InCandidateEdges[0]);
		OutEdge = InCandidateEdges[0];
		Pass = true;
	}
	// else check whether have room to fill that edge in either vertical or horizontal
	else
	{
		for (auto Edge : InCandidateEdges)
		{
			EdgesToRemove.Empty();
			for (int e = 0; e < Extent; e++)
			{
				FTiledLevelEdge TestEdge = Edge.EdgeType == EEdgeType::Horizontal?
					FTiledLevelEdge(Edge.X + e, Edge.Y, Edge.Z, EEdgeType::Horizontal) :
					FTiledLevelEdge(Edge.X, Edge.Y + e, Edge.Z, EEdgeType::Vertical);
				Pass = InCandidateEdges.Contains(TestEdge);
				if (!Pass) break;
				EdgesToRemove.Add(TestEdge);
			}
			if (!Pass) continue;
			OutEdge = Edge;
			break;
		}
	}
	if (Pass)
	{
		for (auto Edge : EdgesToRemove)
		{
			InCandidateEdges.Remove(Edge);
		}
	}
	return Pass;
}

EPlacedShapeType FTiledLevelUtility::PlacedTypeToShape(const EPlacedType& PlacedType)
{
	if (PlacedType == EPlacedType::Block || PlacedType == EPlacedType::Floor)
	{
		return EPlacedShapeType::Shape3D;
	}
	if (PlacedType == EPlacedType::Edge || PlacedType == EPlacedType::Wall)
	{
		return EPlacedShapeType::Shape2D;
	}
	return EPlacedShapeType::Shape1D;
}

// TODO: add allow 90 degrees rotations
void FTiledLevelUtility::ApplyGametimeRequiredTransform(ATiledLevel* TargetTiledLevel, FVector TargetTileSize)
{
	if (!TargetTiledLevel) return;
	const FVector Origin = TargetTiledLevel->GetActorLocation();
	TargetTiledLevel->SetActorRotation(FRotator(0));
	TargetTiledLevel->SetActorScale3D(FVector(1));
	// DO NOT snap if it is already snapped...
	if ((static_cast<int>(Origin.X) % static_cast<int>(TargetTileSize.X) == 0) &&
		(static_cast<int>(Origin.Y) % static_cast<int>(TargetTileSize.Y) == 0) &&
		(static_cast<int>(Origin.Z) % static_cast<int>(TargetTileSize.Z) == 0))
	{
		DEV_LOG("skip snap")
		return;
	}
	FVector SnappedLocation = FVector(FIntVector(Origin / TargetTileSize)) * TargetTileSize;
	if (Origin.X < 0) SnappedLocation.X -= TargetTileSize.X;
	if (Origin.Y < 0) SnappedLocation.Y -= TargetTileSize.Y;
	if (Origin.Z < 0) SnappedLocation.Z -= TargetTileSize.Z;
	TargetTiledLevel->SetActorLocation(SnappedLocation);
}

bool FTiledLevelUtility::CheckOverlappedTiledLevels(ATiledLevel* FirstLevel, ATiledLevel* SecondLevel)
{
	if (!FirstLevel || ! SecondLevel) return false;
	FBox B1 = FirstLevel->GetBoundaryBox();
	B1.Min += FVector(5.0f);
	B1.Max -= FVector(5.0f);
	FBox B2 = SecondLevel->GetBoundaryBox();
	B2.Min += FVector(5.0f);
	B2.Max -= FVector(5.0f);
	return B1.Intersect(B2);
}

FMeshDescription FTiledLevelUtility::BuildMeshDescription(UProceduralMeshComponent* ProcMeshComp)
{
	FMeshDescription MeshDescription;
	FStaticMeshAttributes AttributeGetter(MeshDescription);
	AttributeGetter.Register();
	
	TPolygonGroupAttributesRef<FName> PolygonGroupNames = AttributeGetter.GetPolygonGroupMaterialSlotNames();
	TVertexAttributesRef<FVector3f> VertexPositions = AttributeGetter.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector3f> Tangents = AttributeGetter.GetVertexInstanceTangents();
	TVertexInstanceAttributesRef<float> BinormalSigns = AttributeGetter.GetVertexInstanceBinormalSigns();
	TVertexInstanceAttributesRef<FVector3f> Normals = AttributeGetter.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector4f> Colors = AttributeGetter.GetVertexInstanceColors();
	TVertexInstanceAttributesRef<FVector2f> UVs = AttributeGetter.GetVertexInstanceUVs();
	
	// Materials to apply to new mesh
	const int32 NumSections = ProcMeshComp->GetNumSections();
	int32 VertexCount = 0;
	int32 VertexInstanceCount = 0;
	int32 PolygonCount = 0;
	
	TMap<UMaterialInterface*, FPolygonGroupID> UniqueMaterials;
	UniqueMaterials.Reserve(NumSections);
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FProcMeshSection *ProcSection =
			ProcMeshComp->GetProcMeshSection(SectionIdx);
		UMaterialInterface *Material = ProcMeshComp->GetMaterial(SectionIdx);
		if (Material == nullptr)
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}
	
		if (!UniqueMaterials.Contains(Material))
		{
			FPolygonGroupID NewPolygonGroup = MeshDescription.CreatePolygonGroup();
			UniqueMaterials.Add(Material, NewPolygonGroup);
			PolygonGroupNames[NewPolygonGroup] = Material->GetFName();
		}
	}
	TArray<FPolygonGroupID> PolygonGroupForSection;
	PolygonGroupForSection.Reserve(NumSections);
	
	// Calculate the totals for each ProcMesh element type
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FProcMeshSection *ProcSection =
			ProcMeshComp->GetProcMeshSection(SectionIdx);
		VertexCount += ProcSection->ProcVertexBuffer.Num();
		VertexInstanceCount += ProcSection->ProcIndexBuffer.Num();
		PolygonCount += ProcSection->ProcIndexBuffer.Num() / 3;
	}
	MeshDescription.ReserveNewVertices(VertexCount);
	MeshDescription.ReserveNewVertexInstances(VertexInstanceCount);
	MeshDescription.ReserveNewPolygons(PolygonCount);
	MeshDescription.ReserveNewEdges(PolygonCount * 2);
	UVs.SetNumChannels(4);
	
	// Create the Polygon Groups
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FProcMeshSection *ProcSection =
			ProcMeshComp->GetProcMeshSection(SectionIdx);
		UMaterialInterface *Material = ProcMeshComp->GetMaterial(SectionIdx);
		if (Material == nullptr)
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}
	
		FPolygonGroupID *PolygonGroupID = UniqueMaterials.Find(Material);
		check(PolygonGroupID != nullptr);
		PolygonGroupForSection.Add(*PolygonGroupID);
	}
	
	
	// Add Vertex and VertexInstance and polygon for each section
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FProcMeshSection *ProcSection =
			ProcMeshComp->GetProcMeshSection(SectionIdx);
		FPolygonGroupID PolygonGroupID = PolygonGroupForSection[SectionIdx];
		// Create the vertex
		int32 NumVertex = ProcSection->ProcVertexBuffer.Num();
		TMap<int32, FVertexID> VertexIndexToVertexID;
		VertexIndexToVertexID.Reserve(NumVertex);
		for (int32 VertexIndex = 0; VertexIndex < NumVertex; ++VertexIndex)
		{
			FProcMeshVertex &Vert = ProcSection->ProcVertexBuffer[VertexIndex];
			const FVertexID VertexID = MeshDescription.CreateVertex();
			VertexPositions[VertexID] = FVector3f(Vert.Position);
			VertexIndexToVertexID.Add(VertexIndex, VertexID);
		}
		// Create the VertexInstance
		int32 NumIndices = ProcSection->ProcIndexBuffer.Num();
		int32 NumTri = NumIndices / 3;
		TMap<int32, FVertexInstanceID> IndiceIndexToVertexInstanceID;
		IndiceIndexToVertexInstanceID.Reserve(NumVertex);
		for (int32 IndiceIndex = 0; IndiceIndex < NumIndices; IndiceIndex++)
		{
			const int32 VertexIndex = ProcSection->ProcIndexBuffer[IndiceIndex];
			const FVertexID VertexID = VertexIndexToVertexID[VertexIndex];
			const FVertexInstanceID VertexInstanceID =
				MeshDescription.CreateVertexInstance(VertexID);
			IndiceIndexToVertexInstanceID.Add(IndiceIndex, VertexInstanceID);
	
			FProcMeshVertex &ProcVertex = ProcSection->ProcVertexBuffer[VertexIndex];
	
			Tangents[VertexInstanceID] = FVector3f(ProcVertex.Tangent.TangentX);
			Normals[VertexInstanceID] = FVector3f(ProcVertex.Normal);
			BinormalSigns[VertexInstanceID] =
				ProcVertex.Tangent.bFlipTangentY ? -1.f : 1.f;

			Colors[VertexInstanceID] = FLinearColor(ProcVertex.Color);
	
			UVs.Set(VertexInstanceID, 0, FVector2f(ProcVertex.UV0));
			UVs.Set(VertexInstanceID, 1, FVector2f(ProcVertex.UV1));
			UVs.Set(VertexInstanceID, 2, FVector2f(ProcVertex.UV2));
			UVs.Set(VertexInstanceID, 3, FVector2f(ProcVertex.UV3));
		}
	
		// Create the polygons for this section
		for (int32 TriIdx = 0; TriIdx < NumTri; TriIdx++)
		{
			FVertexID VertexIndexes[3];
			TArray<FVertexInstanceID> VertexInstanceIDs;
			VertexInstanceIDs.SetNum(3);
	
			for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
			{
				const int32 IndiceIndex = (TriIdx * 3) + CornerIndex;
				const int32 VertexIndex = ProcSection->ProcIndexBuffer[IndiceIndex];
				VertexIndexes[CornerIndex] = VertexIndexToVertexID[VertexIndex];
				VertexInstanceIDs[CornerIndex] =
					IndiceIndexToVertexInstanceID[IndiceIndex];
			}
	
			// Insert a polygon into the mesh
			MeshDescription.CreatePolygon(PolygonGroupID, VertexInstanceIDs);
		}
	}
	return MeshDescription;
}

UProceduralMeshComponent* FTiledLevelUtility::ConvertTiledLevelAssetToProcMesh(UTiledLevelAsset* TargetAsset,
	int TargetLOD, UObject* Outer, EObjectFlags Flags)
{
	struct FCollisionVertex
	{
		TArray<FVector> CollisionVertex;
	};
	
	struct FPerSectionData
	{
		UStaticMesh* MeshPtr;
		int LOD;
		int SectionID;
		UMaterialInterface* SectionMaterial;
		TArray<FVector> Vertex;
		TArray<int> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UV;
		TArray<FProcMeshTangent> Tangents;
		TArray<FColor> VertexColor;
		TArray<FCollisionVertex> CollisionData;
	};

	if (!Outer)
		Outer = TargetAsset;
	UProceduralMeshComponent* ProcMeshComp = NewObject<UProceduralMeshComponent>(Outer, NAME_None, Flags);
	TMap<UStaticMesh*, int> MeshLODMap;
	for (UStaticMesh* SMPtr : TargetAsset->GetUsedStaticMeshSet())
	{
		SMPtr->bAllowCPUAccess = true;
		SMPtr->GetNumLODs() - 1 < TargetLOD ?
			MeshLODMap.Add(SMPtr, SMPtr->GetNumLODs()-1) : MeshLODMap.Add(SMPtr, TargetLOD);
	}
	
	// construct template data ... and
	// init proc data
	TArray<FPerSectionData> SectionTemplateData;
	TArray<FPerSectionData> ProcData;
	for (auto MeshLOD : MeshLODMap)
	{
		for (int SectionID = 0; SectionID < MeshLOD.Key->GetNumSections(MeshLOD.Value); SectionID++)
		{
			FPerSectionData NewSectionData;
			NewSectionData.MeshPtr = MeshLOD.Key;
			NewSectionData.LOD = MeshLOD.Value;
			NewSectionData.SectionID = SectionID;
			NewSectionData.SectionMaterial = MeshLOD.Key->GetMaterial(SectionID);
			FPerSectionData InitSectionData;
			InitSectionData.MeshPtr = MeshLOD.Key;
			InitSectionData.LOD = MeshLOD.Value;
			InitSectionData.SectionID = SectionID;
			InitSectionData.SectionMaterial = MeshLOD.Key->GetMaterial(SectionID);
			UKismetProceduralMeshLibrary::GetSectionFromStaticMesh(MeshLOD.Key, MeshLOD.Value, SectionID, NewSectionData.Vertex,
				NewSectionData.Triangles, NewSectionData.Normals, NewSectionData.UV, NewSectionData.Tangents);
			if (MeshLOD.Key->GetBodySetup() != nullptr)
			{
				const int32 NumConvex = MeshLOD.Key->GetBodySetup()->AggGeom.ConvexElems.Num();
				for (int ConvexIndex = 0; ConvexIndex < NumConvex; ConvexIndex++)
				{
					FCollisionVertex CV;
					CV.CollisionVertex = MeshLOD.Key->GetBodySetup()->AggGeom.ConvexElems[ConvexIndex].VertexData;
					NewSectionData.CollisionData.Add(CV);
				}
			}
			
			if (MeshLOD.Value == 0)
			{
				// Note: Num of vertex color will not be the same as num of vertex...
				// The num of vertex is actually 3 times more than original for smoothing purpose EX: 40 VS 12
				TMap<FVector, FColor> VertexColorMap;
				MeshLOD.Key->GetVertexColorData(VertexColorMap);
				for (FVector& v : NewSectionData.Vertex)
				{
					if (VertexColorMap.Contains(v))
						 NewSectionData.VertexColor.Add(VertexColorMap[v]);
					else
						NewSectionData.VertexColor.Add(FColor::White);
				}
			} else
			{
				TArray<FColor> Whites;
				Whites.Init(FColor::White, NewSectionData.Vertex.Num());
				NewSectionData.VertexColor.Append(Whites);
			}
			SectionTemplateData.Add(NewSectionData);	
			ProcData.Add(InitSectionData);
		}
	}

	// fill proc data
	TArray<UStaticMesh*> TargetMeshes;
	TArray<FTransform> TransformMods;
	for (FTiledFloor F : TargetAsset->TiledFloors)
	{
		for (FItemPlacement P : F.GetItemPlacements())
		{
			UStaticMesh* ItemMesh = P.GetItem()->TiledMesh;
			if (ItemMesh)
			{
				TargetMeshes.Add(ItemMesh);
				TransformMods.Add(P.TileObjectTransform);
			}
		}
	}
	FTransform CachedHostLevelTransform = TargetAsset->HostLevel->GetTransform();
	TargetAsset->HostLevel->SetActorTransform(FTransform());
	for (AActor* SpawnedActor : TargetAsset->HostLevel->SpawnedTiledActors)
	{
		for (UActorComponent* AC : SpawnedActor->GetComponents())
		{
			if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(AC))
			{
				if (SMC->GetStaticMesh())
				{
					TargetMeshes.Add(SMC->GetStaticMesh());
					TransformMods.Add(SMC->GetComponentTransform());
				}
			}
		}
	}
	TargetAsset->HostLevel->SetActorTransform(CachedHostLevelTransform);
	
	for (int i = 0; i < TargetMeshes.Num(); i++)
	{
		for (int LOD = 0; LOD < TargetMeshes[i]->GetNumLODs(); LOD++)
		{
			for (int SectionID = 0 ; SectionID < TargetMeshes[i]->GetNumSections(LOD); SectionID++)
			{
				FPerSectionData* DataToFill = ProcData.FindByPredicate([=](const FPerSectionData& Data)
				{
					return Data.SectionID == SectionID && Data.MeshPtr==TargetMeshes[i] && Data.LOD == LOD;
				});
				FPerSectionData* TemplateToCopy = SectionTemplateData.FindByPredicate([=](const FPerSectionData& Data)
				{
					return Data.SectionID == SectionID && Data.MeshPtr==TargetMeshes[i] && Data.LOD == LOD;
				});

				if (!DataToFill) continue;
				// vertex
				int NumOfExistingVertex = DataToFill->Vertex.Num();
				for (FVector v : TemplateToCopy->Vertex)
				{
					DataToFill->Vertex.Add(TransformMods[i].TransformPosition(v));
				}
				
				// triangle
				for (int t : TemplateToCopy->Triangles)
				{
					DataToFill->Triangles.Add(t + NumOfExistingVertex);
				}
				
				// normal
				for (FVector n : TemplateToCopy->Normals)
					DataToFill->Normals.Add(TransformMods[i].GetRotation().RotateVector(n));
				 
				// uv
				DataToFill->UV.Append(TemplateToCopy->UV);
				
				// tangent
				DataToFill->Tangents.Append(TemplateToCopy->Tangents);
				
				// vertex color
				DataToFill->VertexColor.Append(TemplateToCopy->VertexColor);

				// Collision data
				int NumOfCollisions = TemplateToCopy->CollisionData.Num();
				for (int CollisionIndex = 0; CollisionIndex < NumOfCollisions; CollisionIndex++ )
				{
					FCollisionVertex CV;
					for (FVector v : TemplateToCopy->CollisionData[CollisionIndex].CollisionVertex)
					{
						CV.CollisionVertex.Add(TransformMods[i].TransformPosition(v));
					}
					 DataToFill->CollisionData.Add(CV);
				}
			}
		}
	}

	int s = 0;
	for (FPerSectionData  Data: ProcData)
	{
		ProcMeshComp->CreateMeshSection(s, Data.Vertex, Data.Triangles, Data.Normals, Data.UV, Data.VertexColor, Data.Tangents, false);
		ProcMeshComp->SetMaterial(s, Data.SectionMaterial);
		// Collision
		for (FCollisionVertex CV: Data.CollisionData)
			ProcMeshComp->AddCollisionConvexMesh(CV.CollisionVertex);
		s++;
	}
	return ProcMeshComp;
}

