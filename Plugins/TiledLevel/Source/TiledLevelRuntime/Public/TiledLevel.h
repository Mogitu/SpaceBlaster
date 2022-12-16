// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelAsset.h"
#include "TiledLevelUtility.h"
#include "GameFramework/Actor.h"
#include "TiledLevel.generated.h"

DECLARE_DELEGATE(FPreSaveTiledLevelActor)

class UHierarchicalInstancedStaticMeshComponent;
struct FTilePlacement;
struct FTiledLevelGameData;

// TODO: can users inherit this actor? 

UCLASS(BlueprintType, NotBlueprintable)
class TILEDLEVELRUNTIME_API ATiledLevel : public AActor
{
	GENERATED_BODY()

public:

	// Sets default values for this actor's properties
	ATiledLevel();

	// TODO: leave all replication features to next update...
	// void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

#if WITH_EDITOR
	// TODO: leave all replication features to next update...
	// virtual void PostDuplicate(bool bDuplicateForPIE) override;
	
	virtual void PostInitProperties() override;

	void OnPostWorldInitialization(UWorld* World, const UWorld::InitializationValues IVS);
	
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;

#endif	
	virtual void Destroyed() override;
	
	UPROPERTY()
	class USceneComponent* Root;

	// TODO: HISM, ISM do not replicate... they just don't replicate... need other way around to implement it...
	// TODO: with this setup, there is no way to handle same SMptr in different ItemSet!?
	UPROPERTY()
	TMap<UStaticMesh* , UHierarchicalInstancedStaticMeshComponent*> TiledObjectSpawner;

	UPROPERTY()
	TArray<AActor*> SpawnedTiledActors;

	void SetActiveAsset(UTiledLevelAsset* NewAsset) { ActiveAsset = NewAsset; }
	UTiledLevelAsset* GetAsset() const { return ActiveAsset; }
	void RemoveAsset();
	template <typename T>
	void PopulateSinglePlacement(const T& Placement);
	// Never use clear instance
	void ClearItemInstances(const UStaticMesh* MeshPtr);
	template <typename T>
	void RemovePlacements(const TArray<T>& PlacementsToDelete);
	void RemoveInstances(const TMap<UStaticMesh*, TArray<int32>>& TargetInstancesData);
	void DestroyTiledActorByPlacement(const FTilePlacement& Placement);
	void DestroyTiledActorByPlacement(const FEdgePlacement& Placement);
	void DestroyTiledActorByPlacement(const FPointPlacement& Placement);
	void EraseItem(FIntVector Pos, FIntVector Extent, bool bIsFloor = false, bool Both = false);
	void EraseItem(FTiledLevelEdge Edge, FIntVector Extent, bool bIsEdge = false, bool Both = false);
	void EraseItem(FIntVector Pos, int ZExtent, bool bIsPoint = false, bool Both = false);
	void EraseItem_Any(FIntVector Pos, FIntVector Extent);
	// for quick eraser that only focus on specific item
	void EraseSingleItem(FIntVector Pos, FIntVector Extent, FGuid TargetID);
	void EraseSingleItem(FTiledLevelEdge Edge, FIntVector Extent, FGuid TargetID);
	void EraseSingleItem(FIntVector Pos, int ZExtent, FGuid TargetID);

	void ResetAllInstance(bool IgnoreVersion = false);
	void ResetAllInstanceFromData();
	// UFUNCTION(Server, Reliable)
	// void SetGametimeData(const FTiledLevelGameData& NewGametimeData) { GametimeData = NewGametimeData; }
	
	void MakeEditable();

	// TODO: leave all replication features to next update...
	// UPROPERTY(ReplicatedUsing=OnRep_GametimeData)
	UPROPERTY()
	FTiledLevelGameData GametimeData;

	UFUNCTION()
	void OnRep_GametimeData();

#if WITH_EDITOR	
	// break into separate static mesh and actors, but organized properly
	UFUNCTION()
	void Break();
#endif

	// For game time supports
	FBox GetBoundaryBox();
	FTiledLevelGameData MakeGametimeData();
	
	UPROPERTY()
	bool IsInstance;
	
	bool IsInTiledLevelEditor = false;

	FPreSaveTiledLevelActor PreSaveTiledLevelActor;
	
	uint32 VersionNumber;

private:

	UPROPERTY()
	UTiledLevelAsset* ActiveAsset = nullptr;
	
	TArray<UTiledLevelItem*> GetEraserActiveItems() const;
	void CreateNewHISM(UStaticMesh* MeshPtr, const TArray<class UMaterialInterface*>& OverrideMaterials);
	AActor* SpawnActorPlacement(const FItemPlacement& ItemPlacement);
	AActor* SpawnMirroredPlacement(const FItemPlacement& ItemPlacement);
#if WITH_EDITOR
	FDelegateHandle OnPostWorldInitializationDelegateHandle;
#endif

	// To fix ai nav issue, make it static mesh when begin play, and revert back when end play
	
};


// template implementations...

/*
 * For actor item: use tags to store position, extent, and guid. WARNING: this may block users to use the last 3 tags...
 * For mesh item: Use custom data to store position and extent information
 */
template <typename T>
void ATiledLevel::PopulateSinglePlacement(const T& Placement)
{
	if (Placement.GetItem()->SourceType == ETLSourceType::Actor)
	{
		if (!IsValid(Placement.GetItem()->TiledActor)) return;
		if (AActor* NewActor = SpawnActorPlacement(Placement))
		{
			FTiledLevelUtility::SetSpawnedActorTag(Placement, NewActor);
			SpawnedTiledActors.Add(NewActor);
		}
	}
	else if (Placement.IsMirrored)
	{
		AActor* NewMirrored = SpawnMirroredPlacement(Placement);
		FTiledLevelUtility::SetSpawnedActorTag(Placement, NewMirrored);
		SpawnedTiledActors.Add(NewMirrored);
	} else
	{
		if (!Placement.GetItem()->TiledMesh) return;
		
		CreateNewHISM(Placement.GetItem()->TiledMesh, Placement.GetItem()->OverrideMaterials);
		const int InstanceIndex = TiledObjectSpawner[Placement.GetItem()->TiledMesh]->AddInstance(Placement.TileObjectTransform);
		const TArray<float> InstanceData = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(Placement);
		TiledObjectSpawner[Placement.GetItem()->TiledMesh]->SetCustomData(InstanceIndex, InstanceData);
	}
}

template <typename T>
void ATiledLevel::RemovePlacements(const TArray<T>& PlacementsToDelete)
{
	TMap<UStaticMesh*, TArray<int32>> TargetInstanceData;
	for (auto P : PlacementsToDelete)
	{
		if (P.GetItem()->SourceType == ETLSourceType::Actor || P.IsMirrored)
		{
			DestroyTiledActorByPlacement(P);
		}
		else
		{
			if (!TargetInstanceData.Contains(P.GetItem()->TiledMesh))
				TargetInstanceData.Add(P.GetItem()->TiledMesh, TArray<int32>{});
			TArray<float> TargetInfo = FTiledLevelUtility::ConvertPlacementToHISM_CustomData(P);
			FTiledLevelUtility::FindInstanceIndexByPlacement(TargetInstanceData[P.GetItem()->TiledMesh],TiledObjectSpawner[P.GetItem()->TiledMesh]->PerInstanceSMCustomData, TargetInfo);
		}
	}
	ActiveAsset->RemovePlacements(PlacementsToDelete);
	RemoveInstances(TargetInstanceData);
}

