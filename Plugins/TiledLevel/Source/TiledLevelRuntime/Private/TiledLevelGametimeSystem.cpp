// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelGametimeSystem.h"
#include "TiledLevel.h"
#include "TiledLevelEditorHelper.h"
#include "TiledLevelEditorLog.h"
#include "TiledLevelRestrictionHelper.h"
#include "TiledLevelUtility.h"
#include "DrawDebugHelpers.h"
#include "TiledLevelItem.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"

bool UTiledLevelGametimeSystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

void UTiledLevelGametimeSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	OnBeginSystem();
}

void UTiledLevelGametimeSystem::Deinitialize()
{
	Super::Deinitialize();
}

void UTiledLevelGametimeSystem::OnBeginSystem_Implementation()
{
}

void UTiledLevelGametimeSystem::InitializeGametimeSystemFromData(const UObject* WorldContextObject,
                                                                 UTiledItemSet* StartupItemSet, const FTiledLevelGameData& InData, TArray<ATiledLevel*> TiledLevelsInsideData,
                                                                 bool bUnbound)
{
	UWorld* InWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	// if (!UGameplayStatics::GetPlayerController(InWorld,0)->HasAuthority()) return;

	if (!InWorld || !StartupItemSet)
	{
		GametimeMode = Uninitialized;
		return;
	}
	if (StartupItemSet->GetTileSize() != TileSize)
	{
		GametimeMode = Uninitialized;
		return;
	}

	DeactivateEraserMode();
	DeactivatePreviewItem();
	GametimeMode = bUnbound? EGametimeMode::Infinite : EGametimeMode::BoundToExistingLevels;
	SourceItemSet = StartupItemSet;
	if (GametimeLevel) GametimeLevel->Destroy();
	
	// delete existing levels
	for (ATiledLevel* Atl : TiledLevelsInsideData)
	{	if (!Atl) continue;
		Atl->Destroy();
	}
	
	// init data
	GametimeData = InData;
	
	// spawn required actors
	FActorSpawnParameters SpawnParams;
	SpawnParams.bNoFail = true; // this stuck me so long... force it spawn... no matter the collision...
	GametimeLevel = InWorld->SpawnActor<ATiledLevel>(ATiledLevel::StaticClass(), FVector(0, 0, 0), FRotator(0), SpawnParams);
	// GametimeLevel->SetSystem(this);
	GametimeLevel->GametimeData = GametimeData;
	// GametimeLevel->ResetAllInstance(GametimeData);
	
	Helper = InWorld->SpawnActor<ATiledLevelEditorHelper>(ATiledLevelEditorHelper::StaticClass(), FVector(0), FRotator(0), SpawnParams);
	Helper->SetTileSize(TileSize);
	Helper->SetShouldUsePreviewMaterial(ShouldUsePreviewMaterial);
	if (ShouldUsePreviewMaterial)
	{
		if (PreviewMaterial_CanBuildHere)
			 Helper->SetCanBuildPreviewMaterial(PreviewMaterial_CanBuildHere);
		if (PreviewMaterial_CanNotBuildHere)
			 Helper->SetCanNotBuildPreviewMaterial(PreviewMaterial_CanNotBuildHere);
	}
	Helper->SetActorTransform(GametimeLevel->GetActorTransform());
	Helper->AttachToActor(GametimeLevel, FAttachmentTransformRules::KeepWorldTransform);
}

void UTiledLevelGametimeSystem::InitializeGametimeSystem(const UObject* WorldContextObject, UTiledItemSet* StartupItemSet, const TArray<ATiledLevel*>& ExistingTiledLevels, bool bUnbound)
{
	UWorld* InWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!InWorld)
	{
		ERROR_LOG("No valid world to start gametime system")
		GametimeMode = Uninitialized;
		return;
	}
	if (!StartupItemSet)
	{
		ERROR_LOG("Forget to set startup item set?")
		GametimeMode = Uninitialized;
		return;
	}
	if (StartupItemSet->GetTileSize() != TileSize)
	{
		ERROR_LOG("System tile size mismatched with startup item set")
		GametimeMode = Uninitialized;
		return;
	}
	
	SourceItemSet = StartupItemSet;
	if (GametimeLevel) GametimeLevel->Destroy();

	// mark other tiled levels as static...
	TArray<AActor*> AllTL;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATiledLevel::StaticClass(), AllTL);
	for (auto& TL: AllTL)
	{
		if (ExistingTiledLevels.Contains(TL)) continue;
		TL->Tags.Add(FName("StaticTiledLevel"));
	}
	
	// check existing levels and get initialization result
	for (ATiledLevel* Atl : ExistingTiledLevels)
		FTiledLevelUtility::ApplyGametimeRequiredTransform(Atl, TileSize);
	TArray<ATiledLevel*> PairedLevels = ExistingTiledLevels;
	for (ATiledLevel* Atl1 : ExistingTiledLevels)
	{
		for (ATiledLevel* Atl2 : PairedLevels)
		{
			if (Atl1 == Atl2) continue;
			if (FTiledLevelUtility::CheckOverlappedTiledLevels(Atl1, Atl2))
			{
				GametimeMode = Uninitialized;
				return;
			}
		}
		PairedLevels.Remove(Atl1);
	}
	GametimeMode = Infinite;
	if (ExistingTiledLevels.Num() > 0 && !bUnbound)
	{
		GametimeMode = BoundToExistingLevels;
	}

	// init data
	GametimeData.Empty();
	for (ATiledLevel* Atl : ExistingTiledLevels)
	{
		if (!Atl) continue;
		if (Atl->GetAsset()->GetTileSize() == TileSize)
		{
			GametimeData += Atl->MakeGametimeData();
		}
		Atl->Destroy();
	}
	
	// spawn required actors
	FActorSpawnParameters SpawnParams;
	SpawnParams.bNoFail = true; // this stuck me so long... force it spawn... no matter the collision...
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(InWorld, 0);
	SpawnParams.Instigator = PlayerController->GetInstigator();
	SpawnParams.Owner = PlayerController;
	Helper = InWorld->SpawnActor<ATiledLevelEditorHelper>(ATiledLevelEditorHelper::StaticClass(), FVector(0), FRotator(0), SpawnParams);
	Helper->SetTileSize(TileSize);
	Helper->SetShouldUsePreviewMaterial(ShouldUsePreviewMaterial);
	
	if (ShouldUsePreviewMaterial)
	{
		if (PreviewMaterial_CanBuildHere)
			Helper->SetCanBuildPreviewMaterial(PreviewMaterial_CanBuildHere);
		if (PreviewMaterial_CanNotBuildHere)
			Helper->SetCanNotBuildPreviewMaterial(PreviewMaterial_CanNotBuildHere);
	}
	
	GametimeLevel = InWorld->SpawnActor<ATiledLevel>(FVector(0, 0, 0), FRotator(0), SpawnParams);
	GametimeLevel->GametimeData = GametimeData;
	GametimeLevel->ResetAllInstanceFromData();

	// TODO: leave for next update for replication...
	/*if (UKismetSystemLibrary::IsServer(InWorld))
	{
		GametimeLevel = InWorld->SpawnActor<ATiledLevel>(FVector(0, 0, 0), FRotator(0), SpawnParams);
		GametimeLevel->SetActorTransform(FTransform());
	}
	else
	{
		if (ATiledLevel* ReplicatedLevel = Cast<ATiledLevel>(UGameplayStatics::GetActorOfClass(GetWorld(), ATiledLevel::StaticClass())))
		{
			GametimeLevel = ReplicatedLevel;
		}
	}
	GametimeLevel->GametimeData = GametimeData;
	if (UKismetSystemLibrary::IsServer(InWorld))
	{
		GametimeLevel->ResetAllInstanceFromData();
	}*/

}

