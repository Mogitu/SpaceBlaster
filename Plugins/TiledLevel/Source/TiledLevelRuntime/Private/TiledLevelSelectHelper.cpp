// Copyright 2022 PufStudio. All Rights Reserved.


#include "TiledLevelSelectHelper.h"

#include "TiledLevelItem.h"


// Sets default values
ATiledLevelSelectHelper::ATiledLevelSelectHelper()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	SelectionArea = CreateDefaultSubobject<UTiledLevelGrid>(TEXT("SelectionArea"));
	SelectionArea->SetupAttachment(Root);
	SelectionArea->SetBoxColor(FColor::Yellow);
	SelectionArea->SetLineThickness(5);
	SelectionArea->SetVisibility(false);
	
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

void ATiledLevelSelectHelper::SyncToSelection(FVector InTileSize, const TArray<FTilePlacement>& InTiles,
    const TArray<FEdgePlacement>& InEdges, const TArray<FPointPlacement>& InPoints, bool bForTemplate)
{
    EmptyHints();
    EmptyData(bForTemplate);
    TArray<FTilePlacement>* TileStorage = &SelectedTiles;
    TArray<FEdgePlacement>* EdgeStorage = &SelectedEdges;
    TArray<FPointPlacement>* PointStorage = &SelectedPoints;
    if (bForTemplate)
    {
        TileStorage = &TemplateTiles;
        EdgeStorage = &TemplateEdges;
        PointStorage = &TemplatePoints;
    }
	*TileStorage = InTiles;
    *EdgeStorage = InEdges;
    *PointStorage = InPoints;
	TileSize = InTileSize;

	// Reevaluate extent based on actually selected placements
	FIntVector MinSelect = {1024, 1024, 1024};
	FIntVector MaxSelect = {0, 0, 0};

	for (auto Tile : *TileStorage)
	{
		if (Tile.GridPosition.X < MinSelect.X)
			MinSelect.X = Tile.GridPosition.X;
		if (Tile.GridPosition.X + Tile.Extent.X > MaxSelect.X)
			MaxSelect.X = Tile.GridPosition.X + Tile.Extent.X - 1;
		if (Tile.GridPosition.Y < MinSelect.Y)
			MinSelect.Y = Tile.GridPosition.Y;
		if (Tile.GridPosition.Y + Tile.Extent.Y > MaxSelect.Y)
			MaxSelect.Y = Tile.GridPosition.Y + Tile.Extent.Y - 1;
		if (Tile.GridPosition.Z < MinSelect.Z)
			MinSelect.Z = Tile.GridPosition.Z;
		if (Tile.GridPosition.Z + Tile.Extent.Z > MaxSelect.Z)
			MaxSelect.Z = Tile.GridPosition.Z + Tile.Extent.Z - 1;
	}

	for (auto Edge : *EdgeStorage)
	{
		if (Edge.GetEdgePosition().X < MinSelect.X)
			 MinSelect.X = Edge.GetEdgePosition().X;
		if (Edge.GetEdgePosition().Y < MinSelect.Y)
			 MinSelect.Y = Edge.GetEdgePosition().Y;
		if (Edge.Edge.EdgeType == EEdgeType::Horizontal)
		{
			if (Edge.GetEdgePosition().X + Edge.GetItem()->Extent.X - 1 > MaxSelect.X)
				MaxSelect.X = Edge.GetEdgePosition().X + Edge.GetItem()->Extent.X - 1;
			if (Edge.GetEdgePosition().Y - 1 > MaxSelect.Y)
				MaxSelect.Y = Edge.GetEdgePosition().Y - 1;
		}
		else
		{
			if (Edge.GetEdgePosition().X - 1 > MaxSelect.X)
				MaxSelect.X = Edge.GetEdgePosition().X - 1;
			if (Edge.GetEdgePosition().Y + Edge.GetItem()->Extent.X - 1 > MaxSelect.Y)
				MaxSelect.Y = Edge.GetEdgePosition().Y + Edge.GetItem()->Extent.X - 1;
		}
		if (Edge.GetEdgePosition().Z < MinSelect.Z)
			MinSelect.Z = Edge.GetEdgePosition().Z;
		if (Edge.GetEdgePosition().Z > MaxSelect.Z)
			MaxSelect.Z = Edge.GetEdgePosition().Z;
	}
	for (auto Point : *PointStorage)
	{
		if (Point.GridPosition.X < MinSelect.X)
			MinSelect.X = Point.GridPosition.X;
		if (Point.GridPosition.X - 1 > MaxSelect.X)
			MaxSelect.X = Point.GridPosition.X - 1;
		if (Point.GridPosition.Y < MinSelect.Y)
			MinSelect.Y = Point.GridPosition.Y;
		if (Point.GridPosition.Y - 1 > MaxSelect.Y - 1)
			MaxSelect.Y = Point.GridPosition.Y - 1;
		if (Point.GridPosition.Z < MinSelect.Z)
			MinSelect.Z = Point.GridPosition.Z;
		if (Point.GridPosition.Z > MaxSelect.Z)
			MaxSelect.Z = Point.GridPosition.Z;
	}

	for (FTilePlacement& Tile : *TileStorage)
		Tile.OffsetGridPosition(MinSelect * -1, TileSize);
	for (FEdgePlacement& Edge: *EdgeStorage)
		Edge.OffsetEdgePosition(MinSelect * -1, TileSize);
	for (FPointPlacement& Point : *PointStorage)
		Point.OffsetPointPosition(MinSelect * -1, TileSize);
    if (bForTemplate)
        TemplateExtent = MaxSelect - MinSelect + FIntVector(1);
    else
        SelectionExtent = MaxSelect - MinSelect + FIntVector(1);
}

