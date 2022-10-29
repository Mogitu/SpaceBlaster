// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "TiledLevelGrid.generated.h"

UCLASS()
class TILEDLEVELRUNTIME_API UTiledLevelGrid : public UBoxComponent
{
	GENERATED_BODY()

public:
	void SetLineThickness(float NewThickness)
	{
		LineThickness = NewThickness;
	}
	
	void SetBoxColor(FColor NewColor)
	{
		ShapeColor = NewColor;
	}

};