bool UTiledLevelGametimeSystem::ChangeItemSet(UTiledItemSet* NewItemSet)
{
	if (NewItemSet->GetTileSize() != TileSize) return false;
	SourceItemSet = NewItemSet;
	return true;
}


void UTiledLevelGametimeSystem::ActivatePreviewItem(UTiledLevelItem* PreviewItem)
{
	if (GametimeMode == Uninitialized)
	{
		DEV_LOG("Error! system is not init")
		return;
	}
	if (!PreviewItem)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Force quit game since preview item is invalid"));
		// BUG detected (UE4.26), when quit game right after begin play in PIE, it will just crash!!
		UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
		return;
	}
	if (!GametimeLevel)
	{
		ERROR_LOG("No gametime level... why...")
		return;
	}
	ActiveItem = PreviewItem;
	const uint8 CachedRotationIndex = Helper->BrushRotationIndex;
	Helper->ResetBrush();
	Helper->SetupPreviewBrushInGame(ActiveItem);
	for (uint8 i = 0; i < CachedRotationIndex; i++)
		RotatePreviewItem();
	if (ActiveItem->IsShape2D())
		MoveEdgePreviewItem(CurrentEdge, false);
	else
		MovePreviewItem(CurrentTilePosition, false);
}

void UTiledLevelGametimeSystem::DeactivatePreviewItem()
{
	if (!GametimeLevel) return;
	ActiveItem = nullptr;
	Helper->ResetBrush();
	CurrentEdge = FTiledLevelEdge(9999,9999,9999,EEdgeType::Vertical);
	CurrentTilePosition = FIntVector(9999);
}

bool UTiledLevelGametimeSystem::MovePreviewItemToScreenPosition(FVector2D ScreenPosition, int FloorPosition, int PlayerIndex)
{
	if (!IsPreviewItemActivated()) return false;
	EPlacedShapeType ShapeType = FTiledLevelUtility::PlacedTypeToShape(ActiveItem->PlacedType);
	bool bFound = false;
	switch (ShapeType)
	{
	case Shape3D:
		{
			FIntVector FoundPosition = GetTilePositionOnScreen(FloorPosition, ScreenPosition, bFound, PlayerIndex);
			MovePreviewItem(FoundPosition);
			break;
		}
	case Shape2D:
		{
			EEdgeType NewEdgeType = ShouldRotatePreviewBrush? EEdgeType::Vertical: EEdgeType::Horizontal;
			FTiledLevelEdge FoundEdge = GetEdgeOnScreen(FloorPosition, ScreenPosition, NewEdgeType, bFound, PlayerIndex);
			MoveEdgePreviewItem(FoundEdge);
			break;
		}
	case Shape1D:
		{
			FIntVector FoundPoint = GetPointOnScreen(FloorPosition, ScreenPosition, bFound, PlayerIndex);
			MovePreviewItem(FoundPoint);
			break;
		}
	default: ;
	}
	return bFound;
}

bool UTiledLevelGametimeSystem::MovePreviewItemToCursor(int FloorPosition, int PlayerIndex)
{
	if (!IsPreviewItemActivated()) return false;
	EPlacedShapeType ShapeType = FTiledLevelUtility::PlacedTypeToShape(ActiveItem->PlacedType);
	bool bFound = false;
	switch (ShapeType)
	{
	case Shape3D:
		{
			FIntVector FoundPosition = GetTilePositionUnderCursor(FloorPosition, bFound, PlayerIndex);
			MovePreviewItem(FoundPosition);
			break;
		}
	case Shape2D:
		{
			EEdgeType NewEdgeType = ShouldRotatePreviewBrush? EEdgeType::Vertical: EEdgeType::Horizontal;
			FTiledLevelEdge FoundEdge = GetEdgeUnderCursor(FloorPosition, NewEdgeType, bFound, PlayerIndex);
			MoveEdgePreviewItem(FoundEdge);
			break;
		}
	case Shape1D:
		{
			FIntVector FoundPoint = GetPointUnderCursor(FloorPosition, bFound, PlayerIndex);
			MovePreviewItem(FoundPoint);
			break;
		}
	default: ;
	}
	return bFound;
}

bool UTiledLevelGametimeSystem::MovePreviewItemToWorldPosition(FVector TargetLocation)
{
	if (!IsPreviewItemActivated()) return false;
	EPlacedShapeType ShapeType = FTiledLevelUtility::PlacedTypeToShape(ActiveItem->PlacedType);
	bool bFound = false;
	switch (ShapeType)
	{
	case Shape3D:
		{
			FIntVector FoundPosition = GetTilePosition(TargetLocation, bFound);
			MovePreviewItem(FoundPosition);
			break;
		}
	case Shape2D:
		{
			EEdgeType NewEdgeType = ShouldRotatePreviewBrush? EEdgeType::Vertical: EEdgeType::Horizontal;
			FTiledLevelEdge FoundEdge = GetEdge(TargetLocation, NewEdgeType, bFound);
			MoveEdgePreviewItem(FoundEdge);
			break;
		}
	case Shape1D:
		{
			FIntVector FoundPoint = GetPoint(TargetLocation, bFound);
			MovePreviewItem(FoundPoint);
			break;
		}
	default: ;
	}
	return bFound;
}

void UTiledLevelGametimeSystem::RotatePreviewItem(bool IsClockwise)
{
	if (!GametimeLevel) return;
	if (!IsPreviewItemActivated()) return;
	Helper->RotateBrush(IsClockwise, ActiveItem->PlacedType, ShouldRotatePreviewBrush);
	OnItemRotate.Broadcast(ActiveItem, GetBuildLocation(), IsClockwise);
}

void UTiledLevelGametimeSystem::OnLeaveSystemBoundary_Implementation()
{
	if (ActiveItem != nullptr) DeactivatePreviewItem();
	if (IsEraserMode) DeactivateEraserMode();
}

bool UTiledLevelGametimeSystem::CanBuildItem_Implementation(UTiledLevelItem* ItemToBuild)
{
	return true;
}

