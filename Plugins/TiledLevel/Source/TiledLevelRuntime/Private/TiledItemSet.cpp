// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledItemSet.h"
#include "TiledLevelItem.h"
#include "TiledLevelAsset.h"
#include "TiledLevelRestrictionHelper.h"
#include "TiledLevelSelectHelper.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"

#define LOCTEXT_NAMESPACE "TiledLevel"

void UTiledItemSet::AddNewItem(UStaticMesh* TiledMesh, const EPlacedType& PlacedType, const ETLStructureType& StructureType,
                               const FVector& Extent)
{
	// No transaction for each add new item! (only one transaction for a batch of these instructions)
	UTiledLevelItem* NewItem = NewObject<UTiledLevelItem>(this, NAME_None, RF_Transactional);
	NewItem->ItemID = FGuid::NewGuid();
	NewItem->SourceType = ETLSourceType::Mesh;
	NewItem->TiledMesh = TiledMesh;
	NewItem->PlacedType = PlacedType;
	NewItem->StructureType = StructureType;
	NewItem->Extent = Extent;
	NewItem->bAutoPlacement = DefaultAutoPlacement;
	NewItem->PivotPosition = GetDefaultPivotPosition(PlacedType);
	ItemSet.Add(NewItem);
}

void UTiledItemSet::AddNewItem(UObject* NewActorObject, const EPlacedType& PlacedType, const ETLStructureType& StructureType,
	const FVector& Extent)
{
	UTiledLevelItem* NewItem = NewObject<UTiledLevelItem>(this, NAME_None, RF_Transactional);
	NewItem->ItemID = FGuid::NewGuid();
	NewItem->SourceType = ETLSourceType::Actor;
	NewItem->TiledActor = NewActorObject;
	NewItem->PlacedType = PlacedType;
	NewItem->StructureType = StructureType;
	NewItem->Extent = Extent;
	NewItem->bAutoPlacement = DefaultAutoPlacement;
	NewItem->PivotPosition = GetDefaultPivotPosition(PlacedType);
	ItemSet.Add(NewItem);
}

void UTiledItemSet::AddSpecialItem_Restriction()
{
	UTiledLevelRestrictionItem* NewItem = NewObject<UTiledLevelRestrictionItem>(this, NAME_None, RF_Transactional);
	NewItem->ItemID = FGuid::NewGuid();
	NewItem->SourceType = ETLSourceType::Actor;
	NewItem->TiledActor = ATiledLevelRestrictionHelper::StaticClass()->GetDefaultObject();
	NewItem->PlacedType = EPlacedType::Block;
	NewItem->StructureType = ETLStructureType::Prop;
	NewItem->Extent = FVector(1);
	NewItem->bAutoPlacement = false;
	NewItem->PivotPosition = EPivotPosition::Center;
	ItemSet.Add(NewItem);
}

void UTiledItemSet::AddSpecialItem_Template()
{
	UTiledLevelTemplateItem* NewItem = NewObject<UTiledLevelTemplateItem>(this, NAME_None, RF_Transactional);
    NewItem->SourceType = ETLSourceType::Actor;
	NewItem->TiledActor = ATiledLevelSelectHelper::StaticClass()->GetDefaultObject();
	NewItem->PlacedType = EPlacedType::Block;
	NewItem->StructureType = ETLStructureType::Structure;
	NewItem->Extent = FVector(1);
	NewItem->bAutoPlacement = false;
	NewItem->PivotPosition = EPivotPosition::Corner;
	ItemSet.Add(NewItem);
}

void UTiledItemSet::RemoveItem(UTiledLevelItem* ItemPtr)
{
	for (UTiledLevelAsset* Asset : ItemPtr->AssetsUsingThisItem)
	{
		Asset->Modify();
		Asset->ClearItem(ItemPtr->ItemID);
		Asset->VersionNumber += 100;
	}
	ItemSet.Remove(ItemPtr);
}

UTiledLevelItem* UTiledItemSet::GetItem(const FGuid& ItemID) const
{
	for (UTiledLevelItem* I : ItemSet)
	{
		if (I)
			if (I->ItemID == ItemID) return I;
	}
	return nullptr;
}

TSet<UStaticMesh*> UTiledItemSet::GetAllItemMeshes() const
{
	TSet<UStaticMesh*> Out;
	for (UTiledLevelItem* Item : ItemSet)
	{
		if (Item->SourceType == ETLSourceType::Mesh) Out.Add(Item->TiledMesh);
	}
	return Out;
}

TSet<UObject*> UTiledItemSet::GetAllItemBlueprintObject() const
{
	TSet<UObject*> Out;
	for (UTiledLevelItem* Item : ItemSet)
	{
		if (Item->SourceType == ETLSourceType::Actor) Out.Add(Item->TiledActor);
	}
	return Out;
}



void UTiledItemSet::GetAssetRegistryTags(TArray<FAssetRegistryTag>& AssetRegistryTags) const
{
	const FString TileSizeStr = FString::Printf(TEXT("%dx%dx%d"), FMath::RoundToInt(TileSizeX), FMath::RoundToInt(TileSizeY), FMath::RoundToInt(TileSizeZ));
	AssetRegistryTags.Add( FAssetRegistryTag("Tile Size", TileSizeStr, FAssetRegistryTag::TT_Dimensional) );
	AssetRegistryTags.Add(FAssetRegistryTag("Items",FString::FromInt(ItemSet.Num()), FAssetRegistryTag::TT_Numerical));
	UObject::GetAssetRegistryTags(AssetRegistryTags);
}

#if WITH_EDITOR

void UTiledItemSet::InitializeData()
{
	if (CustomDataTable)
	{
		FString CSV_String = CustomDataTable->GetTableAsCSV();
		const UScriptStruct* US = CustomDataTable->GetRowStruct();
		int NumOfProperties = 0;
		for(TFieldIterator<FProperty> PropertyIt(US); PropertyIt; ++PropertyIt)
		{
			NumOfProperties += 1;
		}
		CSV_String.Append("\n");
		for (UTiledLevelItem* Item : ItemSet)
		{
		    if (Item->IsA<UTiledLevelTemplateItem>() || Item->IsA<UTiledLevelRestrictionItem>())
		        continue;
			if (CustomDataTable->FindRowUnchecked(FName(Item->ItemID.ToString())))
				continue;
			CSV_String.Append(Item->ItemID.ToString());
			for (int i = 0; i < NumOfProperties; i++)
			{
				CSV_String.Append(",");
				CSV_String.Append("\"\"");
			}
			CSV_String.Append("\n");
		}
		CustomDataTable->CreateTableFromCSVString(CSV_String);
		CustomDataTable->MarkPackageDirty();
	}
}

void UTiledItemSet::PostEditUndo()
{
	ItemSetPostUndo.Broadcast();
	UObject::PostEditUndo();
}

#endif

EPivotPosition UTiledItemSet::GetDefaultPivotPosition(EPlacedType TargetPlacedType)
{
	switch (TargetPlacedType) {
		case EPlacedType::Block:
			return DefaultBlockPivotPosition;
		case EPlacedType::Floor:
			return DefaultFloorPivotPosition;
		case EPlacedType::Wall:
			return DefaultWallPivotPosition;
		case EPlacedType::Edge:
			return DefaultEdgePivotPosition;
		case EPlacedType::Pillar:
			return DefaultPillarPivotPosition;
		case EPlacedType::Point:
			return EPivotPosition::Center;
		default: ;
	}
	return EPivotPosition::Bottom;
}

#undef LOCTEXT_NAMESPACE