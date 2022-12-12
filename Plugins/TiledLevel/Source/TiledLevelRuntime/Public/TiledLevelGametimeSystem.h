// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TiledLevelGametimeSystem.generated.h"

class ATiledLevel;
class UTiledLevelItem;
class UMaterialInterface;

enum EGametimeMode
{
	Uninitialized,
	BoundToExistingLevels,
	Infinite
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemBuilt, UTiledLevelItem*, BuiltItem, FVector, BuiltWorldPosition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FItemRotate, UTiledLevelItem*, BuiltItem, FVector, BuiltWorldPosition, bool, IsClockwise);
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class TILEDLEVELRUNTIME_API UTiledLevelGametimeSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// Override
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	UWorld* GetWorld() const override;
	// end of override

	// called when start a game
	UFUNCTION(BlueprintNativeEvent, Category="TiledLevelGametimeSystem | Init")
	void OnBeginSystem();

	// allows you to override implementation in c++
	virtual void OnBeginSystem_Implementation();
	
	/*
	 * Initialize the gametime system,
	 * Must provide valid world and a Tiled Item Set with same tile size
	 * Can include existing tiled levels by applying required configuration, copy their data to GamatimeData, then delete them
	 * The functionality of this system will bound to the extent of each existing tiled Levels if unbound is false,
	 * otherwise, gametime system will work on everywhere.
	 */
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Init", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm="ExistingTiledLevels"))
	void InitializeGametimeSystem(const UObject* WorldContextObject, UTiledItemSet* StartupItemSet, const TArray<class ATiledLevel*>& ExistingTiledLevels, bool bUnbound = false);

	/*
	 * Initialize the gametime system through existing SAVE data
	 * if some of the save data is from existing levels, you should include them in "TiledLevelsInsideData"
	 */
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Init", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm="TiledLevelsInsideData"))
	void InitializeGametimeSystemFromData(const UObject* WorldContextObject, UTiledItemSet* StartupItemSet,
		const FTiledLevelGameData& InData, TArray<class ATiledLevel*> TiledLevelsInsideData, bool bUnbound = false);

	// fail to change if the tile size is not the same
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Init" )
	bool ChangeItemSet(UTiledItemSet* NewItemSet);

	// the tile size set here, the client s are now allowed to change item set as the wish??
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="TiledLevelGametimeSystem | Init")
	FVector TileSize;
	
	// display preview item with preview materials
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Preview")
	void ActivatePreviewItem(UTiledLevelItem* PreviewItem);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="TiledLevelGamtimeSystem | Preview")
	bool IsPreviewItemActivated() const { return ActiveItem != nullptr && GametimeMode != Uninitialized; }
	
	// deactivate preview item
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Preview")
	void DeactivatePreviewItem();
	
	// try to move preview item based on world position (such as the forward position of your character?)
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Preview")
	bool MovePreviewItemToWorldPosition(FVector TargetLocation);
	
	// try to move preview item to screen position
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Preview")
	bool MovePreviewItemToScreenPosition(FVector2D ScreenPosition, int FloorPosition, int PlayerIndex = 0);
	
	// try to move preview item to cursor
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Preview")
	bool MovePreviewItemToCursor(int FloorPosition, int PlayerIndex = 0);

	// rotate preview item
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Preview")
	void RotatePreviewItem(bool IsClockwise = true);

	
	/*
	 * called when leave system boundary, by default, it will deactivate preview item and deactivate eraser mode...
	 */
	UFUNCTION(BlueprintNativeEvent, Category="TiledLevelGameSystem | Preview")
	void OnLeaveSystemBoundary();

	virtual void OnLeaveSystemBoundary_Implementation();
	
	/*
	 * Final check for if you can build specific item now.
	 * Override this function to meet your gameplay need.
	 * ex: add logic to check if required resource is enough
	 */ 
	UFUNCTION(BlueprintNativeEvent, Category="TiledLevelGametimeSystem | Build")
	bool CanBuildItem(UTiledLevelItem* ItemToBuild);

	// allows you to override implementation in c++
	virtual bool CanBuildItem_Implementation(UTiledLevelItem* ItemToBuild);

	/*
	 * Will build this item by current transformation
	 * there is no 'replacement' in game support, if you want to build structural item on top of another structure, you must destroy previous one
	*/
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Build")
	void BuildItem();

	/*
	 * Use single line trace to detect and remove the first hit tiled item,
	 * return false if no item or can remove item is false
	 */
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Remove")
	bool RemoveItem_LineTraceSingle(FVector TraceStart, FVector TraceEnd, bool bShowDebug = false);

	// Remove item from HitResult, you should use it after any RayCast
	// Return false if nothing is removed
	// If all you need is line trace, using RemoveItem_LineTraceSingle is enough
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Remove")
	bool RemoveItem_FromHit(const FHitResult& HitResult);
	
	// Remove item under cursor 
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Remove")
	bool RemoveItem_UnderCursor(int32 PlayerIndex = 0);
	
	// maybe some resource is required to remove something?	
	UFUNCTION(BlueprintNativeEvent, Category="TiledLevelGametimeSystem | Remove")
	bool CanRemoveItem(UTiledLevelItem* ItemToRemove);

	// allows you to override implementation in c++
	virtual bool CanRemoveItem_Implementation(UTiledLevelItem* ItemToRemove);
	
	// TODO: how to set custom eraser mesh? instead of box in helper?
	// Activate eraser mode in runtime
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Remove")
	void ActivateEraserMode(bool bAnyType, EPlacedType TargetType, FIntVector EraserSize);

	// Move eraser to world position if inside target level
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Remove")
	bool MoveEraserToWorldPosition(FVector TargetLocation);

	// Move eraser to screen position if inside target level
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Remove")
	bool MoveEraserToScreenPosition( FVector2D ScreenPosition, int FloorPosition );

	//  Move eraser to cursor if inside target level
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Remove")
	bool MoveEraserToCursor(int FloorPosition);

	// Rotate the eraser
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Remove")
	void RotateEraser(bool bIsClockwise = true);

	// Perform erase for ANY tiled items inside eraser (ignore all the block from eraser setup for now) 
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Remove")
	void EraseItem();

	// deactivate the eraser mode
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Remove")
	void DeactivateEraserMode();

	// Get item to use from UID
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="TiledLevelGametimeSystem | Utility")
	UTiledLevelItem* QueryItemByUID(FName ItemID);

	// Null if not activate yet
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="TiledLevelGametimeSystem | Utility")
	UTiledLevelItem* GetActivePreviewItem() const { return ActiveItem; }

	// Hides all placements in other floors, make you focus on this floor only
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Utility")
	void FocusFloor(int FloorPosition);

	// Unhide all placements
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Utility")
	void UnfocusFloor();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="TiledLevelGametimeSystem | Utility")
	bool HasAnyFocusedFloor();

	///////////////////////// V2.2
	// save as asset, and the player can use it later... for which it is still in a dynamic manner
	UFUNCTION(BlueprintCallable, Category="TiledLevelGametimeSystem | Conversion")
	bool SaveAsTiledLevelAsset(FString TargetFile);
	
	////////////////////////
	
	// Called when successfully build something, maybe you want to spawn particles, play sound effect, and calculate resource cost?
	UPROPERTY(BlueprintAssignable, Category="TiledLevelGametimeSystem | Event")
	FItemBuilt OnItemBuilt;
	
	// Called when successfully remove something, maybe you want to spawn particles, play sound effect, and spawn some actor (pickup)?
	UPROPERTY(BlueprintAssignable, Category="TiledLevelGametimeSystem | Event")
	FItemBuilt OnItemRemoved;
	
	// Called when fail to build something, maybe you want to spawn particles and play sound effect?
	UPROPERTY(BlueprintAssignable, Category="TiledLevelGametimeSystem | Event")
	FItemBuilt OnItemFailToBuilt;
	
	// Called when fail to remove something, maybe you can display something that the conditions is not met yet to remove it!
	UPROPERTY(BlueprintAssignable, Category="TiledLevelGametimeSystem | Event")
	FItemBuilt OnItemFailToRemove;
	
	// Called when successfully build something, maybe you want to spawn particles, play sound effect, and process resource cost?
	UPROPERTY(BlueprintAssignable, Category="TiledLevelGametimeSystem | Event")
	FItemRotate OnItemRotate;
	
	// if false, will display the original material for preview item, and hide preview item if cant not build.
	UPROPERTY(EditDefaultsOnly, Category="TiledLevelGametimeSystem | Preview")
	bool ShouldUsePreviewMaterial = true;

	// if not set, will use the default material this plugin provides
	UPROPERTY(EditDefaultsOnly, Category="TiledLevelGametimeSystem | Preview", meta=(EditCondition="ShouldUsePreviewMaterial"))
	UMaterialInterface* PreviewMaterial_CanBuildHere;
	
	// if not set, will use the default material this plugin provides
	UPROPERTY(EditDefaultsOnly, Category="TiledLevelGametimeSystem | Preview", meta=(EditCondition="ShouldUsePreviewMaterial"))
	UMaterialInterface* PreviewMaterial_CanNotBuildHere;

	// Where gametime data stored
	UPROPERTY(BlueprintReadWrite, Category="TiledLevelGametimeSystem | Data")
	FTiledLevelGameData GametimeData;

	// Lock build everywhere unless that position is explicitly allowed
	UPROPERTY(EditDefaultsOnly, Category="TiledLevelGametimeSystem | Restriction")
	bool bLockBuild = false;

	// Lock remove everywhere unless that position is explicitly allowed
	UPROPERTY(EditDefaultsOnly, Category="TiledLevelGametimeSystem | Restriction")
	bool bLockRemove = false;