void UTiledLevelGametimeSystem:: BuildItem()
{
	// check can build
	if (!IsPreviewItemActivated()) return;
	if (!CanBuildHere)
	{
		OnItemFailToBuilt.Broadcast(ActiveItem, GetBuildLocation());
		return;
	}
	EPlacedShapeType ShapeType = FTiledLevelUtility::PlacedTypeToShape(ActiveItem->PlacedType);
	FTilePlacement NewTile;
	FEdgePlacement NewEdge;
	FPointPlacement NewPoint;
	FItemPlacement* NewPlacement = nullptr;
	switch (ShapeType)
	{
		case Shape3D:
			NewPlacement = &NewTile; 
			break;
		case Shape2D:
			NewPlacement = &NewEdge; 
			break;
		case Shape1D:
			NewPlacement = &NewPoint; 
			break;
	}
	
	NewPlacement->ItemSet = SourceItemSet;
	NewPlacement->ItemID = ActiveItem->ItemID;
	// TODO: no mirror for game support for now...
	// NewPlacement->IsMirrored = IsMirrored_X || IsMirrored_Y || IsMirrored_Z;
	NewPlacement->IsMirrored = false;

	switch (ShapeType) {
		case Shape3D:
			NewTile.GridPosition = CurrentTilePosition;
			NewTile.Extent = ShouldRotatePreviewBrush? FIntVector(ActiveItem->Extent.Y, ActiveItem->Extent.X, ActiveItem->Extent.Z) : FIntVector(ActiveItem->Extent);
			break;
		case Shape2D:
			NewEdge.Edge = CurrentEdge;
			break;
		case Shape1D:
			NewPoint.GridPosition = CurrentTilePosition;
			break;
	}
	FTransform G = Helper->GetPreviewPlacementWorldTransform();
	NewPlacement->TileObjectTransform = G.GetRelativeTransform(GametimeLevel->GetTransform());

	switch (ShapeType) {
		case Shape3D:
			if (ActiveItem->PlacedType == EPlacedType::Block)
				GametimeData.BlockPlacements.Add(NewTile);
			else
				GametimeData.FloorPlacements.Add(NewTile);
			GametimeLevel->PopulateSinglePlacement(NewTile);
			break;
		case Shape2D:
			if (ActiveItem->PlacedType == EPlacedType::Edge)
				GametimeData.EdgePlacements.Add(NewEdge);
			else
				GametimeData.WallPlacements.Add(NewEdge);
			GametimeLevel->PopulateSinglePlacement(NewEdge);
			break;
		case Shape1D:
			// TargetLevel->GetAsset()->AddNewPointPlacement(NewPoint);
			if (ActiveItem->PlacedType == EPlacedType::Point)
				GametimeData.PointPlacements.Add(NewPoint);
			else
				GametimeData.PillarPlacements.Add(NewPoint);
			GametimeLevel->PopulateSinglePlacement(NewPoint);
			break;
	}
	OnItemBuilt.Broadcast(ActiveItem, GetBuildLocation());
	CanBuildHere = false;
	Helper->UpdatePreviewHint(CanBuildHere);
}

bool UTiledLevelGametimeSystem::RemoveItem_LineTraceSingle(FVector TraceStart, FVector TraceEnd, bool bShowDebug)
{
	FHitResult HitResult;
	FVector DebugEnd = TraceEnd;
	
	if (bShowDebug)
	{
		DrawDebugLine(GetWorld(), TraceStart, DebugEnd, FColor::Blue, false, 5.0f, 0, 1);
	}
	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility) )
	{
		DebugEnd = HitResult.Location;
		if (bShowDebug)
			DrawDebugBox(GetWorld(),  DebugEnd, FVector(4), FColor::Blue, false, 5, 0, 1);
		return RemoveItem_FromHit(HitResult);
	}
	return false;
}

bool UTiledLevelGametimeSystem::RemoveItem_FromHit(const FHitResult& HitResult)
{
	UTiledLevelItem* HitItem = nullptr;
	FTransform PlacedTransform;
	if (HitResult.GetActor() == nullptr) return false;
	FVector BuildPosition, TilePosition, TileExtent;
	EPlacedShapeType ShapeType;

	if (HitResult.GetActor() == GametimeLevel && HitResult.Component->IsA(UHierarchicalInstancedStaticMeshComponent::StaticClass()))
	{
		UHierarchicalInstancedStaticMeshComponent* HISM = Cast<UHierarchicalInstancedStaticMeshComponent>(HitResult.Component);
		// check if pass CanRemoveItem
		for (UTiledLevelItem* Item : SourceItemSet->GetItemSet())
		{
			 if (Item->TiledMesh == HISM->GetStaticMesh())
			 {
				  HitItem = Item;
				  break;
			 }
		}
		if (!HitItem) return false;
		if (!CanRemoveItem(HitItem) || IsRemoveRestricted(HitItem, HitResult.ImpactPoint))
		{
			OnItemFailToRemove.Broadcast(HitItem, BuildPosition);
			return false;
		}
		// Get build position from HISM data
		ShapeType = FTiledLevelUtility::PlacedTypeToShape(HitItem->PlacedType);
		TilePosition = FVector(
			 HISM->PerInstanceSMCustomData[HitResult.Item*6], 
			 HISM->PerInstanceSMCustomData[HitResult.Item*6 + 1], 
			 HISM->PerInstanceSMCustomData[HitResult.Item*6 + 2]
		);
		TileExtent = FVector(
			 HISM->PerInstanceSMCustomData[HitResult.Item*6 + 3], 
			 HISM->PerInstanceSMCustomData[HitResult.Item*6 + 4], 
			 HISM->PerInstanceSMCustomData[HitResult.Item*6 + 5]
			 );
		BuildPosition = GetBuildLocation(ShapeType, TilePosition, TileExtent);
		// remove data 
		HISM->GetInstanceTransform(HitResult.Item, PlacedTransform);
		GametimeData.RemovePlacement(PlacedTransform, HitItem->ItemID);
		// remove that instance
		HISM->RemoveInstance(HitResult.Item);
		OnItemRemoved.Broadcast(HitItem, BuildPosition);
		return true;
	}
	if (HitResult.GetActor()->IsAttachedTo(GametimeLevel))
	{
		FGuid HitItemID;
		FGuid::Parse(HitResult.GetActor()->Tags.Last().ToString(), HitItemID);
		for (UTiledLevelItem* Item : SourceItemSet->GetItemSet())
		{
			 if (Item->ItemID == HitItemID)
			 {
				  HitItem = Item;
				  break;
			 }
		}
		if (!HitItem) return nullptr;
		if (!CanRemoveItem(HitItem) || IsRemoveRestricted(HitItem, HitResult.ImpactPoint))
		{
			OnItemFailToRemove.Broadcast(HitItem, BuildPosition);
			return false;
		}
		ShapeType = FTiledLevelUtility::PlacedTypeToShape(HitItem->PlacedType);
		int NumTags = HitResult.GetActor()->Tags.Num();
		TilePosition.InitFromString(HitResult.GetActor()->Tags[NumTags - 3].ToString());
		TileExtent.InitFromString(HitResult.GetActor()->Tags[NumTags - 2].ToString());
		BuildPosition = GetBuildLocation(ShapeType, TilePosition, TileExtent);
		if (!CanRemoveItem(HitItem))
		{
			OnItemFailToRemove.Broadcast(HitItem, BuildPosition);
			return false;
		}
		// remove data
		PlacedTransform = HitResult.GetActor()->GetActorTransform().GetRelativeTransform(GametimeLevel->GetTransform());
		GametimeData.RemovePlacement(PlacedTransform, HitItem->ItemID);
		// remove that instance
		HitResult.GetActor()->Destroy();
		OnItemRemoved.Broadcast(HitItem, BuildPosition);
		return true;
	}
	return false;
}

