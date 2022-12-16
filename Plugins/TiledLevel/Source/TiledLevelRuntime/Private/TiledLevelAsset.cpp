// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelAsset.h"
#include "TiledLevel.h"
#include "TiledLevelEditorLog.h"
#include "TiledLevelItem.h"
#include "TiledLevelUtility.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "TiledLevel"

UTiledLevelAsset::UTiledLevelAsset()
{
}

void UTiledLevelAsset::ConfirmTileSize()
{
	CanEditTileSize = false;
}

void UTiledLevelAsset::ResetTileSize()
{
	bool Proceed = true;
	const int TotalPlacements = GetNumOfAllPlacements();
	if (TotalPlacements > 0)
	{
		FText Message = FText::Format(LOCTEXT("ResetAssetTileSize", "Reset Tile Size will clear all placements! ({0}) Are you sure?"), FText::AsNumber(TotalPlacements));
		Proceed = FMessageDialog::Open(EAppMsgType::OkCancel, Message) == EAppReturnType::Ok;
	}
	if (Proceed)
	{
		this->Modify();
		CanEditTileSize = true;
		EmptyAllFloors();
		if (HostLevel)
			HostLevel->ResetAllInstance();
		OnResetTileSize.ExecuteIfBound();
	}
}

UTiledLevelAsset* UTiledLevelAsset::CloneAsset(UObject* OuterForClone)
{
	// fixed! with add additional flags  Ensure condition failed: !(bDuplicateForPIE && DupObjectInfo.DuplicatedObject->HasAnyFlags(RF_Standalone))
	// why this issue not happened in paper2D which have to flags set? 
	return CastChecked<UTiledLevelAsset>(StaticDuplicateObject(this, OuterForClone, NAME_None, EObjectFlags::RF_Transactional));
}

#if WITH_EDITOR

void UTiledLevelAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	const TArray<FName>AreaRelatedPropertyNames =
	{
		GET_MEMBER_NAME_CHECKED(UTiledLevelAsset, X_Num),
		GET_MEMBER_NAME_CHECKED(UTiledLevelAsset, Y_Num),
		GET_MEMBER_NAME_CHECKED(UTiledLevelAsset, TileSizeX),
		GET_MEMBER_NAME_CHECKED(UTiledLevelAsset, TileSizeY),
		GET_MEMBER_NAME_CHECKED(UTiledLevelAsset, TileSizeZ),
	};
	if (AreaRelatedPropertyNames.Contains(PropertyName))
	{
		OnTiledLevelAreaChanged.ExecuteIfBound(TileSizeX, TileSizeY, TileSizeZ, X_Num, Y_Num, TiledFloors.Num(), FMath::Max(TiledFloors).FloorPosition);
	}
	if (FTiledFloor* ActiveFloor = GetActiveFloor())
	{
		MoveHelperFloorGrids.ExecuteIfBound(ActiveFloor->FloorPosition);
	}
	VersionNumber += 1;
	UObject::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

int UTiledLevelAsset::AddNewFloor(int32 InsertPosition)
{
	int ActualInsertPosition = InsertPosition;
	// the operator< overload is in reversed way... so use "min to get max", "max to get min"
	int32 MaxFloorPosition = FMath::Min(TiledFloors).FloorPosition;
	int32 MinFloorPosition = FMath::Max(TiledFloors).FloorPosition;
	if (InsertPosition > MaxFloorPosition)
	{
		ActualInsertPosition = MaxFloorPosition + 1;
	}
	else if (InsertPosition < MinFloorPosition)
	{
		ActualInsertPosition = MinFloorPosition - 1;
	}
	if (ActualInsertPosition >= 0)
	{
		for (FTiledFloor& F: TiledFloors)
		{
			if (F.FloorPosition >= ActualInsertPosition)
				F.UpdatePosition(F.FloorPosition + 1, TileSizeZ);
		}
	} else
	{
		for (FTiledFloor& F: TiledFloors)
		{
			if (F.FloorPosition <= ActualInsertPosition)
				F.UpdatePosition(F.FloorPosition - 1, TileSizeZ);
		}
	}
	TiledFloors.Add(FTiledFloor(ActualInsertPosition));
	OnTiledLevelAreaChanged.ExecuteIfBound(TileSizeX, TileSizeY, TileSizeZ, X_Num, Y_Num, TiledFloors.Num(), FMath::Max(TiledFloors).FloorPosition);
	return ActualInsertPosition;
}

