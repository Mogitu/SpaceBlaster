// Copyright 2022 PufStudio. All Rights Reserved.

#include "STiledLevelEditorViewportToolbar.h"
#include "SViewportToolBarIconMenu.h"
#include "SEditorViewportToolBarMenu.h"
#include "EditorViewportCommands.h"
#include "SlateOptMacros.h"
#include "STiledLevelEditorViewport.h"
#include "TiledItemSetEditor/STiledItemSetEditorViewportToolbar.h"
#include "Widgets/Input/SSlider.h"

#define LOCTEXT_NAMESPACE "TiledLevelEditorViewportToolbar"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void STiledLevelEditorViewportToolbar::Construct(const FArguments& InArgs)
{
	EditorViewport = InArgs._EditorViewport;
	
	// camera speed control menu
	FToolBarBuilder CameraSpeedToolbar(EditorViewport.Pin()->GetCommandList(), FMultiBoxCustomization::None, MakeShareable(new FExtender()));
	CameraSpeedToolbar.SetStyle(&FAppStyle::Get(), "ViewportMenu");
	CameraSpeedToolbar.SetLabelVisibility(EVisibility::Collapsed);
	CameraSpeedToolbar.SetIsFocusable(false);
	CameraSpeedToolbar.BeginSection("CameraSpeed");
	CameraSpeedToolbar.BeginBlockGroup();
	{
		CameraSpeedToolbar.AddWidget(
			SNew(SEditorViewportToolbarMenu)
			.ParentToolBar(SharedThis(this))
			.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("CameraSpeedButton")))
			.ToolTipText(LOCTEXT("CameraSpeed_ToolTip","Camera Speed"))
			.LabelIcon(FAppStyle::Get().GetBrush("EditorViewport.CamSpeedSetting"))
			.Label(this, &STiledLevelEditorViewportToolbar::GetCameraSpeedLabel)
			.OnGetMenuContent(this, &STiledLevelEditorViewportToolbar::FillCameraSpeedMenu)
		);
	}
	CameraSpeedToolbar.EndBlockGroup();
	CameraSpeedToolbar.EndSection();
	
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("NoBorder"))
		.ColorAndOpacity(this, &SViewportToolBar::GetColorAndOpacity)
		.ForegroundColor(FAppStyle::GetSlateColor("DefaultForeground"))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f, 2.0f)
			[
				SNew( SEditorViewportToolbarMenu )
				.ParentToolBar( SharedThis( this ) )
				.Cursor( EMouseCursor::Default )
				.Label(this, &STiledLevelEditorViewportToolbar::GetCameraMenuLabel)
				.LabelIcon(this, &STiledLevelEditorViewportToolbar::GetCameraMenuLabelIcon)
				.OnGetMenuContent(this, &STiledLevelEditorViewportToolbar::GenerateCameraMenu)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f, 2.0f)
			[
				SNew( SEditorViewportToolbarMenu )
				.ParentToolBar( SharedThis( this ) )
				.Cursor( EMouseCursor::Default )
				.Label(this, &STiledLevelEditorViewportToolbar::GetViewMenuLabel)
				.LabelIcon(this, &STiledLevelEditorViewportToolbar::GetViewMenuLabelIcon)
				.OnGetMenuContent(this, &STiledLevelEditorViewportToolbar::GenerateViewMenu)
			]
			// + SHorizontalBox::Slot() // "Show" menu TODO: future updates...
			// .AutoWidth()
   //          .Padding(2.0f, 2.0f)
   //          [
   //          	SNew(SEditorViewportToolbarMenu)
   //          	.Label(LOCTEXT("ShowMenuTitle", "Show"))
   //          	.Cursor(EMouseCursor::Default)
   //          	.ParentToolBar(SharedThis(this))
   //          	.OnGetMenuContent(this, &STiledLevelEditorViewportToolbar::GenerateShowMenu)
   //          ]
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			[
				CameraSpeedToolbar.MakeWidget()
			]
		]
	];

	SViewportToolBar::Construct(SViewportToolBar::FArguments());
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FText STiledLevelEditorViewportToolbar::GetCameraSpeedLabel() const
{
	if (EditorViewport.Pin().IsValid() && EditorViewport.Pin()->GetViewportClient().IsValid())
	{
		return FText::AsNumber(EditorViewport.Pin()->GetViewportClient()->GetCameraSpeedSetting());
	}
	return FText::GetEmpty();
}

