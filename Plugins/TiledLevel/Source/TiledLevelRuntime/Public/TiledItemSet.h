// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TiledLevelTypes.h"
#include "TiledItemSet.generated.h"

/**
 * 
 */
DECLARE_MULTICAST_DELEGATE(FItemSetPostUndo);
DECLARE_DELEGATE(FCleanUpPlacements)

class UStaticMesh;
class UTiledLevelItem;

UCLASS(BlueprintType)
class TILEDLEVELRUNTIME_API UTiledItemSet : public UObject
{
	GENERATED_BODY()
	
public:

	void AddNewItem(UStaticMesh* TiledMesh, const EPlacedType& PlacedType, const ETLStructureType& StructureType, const FVector& Extent);
	void AddNewItem(UObject* NewActorObject, const EPlacedType& PlacedType, const ETLStructureType& StructureType, const FVector& Extent);
	void AddSpecialItem_Restriction();
	void AddSpecialItem_Template();
    
	void RemoveItem(UTiledLevelItem* ItemPtr);

	UFUNCTION(BlueprintPure, BlueprintPure, Category="TiledItemSet | Info")
	UTiledLevelItem* GetItem(const FGuid& ItemID) const;
	
	UFUNCTION(BlueprintPure, BlueprintPure, Category="TiledItemSet | Info")
	TArray<UTiledLevelItem*> GetItemSet() const { return ItemSet;}
	
	TSet<UStaticMesh*> GetAllItemMeshes() const;
	TSet<UObject*> GetAllItemBlueprintObject() const;

	UFUNCTION(BlueprintPure, BlueprintPure, Category="TiledItemSet | Info")
	FVector GetTileSize() const { return FVector(TileSizeX, TileSizeY, TileSizeZ) ;}

	FItemSetPostUndo ItemSetPostUndo;
	FCleanUpPlacements CleanUpPlacements;

	UPROPERTY()
	int32 VersionNumber = 0;
	
	UPROPERTY(EditAnywhere, Category="Setup", meta=(UIMin = 1, ClampMin = 1, ClampMax=2048))
	float TileSizeX = 100.f;
	
	UPROPERTY(EditAnywhere, Category="Setup", meta=(UIMin = 1, ClampMin = 1, ClampMax=2048))
	float TileSizeY = 100.f;
	
	UPROPERTY(EditAnywhere, Category="Setup", meta=(UIMin = 1, ClampMin = 1, ClampMax=2048))
	float TileSizeZ = 100.f;

	// The threshold for how extent size prediction, Ex: threshold: 0.5, tile size: 100x100x100 and add a new item with mesh size of 151x149x100 will return 2x1x1 
	UPROPERTY(EditAnywhere, Category="NewItem", meta=(UIMin = 0, ClampMin=0, ClampMax=1))
	float AutoExtentPredictionThreshold = 0.5;
	
	UPROPERTY(EditAnywhere, Category="NewItem")
	ETLStructureType DefaultStructureType = ETLStructureType::Structure;
	
	UPROPERTY(EditAnywhere, Category="NewItem")
	EPivotPosition DefaultBlockPivotPosition = EPivotPosition::Bottom;
	
	UPROPERTY(EditAnywhere, Category="NewItem")
	bool IsFloorIncluded = false;
	
	UPROPERTY(EditAnywhere, Category="NewItem")
	EPivotPosition DefaultFloorPivotPosition = EPivotPosition::Bottom;
	
	UPROPERTY(EditAnywhere, Category="NewItem")
	bool IsWallIncluded = false;
	
	UPROPERTY(EditAnywhere, Category="NewItem")
	EPivotPosition DefaultWallPivotPosition = EPivotPosition::Bottom;

	UPROPERTY(EditAnywhere, Category="NewItem")
	bool IsEdgeIncluded = false;
	
	UPROPERTY(EditAnywhere, Category="NewItem")
	EPivotPosition DefaultEdgePivotPosition = EPivotPosition::Center;
	
	UPROPERTY(EditAnywhere, Category="NewItem")
	bool IsPillarIncluded = false;
	
	UPROPERTY(EditAnywhere, Category="NewItem")
	EPivotPosition DefaultPillarPivotPosition = EPivotPosition::Bottom;
	
	UPROPERTY(EditAnywhere, Category="NewItem")
	bool IsPointIncluded = false;
	
	UPROPERTY(EditAnywhere, Category="NewItem")
	bool DefaultAutoPlacement = false;

	UPROPERTY(EditAnywhere, Category="CustomData")
	class UDataTable* CustomDataTable;


	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	
#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category="CustomData")
	void InitializeData();
	virtual void PostEditUndo() override;
#endif

private:

	UPROPERTY()
	TArray<UTiledLevelItem*> ItemSet;

	EPivotPosition GetDefaultPivotPosition(EPlacedType TargetPlacedType);
};
