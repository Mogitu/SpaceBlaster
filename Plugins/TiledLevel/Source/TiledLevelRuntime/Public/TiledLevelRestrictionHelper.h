// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TiledLevelRestrictionHelper.generated.h"

UCLASS(NotPlaceable, NotBlueprintable)
class TILEDLEVELRUNTIME_API ATiledLevelRestrictionHelper : public AActor
{
	GENERATED_BODY()

	ATiledLevelRestrictionHelper();
	
public:
	UPROPERTY()
	FGuid SourceItemID;

	UPROPERTY()
	class UTiledItemSet* SourceItemSet; 
	
	class UTiledLevelRestrictionItem* GetSourceItem() const;
	
	void UpdatePreviewVisual();

	bool IsInsideTargetedTilePositions(const TArray<FIntVector>& PositionsToCheck);

	UPROPERTY()
	TArray<FIntVector> TargetedTilePositions;
	
	void AddTargetedPositions(FIntVector NewPoint);
	void RemoveTargetedPositions(FIntVector PointToRemove);

protected:
	UPROPERTY()
	class USceneComponent* Root;
	
	UPROPERTY()
	class UProceduralMeshComponent* AreaHint;
	
	virtual void OnConstruction(const FTransform& Transform) override;
	
private:
	
	void UpdateVisual();

	UPROPERTY()
	class UMaterialInstanceDynamic* M_AreaHint; 
	
};
