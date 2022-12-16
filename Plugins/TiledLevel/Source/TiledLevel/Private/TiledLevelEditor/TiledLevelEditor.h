// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"


class TILEDLEVEL_API FTiledLevelEditor : public FAssetEditorToolkit, public FGCObject
{
public:
	FTiledLevelEditor();
	~FTiledLevelEditor();
	
	// IToolkit
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	//

	// AssetEditorToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void OnToolkitHostingStarted(const TSharedRef<IToolkit>& Toolkit) override;
	virtual void OnToolkitHostingFinished(const TSharedRef<IToolkit>& Toolkit) override;
	//

	// GC
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return "FTiledLevelEditor"; }
	//
	
	void InitTileLevelEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, class UTiledLevelAsset* InTiledLevel);
	UTiledLevelAsset* GetTiledLevelBeingEdited() const { return TiledLevelBeingEdited; }

	// TODO: Very dirty fix for toolbox not appear in newest ue5...
	/*
	 * OnToolkitHostingStarted was not triggered, so I just expose this public and set its content outside....
	 */
	TSharedPtr<class SBorder> ToolboxPtr;

	
protected:
	virtual void SaveAsset_Execute() override;
	
private:
	UTiledLevelAsset* TiledLevelBeingEdited;
	TSharedPtr<class STiledLevelEditorViewport> ViewportPtr;
	
	void OnPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);
	FDelegateHandle OnPropertyChangedHandle;
	
	void BindCommands();
	void ExtendMenu();
	void ExtendToolbar();

	TSharedRef<SDockTab> SpawnTab_Toolbox(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_PreviewSettings(const FSpawnTabArgs& Args);

	void UpdateLevels();
	bool CanUpdateLevels() const;
	void MergeToStaticMesh();
};