private:
	FIntVector GetTilePosition(FVector WorldLocation, bool& Found);
	FIntVector GetTilePositionOnScreen(int FloorPosition, FVector2D ScreenPosition, bool& Found, int PlayerIndex = 0);
	FIntVector GetTilePositionUnderCursor(int FloorPosition, bool& Found, int PlayerIndex = 0);
	FTiledLevelEdge GetEdge(FVector WorldLocation, EEdgeType EdgeType, bool& Found);
	FTiledLevelEdge GetEdgeOnScreen(int FloorPosition, FVector2D ScreenPosition, EEdgeType EdgeType, bool& Found, int PlayerIndex = 0);
	FTiledLevelEdge GetEdgeUnderCursor(int FloorPosition, EEdgeType EdgeType, bool& Found, int PlayerIndex = 0);
	FIntVector GetPoint(FVector WorldLocation, bool& Found);
	FIntVector GetPointOnScreen(int FloorPosition, FVector2D ScreenPosition, bool& Found, int PlayerIndex = 0);
	FIntVector GetPointUnderCursor(int FloorPosition, bool& Found, int PlayerIndex = 0);
	void MovePreviewItem(FIntVector NewLocation, bool IgnoreSamePosition = true);
	void MoveEdgePreviewItem(FTiledLevelEdge NewEdge, bool IgnoreSameEdge = true);
	bool HasEnoughSpaceToBuild(); // Check has enough place to build active item to current position and restriction rules
	bool IsRemoveRestricted(UTiledLevelItem* TestItem, FVector HitPosition);
	FVector GetBuildLocation(); // return grid bottom center...
	FVector GetBuildLocation(EPlacedShapeType Shape, FVector InTilePosition, FVector InTileExtent);
	
	UPROPERTY()
	class UTiledItemSet* SourceItemSet = nullptr;
	
	UPROPERTY()
	ATiledLevel* GametimeLevel = nullptr;

	UPROPERTY()
	class ATiledLevelEditorHelper* Helper = nullptr;

	UPROPERTY()
	UTiledLevelItem* ActiveItem = nullptr;

	EGametimeMode GametimeMode = EGametimeMode::Uninitialized;
	bool ShouldRotatePreviewBrush = false;
	FIntVector CurrentTilePosition;
	FTiledLevelEdge CurrentEdge;
	bool CanBuildHere = false;
	EPlacedType EraserType;
	bool IsEraserMode = false;
	FIntVector EraserExtent;

	// struct FTimerHandle ClientInitTimer;
	// UFUNCTION()
	// void InitClient();
};

