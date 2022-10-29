// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "TiledLevelFactory.generated.h"

UCLASS()
class TILEDLEVEL_API UTiledLevelFactory : public UFactory
{
	GENERATED_BODY()

public:
	UTiledLevelFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
		UObject* Context, FFeedbackContext* Warn) override;

	// Init item set, can be nullptr
	UPROPERTY()
	class UTiledItemSet* InitialItemSet;
};