TSharedRef<SWidget> STiledLevelEditorViewportToolbar::FillCameraSpeedMenu()
{
	TSharedRef<SWidget> ReturnWidget = SNew(SBorder)
	.BorderImage(FAppStyle::GetBrush(TEXT("Menu.Background")))
	[
		SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding( FMargin(8.0f, 2.0f, 60.0f, 2.0f) )
		.HAlign( HAlign_Left )
		[
			SNew( STextBlock )
			.Text( LOCTEXT("MouseSettingsCamSpeed", "Camera Speed")  )
			.Font( FAppStyle::GetFontStyle( TEXT( "MenuItem.Font" ) ) )
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding( FMargin(8.0f, 4.0f) )
		[	
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding( FMargin(0.0f, 2.0f) )
			[
				SAssignNew(CamSpeedSlider, SSlider)
				.Value(this, &STiledLevelEditorViewportToolbar::GetCamSpeedSliderPosition)
				.OnValueChanged(this, &STiledLevelEditorViewportToolbar::OnSetCamSpeed)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 8.0f, 2.0f, 0.0f, 2.0f)
			[
				SNew( STextBlock )
				.Text(this, &STiledLevelEditorViewportToolbar::GetCameraSpeedLabel )
				.Font( FAppStyle::GetFontStyle( TEXT( "MenuItem.Font" ) ) )
			]
		] // Camera Speed Scalar
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(8.0f, 2.0f, 60.0f, 2.0f))
			.HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MouseSettingsCamSpeedScalar", "Camera Speed Scalar"))
				.Font(FAppStyle::GetFontStyle(TEXT("MenuItem.Font")))
			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(8.0f, 4.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding(FMargin(0.0f, 2.0f))
			[
				SAssignNew(CamSpeedScalarBox, SSpinBox<float>)
				.MinValue(1)
 			    .MaxValue(TNumericLimits<int32>::Max())
			    .MinSliderValue(1)
			    .MaxSliderValue(128)
				.Value(this, &STiledLevelEditorViewportToolbar::GetCamSpeedScalarBoxValue)
				.OnValueChanged(this, &STiledLevelEditorViewportToolbar::OnSetCamSpeedScalarBoxValue)
				.ToolTipText(LOCTEXT("CameraSpeedScalar_ToolTip", "Scalar to increase camera movement range"))
			]
		]
	];

	return ReturnWidget;
}

float STiledLevelEditorViewportToolbar::GetCamSpeedSliderPosition() const
{
	float SliderPos = 0.f;
	auto ViewportPin = EditorViewport.Pin();
	if (ViewportPin.IsValid() && ViewportPin->GetViewportClient().IsValid())
	{
		SliderPos = (ViewportPin->GetViewportClient()->GetCameraSpeedSetting() - 1) / ((float)FEditorViewportClient::MaxCameraSpeeds - 1);
	}
	return SliderPos;
}

void STiledLevelEditorViewportToolbar::OnSetCamSpeed(float NewValue)
{
	auto ViewportPin = EditorViewport.Pin();
	if (ViewportPin.IsValid() && ViewportPin->GetViewportClient().IsValid())
	{
		const int32 OldSpeedSetting = ViewportPin->GetViewportClient()->GetCameraSpeedSetting();
		const int32 NewSpeedSetting = NewValue * ((float)FEditorViewportClient::MaxCameraSpeeds - 1) + 1;

		if (OldSpeedSetting != NewSpeedSetting)
		{
			ViewportPin->GetViewportClient()->SetCameraSpeedSetting(NewSpeedSetting);
		}
	}
}

float STiledLevelEditorViewportToolbar::GetCamSpeedScalarBoxValue() const
{
	float CamSpeedScalar = 1.f;

	auto ViewportPin = EditorViewport.Pin();
	if (ViewportPin.IsValid() && ViewportPin->GetViewportClient().IsValid())
	{
		CamSpeedScalar = (ViewportPin->GetViewportClient()->GetCameraSpeedScalar());
	}

	return CamSpeedScalar;
}

void STiledLevelEditorViewportToolbar::OnSetCamSpeedScalarBoxValue(float NewValue)
{
	auto ViewportPin = EditorViewport.Pin();
	if (ViewportPin.IsValid() && ViewportPin->GetViewportClient().IsValid())
	{		
		ViewportPin->GetViewportClient()->SetCameraSpeedScalar(NewValue);
		// OnCamSpeedScalarChanged.ExecuteIfBound(NewValue);
	}
}

