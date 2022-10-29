// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"


/*
 * fixed items info:
 * TMap
 */


class TILEDLEVEL_API FTiledItemSetEditor : public FAssetEditorToolkit, public FGCObject
{
	
public:
	FTiledItemSetEditor();
	~FTiledItemSetEditor();
	
	// IToolkit interface
    virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	// End of IToolkit interface
	
	// FAssetEditorToolkit
    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
    virtual FLinearColor GetWorldCentricTabColorScale() const override;
    virtual FString GetWorldCentricTabPrefix() const override;
	// End of FAssetToolkit

	// GC interface
    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return "FTiledItemSetEditor"; }
	// end of GC interface

	void InitTiledItemSetEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, class UTiledItemSet* InItemSet);
	UTiledItemSet* GetTiledItemSetBeingEdited() const { return TiledItemSetBeingEdited; }
	void UpdatePreviewScene(class UTiledLevelItem* TargetItem);
	void RefreshPalette();
	
	TMap<UTiledLevelItem*, TArray<FIntPoint>> FixedItemsInfo;
private:
	UTiledItemSet* TiledItemSetBeingEdited;
	TSharedPtr<class STiledItemSetEditorViewport> ViewportPtr;
	TSharedPtr<class STiledPalette> PalettePtr;
	UTiledLevelItem* ActiveItem;

	void OnPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);
	FDelegateHandle OnPropertyChangedHandle;
	
	void BindCommands();
	void ExtendMenu();
	void ExtendToolbar();
	
	TSharedRef<SWidget> GeneratePositionButton(int BtnIndex);
	TSharedRef<SWidget> CreateFixedItemWidget();
	FButtonStyle UnfixedButton;
	FButtonStyle FixedButton;
	FButtonStyle DisabledButton;
	TArray<TSharedPtr<SButton>> PositionButtons;
	int FixedFloor = 0;
	float GetFixedFloor() const { return FixedFloor; }
	void SetFixedFloor(float NewFloor);
	void SetupButtonState();
	void FixActiveItem(FIntPoint PreviewPosition);
	void UnfixActiveItem(FIntPoint PreviewPosition);
	void UpdateFixedItemsStartPoint(FVector SurroundExtent);
	
	/*
	 * (f, p) f: floor, could only be -1, 0, 1
	 * p: position as below
	 * 012
	 * 345
	 * 678
	 */
	FVector ExtractGridPosition(FIntPoint Point);
	void ClearAllFixedItem();

	TSharedRef<SDockTab> SpawnTab_Palette(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Preview(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_ContentBrowser(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_PreviewSettings(const FSpawnTabArgs& Args);

	AActor* PreviewSceneActor;
	class UTiledLevelGrid* PreviewCenter;
	UStaticMeshComponent* PreviewMesh;
	class UTiledLevelGrid* PreviewBrush;
	AActor* PreviewItemActor;
	TArray<AActor*> SpawnedFixedActors;
	void UpdatePreviewMesh(UTiledLevelItem* Item);
};
