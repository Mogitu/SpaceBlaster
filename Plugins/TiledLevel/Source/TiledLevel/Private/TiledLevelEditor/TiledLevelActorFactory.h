// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "ActorFactories/ActorFactory.h"
#include "TiledLevelActorFactory.generated.h"

UCLASS()
class TILEDLEVEL_API UTiledLevelActorFactory : public UActorFactory
{
    GENERATED_UCLASS_BODY()
	
    virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
    virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	
};
