// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TiledLevelAsset.h"
#include "Widgets/SCompoundWidget.h"

class TILEDLEVEL_API STiledFloor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STiledFloor) {}
		SLATE_EVENT(FSimpleDelegate, OnResetInstances)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, int32 InPosition, class UTiledLevelAsset* InLevel, FIsSelected InIsSelectedDelegate);
	
	int32 MyPosition = 0;
protected:
	UTiledLevelAsset* MyLevel = nullptr;
	TSharedPtr<SButton> VisibilityButton;
	const FSlateBrush* EyeClosed = nullptr;
	const FSlateBrush* EyeOpened = nullptr;
	TSharedPtr<SInlineEditableTextBlock> LayerNameWidget;
	
	FTiledFloor* GetMyFloor() const { return MyLevel->GetFloorFromPosition(MyPosition); }
	const FSlateBrush* GetVisibilityBrushForLayer() const;
	FSlateColor GetForegroundColorForVisibilityButton() const;
	FReply OnToggleVisibility();
	FSimpleDelegate OnResetInstances;
	
};
