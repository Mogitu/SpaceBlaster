// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FTiledLevelModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	TSharedRef<class FTiledLevelEditor> CreateTiledLevelEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost> InitToolkitHost, class UTiledLevelAsset* TiledLevel);
private:
	TArray<TSharedRef<class IAssetTypeActions>> CreatedAssetTypeActions;
	void AddLevelViewportMenuExtender();
	void RemoveLevelViewportMenuExtender();
	FDelegateHandle LevelViewportExtenderHandle;
	void BreakTiledLevel(class ATiledLevel* TargetTiledLevel);
	void MergeTiledLevel(class ATiledLevel* TargetTiledLevel);
	void MergeTiledLevelAndReplace(class ATiledLevel* TargetTiledLevel);
	void RevertStaticTiledLevel(class AStaticTiledLevel* TargetStaticTiledLevel);
	
};
