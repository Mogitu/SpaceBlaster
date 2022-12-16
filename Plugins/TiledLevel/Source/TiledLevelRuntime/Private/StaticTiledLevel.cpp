// Copyright 2022 PufStudio. All Rights Reserved.

#include "StaticTiledLevel.h"
#include "TiledLevel.h"
#include "Engine/World.h"


AStaticTiledLevel::AStaticTiledLevel()
{
	PrimaryActorTick.bCanEverTick = false; 
}

void AStaticTiledLevel::RevertToTiledLevel()
{
	if (SourceAsset)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.bNoFail = true;
		ATiledLevel* NewLevel = GetWorld()->SpawnActor<ATiledLevel>(SpawnParameters);
		// there is no modification of origin during static mesh conversion... there should be no need to handle it...
		NewLevel->SetActorTransform(GetActorTransform());
		NewLevel->SetActiveAsset(SourceAsset);
		NewLevel->ResetAllInstance(true);
		Destroy();
	}
}

