// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "TiledLevelTypes.h"

class TILEDLEVEL_API FTiledItemSetDetailCustomization : public IDetailCustomization
{
public:
	FTiledItemSetDetailCustomization();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	static TSharedRef<IDetailCustomization> MakeInstance();

private:
	TSharedRef<SWidget> CreatePivotPositionWidget(EPlacedType TargetPlacedType);
	TSharedPtr<IPropertyHandle> Property_IsFloorIncluded;
	TSharedPtr<IPropertyHandle> Property_IsWallIncluded;
	TSharedPtr<IPropertyHandle> Property_IsPillarIncluded;
	TSharedPtr<IPropertyHandle> Property_IsEdgeIncluded;
	TSharedPtr<IPropertyHandle> Property_DefaultBlockPivotPosition;
	TSharedPtr<IPropertyHandle> Property_DefaultFloorPivotPosition;
	TSharedPtr<IPropertyHandle> Property_DefaultWallPivotPosition;
	TSharedPtr<IPropertyHandle> Property_DefaultPillarPivotPosition;
	TSharedPtr<IPropertyHandle> Property_DefaultEdgePivotPosition;
	TArray<TSharedPtr<FString>> BlockOptions;
	TArray<TSharedPtr<FString>> FloorOptions;
	TArray<TSharedPtr<FString>> WallOptions;
	TArray<TSharedPtr<FString>> PillarOptions;
	TArray<TSharedPtr<FString>> EdgeOptions;
	
};