bool UTiledLevelGametimeSystem::RemoveItem_UnderCursor(int32 PlayerIndex)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), PlayerIndex);
	FHitResult HitResult;
	if (PC->GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, HitResult))
	{
		return RemoveItem_FromHit(HitResult);
	}
	return false;
}

bool UTiledLevelGametimeSystem::CanRemoveItem_Implementation(UTiledLevelItem* ItemToRemove)
{

 	return true;
}

void UTiledLevelGametimeSystem::ActivateEraserMode(bool bAnyType, EPlacedType TargetType, FIntVector EraserSize)
{
	if (bAnyType)
		TargetType = EPlacedType::Any;
	EraserType = TargetType;
	IsEraserMode = true;
	EraserExtent = EraserSize;
	Helper->SetupEraserBrush(EraserSize, TargetType);
}

bool UTiledLevelGametimeSystem::MoveEraserToWorldPosition(FVector TargetLocation)
{
	bool Found = false;
	TArray<EPlacedType> GridMovementTypes = { EPlacedType::Any, EPlacedType::Block, EPlacedType::Floor};
	if (GridMovementTypes.Contains(EraserType))
	{
		FIntVector TilePosition = GetTilePosition(TargetLocation, Found);
		if (CurrentTilePosition == TilePosition)
			return false;
		CurrentTilePosition = TilePosition;
		Helper->MoveBrush(TilePosition, false);
	}
	else if (EraserType ==EPlacedType::Edge || EraserType == EPlacedType::Wall)
	{
		EEdgeType NewEdgeType = ShouldRotatePreviewBrush? EEdgeType::Vertical: EEdgeType::Horizontal;
		FTiledLevelEdge TileEdge = GetEdge(TargetLocation, NewEdgeType ,Found);
		if (CurrentEdge == TileEdge)
			return false;
		CurrentEdge = TileEdge;
		Helper->MoveBrush(TileEdge, false);
	}
	else
	{
		FIntVector TilePoint = GetPoint(TargetLocation, Found);
		if (CurrentTilePosition == TilePoint)
			return false;
		CurrentTilePosition = TilePoint;
		Helper->MoveBrush(TilePoint, false);
	}
	return Found;
}

bool UTiledLevelGametimeSystem::MoveEraserToScreenPosition(FVector2D ScreenPosition, int FloorPosition)
{
	bool Found = false;
	TArray<EPlacedType> GridMovementTypes = { EPlacedType::Any, EPlacedType::Block, EPlacedType::Floor};
	if (GridMovementTypes.Contains(EraserType))
	{
		FIntVector TilePosition = GetTilePositionOnScreen(FloorPosition, ScreenPosition, Found);
		if (CurrentTilePosition == TilePosition)
			return false;
		CurrentTilePosition = TilePosition;
		Helper->MoveBrush(TilePosition, false);
	}
	else if (EraserType ==EPlacedType::Edge || EraserType == EPlacedType::Wall)
	{
		EEdgeType NewEdgeType = ShouldRotatePreviewBrush? EEdgeType::Vertical: EEdgeType::Horizontal;
		FTiledLevelEdge TileEdge = GetEdgeOnScreen(FloorPosition, ScreenPosition, NewEdgeType, Found);
		if (CurrentEdge == TileEdge)
			return false;
		CurrentEdge = TileEdge;
		Helper->MoveBrush(TileEdge, false);
	}
	else
	{
		FIntVector TilePoint = GetPointOnScreen(FloorPosition, ScreenPosition, Found);
		if (CurrentTilePosition == TilePoint)
			return false;
		CurrentTilePosition = TilePoint;
		Helper->MoveBrush(TilePoint, false);
	}
	return Found;
}

bool UTiledLevelGametimeSystem::MoveEraserToCursor(int FloorPosition)
{
	bool Found = false;
	TArray<EPlacedType> GridMovementTypes = { EPlacedType::Any, EPlacedType::Block, EPlacedType::Floor};
	if (GridMovementTypes.Contains(EraserType))
	{
		FIntVector TilePosition = GetTilePositionUnderCursor(FloorPosition, Found);
		if (CurrentTilePosition == TilePosition)
			return false;
		CurrentTilePosition = TilePosition;
		Helper->MoveBrush(TilePosition, false);
	}
	else if (EraserType ==EPlacedType::Edge || EraserType == EPlacedType::Wall)
	{
		EEdgeType NewEdgeType = ShouldRotatePreviewBrush? EEdgeType::Vertical: EEdgeType::Horizontal;
		FTiledLevelEdge TileEdge = GetEdgeUnderCursor(FloorPosition, NewEdgeType, Found);
		if (CurrentEdge == TileEdge)
			return false;
		CurrentEdge = TileEdge;
		Helper->MoveBrush(TileEdge, false);
	}
	else
	{
		FIntVector TilePoint = GetPointUnderCursor(FloorPosition, Found);
		if (CurrentTilePosition == TilePoint)
			return false;
		CurrentTilePosition = TilePoint;
		Helper->MoveBrush(TilePoint, false);
	}
	return Found;
}

void UTiledLevelGametimeSystem::RotateEraser(bool bIsClockwise)
{
	if (!GametimeLevel) return;
	Helper->RotateEraserBrush(bIsClockwise, ShouldRotatePreviewBrush);
}

