// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "TiledLevelPromotionFactory.generated.h"

UCLASS()
class TILEDLEVEL_API UTiledLevelPromotionFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
	
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
		UObject* Context, FFeedbackContext* Warn) override;
	
	UPROPERTY()
	class UTiledLevelAsset* AssetToRename = nullptr;
	
};