void ATiledLevelSelectHelper::PopulateCopied(bool bForTemplate)
{
    FVector TargetExtent = bForTemplate? FVector(TemplateExtent) : FVector(SelectionExtent);
	FVector LocationOffsetMod = {0,0, 0 };
    if (TargetExtent.X == 0)
    {
	    TargetExtent.X = 0.25;
    	LocationOffsetMod = FVector(-0.125 * TileSize.Y, 0, 0);
    }
    if (TargetExtent.Y == 0)
    {
	    TargetExtent.Y = 0.25;
    	LocationOffsetMod = FVector(0, -0.125 * TileSize.Y, 0);
    }
	// reset rotation
	RotationIndex = 0;
	SelectionArea->SetRelativeRotation(FRotator(0, 0, 0));
	SelectionArea->SetBoxExtent(TargetExtent * TileSize * 0.5);
	SelectionArea->SetRelativeLocation(TargetExtent * TileSize * 0.5 + LocationOffsetMod);
	SelectionArea->SetVisibility(true);

	TArray<FItemPlacement> AllCopied;
    if (bForTemplate)
    {
        DisplayStatus = Template;
        AllCopied.Append(TemplateTiles);
        AllCopied.Append(TemplateEdges);
        AllCopied.Append(TemplatePoints);
    }
    else
    {
        DisplayStatus = Selection;
        AllCopied.Append(SelectedTiles);
        AllCopied.Append(SelectedEdges);
        AllCopied.Append(SelectedPoints);
    }
	for (FItemPlacement& P : AllCopied)
	{
		UTiledLevelItem* Item = P.GetItem();
		if (Item->SourceType == ETLSourceType::Mesh)
		{
			CreateHISM(Item->TiledMesh);
			HintMeshSpawner[Item->TiledMesh]->AddInstance(P.TileObjectTransform);
		}
		else
		{
			AActor* NewActor = GetWorld()->SpawnActor(Cast<UBlueprint>(Item->TiledActor)->GeneratedClass);
			NewActor->SetActorLocation(GetActorLocation());
			NewActor->AttachToComponent(SelectionArea, FAttachmentTransformRules::KeepRelativeTransform);
			NewActor->SetActorRelativeTransform(P.TileObjectTransform);
			const FVector Offset = FVector(TargetExtent) * TileSize * -0.5;
			NewActor->AddActorWorldOffset(Offset); // stuck me whole day... and it's add WORLD offset...
			HintActors.Add(NewActor);
		}
	}
}

void ATiledLevelSelectHelper::Move(FIntVector NewPosition, FVector InTileSize, FTransform AnchorTransform)
{
    SetActorLocation(AnchorTransform.TransformPosition(FVector(NewPosition) * InTileSize));
}