void UTiledLevelGametimeSystem::EraseItem()
{
	if (!IsEraserMode) return;
	TMap<UStaticMesh*, TArray<int32>> TargetInstanceData;
	TArray<FTilePlacement> TilesToDelete;
	TArray<FEdgePlacement> EdgesToDelete;
	TArray<FPointPlacement> PointsToDelete;

	EPlacedShapeType EraserShape = FTiledLevelUtility::PlacedTypeToShape(EraserType);
	if (EraserShape == EPlacedShapeType::Shape3D || EraserType == EPlacedType::Any)
	{
		TArray<FTilePlacement> TargetTilePlacements;
		FTilePlacement TestPlacement;
		TestPlacement.GridPosition = CurrentTilePosition;
		TestPlacement.Extent = EraserExtent;
		if (EraserType == EPlacedType::Any)
		{
			TargetTilePlacements.Append(GametimeData.BlockPlacements);
			TargetTilePlacements.Append(GametimeData.FloorPlacements);
		}
		else if (EraserType == EPlacedType::Block)
			TargetTilePlacements.Append(GametimeData.BlockPlacements);
		else
			TargetTilePlacements.Append(GametimeData.FloorPlacements);
		for (FTilePlacement& Placement : TargetTilePlacements)
		{
			
			if (FTiledLevelUtility::IsTilePlacementOverlapping(TestPlacement, Placement))
			{
				TilesToDelete.Add(Placement);
				if (Placement.GetItem()->SourceType == ETLSourceType::Actor || Placement.IsMirrored)
				{
					GametimeLevel->DestroyTiledActorByPlacement(Placement);
				}
				else
				{
					 UStaticMesh* TiledMesh = Placement.GetItem()->TiledMesh;
					 if (!TargetInstanceData.Contains(TiledMesh))				
						  TargetInstanceData.Add(TiledMesh, TArray<int32>{});
					 TArray<float> TargetInfo = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(Placement);
					 FTiledLevelUtility::FindInstanceIndexByPlacement(TargetInstanceData[TiledMesh],GametimeLevel->TiledObjectSpawner[TiledMesh]->PerInstanceSMCustomData, TargetInfo);
				}
			}
		}
	}

	if (EraserShape == EPlacedShapeType::Shape2D || EraserType == EPlacedType::Any)
	{
		 TArray<FEdgePlacement> TargetEdgePlacements;
		 if (EraserType == EPlacedType::Any)
		 {
			  TargetEdgePlacements.Append(GametimeData.WallPlacements);
			  TargetEdgePlacements.Append(GametimeData.EdgePlacements);
		 }
		 else if (EraserType == EPlacedType::Wall)
			  TargetEdgePlacements.Append(GametimeData.WallPlacements);
		 else
			  TargetEdgePlacements.Append(GametimeData.EdgePlacements);
		 for (FEdgePlacement& Placement : TargetEdgePlacements)
		 {
		 	  bool Con;
		 	  if (EraserType == EPlacedType::Any)
		 	  	  Con = FTiledLevelUtility::IsEdgeInsideTile(Placement.Edge, FIntVector(Placement.GetItem()->Extent), CurrentTilePosition, EraserExtent);
		      else
		      	  Con = FTiledLevelUtility::IsEdgeOverlapping(CurrentEdge, FVector(EraserExtent), Placement.Edge, Placement.GetItem()->Extent);
			  if (Con)
			  {
				  EdgesToDelete.Add(Placement);
				  if (Placement.GetItem()->SourceType == ETLSourceType::Actor || Placement.IsMirrored)
				  {
					   GametimeLevel->DestroyTiledActorByPlacement(Placement);
				  }
				  else
				  {
						UStaticMesh* TiledMesh = Placement.GetItem()->TiledMesh;
						if (!TargetInstanceData.Contains(TiledMesh))				
							  TargetInstanceData.Add(TiledMesh, TArray<int32>{});
						TArray<float> TargetInfo = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(Placement);
						FTiledLevelUtility::FindInstanceIndexByPlacement(TargetInstanceData[TiledMesh],GametimeLevel->TiledObjectSpawner[TiledMesh]->PerInstanceSMCustomData, TargetInfo);
				  }
			  }
		 }
	}

	if (EraserShape == EPlacedShapeType::Shape1D || EraserType == EPlacedType::Any)
	{
		 TArray<FPointPlacement> TargetPointPlacements;
		 FPointPlacement TestPlacement;
		 TestPlacement.GridPosition = CurrentTilePosition;
		 if (EraserType == EPlacedType::Any)
		 {
			  TargetPointPlacements.Append(GametimeData.PillarPlacements);
			  TargetPointPlacements.Append(GametimeData.PointPlacements);
		 }
		 else if (EraserType == EPlacedType::Pillar)
			  TargetPointPlacements.Append(GametimeData.PillarPlacements);
		 else
			  TargetPointPlacements.Append(GametimeData.PointPlacements);
		 for (FPointPlacement& Placement : TargetPointPlacements)
		 {
		 	  bool Con;
		 	  if (EraserType == EPlacedType::Any)
		 	  	  Con = FTiledLevelUtility::IsPointInsideTile(Placement.GridPosition, Placement.GetItem()->Extent.Z, CurrentTilePosition, EraserExtent);
		 	  else
			      Con = FTiledLevelUtility::IsPointPlacementOverlapping(TestPlacement, EraserExtent.Z, Placement, Placement.GetItem()->Extent.Z);
		 	
		 	  if (Con)
			  {
				  PointsToDelete.Add(Placement);
				  if (Placement.GetItem()->SourceType == ETLSourceType::Actor || Placement.IsMirrored)
				  {
					   GametimeLevel->DestroyTiledActorByPlacement(Placement);
				  }
				  else
				  {
						UStaticMesh* TiledMesh = Placement.GetItem()->TiledMesh;
						if (!TargetInstanceData.Contains(TiledMesh))				
							  TargetInstanceData.Add(TiledMesh, TArray<int32>{});
						TArray<float> TargetInfo = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(Placement);
						FTiledLevelUtility::FindInstanceIndexByPlacement(TargetInstanceData[TiledMesh],GametimeLevel->TiledObjectSpawner[TiledMesh]->PerInstanceSMCustomData, TargetInfo);
				  }
			  }
		 }
	}
	GametimeData.RemovePlacements(TilesToDelete);
	GametimeData.RemovePlacements(EdgesToDelete);
	GametimeData.RemovePlacements(PointsToDelete);
	GametimeLevel->RemoveInstances(TargetInstanceData);
}

void UTiledLevelGametimeSystem::DeactivateEraserMode()
{
	IsEraserMode = false;
	Helper->ResetBrush();
}

UTiledLevelItem* UTiledLevelGametimeSystem::QueryItemByUID(FName ItemID)
{
	FGuid UID;
	FGuid::Parse(ItemID.ToString(), UID);
	if (SourceItemSet)
	{
		return *SourceItemSet->GetItemSet().FindByPredicate([=](UTiledLevelItem* Item)
		{
			return Item->ItemID == UID;
		});
	}
	return nullptr;
}

void UTiledLevelGametimeSystem::FocusFloor(int FloorPosition)
{
	GametimeData.SetFocusFloor(FloorPosition);
	GametimeLevel->GametimeData = GametimeData;
	// GametimeLevel->ResetAllInstance(GametimeData);
}

void UTiledLevelGametimeSystem::UnfocusFloor()
{
	GametimeData.HiddenFloors.Empty();
	GametimeLevel->GametimeData = GametimeData;
	// GametimeLevel->ResetAllInstance(GametimeData);
}

bool UTiledLevelGametimeSystem::HasAnyFocusedFloor()
{
	return GametimeData.HiddenFloors.Num() != 0;
}

bool UTiledLevelGametimeSystem::SaveAsTiledLevelAsset(FString TargetFile)
{
	return false;
}


FIntVector UTiledLevelGametimeSystem::GetTilePosition(FVector WorldLocation, bool& Found)
{
	FIntVector FoundPosition = FIntVector(-9999);
	Found = false;
	if (!GametimeLevel) return FoundPosition;

	if (GametimeMode == BoundToExistingLevels)
	{
		bool HasAnyInside = false;
		for (FBox B : GametimeData.Boundaries)
		{
			if (B.IsInside(WorldLocation))
			{
				HasAnyInside = true;
				break;
			}
		}
		if (!HasAnyInside)
			 return FoundPosition;
	}
	Found = true;
	const int X_Mod = WorldLocation.X < 0? -1 : 0;
	const int Y_Mod = WorldLocation.Y < 0? -1 : 0;
	const int Z_Mod = WorldLocation.Z < 0? -1 : 0;
	FoundPosition = FIntVector(WorldLocation.X / TileSize.X + X_Mod, WorldLocation.Y / TileSize.Y + Y_Mod, WorldLocation.Z / TileSize.Z + Z_Mod);
	return FoundPosition;
}