void UTiledLevelAsset::DuplicatedFloor(int SourcePosition)
{
	FTiledFloor* SourceFloor = GetFloorFromPosition(SourcePosition);
	TArray<FTilePlacement> BP = SourceFloor->BlockPlacements;
	TArray<FTilePlacement> FP = SourceFloor->FloorPlacements;
	TArray<FPointPlacement> PiP = SourceFloor->PillarPlacements;
	TArray<FEdgePlacement> WP = SourceFloor->WallPlacements;
	TArray<FEdgePlacement> BmP = SourceFloor->EdgePlacements;
	TArray<FPointPlacement> PP = SourceFloor->PointPlacements;
	for (auto& P : BP)
	{
		P.OnMoveFloor(TileSizeZ, true, 1);
	}
	for (auto& P : FP) // forget to add "&" stuck me one hour...
	{
		P.OnMoveFloor(TileSizeZ, true, 1);
	}
	for (auto& P : PiP)
	{
		P.OnMoveFloor(TileSizeZ, true, 1);
	}
	for (auto& P : WP)
	{
		P.OnMoveFloor(TileSizeZ, true, 1);
	}
	for (auto& P : BmP)
	{
		P.OnMoveFloor(TileSizeZ, true, 1);
	}
	for (auto& P : PP)
	{
		P.OnMoveFloor(TileSizeZ, true, 1);
	}
	int NewFloorPosition = AddNewFloor(SourcePosition + 1);
	FTiledFloor* TargetFloor = GetFloorFromPosition(NewFloorPosition);
	TargetFloor->BlockPlacements = BP;
	TargetFloor->FloorPlacements = FP;
	TargetFloor->PillarPlacements = PiP;
	TargetFloor->WallPlacements = WP;
	TargetFloor->EdgePlacements = BmP;
	TargetFloor->PointPlacements = PP;
}

void UTiledLevelAsset::MoveAllFloors(bool Up)
{
	for (auto& F : TiledFloors)
	{
		if (Up)
			F.UpdatePosition(F.FloorPosition + 1, TileSizeZ);
		else
			F.UpdatePosition(F.FloorPosition - 1, TileSizeZ);
	}
	OnTiledLevelAreaChanged.ExecuteIfBound(TileSizeX, TileSizeY, TileSizeZ, X_Num, Y_Num, TiledFloors.Num(), FMath::Max(TiledFloors).FloorPosition);
}

void UTiledLevelAsset::RemoveFloor(int32 DeleteIndex)
{
	TiledFloors.Remove(FTiledFloor(DeleteIndex));
	// reorder
	if (DeleteIndex >= 0)
	{
		for (auto& F : TiledFloors)
		{
			if (F.FloorPosition > DeleteIndex)
				F.UpdatePosition(F.FloorPosition - 1, TileSizeZ);
		}
	} else
	{
		for (auto& F : TiledFloors)
			if (F.FloorPosition < DeleteIndex)
				F.UpdatePosition(F.FloorPosition + 1, TileSizeZ);
	}
	OnTiledLevelAreaChanged.ExecuteIfBound(TileSizeX, TileSizeY, TileSizeZ, X_Num, Y_Num, TiledFloors.Num(), FMath::Max(TiledFloors).FloorPosition);
}

void UTiledLevelAsset::EmptyFloor(int FloorPosition)
{
	if ( FTiledFloor* F = TiledFloors.FindByKey(FloorPosition))
	{
		F->BlockPlacements.Empty();
		F->FloorPlacements.Empty();
		F->WallPlacements.Empty();
		F->EdgePlacements.Empty();
		F->PillarPlacements.Empty();
	}
	VersionNumber += 100;
}

