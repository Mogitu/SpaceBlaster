// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelPromotionFactory.h"
#include "TiledLevelAsset.h"

UTiledLevelPromotionFactory::UTiledLevelPromotionFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = false;
	bEditAfterNew = true;
	SupportedClass = UTiledLevelAsset::StaticClass();
}

UObject* UTiledLevelPromotionFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	AssetToRename->SetFlags(Flags | RF_Transactional);
	AssetToRename->Modify();
	AssetToRename->Rename(*InName.ToString(), InParent);
	return AssetToRename;
}