void ATiledLevelSelectHelper::RotateSelection(bool IsClockwise)
{
    const FIntVector TargetExtent = DisplayStatus == Template? TemplateExtent : SelectionExtent;
	if  (IsClockwise)
	{
		RotationIndex = RotationIndex == 3? 0 : RotationIndex + 1;
	}
	else
	{
		RotationIndex = RotationIndex == 0? 3 : RotationIndex - 1;
	}
	switch (RotationIndex)
	{
	case 0:
		SelectionArea->SetRelativeRotation(FRotator(0, 0, 0));
		SelectionArea->SetRelativeLocation(FVector(TargetExtent) * TileSize * 0.5);
		break;
	case 1:
		SelectionArea->SetRelativeRotation(FRotator(0, 90, 0));
		SelectionArea->SetRelativeLocation(FVector(TargetExtent.Y, TargetExtent.X, TargetExtent.Z) * TileSize * 0.5);
		break;
	case 2:
		SelectionArea->SetRelativeRotation(FRotator(0, 180, 0));
		SelectionArea->SetRelativeLocation(FVector(TargetExtent) * TileSize * 0.5);
		break;
	case 3:
		SelectionArea->SetRelativeRotation(FRotator(0, 270, 0));
		SelectionArea->SetRelativeLocation(FVector(TargetExtent.Y, TargetExtent.X, TargetExtent.Z) * TileSize * 0.5);
		break;
	default:
		break;
	}
}

#if WITH_EDITOR
void ATiledLevelSelectHelper::Hide()
{
	SelectionArea->SetVisibility(false);
	for (AActor* Actor : HintActors)
	{
		Actor->SetIsTemporarilyHiddenInEditor(true);
	}
	for (auto& elem : HintMeshSpawner)
	{
		elem.Value->SetVisibility(false);
	}
}

void ATiledLevelSelectHelper::Unhide()
{
    SelectionArea->SetVisibility(true);
    for (AActor* Actor : HintActors)
    {
        Actor->SetIsTemporarilyHiddenInEditor(false);
    }
    for (auto& elem : HintMeshSpawner)
    {
        elem.Value->SetVisibility(true);
    }
}
#endif

int ATiledLevelSelectHelper::GetNumOfCopiedPlacements() const
{
    switch (DisplayStatus) {
        case Selection:
            return SelectedTiles.Num() + SelectedEdges.Num() + SelectedPoints.Num();
        case Template:
            return TemplateTiles.Num() + TemplateEdges.Num() + TemplatePoints.Num();
        default:
            return 0;
    }
}

void ATiledLevelSelectHelper::GetCopiedPlacements(TArray<FTilePlacement>& OutTiles, TArray<FEdgePlacement>& OutEdges,
    TArray<FPointPlacement>& OutPoints, bool bFromTemplate)
{
    OutTiles = GetModifiedTiles(bFromTemplate);
    OutEdges = GetModifiedEdges(bFromTemplate);
    OutPoints = GetModifiedPoints(bFromTemplate);
}

FIntVector ATiledLevelSelectHelper::GetCopiedExtent(bool bForTemplate)
{
    const FIntVector TargetExtent = bForTemplate? TemplateExtent : SelectionExtent;
	if (RotationIndex == 1 || RotationIndex == 3)
		return FIntVector(TargetExtent.Y, TargetExtent.X, TargetExtent.Z);
	return TargetExtent;
}