void UTiledLevelAsset::EmptyAllFloors()
{
	for (FTiledFloor& F : TiledFloors)
	{
		F.BlockPlacements.Empty();
		F.FloorPlacements.Empty();
		F.WallPlacements.Empty();
		F.EdgePlacements.Empty();
		F.PillarPlacements.Empty();
	}
	VersionNumber += 100;
}

bool UTiledLevelAsset::IsFloorExists(int32 PositionIndex)
{
	TArray<int> ExistsFloorPosition;
	for (const FTiledFloor& F : TiledFloors)
	{
		ExistsFloorPosition.Add(F.FloorPosition);
	}
	return ExistsFloorPosition.Contains(PositionIndex);
}

void UTiledLevelAsset::RemovePlacements(const TArray<FTilePlacement>& TilesToDelete)
{
	for (FTiledFloor& F : TiledFloors)
	{
		F.BlockPlacements.RemoveAll([=](const FTilePlacement& P)
		{
			return TilesToDelete.Contains(P);
		});
		F.FloorPlacements.RemoveAll([=](const FTilePlacement& P)
		{
			return TilesToDelete.Contains(P);
		});
	}
}

void UTiledLevelAsset::RemovePlacements(const TArray<FEdgePlacement>& WallsToDelete)
{
	for (FTiledFloor& F : TiledFloors)
	{
		F.WallPlacements.RemoveAll([=](const FEdgePlacement& P)
		{
			return WallsToDelete.Contains(P);
		});
		F.EdgePlacements.RemoveAll([=](const FEdgePlacement& P)
		{
			return WallsToDelete.Contains(P);
		});
	}
}

void UTiledLevelAsset::RemovePlacements(const TArray<FPointPlacement>& PointsToDelete)
{
	for (FTiledFloor& F : TiledFloors)
	{
		F.PillarPlacements.RemoveAll([=](const FPointPlacement& P)
		{
			return PointsToDelete.Contains(P);
		});
		F.PointPlacements.RemoveAll([=](const FPointPlacement& P)
		{
			return PointsToDelete.Contains(P);
		});
	}
}

void UTiledLevelAsset::ClearItem(const FGuid& ItemID)
{
	for (FTiledFloor& F : TiledFloors)
	{
		F.BlockPlacements.RemoveAll([=](const FTilePlacement& P)
		{
			return P.ItemID == ItemID;
		});
		F.FloorPlacements.RemoveAll([=](const FTilePlacement& P)
		{
			return P.ItemID == ItemID;
		});

		F.WallPlacements.RemoveAll([=](const FEdgePlacement& P)
		{
			return P.ItemID == ItemID;
		});
		
		F.EdgePlacements.RemoveAll([=](const FEdgePlacement& P)
		{
			return P.ItemID == ItemID;
		});
		
		F.PillarPlacements.RemoveAll([=](const FPointPlacement& P)
		{
			return P.ItemID == ItemID;
		});

		F.PointPlacements.RemoveAll([=](const FPointPlacement& P)
		{
			return P.ItemID == ItemID;
		});
	}
	VersionNumber += 1;
}

void UTiledLevelAsset::ClearItemInActiveFloor(const FGuid& ItemID)
{
	GetActiveFloor()->BlockPlacements.RemoveAll([=](const FTilePlacement& P)
	{
		return P.ItemID == ItemID;
	});
	GetActiveFloor()->FloorPlacements.RemoveAll([=](const FTilePlacement& P)
	{
		return P.ItemID == ItemID;
	});
	GetActiveFloor()->WallPlacements.RemoveAll([=](const FEdgePlacement& P)
	{
		return P.ItemID == ItemID;
	});
	GetActiveFloor()->EdgePlacements.RemoveAll([=](const FEdgePlacement& P)
	{
		return P.ItemID == ItemID;
	});
	GetActiveFloor()->PillarPlacements.RemoveAll([=](const FPointPlacement& P)
	{
		return P.ItemID == ItemID;
	});
	GetActiveFloor()->PointPlacements.RemoveAll([=](const FPointPlacement& P)
	{
		return P.ItemID == ItemID;
	});
	VersionNumber += 1;
}