FIntVector UTiledLevelGametimeSystem::GetTilePositionOnScreen(int FloorPosition, FVector2D ScreenPosition, bool& Found, int PlayerIndex)
{
	Found = false;
	FIntVector FoundPosition = FIntVector(-9999);
	APlayerCameraManager* PlayerCameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), PlayerIndex);
	FVector CameraDirection = PlayerCameraManager->GetCameraRotation().Vector().GetSafeNormal();
	FVector TraceStart;
	if (PlayerController->DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, TraceStart, CameraDirection))
	{
		FVector TraceEnd = TraceStart + CameraDirection * HALF_WORLD_MAX;
		float FloorZ = GametimeLevel->GetAsset()->TileSizeZ * FloorPosition + 50;
		FVector InterceptLocation = FMath::LinePlaneIntersection(TraceStart, TraceEnd, FVector(0, 0, FloorZ), FVector(0, 0, 1));
		FoundPosition = GetTilePosition(InterceptLocation, Found);
	}
	return FoundPosition;
}

FIntVector UTiledLevelGametimeSystem::GetTilePositionUnderCursor(int FloorPosition, bool& Found, int PlayerIndex)
{
	Found = false;
	FIntVector FoundPosition = FIntVector(-9999);
	APlayerCameraManager* PlayerCameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), PlayerIndex);
	float MouseX;
	float MouseY;
	PlayerController->GetMousePosition(MouseX, MouseY);
	FVector CameraDirection = PlayerCameraManager->GetCameraRotation().Vector().GetSafeNormal();
	FVector TraceStart;
	if (PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, TraceStart, CameraDirection))
	{
		FVector TraceEnd = TraceStart + CameraDirection * HALF_WORLD_MAX;
		float FloorZ = + GametimeLevel->GetAsset()->TileSizeZ * FloorPosition + 50;
		FVector InterceptLocation = FMath::LinePlaneIntersection(TraceStart, TraceEnd, FVector(0, 0, FloorZ), FVector(0, 0, 1));
		FoundPosition = GetTilePosition(InterceptLocation, Found);
	}
	return FoundPosition;
}

FTiledLevelEdge UTiledLevelGametimeSystem::GetEdge(FVector WorldLocation, EEdgeType EdgeType, bool& Found)
{
	FTiledLevelEdge FoundEdge = FTiledLevelEdge(-9999, -9999, -9999, EdgeType);
	Found = false;
	if (!GametimeLevel) return FoundEdge;
	if (GametimeMode == EGametimeMode::BoundToExistingLevels)
	{
		bool HasAnyInside = false;
		for (FBox TestBoundary: GametimeData.Boundaries)
		{
			TestBoundary.Min -= TileSize*0.5;
			if (EdgeType == EEdgeType::Horizontal)
				 TestBoundary.Max += TileSize * FVector(0, 0.5, 0.5);
			else
				 TestBoundary.Max += TileSize * FVector(0.5, 0, 0.5);
			if (TestBoundary.IsInside(WorldLocation))
			{
				HasAnyInside = true;
				break;
			}
		}
		if (!HasAnyInside)
			return FoundEdge;
	}
	Found = true;
	const int X_Mod = WorldLocation.X < 0? -1 : 0;
	const int Y_Mod = WorldLocation.Y < 0? -1 : 0;
	const int Z_Mod = WorldLocation.Z < 0? -1 : 0;
	// shift 0.5 tile size as detection zone, should modify in either X or Y based on edge type...
	if (EdgeType == EEdgeType::Horizontal)
		FoundEdge = FTiledLevelEdge(WorldLocation.X / TileSize.X + X_Mod, (WorldLocation.Y + 0.5 * TileSize.Y) / TileSize.Y + Y_Mod, WorldLocation.Z / TileSize.Z + Z_Mod, EEdgeType::Horizontal);
	else
		FoundEdge = FTiledLevelEdge((WorldLocation.X + 0.5 * TileSize.X) / TileSize.X + X_Mod, WorldLocation.Y / TileSize.Y + Y_Mod, WorldLocation.Z / TileSize.Z + Z_Mod, EEdgeType::Vertical);
	return FoundEdge;
}

FTiledLevelEdge UTiledLevelGametimeSystem::GetEdgeOnScreen(int FloorPosition, FVector2D ScreenPosition, EEdgeType EdgeType,
	bool& Found, int PlayerIndex)
{
	Found = false;
	FTiledLevelEdge FoundEdge = {-9999, -9999, -9999, EdgeType};
	APlayerCameraManager* PlayerCameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), PlayerIndex);
	FVector CameraDirection = PlayerCameraManager->GetCameraRotation().Vector().GetSafeNormal();
	FVector TraceStart;
	if (PlayerController->DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, TraceStart, CameraDirection))
	{
		FVector TraceEnd = TraceStart + CameraDirection * HALF_WORLD_MAX;
		float FloorZ = GametimeLevel->GetAsset()->TileSizeZ * FloorPosition + 50;
		FVector InterceptLocation = FMath::LinePlaneIntersection(TraceStart, TraceEnd, FVector(0, 0, FloorZ), FVector(0, 0, 1));
		FoundEdge = GetEdge(InterceptLocation, EdgeType, Found);
	}
	return FoundEdge;

}

FTiledLevelEdge UTiledLevelGametimeSystem::GetEdgeUnderCursor(int FloorPosition, EEdgeType EdgeType, bool& Found, int PlayerIndex)
{
	Found = false;
	FTiledLevelEdge FoundEdge = {-9999, -9999, -9999, EdgeType};
	APlayerCameraManager* PlayerCameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), PlayerIndex);
	float MouseX;
	float MouseY;
	PlayerController->GetMousePosition(MouseX, MouseY);
	FVector CameraDirection = PlayerCameraManager->GetCameraRotation().Vector().GetSafeNormal();
	FVector TraceStart;
	if (PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, TraceStart, CameraDirection))
	{
		FVector TraceEnd = TraceStart + CameraDirection * HALF_WORLD_MAX;
		float FloorZ = GametimeLevel->GetAsset()->TileSizeZ * FloorPosition + 50;
		FVector InterceptLocation = FMath::LinePlaneIntersection(TraceStart, TraceEnd, FVector(0, 0, FloorZ), FVector(0, 0, 1));
		FoundEdge = GetEdge(InterceptLocation, EdgeType,Found);
	}
	return FoundEdge;
}

FIntVector UTiledLevelGametimeSystem::GetPoint(FVector WorldLocation, bool& Found)
{
	FIntVector FoundPosition = FIntVector(-9999);
	Found = false;
	if (!GametimeLevel) return FoundPosition;
	if (GametimeMode == EGametimeMode::BoundToExistingLevels)
	{
		bool HasAnyInside = false;
		for (FBox B: GametimeData.Boundaries)
		{
			B.Min -= TileSize*0.5;
			B.Max += TileSize*0.5;
			if (B.IsInside(WorldLocation))
			{
				HasAnyInside = true;
				break;
			}
		}
		if (!HasAnyInside)
			return FoundPosition;
	}
	Found = true;
	const int X_Mod = WorldLocation.X < 0? -1 : 0;
	const int Y_Mod = WorldLocation.Y < 0? -1 : 0;
	const int Z_Mod = WorldLocation.Z < 0? -1 : 0;
	FoundPosition = FIntVector((WorldLocation.X + 0.5 * TileSize.X) / TileSize.X + X_Mod, (WorldLocation.Y + 0.5 * TileSize.Y) / TileSize.Y + Y_Mod,WorldLocation.Z / TileSize.Z + Z_Mod);
	return FoundPosition;
}

