// Copyright 2022 PufStudio. All Rights Reserved.

#include "STiledFloor.h"
#include "EditorModeManager.h"
#include "TiledLevelStyle.h"
#include "STiledFloorList.h"
#include "TiledLevelAsset.h"
#include "TiledLevelEditorLog.h"

#define LOCTEXT_NAMESPACE "TiledLevel"

void STiledFloor::Construct(const FArguments& InArgs, int32 InPosition, class UTiledLevelAsset* InLevel , FIsSelected InIsSelectedDelegate)
{
	
	MyLevel = InLevel;
	MyPosition = InPosition;
	static const FName EyeClosedBrushName("TiledLevel.EyeClosed");
	static const FName EyeOpenedBrushName("TiledLevel.EyeOpened");
	EyeClosed = FTiledLevelStyle::Get().GetBrush(EyeClosedBrushName);
	EyeOpened = FTiledLevelStyle::Get().GetBrush(EyeOpenedBrushName);

	OnResetInstances = InArgs._OnResetInstances;

	ChildSlot
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SAssignNew( VisibilityButton, SButton )
			.ContentPadding(FMargin(4.0f, 4.0f, 4.0f, 4.0f))
			.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
			.OnClicked( this, &STiledFloor::OnToggleVisibility )
			.ToolTipText( LOCTEXT("FloorVisibilityButtonToolTip", "Toggle Floor Visibility") )
			.ForegroundColor( FSlateColor::UseForeground() )
			.HAlign( HAlign_Center )
			.VAlign( VAlign_Center )
			.Content()
			[
				SNew(SImage)
				.Image(this, &STiledFloor::GetVisibilityBrushForLayer)
				.ColorAndOpacity(this, &STiledFloor::GetForegroundColorForVisibilityButton)
			]
		]
		+SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.Padding(FMargin(4.0f, 4.0f, 4.0f, 4.0f))
		[
			SNew(STextBlock).Text(FText::FromName(GetMyFloor()->FloorName)) 
		]
	];
}

const FSlateBrush* STiledFloor::GetVisibilityBrushForLayer() const
{
	return GetMyFloor()->ShouldRenderInEditor ? EyeOpened : EyeClosed;
}

FSlateColor STiledFloor::GetForegroundColorForVisibilityButton() const
{
	static const FName InvertedForeground("InvertedForeground");
	return FEditorStyle::GetSlateColor(InvertedForeground);
}

FReply STiledFloor::OnToggleVisibility()
{
	const FScopedTransaction Transaction(LOCTEXT("TiledLevelToggleFloorVisibilityTransaction", "Toggle floor visibility"));
	MyLevel->Modify();
	
 	FTiledFloor* MyFloor = GetMyFloor();
 	MyFloor->ShouldRenderInEditor = !MyFloor->ShouldRenderInEditor;
	OnResetInstances.ExecuteIfBound();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