void UTiledLevelAsset::ClearInvalidPlacements()
{
	TArray<FGuid> InvalidIndices;
	for (FTiledFloor& F : TiledFloors)
	{
		for (FItemPlacement& P : F.GetItemPlacements())
		{
			if (!P.GetItem())
				InvalidIndices.AddUnique(P.ItemID);
		}
	}
	for (FGuid& TestID : InvalidIndices)
	{
		ClearItem(TestID);
	}
}

FTiledFloor* UTiledLevelAsset::GetActiveFloor() 
{
	return TiledFloors.FindByPredicate([this](const FTiledFloor& TestFloor)
	{
		return TestFloor.FloorPosition == ActiveFloorPosition;
	});
}

FTiledFloor* UTiledLevelAsset::GetBelowActiveFloor()
{
	return GetFloorFromPosition(ActiveFloorPosition - 1);
}

FTiledFloor* UTiledLevelAsset::GetFloorFromPosition(int PositionIndex)
{
	return TiledFloors.FindByPredicate([=](const FTiledFloor& TestFloor)
	{
		return TestFloor.FloorPosition == PositionIndex;
	});
}

void UTiledLevelAsset::AddNewTilePlacement(FTilePlacement NewTile)
{
	UTiledLevelItem* Item = NewTile.GetItem();
	if (!Item) return;
    FTiledFloor* TargetFloor = GetFloorFromPosition(NewTile.GridPosition.Z);
    switch (Item->PlacedType) {
        case EPlacedType::Block:
            TargetFloor->BlockPlacements.Add(NewTile);
            break;
        case EPlacedType::Floor:
            TargetFloor->FloorPlacements.Add(NewTile);
            break;
        default: ;
    }
}

void UTiledLevelAsset::AddNewEdgePlacement(FEdgePlacement NewEdge)
{
	UTiledLevelItem* Item = NewEdge.GetItem();
	if (!Item) return;
    FTiledFloor* TargetFloor = GetFloorFromPosition(NewEdge.Edge.Z);
    switch (Item->PlacedType) {
        case EPlacedType::Wall:
            TargetFloor->WallPlacements.Add(NewEdge);
            break;
        case EPlacedType::Edge:
            TargetFloor->EdgePlacements.Add(NewEdge);
        default: ;
    }
}

void UTiledLevelAsset::AddNewPointPlacement(FPointPlacement NewPoint)
{
	UTiledLevelItem* Item = NewPoint.GetItem();
	if (!Item) return;
    FTiledFloor* TargetFloor = GetFloorFromPosition(NewPoint.GridPosition.Z);
    switch (Item->PlacedType) {
        case EPlacedType::Pillar:
            TargetFloor->PillarPlacements.Add(NewPoint);
            break;
        case EPlacedType::Point:
            TargetFloor->PointPlacements.Add(NewPoint);
            break;
        default: ;
    }
}

TArray<FTilePlacement> UTiledLevelAsset::GetAllBlockPlacements() const
{
	TArray<FTilePlacement> OutArray;
	for (FTiledFloor F : TiledFloors)
	{
		OutArray.Append(F.BlockPlacements);
	}
	return OutArray;
}

TArray<FTilePlacement> UTiledLevelAsset::GetAllFloorPlacements() const
{
	TArray<FTilePlacement> OutArray;
	for (FTiledFloor F : TiledFloors)
	{
		OutArray.Append(F.FloorPlacements);
	}
	return OutArray;
}

TArray<FPointPlacement> UTiledLevelAsset::GetAllPillarPlacements() const
{
	TArray<FPointPlacement> OutArray;
	for (FTiledFloor F : TiledFloors)
	{
		OutArray.Append(F.PillarPlacements);
	}
	return OutArray;
}