FIntVector UTiledLevelGametimeSystem::GetPointOnScreen(int FloorPosition, FVector2D ScreenPosition, bool& Found, int PlayerIndex)
{
	Found = false;
	FIntVector FoundPoint = FIntVector(-9999);
	APlayerCameraManager* PlayerCameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), PlayerIndex);
	FVector CameraDirection = PlayerCameraManager->GetCameraRotation().Vector().GetSafeNormal();
	FVector TraceStart;
	if (PlayerController->DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, TraceStart, CameraDirection))
	{
		FVector TraceEnd = TraceStart + CameraDirection * HALF_WORLD_MAX;
		float FloorZ = GametimeLevel->GetAsset()->TileSizeZ * FloorPosition + 50;
		FVector InterceptLocation = FMath::LinePlaneIntersection(TraceStart, TraceEnd, FVector(0, 0, FloorZ), FVector(0, 0, 1));
		FoundPoint = GetPoint(InterceptLocation, Found);
	}
	return FoundPoint;
}

FIntVector UTiledLevelGametimeSystem::GetPointUnderCursor(int FloorPosition, bool& Found, int PlayerIndex)
{
	Found = false;
	FIntVector FoundPoint = FIntVector(-9999);
	APlayerCameraManager* PlayerCameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), PlayerIndex);
	float MouseX;
	float MouseY;
	PlayerController->GetMousePosition(MouseX, MouseY);
	FVector CameraDirection = PlayerCameraManager->GetCameraRotation().Vector().GetSafeNormal();
	FVector TraceStart;
	if (PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, TraceStart, CameraDirection))
	{
		FVector TraceEnd = TraceStart + CameraDirection * HALF_WORLD_MAX;
		float FloorZ = GametimeLevel->GetAsset()->TileSizeZ * FloorPosition + 50;
		FVector InterceptLocation = FMath::LinePlaneIntersection(TraceStart, TraceEnd, FVector(0, 0, FloorZ), FVector(0, 0, 1));
		FoundPoint = GetPoint(InterceptLocation, Found);
	}
	return FoundPoint;
}

void UTiledLevelGametimeSystem::MovePreviewItem(FIntVector NewLocation, bool IgnoreSamePosition)
{
	if (IgnoreSamePosition && CurrentTilePosition == NewLocation) return;
	if (NewLocation == FIntVector(-9999))
	{
		OnLeaveSystemBoundary();
		return;
	}
	if (!IsPreviewItemActivated()) return;
	CurrentTilePosition = NewLocation;
	Helper->MoveBrush(CurrentTilePosition, false);
	CanBuildHere = CanBuildItem(ActiveItem) && HasEnoughSpaceToBuild();
	Helper->UpdatePreviewHint(CanBuildHere);
}

void UTiledLevelGametimeSystem::MoveEdgePreviewItem(FTiledLevelEdge NewEdge, bool IgnoreSamePosition)
{
	if (IgnoreSamePosition && CurrentEdge == NewEdge) return;
	if (NewEdge.GetEdgePosition() == FIntVector(-9999))
	{
		OnLeaveSystemBoundary();
		return;
	}
	if (!IsPreviewItemActivated()) return;
	CurrentEdge = NewEdge;
	Helper->MoveBrush(CurrentEdge, false);
	CanBuildHere = CanBuildItem(ActiveItem) && HasEnoughSpaceToBuild();
	Helper->UpdatePreviewHint(CanBuildHere);
}

bool UTiledLevelGametimeSystem::HasEnoughSpaceToBuild()
{
	if (!IsPreviewItemActivated()) return false;
	
	TArray<ATiledLevelRestrictionHelper*> Restrictions;
	for (AActor* A : GametimeLevel->SpawnedTiledActors)
	{
		if (ATiledLevelRestrictionHelper* Arh = Cast<ATiledLevelRestrictionHelper>(A))
		{
			Restrictions.Add(Arh);
		}
	}
	TArray<FIntVector> PointsToCheck;
	if (ActiveItem->PlacedType == EPlacedType::Edge ||ActiveItem->PlacedType == EPlacedType::Wall)
		PointsToCheck = FTiledLevelUtility::GetOccupiedPositions(ActiveItem, CurrentEdge);
	else
		PointsToCheck = FTiledLevelUtility::GetOccupiedPositions(ActiveItem, CurrentTilePosition, ShouldRotatePreviewBrush);
	
	if (bLockBuild)
	{
		bool bPassBlockBuild = false;
		// check if it is explicitly allowed building here 
		for (ATiledLevelRestrictionHelper* Arh : Restrictions)
		{
			if (Arh->GetSourceItem()->RestrictionType == ERestrictionType::AllowBuilding || Arh->GetSourceItem()->RestrictionType == ERestrictionType::AllowBuildingAndRemoving)
			{
				if (Arh->GetSourceItem()->bTargetAllItems || Arh->GetSourceItem()->TargetItems.Contains(ActiveItem->ItemID))
				{
					if (Arh->IsInsideTargetedTilePositions(PointsToCheck))
					{
						bPassBlockBuild = true;
						break;
					}
				}
			}
		}
		if (!bPassBlockBuild)
			return false;
	}
	else
	{
		// check if it is explicitly freeze building here
		for (ATiledLevelRestrictionHelper* Arh : Restrictions)
		{
			if (Arh->GetSourceItem()->RestrictionType == ERestrictionType::DisallowBuilding || Arh->GetSourceItem()->RestrictionType == ERestrictionType::DisallowBuildingAndRemoving)
			{
				if (Arh->GetSourceItem()->bTargetAllItems || Arh->GetSourceItem()->TargetItems.Contains(ActiveItem->ItemID))
					if (Arh->IsInsideTargetedTilePositions(PointsToCheck))
						return false;
			}
		}
	}
	
	EPlacedShapeType ActiveShape = FTiledLevelUtility::PlacedTypeToShape(ActiveItem->PlacedType);
	switch (ActiveShape)
	{
	case Shape3D:
		{
			FTilePlacement TestPlacement;
			TestPlacement.GridPosition = CurrentTilePosition;
			const FIntVector ItemExtent = FIntVector(ActiveItem->Extent);
			TestPlacement.Extent = ShouldRotatePreviewBrush? FIntVector(ItemExtent.Y, ItemExtent.X, ItemExtent.Z) : ItemExtent;
			TArray<FTilePlacement> OverlappingPlacements;
			if (ActiveItem->PlacedType == EPlacedType::Block)
			{
				OverlappingPlacements = GametimeData.BlockPlacements.FilterByPredicate([=](const FTilePlacement& Block)
				{
					 // exclude special helpers from including as overlapped...
					 if (Cast<UTiledLevelRestrictionItem>(Block.GetItem())) return false;
					 return FTiledLevelUtility::IsTilePlacementOverlapping(TestPlacement, Block);
				});
			}
			else
			{
				OverlappingPlacements = GametimeData.FloorPlacements.FilterByPredicate([=](const FTilePlacement& Floor)
				{
					 return FTiledLevelUtility::IsTilePlacementOverlapping(TestPlacement, Floor);
				});
			}
			if (OverlappingPlacements.Num() == 0)
				return true;
			if (ActiveItem->StructureType == ETLStructureType::Structure)
				return false;
			for (FTilePlacement P : OverlappingPlacements)
			{
				if (P.GetItem() == ActiveItem)
					 return false;
			}
			return true;
		}
	case Shape2D:
		{
			FEdgePlacement TestPlacement;
			TestPlacement.Edge = CurrentEdge;
			TestPlacement.ItemSet = SourceItemSet;
			TestPlacement.ItemID = ActiveItem->ItemID;
			TArray<FEdgePlacement> OverlappingPlacements;
			if (ActiveItem->PlacedType == EPlacedType::Wall)
			{
				OverlappingPlacements = GametimeData.WallPlacements.FilterByPredicate([=](const FEdgePlacement& Wall)
				{
						 return FTiledLevelUtility::IsEdgePlacementOverlapping(TestPlacement, Wall);
				});
			}
			else
			{
				OverlappingPlacements = GametimeData.EdgePlacements.FilterByPredicate([=](const FEdgePlacement& Edge)
				{
						 return FTiledLevelUtility::IsEdgePlacementOverlapping(TestPlacement, Edge);
				});
			}
			if (OverlappingPlacements.Num() == 0)
				return true;
			if (ActiveItem->StructureType == ETLStructureType::Structure)
				return false;
			for (FEdgePlacement P : OverlappingPlacements)
			{
				if (P.GetItem() == ActiveItem)
					return false;
			}
			return true;
		}
	case Shape1D:
		{
			FPointPlacement TestPlacement;
			TestPlacement.GridPosition = CurrentTilePosition;
			TArray<FPointPlacement> OverlappingPlacements;
			if (ActiveItem->PlacedType == EPlacedType::Pillar)
			{
			   OverlappingPlacements = GametimeData.PillarPlacements.FilterByPredicate([=](const FPointPlacement& Pillar)
			   {
			   		return FTiledLevelUtility::IsPointPlacementOverlapping(TestPlacement, ActiveItem->Extent.Z, Pillar, Pillar.GetItem()->Extent.Z );
			   });
			}
			else
			{
			   OverlappingPlacements = GametimeData.PointPlacements.FilterByPredicate([=](const FPointPlacement& Point)
			   {
			   		return FTiledLevelUtility::IsPointPlacementOverlapping(TestPlacement, ActiveItem->Extent.Z, Point, Point.GetItem()->Extent.Z);
			   });
			}
			if (OverlappingPlacements.Num() == 0)
				return true;
			if (ActiveItem->StructureType == ETLStructureType::Structure)
				return false;
			for (FPointPlacement P : OverlappingPlacements)
			{
				if (P.GetItem() == ActiveItem)
					return false;
			}
			return true;
		}
	default:
		return false;
	}
}