// finally correct now??
TArray<FTilePlacement> ATiledLevelSelectHelper::GetModifiedTiles(bool IsFromTemplate)
{
    TArray<FTilePlacement> CopiedTiles = IsFromTemplate? TemplateTiles : SelectedTiles;
    const FIntVector TargetExtent = IsFromTemplate? TemplateExtent : SelectionExtent;
	TArray<FTilePlacement> CopiedTiles_Mod;
	for (FTilePlacement CopiedTile : CopiedTiles)
	{
		FQuat ModRotation = SelectionArea->GetRelativeTransform().TransformRotation(CopiedTile.TileObjectTransform.GetRotation());
		FVector ModPosition = SelectionArea->GetRelativeTransform().TransformPosition(CopiedTile.TileObjectTransform.GetLocation()); // get new translation??
		const FIntVector InitPos = CopiedTile.GridPosition;
		const FIntVector OriginExtent = CopiedTile.Extent;
		FVector ExtentOffset = FVector(TargetExtent) * TileSize * 0.5;

		switch (RotationIndex)
		{
		case 0:
			ModPosition -=  FVector(TargetExtent) * TileSize * 0.5;
			break;
		case 1:
		 	ModPosition -= FRotator(0, 90, 0).RotateVector(ExtentOffset);
			CopiedTile.GridPosition = FIntVector(TargetExtent.Y - InitPos.Y - 1, InitPos.X, InitPos.Z);
			CopiedTile.Extent = FIntVector(OriginExtent.Y, OriginExtent.X, OriginExtent.Z);
			break;
		case 2:
		 	ModPosition -= FRotator(0, 180, 0).RotateVector(ExtentOffset);
			CopiedTile.GridPosition = FIntVector(TargetExtent.X - InitPos.X - 1 - (OriginExtent.X - 1), TargetExtent.Y - InitPos.Y - 1 - (OriginExtent.Y - 1), InitPos.Z);
			break;
		case 3:
		 	ModPosition -= FRotator(0, 270, 0).RotateVector(ExtentOffset);
			CopiedTile.GridPosition = FIntVector(InitPos.Y, TargetExtent.X - InitPos.X - 1 , InitPos.Z);
			CopiedTile.Extent = FIntVector(OriginExtent.Y, OriginExtent.X, OriginExtent.Z);
			break;
		default:
			break;		
		}
		CopiedTile.TileObjectTransform.SetRotation(ModRotation);
		CopiedTile.TileObjectTransform.SetLocation(ModPosition);
		CopiedTiles_Mod.Add(CopiedTile);
	}
	return CopiedTiles_Mod;
}

TArray<FEdgePlacement> ATiledLevelSelectHelper::GetModifiedEdges(bool IsFromTemplate)
{
    TArray<FEdgePlacement> CopiedEdges = IsFromTemplate? TemplateEdges : SelectedEdges;
    const FIntVector TargetExtent = IsFromTemplate? TemplateExtent : SelectionExtent;
	TArray<FEdgePlacement> CopiedEdges_Mod;
	FVector ExtentOffset = FVector(TargetExtent) * TileSize * 0.5;
	for (FEdgePlacement CopiedEdge : CopiedEdges)
	{
		FQuat ModRotation = SelectionArea->GetRelativeTransform().TransformRotation(CopiedEdge.TileObjectTransform.GetRotation());
		FVector ModPosition = SelectionArea->GetRelativeTransform().TransformPosition(CopiedEdge.TileObjectTransform.GetLocation()); // get new translation??
		const FVector InitPos = CopiedEdge.GetEdgePosition();
		int Length = CopiedEdge.GetItem()->Extent.X;
		switch (RotationIndex)
		{
		case 0:
			ModPosition -=  ExtentOffset;
			break;
		case 1:
		 	ModPosition -= FRotator(0, 90, 0).RotateVector(ExtentOffset);
			if (CopiedEdge.Edge.EdgeType == EEdgeType::Horizontal)
				CopiedEdge.SetEdgePosition(FIntVector(TargetExtent.Y - InitPos.Y, InitPos.X, InitPos.Z)); // OK
			else
				CopiedEdge.SetEdgePosition(FIntVector(TargetExtent.Y - Length - InitPos.Y, InitPos.X, InitPos.Z)); // Pass
			CopiedEdge.ToggleEdgeType();
			break;
		case 2:
		 	ModPosition -= FRotator(0, 180, 0).RotateVector(ExtentOffset);
			if (CopiedEdge.Edge.EdgeType == EEdgeType::Horizontal)
				CopiedEdge.SetEdgePosition(FIntVector(TargetExtent.X - InitPos.X - Length, TargetExtent.Y - InitPos.Y, InitPos.Z)); // Ok
			else 
				CopiedEdge.SetEdgePosition(FIntVector(TargetExtent.X - InitPos.X, TargetExtent.Y - InitPos.Y - Length, InitPos.Z)); // ok
			break;
		case 3:
		 	ModPosition -= FRotator(0, 270, 0).RotateVector(ExtentOffset);
			if (CopiedEdge.Edge.EdgeType == EEdgeType::Horizontal)
				CopiedEdge.SetEdgePosition(FIntVector(InitPos.Y, TargetExtent.X - Length - InitPos.X, InitPos.Z)); // ok
			else
				CopiedEdge.SetEdgePosition(FIntVector(InitPos.Y, TargetExtent.X - InitPos.X, InitPos.Z)); // ok...
			CopiedEdge.ToggleEdgeType();
			break;
		default:
			break;		
		}
		CopiedEdge.TileObjectTransform.SetRotation(ModRotation);
		CopiedEdge.TileObjectTransform.SetLocation(ModPosition);
		CopiedEdges_Mod.Add(CopiedEdge);
	}
	return CopiedEdges_Mod;
}

