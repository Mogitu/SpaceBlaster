// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelAsset.h"
#include "TiledLevelTypes.h"
#include "UObject/Object.h"
#include "TiledLevelItem.generated.h"

DECLARE_DELEGATE(FRequestForRefreshPalette)
DECLARE_DELEGATE(FUpdatePreviewMesh)
DECLARE_DELEGATE(FUpdatePreviewActor)

UCLASS(BlueprintType)
class TILEDLEVELRUNTIME_API UTiledLevelItem : public UObject
{
	GENERATED_UCLASS_BODY()

	// Use this ID to query this item in gametime	
	UPROPERTY(VisibleAnywhere, Category="TiledItem")
	FGuid ItemID = FGuid::NewGuid();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="TiledItem")
	const FName GetID();

	/*
	 * Different placed type items will never interfere with each other.
	 * A way to make you place items in a more reasonable manner.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TiledItem")
	EPlacedType PlacedType = EPlacedType::Block;

	/*
	 * Determine the behavior of replace or overlap during item placement
	 * Struct item can not overlap, thus replace previous ones
	 * Prop item can overlap everywhere.
	 * PS: no overlap between same items, it will replace previous one.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TiledItem")
	ETLStructureType StructureType = ETLStructureType::Structure;

	// how much does this item occupy the tile space?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TiledItem")
	FVector Extent{1, 1, 1};

	// Both Actor or StaticMesh types are supported!
	UPROPERTY(BlueprintReadOnly, Category="TiledItem")
	ETLSourceType SourceType = ETLSourceType::Mesh;

	// Source mesh for Mesh type item
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh", meta = (NoResetToDefault))
	class UStaticMesh* TiledMesh; // make reset not possible

	// Material override for tiled item
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh")
	TArray<class UMaterialInterface*> OverrideMaterials; 

	// Source actor for Actor type item
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Actor", meta = (NoResetToDefault))
	UObject* TiledActor;
	
	// Automatically setup mesh origin and scale to meet tile item boundary as possible
	UPROPERTY(EditAnywhere, Category="Placement")
	bool bAutoPlacement = false;

	// Set mesh origin to different position
	UPROPERTY(EditAnywhere, Category="Placement")
	EPivotPosition PivotPosition = EPivotPosition::Bottom;

	// should override height? (floor only)
	UPROPERTY(EditAnywhere, DisplayName="Should Override Height?", Category="Placement")
	bool bHeightOverride = false;

	// New overriden height 
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bHeightOverride", UIMin = 10, ClampMin = 10, ClampMax=500), Category="Placement") // min max value limited by tile size???
	float NewHeight = 10;

	// should override thickness (wall only)
	UPROPERTY(EditAnywhere, DisplayName="Should Override Thinkness?", Category="Placement")
	bool bThicknessOverride = false;

	// New overriden thickness
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bThicknessOverride", UIMin = 10, ClampMin = 10, ClampMax=500), Category="Placement")
	float NewThickness = 10;

	// Should auto snap to floor?
	UPROPERTY(EditAnywhere, Category="Placement")
	bool bSnapToFloor = false;
	
	// Should auto snap to wall if exists (Only for wall props)
	UPROPERTY(EditAnywhere, Category="Placement")
	bool bSnapToWall = false;

	// final transformation adjustment
	UPROPERTY(EditAnywhere, Category="Placement")
	FTransform TransformAdjustment;

	UPROPERTY()
	bool bIsEraserAllowed = true;

	// fill tool vars
	UPROPERTY()
	float FillCoefficient = 1.0f;

	UPROPERTY()
	bool bAllowRandomRotation = false;
	
	UPROPERTY()
	bool bAllowOverlay = false;

	virtual FString GetItemName() const;
	virtual FString GetItemNameWithInfo() const;

	bool IsShape2D() const
	{
		return PlacedType == EPlacedType::Edge || PlacedType == EPlacedType::Wall;
	}
	
	bool operator==(const UTiledLevelItem& Other) const { return ItemID == Other.ItemID; }

	FRequestForRefreshPalette RequestForRefreshPalette;
	FUpdatePreviewMesh UpdatePreviewMesh;
	FUpdatePreviewActor UpdatePreviewActor;

	TArray<class UTiledLevelAsset*> AssetsUsingThisItem;

	// hide some properties when not in set editor...
	bool IsEditInSet = false;
	
	
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY()
	UObject* PreviousTiledActorObject = nullptr;
	
};

DECLARE_DELEGATE(FUpdatePreviewRestriction)

UCLASS()
class TILEDLEVELRUNTIME_API UTiledLevelRestrictionItem : public UTiledLevelItem
{
	GENERATED_UCLASS_BODY()
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RestrictionItem|Visual")
	FLinearColor BorderColor = {0.8f, 0.2f, 0.2f, 0.2f};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RestrictionItem|Visual", meta=(ClampMax="0.5", ClampMin="0.0", UIMax="0.5", UIMin="0.0"))
	float BorderWidth = 0.5f;

	// Set false when you want to see the visuals in game time
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RestrictionItem|Debug")
	bool bHiddenInGame = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RestrictionItem|Rule")
	ERestrictionType RestrictionType = ERestrictionType::DisallowBuilding;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RestrictionItem|Rule")
	bool bTargetAllItems = true;

	UPROPERTY(BlueprintReadOnly, Category="RestrictionItem|Rule")
	TArray<FGuid> TargetItems;

	FString GetItemName() const override;
	FString GetItemNameWithInfo() const override;
	
	FUpdatePreviewRestriction UpdatePreviewRestriction;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
};


USTRUCT(BlueprintType)
struct FTemplateAsset
{
    GENERATED_USTRUCT_BODY()
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Asset")
	UObject* TemplateAsset = nullptr;
};

// The template item that can reuse existing tiled levels...
UCLASS(BlueprintType)
class TILEDLEVELRUNTIME_API UTiledLevelTemplateItem : public UTiledLevelItem
{
	GENERATED_UCLASS_BODY()

    // must have the same tile size
    UPROPERTY(EditAnywhere, Category="TemplateItem")
    FTemplateAsset TemplateAsset;

    UTiledLevelAsset* GetAsset() const { return Cast<UTiledLevelAsset>(TemplateAsset.TemplateAsset); }

    FString GetItemName() const override;
    FString GetItemNameWithInfo() const override;

};