// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelTypes.h"
#include "TiledItemSet.h"

UTiledLevelItem* FItemPlacement::GetItem() const
{
	if (ItemSet)
		return ItemSet->GetItem(ItemID);
	return nullptr;
}

#define SEARCH_REMOVE_PLACEMENT(Type)\
FoundID = ##Type.IndexOfByPredicate([=](FItemPlacement P) \
{\
	return P.ItemID == ItemID && P.TileObjectTransform.Equals(CompareTransform); \
}); \
if (FoundID != INDEX_NONE) \
{ \
	##Type.RemoveAt(FoundID);\
	return true; \
}

void FTiledLevelGameData::Empty()
{
	BlockPlacements.Empty();
	FloorPlacements.Empty();
	WallPlacements.Empty();
	EdgePlacements.Empty();
	PillarPlacements.Empty();
	PointPlacements.Empty();
}

bool FTiledLevelGameData::RemovePlacement(FTransform CompareTransform, FGuid ItemID)
{
	int FoundID;
	SEARCH_REMOVE_PLACEMENT(BlockPlacements)
	SEARCH_REMOVE_PLACEMENT(FloorPlacements)
	SEARCH_REMOVE_PLACEMENT(WallPlacements)
	SEARCH_REMOVE_PLACEMENT(EdgePlacements)
	SEARCH_REMOVE_PLACEMENT(PillarPlacements)
	SEARCH_REMOVE_PLACEMENT(PointPlacements)
	return false;
}

void FTiledLevelGameData::RemovePlacements(const TArray<FTilePlacement>& ToDelete)
{
	BlockPlacements.RemoveAll([=](const FTilePlacement& P)
	{
		return ToDelete.Contains(P);
	});
	FloorPlacements.RemoveAll([=](const FTilePlacement& P)
	{
		return ToDelete.Contains(P);
	});
}

void FTiledLevelGameData::RemovePlacements(const TArray<FEdgePlacement>& ToDelete)
{
	WallPlacements.RemoveAll([=](const FEdgePlacement& P)
	{
		return ToDelete.Contains(P);
	});
	EdgePlacements.RemoveAll([=](const FEdgePlacement& P)
	{
		return ToDelete.Contains(P);
	});
}

void FTiledLevelGameData::RemovePlacements(const TArray<FPointPlacement>& ToDelete)
{
	PillarPlacements.RemoveAll([=](const FPointPlacement& P)
	{
		return ToDelete.Contains(P);
	});
	PointPlacements.RemoveAll([=](const FPointPlacement& P)
	{
		return ToDelete.Contains(P);
	});
}


void FTiledLevelGameData::SetFocusFloor(int FloorPosition)
{
	HiddenFloors.Empty();
	for (auto P : BlockPlacements)
		HiddenFloors.AddUnique(P.GridPosition.Z);
	for (auto P : FloorPlacements)
		HiddenFloors.AddUnique(P.GridPosition.Z);
	for (auto P : WallPlacements)
		HiddenFloors.AddUnique(P.GetEdgePosition().Z);
	for (auto P : EdgePlacements)
		HiddenFloors.AddUnique(P.GetEdgePosition().Z);
	for (auto P : PillarPlacements)
		HiddenFloors.AddUnique(P.GridPosition.Z);
	for (auto P : PointPlacements)
		HiddenFloors.AddUnique(P.GridPosition.Z);
	if (HiddenFloors.Contains(FloorPosition))
	{
		HiddenFloors.Remove(FloorPosition);
	}
	else if (FloorPosition > FMath::Max(HiddenFloors))
	{
		HiddenFloors.Remove(FMath::Max(HiddenFloors));
	}
	else
	{
		HiddenFloors.Remove(FMath::Min(HiddenFloors));
	}
}
