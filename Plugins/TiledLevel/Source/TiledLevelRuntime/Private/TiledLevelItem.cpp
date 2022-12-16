// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelItem.h"
#include "Engine/StaticMesh.h"
#include "TiledLevelEditorLog.h"

UTiledLevelItem::UTiledLevelItem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

const FName UTiledLevelItem::GetID()
{
	return FName(ItemID.ToString());
}

FString UTiledLevelItem::GetItemName() const
{
	if (SourceType == ETLSourceType::Actor)
	{
		if (TiledActor != NULL)
			return TiledActor->GetName();
	}
	if (TiledMesh != nullptr)
		return TiledMesh->GetName();
	return TEXT("");
}

FString UTiledLevelItem::GetItemNameWithInfo() const
{
	FString P;
	switch (PlacedType) {
		case EPlacedType::Block:
			P = TEXT("Block");
			break;
		case EPlacedType::Floor:
			P = TEXT("Floor");
			break;
		case EPlacedType::Wall:
			P = TEXT("Wall");
			break;
		case EPlacedType::Edge:
			P = TEXT("Edge");
			break;
		case EPlacedType::Pillar:
			P = TEXT("Pillar");
			break;
		case EPlacedType::Point:
			P = TEXT("Point");
			break;
		default: ;
	}
	FString St = StructureType == ETLStructureType::Structure ? TEXT("Structure") : TEXT("Prop");
	FString So = SourceType == ETLSourceType::Mesh ? TEXT("Mesh") : TEXT("Actor");
	return FString::Printf(TEXT("%s (%s-%s-%s)"), *GetItemName(), *P, *St, *So);
}

#if WITH_EDITOR
void UTiledLevelItem::PreEditChange(FProperty* PropertyAboutToChange)
{
	UObject::PreEditChange(PropertyAboutToChange);
	if (PropertyAboutToChange->GetFName() == GET_MEMBER_NAME_CHECKED(UTiledLevelItem, TiledActor))
		PreviousTiledActorObject = TiledActor;
}

void UTiledLevelItem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);
	TArray<FName> CorePropertyNames = {
		GET_MEMBER_NAME_CHECKED(UTiledLevelItem, PlacedType),
		GET_MEMBER_NAME_CHECKED(UTiledLevelItem, StructureType),
		GET_MEMBER_NAME_CHECKED(UTiledLevelItem, Extent)
	};

	if (CorePropertyNames.Contains(PropertyChangedEvent.Property->GetFName()))
	{
		RequestForRefreshPalette.ExecuteIfBound();
		if (UTiledItemSet* ItemSet = Cast<UTiledItemSet>(GetOuter()))
		{
			ItemSet->VersionNumber += 1;
		}
	}
	
	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTiledLevelItem, PlacedType))
	{
		const float MaxXY = FMath::Max(Extent.X, Extent.Y);
		switch (PlacedType)
		{
		case EPlacedType::Floor:
			Extent.Z = 1;
			break;
		case EPlacedType::Wall:
			Extent.X = MaxXY;
			Extent.Y = MaxXY;
			break;
		case EPlacedType::Edge:
			Extent.X = MaxXY;
			Extent.Y = MaxXY;
			Extent.Z = 1;
			break;
		case EPlacedType::Pillar:
			Extent.X = 1;
			Extent.Y = 1;
			break;
		case EPlacedType::Point:
			Extent = FVector(1);
		default: ;
		}
	}
	if (PlacedType != EPlacedType::Wall || StructureType != ETLStructureType::Prop)
		bSnapToWall = false;
	
	// forcefully reset actor object if it's just irrelevant
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UTiledLevelItem, TiledActor))
	{
		if (UBlueprint* BP = Cast<UBlueprint>(TiledActor))
		{
			if (
				!BP->ParentClass->IsChildOf(AActor::StaticClass()) ||
				BP->ParentClass->IsChildOf(AController::StaticClass()) ||
				BP->ParentClass->IsChildOf(AInfo::StaticClass())
				)
			{
				TiledActor = PreviousTiledActorObject;
				DEV_LOG("Reset to previous object since picked object is relevant...")
			}
		}
	}
	
	if (IsEditInSet)
	{
		UpdatePreviewMesh.ExecuteIfBound();
		UpdatePreviewActor.ExecuteIfBound();
	}
}

#endif

UTiledLevelRestrictionItem::UTiledLevelRestrictionItem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FString UTiledLevelRestrictionItem::GetItemName() const
{
	return TEXT("Restriction Item");
}

FString UTiledLevelRestrictionItem::GetItemNameWithInfo() const
{
	return TEXT("Restriction Item (Special item)");
}

#if WITH_EDITOR

void UTiledLevelRestrictionItem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// Super::PostEditChangeProperty(PropertyChangedEvent);
	if (IsEditInSet)
	{
		UpdatePreviewRestriction.ExecuteIfBound();
	}
	
}

#endif


UTiledLevelTemplateItem::UTiledLevelTemplateItem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FString UTiledLevelTemplateItem::GetItemName() const
{
    return TEXT("Template Item");
}

FString UTiledLevelTemplateItem::GetItemNameWithInfo() const
{
    if (GetAsset())
        return FString::Printf(TEXT("Template Item (%s)"), *GetAsset()->GetName());
    return GetItemName();
}