bool UTiledLevelGametimeSystem::IsRemoveRestricted(UTiledLevelItem* TestItem, FVector HitPosition)
{
	TArray<ATiledLevelRestrictionHelper*> Restrictions;
 	for (AActor* A : GametimeLevel->SpawnedTiledActors)
 	{
 		if (ATiledLevelRestrictionHelper* Arh = Cast<ATiledLevelRestrictionHelper>(A))
 		{
 			Restrictions.Add(Arh);
 		}
 	}
	const int X_mod = HitPosition.X < 0? -1 : 0;
	const int Y_mod = HitPosition.Y < 0? -1 : 0;
	const int Z_mod = HitPosition.Z < 0? -1 : 0;
	const FIntVector PointToCheck = FIntVector(HitPosition / TileSize) + FIntVector(X_mod, Y_mod, Z_mod);
 	TArray<FIntVector> PointsToCheck = { PointToCheck };
 	if (bLockRemove)
 	{
 		// check if it is explicitly allowed removing here 
 		for (ATiledLevelRestrictionHelper* Arh : Restrictions)
 		{
 			if (Arh->GetSourceItem()->RestrictionType == ERestrictionType::AllowRemoving || Arh->GetSourceItem()->RestrictionType == ERestrictionType::AllowBuildingAndRemoving)
 			{
 				if (Arh->GetSourceItem()->bTargetAllItems || Arh->GetSourceItem()->TargetItems.Contains(TestItem->ItemID))
 				{
					 if (Arh->IsInsideTargetedTilePositions(PointsToCheck))
					 {
						   return false;
					 }
 				}
 			}
 		}
 		return true;
 	}
 	// check if it is explicitly freeze removing here
 	for (ATiledLevelRestrictionHelper* Arh : Restrictions)
 	{
 		if (Arh->GetSourceItem()->RestrictionType == ERestrictionType::DisallowRemoving || Arh->GetSourceItem()->RestrictionType == ERestrictionType::DisallowBuildingAndRemoving)
 		{
 			if (Arh->IsInsideTargetedTilePositions(PointsToCheck))
 			{
 				return true;
 			}
 		}
 	}
	return false;
}

FVector UTiledLevelGametimeSystem::GetBuildLocation()
{
	if (!IsPreviewItemActivated()) return FVector(-1);
	EPlacedType ActivePlacedType = ActiveItem->PlacedType;
	if (ActivePlacedType == EPlacedType::Block || ActivePlacedType == EPlacedType::Floor)
	{
		FVector PlacedExtent = ShouldRotatePreviewBrush? FVector(ActiveItem->Extent.Y, ActiveItem->Extent.X, ActiveItem->Extent.Z) : ActiveItem->Extent;
		return  TileSize * (FVector(CurrentTilePosition) + FVector(0.5, 0.5, 0) * PlacedExtent);
	}
	if (ActivePlacedType == EPlacedType::Edge || ActivePlacedType == EPlacedType::Wall)
	{
		if (CurrentEdge.EdgeType == EEdgeType::Horizontal)
		{
			return  TileSize * FVector(CurrentEdge.X + 0.5 * ActiveItem->Extent.X, CurrentEdge.Y, CurrentEdge.Z);
		}
		return  TileSize * FVector(CurrentEdge.X, CurrentEdge.Y + 0.5 * ActiveItem->Extent.Y, CurrentEdge.Z);
	}
	// point or pillar:
	return  FVector(CurrentTilePosition) * TileSize;
}

FVector UTiledLevelGametimeSystem::GetBuildLocation(EPlacedShapeType Shape, FVector InTilePosition, FVector InTileExtent)
{
	switch (Shape)
	{
	case Shape3D:
		return TileSize * InTilePosition  + TileSize * InTileExtent * FVector(0.5, 0.5, 0);
	case Shape2D:
		if (InTileExtent.Z < -0.5f) // horizontal edge
			return  TileSize * FVector(InTilePosition.X + 0.5 * InTileExtent.X, InTilePosition.Y, InTilePosition.Z );
	    return  TileSize * FVector(InTilePosition.X, InTilePosition.Y + 0.5 * InTileExtent.X, InTilePosition.Z );
	case Shape1D:
		return TileSize * InTilePosition;
	}
	return FVector(-9999);
}

UWorld* UTiledLevelGametimeSystem::GetWorld() const
{
	if (!Helper) return nullptr;
	return Helper->GetWorld();
}

