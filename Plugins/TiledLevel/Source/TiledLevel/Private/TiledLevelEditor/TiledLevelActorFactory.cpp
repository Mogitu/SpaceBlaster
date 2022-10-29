// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelActorFactory.h"
#include "TiledItemSet.h"
#include "TiledLevel.h"
#include "TiledLevelAsset.h"
#include "TiledLevelEditorLog.h"

UTiledLevelActorFactory::UTiledLevelActorFactory(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	DisplayName = NSLOCTEXT("TiledLevel", "TiledLevelFactoryDisplayName", "Tiled Level");
	NewActorClass = ATiledLevel::StaticClass();
}

bool UTiledLevelActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (AssetData.IsValid())
	{
		UClass* AssetClass = AssetData.GetClass();
		if (AssetClass != nullptr &&
			(AssetClass->IsChildOf(UTiledLevelAsset::StaticClass()) || AssetClass->IsChildOf(UTiledItemSet::StaticClass())))
		{
			return true;
		}
		OutErrorMsg = NSLOCTEXT("TiledLevel", "CanCreateActorFrom_NoTiledLevel", "No tiled level was specified.");
	}
	return false;
}

void UTiledLevelActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	ATiledLevel* NewLevel = Cast<ATiledLevel>(NewActor);
	if (UTiledLevelAsset* NewLevelAsset = Cast<UTiledLevelAsset>(Asset))
	{
		NewLevel->SetActiveAsset(NewLevelAsset);
		NewLevelAsset->HostLevel = NewLevel;
		
	}
	else
	{
		UTiledLevelAsset* CreatedLevelAsset = NewObject<UTiledLevelAsset>(NewLevel, UTiledLevelAsset::StaticClass(), NAME_None ,RF_Transactional);
		UTiledItemSet* ReferenceItemSet = Cast<UTiledItemSet>(Asset);
		CreatedLevelAsset->SetActiveItemSet(ReferenceItemSet);
		CreatedLevelAsset->SetTileSize(ReferenceItemSet->GetTileSize());
		CreatedLevelAsset->AddNewFloor(0);
		CreatedLevelAsset->ConfirmTileSize();
		NewLevel->SetActiveAsset(CreatedLevelAsset);
		NewLevel->IsInstance = true;
	}
	NewLevel->ResetAllInstance();
}

