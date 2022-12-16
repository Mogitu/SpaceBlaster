// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledItemSetFactory.h"
#include "TiledItemSet.h"

UTiledItemSetFactory::UTiledItemSetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UTiledItemSet::StaticClass();
}

UObject* UTiledItemSetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	UTiledItemSet* NewTiledItemSet = NewObject<UTiledItemSet>(InParent, InClass, InName, Flags | RF_Transactional);
	return NewTiledItemSet;
}