TArray<FPointPlacement> ATiledLevelSelectHelper::GetModifiedPoints(bool IsFromTemplate)
{
    TArray<FPointPlacement> CopiedPoints = IsFromTemplate? TemplatePoints : SelectedPoints;
    const FIntVector TargetExtent = IsFromTemplate? TemplateExtent : SelectionExtent;
	TArray<FPointPlacement> CopiedPoints_Mod;
	for (FPointPlacement CopiedPoint : CopiedPoints)
	{
		FQuat ModRotation = SelectionArea->GetRelativeTransform().TransformRotation(CopiedPoint.TileObjectTransform.GetRotation());
		FVector ModPosition = SelectionArea->GetRelativeTransform().TransformPosition(CopiedPoint.TileObjectTransform.GetLocation()); // get new translation??
		const FIntVector InitPos = CopiedPoint.GridPosition;

		switch (RotationIndex)
		{
		case 0:
			ModPosition -=  FVector(TargetExtent) * TileSize * 0.5;
			break;
		case 1:
		 	ModPosition -= FVector(-TargetExtent.Y, TargetExtent.X, TargetExtent.Z) * TileSize * 0.5;
			CopiedPoint.GridPosition = FIntVector(SelectionExtent.Y - InitPos.Y, InitPos.X, InitPos.Z);
			break;
		case 2:
			ModPosition -=  FVector(TargetExtent) * FVector(-1, -1, 1) * TileSize * 0.5;
			CopiedPoint.GridPosition = FIntVector(TargetExtent.X - InitPos.X, TargetExtent.Y - InitPos.Y, InitPos.Z);
			break;
		case 3:
		 	ModPosition -= FVector(TargetExtent.Y, -TargetExtent.X, TargetExtent.Z) * TileSize * 0.5;
			CopiedPoint.GridPosition = FIntVector(InitPos.Y, TargetExtent.X - InitPos.X, InitPos.Z);
			break;
		default:
			break;		
		}
		CopiedPoint.TileObjectTransform.SetRotation(ModRotation);
		CopiedPoint.TileObjectTransform.SetLocation(ModPosition);
		CopiedPoints_Mod.Add(CopiedPoint);
	}
	return CopiedPoints_Mod;
}

void ATiledLevelSelectHelper::EmptyHints()
{
	for (AActor* Actor : HintActors)
	{
		Actor->Destroy();
	}
	HintActors.Empty();
	for (auto& elem : HintMeshSpawner)
	{
		// elem.Value->ClearInstances();
		while (elem.Value->GetInstanceCount() > 0)
		{
			elem.Value->RemoveInstance(0);
		}
	}
	HintMeshSpawner.Empty();
	SelectionArea->SetVisibility(false);
}

void ATiledLevelSelectHelper::EmptyData(bool bForTemplate)
{
    if (bForTemplate)
    {
        TemplateTiles.Empty();
        TemplateEdges.Empty();
        TemplatePoints.Empty();
    }
    else
    {
        SelectedTiles.Empty();
        SelectedEdges.Empty();
        SelectedPoints.Empty();
    }
    
	SelectionArea->SetRelativeRotation(FRotator(0));
}

void ATiledLevelSelectHelper::CreateHISM(UStaticMesh* MeshPtr)
{
	if (!HintMeshSpawner.Find(MeshPtr))
	{
		UHierarchicalInstancedStaticMeshComponent* NewHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, NAME_None, RF_Transactional);
		NewHISM->SetStaticMesh(MeshPtr);
		NewHISM->SetWorldLocation(GetActorLocation());
		NewHISM->AttachToComponent(SelectionArea, FAttachmentTransformRules::KeepWorldTransform);
		NewHISM->RegisterComponentWithWorld(GetWorld());
		HintMeshSpawner.Add(MeshPtr, NewHISM);
	}
}

// Called when the game starts or when spawned
void ATiledLevelSelectHelper::BeginPlay()
{
	Super::BeginPlay();
	Destroy();
}