// TODO: Leave this as future updates...
// TSharedRef<SWidget> STiledLevelEditorViewportToolbar::GenerateShowMenu()
// {
// 	// const FTileMapEditorCommands& Commands = FTileMapEditorCommands::Get();
// 	const bool bInShouldCloseWindowAfterMenuSelection = true;
// 	FMenuBuilder ShowMenuBuilder(bInShouldCloseWindowAfterMenuSelection, EditorViewport.Pin()->GetCommandList());
// 	{
// 		ShowMenuBuilder.AddMenuEntry(Commands.SetShowPivot);
// 		
// 		ShowMenuBuilder.AddMenuSeparator();
// 		
// 		ShowMenuBuilder.AddMenuEntry(Commands.SetShowTileGrid);
// 		ShowMenuBuilder.AddMenuEntry(Commands.SetShowLayerGrid);
// 		ShowMenuBuilder.AddMenuEntry(Commands.SetShowTileMapStats);
// 		
// 		ShowMenuBuilder.AddMenuSeparator();
// 		
// 		ShowMenuBuilder.AddMenuEntry(Commands.SetShowCollision);
// 	}
//
// 	return ShowMenuBuilder.MakeWidget();
// }

FText STiledLevelEditorViewportToolbar::GetCameraMenuLabel() const
{
	if(EditorViewport.IsValid())
	{
		return GetCameraMenuLabelFromViewportType(EditorViewport.Pin()->GetViewportClient()->GetViewportType());
	}
	return LOCTEXT("CameraMenuTitle_Default", "Camera");
}

const FSlateBrush* STiledLevelEditorViewportToolbar::GetCameraMenuLabelIcon() const
{
	if(EditorViewport.IsValid())
	{
		return GetCameraMenuLabelIconFromViewportType( EditorViewport.Pin()->GetViewportClient()->GetViewportType() );
	}
	return FAppStyle::GetBrush(NAME_None);
}

TSharedRef<SWidget> STiledLevelEditorViewportToolbar::GenerateCameraMenu()
{
	TSharedPtr<const FUICommandList> CommandList = EditorViewport.IsValid()? EditorViewport.Pin()->GetCommandList(): nullptr;
	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder CameraMenuBuilder(bInShouldCloseWindowAfterMenuSelection, CommandList);

	CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Perspective);
	CameraMenuBuilder.BeginSection("LevelViewportCameraType_Ortho", NSLOCTEXT("BlueprintEditor", "CameraTypeHeader_Ortho", "Orthographic"));
	{
		CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Top);
		CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Bottom);
		CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Left);
		CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Right);
		CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Front);
		CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Back);
	}
	CameraMenuBuilder.EndSection();
 
	return CameraMenuBuilder.MakeWidget();
}

FText STiledLevelEditorViewportToolbar::GetViewMenuLabel() const
{
	FText Label = LOCTEXT( "ViewMenuTitle_Default", "View");
	if (EditorViewport.IsValid())
	{
		switch (EditorViewport.Pin()->GetViewportClient()->GetViewMode())
		{
		case VMI_Lit:
			Label = LOCTEXT("ViewMenuTitle_Lit", "Lit");
			break;
		case VMI_Unlit:
			Label = LOCTEXT("ViewMenuTitle_Unlit", "Unlit");
			break;
		case VMI_BrushWireframe:
			Label = LOCTEXT("ViewMenuTitle_Wireframe", "Wireframe");
			break;
		default: ;	
		}
	}

	return Label;
}

const FSlateBrush* STiledLevelEditorViewportToolbar::GetViewMenuLabelIcon() const
{
	static FName LitModeIconName("EditorViewport.LitMode");
	static FName UnlitModeIconName("EditorViewport.UnlitMode");
	static FName WireframeModeIconName("EditorViewport.WireframeMode");

	FName Icon = NAME_None;

	if (EditorViewport.IsValid())
	{
		switch (EditorViewport.Pin()->GetViewportClient()->GetViewMode())
		{
		case VMI_Lit:
			Icon = LitModeIconName;
			break;
		case VMI_Unlit:
			Icon = UnlitModeIconName;
			break;
		case VMI_BrushWireframe:
			Icon = WireframeModeIconName;
			break;
		default: ;
		}
	}
	return FAppStyle::GetBrush(Icon);
}

TSharedRef<SWidget> STiledLevelEditorViewportToolbar::GenerateViewMenu()
{
	TSharedPtr<const FUICommandList> CommandList = EditorViewport.IsValid() ? EditorViewport.Pin()->GetCommandList() : nullptr;

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder ViewMenuBuilder(bInShouldCloseWindowAfterMenuSelection, CommandList);

	ViewMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().LitMode, NAME_None, NSLOCTEXT("BlueprintEditor", "LitModeMenuOption", "Lit"));
	ViewMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().UnlitMode, NAME_None, NSLOCTEXT("BlueprintEditor", "UnlitModeMenuOption", "Unlit"));
	ViewMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().WireframeMode, NAME_None, NSLOCTEXT("BlueprintEditor", "WireframeModeMenuOption", "Wireframe"));

	return ViewMenuBuilder.MakeWidget();
}


#undef LOCTEXT_NAMESPACE