TArray<FEdgePlacement> UTiledLevelAsset::GetAllWallPlacements() const
{
	TArray<FEdgePlacement> OutArray;
	for (FTiledFloor F : TiledFloors)
	{
		OutArray.Append(F.WallPlacements);
	}
	return OutArray;
}

TArray<FEdgePlacement> UTiledLevelAsset::GetAllEdgePlacements() const
{
	TArray<FEdgePlacement> OutArray;
	for (FTiledFloor F : TiledFloors)
	{
		OutArray.Append(F.EdgePlacements);
	}
	return OutArray;
}

TArray<FPointPlacement> UTiledLevelAsset::GetAllPointPlacements() const
{
	TArray<FPointPlacement> OutArray;
	for (FTiledFloor F : TiledFloors)
	{
		OutArray.Append(F.PointPlacements);
	}
	return OutArray;
}

TArray<FItemPlacement> UTiledLevelAsset::GetAllItemPlacements() const
{
	TArray<FItemPlacement> OutArray;
	for (FTiledFloor F : TiledFloors)
	{
		OutArray.Append(F.GetItemPlacements());
	}
	return OutArray;
}

int UTiledLevelAsset::GetNumOfAllPlacements() const
{
	int Out = 0;
	Out += GetAllBlockPlacements().Num();
	Out += GetAllFloorPlacements().Num();
	Out += GetAllWallPlacements().Num();
	Out += GetAllPillarPlacements().Num();
	Out += GetAllEdgePlacements().Num();
	Out += GetAllPointPlacements().Num();
	return Out;
}

void UTiledLevelAsset::SetActiveItemSet(UTiledItemSet* NewItemSet)
{
	ActiveItemSet = NewItemSet;
}

TSet<UTiledLevelItem*> UTiledLevelAsset::GetUsedItems() const
{
	TSet<UTiledLevelItem*> UsedItemsSet;
	for (FTiledFloor F : TiledFloors)
	{
		for (FItemPlacement& P : F.GetItemPlacements())
		{
		    UsedItemsSet.Add(P.GetItem());
		}
	}
	return UsedItemsSet;
}
/*
 * it takes me so long to get the static mesh inside spawned actors...
 * Must use actors that are actually spawned to get access to them...
 * Actor->BlueprintCreatedComponents only display BP components as it shows, while
 * Actor->GetComponents can actual get both... The reason why I can't get them is because I
 * tried it from "UObject" pointer set in UTiledItem rather the the actual spawned actor pointer 
 */
TSet<UStaticMesh*> UTiledLevelAsset::GetUsedStaticMeshSet() const
{
	TSet<UStaticMesh*> MeshesSet;
	for (UTiledLevelItem* UsedItem : GetUsedItems())
	{
		if (UsedItem->TiledMesh)
		{
			MeshesSet.Add(UsedItem->TiledMesh);
		}
		else if (UsedItem->TiledActor)
		{
			for (AActor* SpawnedActor : HostLevel->SpawnedTiledActors)
			{
				for (auto C : SpawnedActor->GetComponents())
				{
					if ( UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(C))
					{
						if (SMC->GetStaticMesh())
						{
							MeshesSet.Add(SMC->GetStaticMesh());
						}
					}
				}
			}
		}
	}
	return MeshesSet;
}

