// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "TiledItemSetFactory.generated.h"

UCLASS()
class TILEDLEVEL_API UTiledItemSetFactory : public UFactory
{
    GENERATED_BODY()
	
public:
	UTiledItemSetFactory();
    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	    UObject* Context, FFeedbackContext* Warn) override;
};