void UTiledLevelAsset::EmptyRegionData(const TArray<FIntVector>& Points)
{
	if (Points.Num() == 0) return;
 	TArray<FTilePlacement> TileToDelete;
 	TArray<FTilePlacement> TargetTilePlacements;
 	TargetTilePlacements.Append(GetAllBlockPlacements());
 	TargetTilePlacements.Append(GetAllFloorPlacements());
 	TArray<FEdgePlacement> WallToDelete;
 	TArray<FEdgePlacement> TargetWallPlacements;
 	TargetWallPlacements.Append(GetAllWallPlacements());
 	TargetWallPlacements.Append(GetAllEdgePlacements());
 	TArray<FPointPlacement> PointToDelete;
 	TArray<FPointPlacement>  TargetPointPlacements;
 	TargetPointPlacements.Append(GetAllPillarPlacements());
 	TargetPointPlacements.Append(GetAllPointPlacements());

	bool Found = false;
 	for (FTilePlacement& Placement : TargetTilePlacements)
 	{
 		for (FIntVector& Position : Placement.GetOccupiedTilePositions())
 		{
 			if (Points.Contains(Position))
 			{
 				TileToDelete.Add(Placement);
 				break;
 			}
 		}
 	}	
	
 	TSet<FIntVector> Region = TSet<FIntVector>(Points);
 	
 	const TArray<FTiledLevelEdge> InnerEdges = FTiledLevelUtility::GetAreaEdges(Region, false);
 	for (FEdgePlacement& Placement : TargetWallPlacements)
 	{
 		FVector Extent = Placement.GetItem()->Extent;
 		for (FTiledLevelEdge& E : Placement.GetOccupiedEdges(Extent))
 		{
			if (InnerEdges.Contains(Placement.Edge))
			{
				WallToDelete.Add(Placement);
				break;
			}
 		}
 	}
 
 	const TSet<FIntVector> InnerPoints = FTiledLevelUtility::GetAreaPoints(Region, false);
 	
 	for (FPointPlacement& Placement : TargetPointPlacements)
 	{
 		if (InnerPoints.Contains(Placement.GridPosition))
 		{
 			PointToDelete.Add(Placement);
 		}
 	}
 	RemovePlacements(TileToDelete);
 	RemovePlacements(WallToDelete);
 	RemovePlacements(PointToDelete);
}

void UTiledLevelAsset::EmptyEdgeRegionData(const TArray<FTiledLevelEdge>& EdgeRegions)
{
	if (EdgeRegions.Num() == 0 ) return;
 	TArray<FEdgePlacement> WallToDelete;
 	TArray<FEdgePlacement> TargetWallPlacements;
 	TargetWallPlacements.Append(GetAllWallPlacements());
 	TargetWallPlacements.Append(GetAllEdgePlacements());

	for (FEdgePlacement& Placement : TargetWallPlacements)
	{
		FVector EdgeExtent = Placement.GetItem()->Extent;
		
		for (FTiledLevelEdge& Edge : Placement.GetOccupiedEdges(EdgeExtent))
		{
			if (EdgeRegions.Contains(Edge))
			{
				WallToDelete.Add(Placement);
				break;
			}
		}
	}
	RemovePlacements(WallToDelete);
}

void UTiledLevelAsset::GetAssetRegistryTags(TArray<FAssetRegistryTag>& AssetRegistryTags) const
{
	const FString TileSizeStr = FString::Printf(TEXT("%dx%dx%d"), FMath::RoundToInt(TileSizeX), FMath::RoundToInt(TileSizeY), FMath::RoundToInt(TileSizeZ));
	const FString TileAmountStr = FString::Printf(TEXT("%dx%dx%d"), X_Num, Y_Num, TiledFloors.Num());
	AssetRegistryTags.Add( FAssetRegistryTag("Tile Size", TileSizeStr, FAssetRegistryTag::TT_Dimensional));
	AssetRegistryTags.Add( FAssetRegistryTag("Tile Amount", TileAmountStr, FAssetRegistryTag::TT_Dimensional));
	if (TiledFloors.IsValidIndex(0))
	{
		const FString FloorsInfoStr = FString::Printf(TEXT("%s to %s"), *TiledFloors[0].FloorName.ToString(), *TiledFloors.Last().FloorName.ToString());
		AssetRegistryTags.Add(FAssetRegistryTag("Floors", FloorsInfoStr, FAssetRegistryTag::TT_Alphabetical));
	}
	AssetRegistryTags.Add(FAssetRegistryTag("Total Placements",FString::FromInt(GetNumOfAllPlacements()), FAssetRegistryTag::TT_Numerical));
	
	UObject::GetAssetRegistryTags(AssetRegistryTags);
}

#undef LOCTEXT_NAMESPACE
