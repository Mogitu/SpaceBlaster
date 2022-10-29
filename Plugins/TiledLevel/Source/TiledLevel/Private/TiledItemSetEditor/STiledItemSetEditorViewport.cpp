// Copyright 2022 PufStudio. All Rights Reserved.

#include "STiledItemSetEditorViewport.h"
#include "TiledItemSetEditorViewportClient.h"
#include "AdvancedPreviewScene.h"
#include "STiledItemSetEditorViewportToolbar.h"
#include "TiledLevelItem.h"
#include "TiledItemSet.h"
#include "TiledItemSetEditor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"

#define LOCTEXT_NAMESPACE "TiledItemSetEditor"

STiledItemSetEditorViewport::FArguments::FArguments()
{
}

void STiledItemSetEditorViewport::Construct(const FArguments& InArgs, TSharedPtr<FTiledItemSetEditor> InEditor)
{
	TiledItemSetEditorPtr = InEditor;
	
	{
		FAdvancedPreviewScene::ConstructionValues CVS;
		CVS.bCreatePhysicsScene = true; // this is a must for line trace, the world must contain physics scene!
		CVS.LightBrightness = 2;
		CVS.SkyBrightness = 1;
		PreviewScene = MakeShareable(new FAdvancedPreviewScene(CVS));
	}
	
	SEditorViewport::Construct(SEditorViewport::FArguments());
    Skylight = NewObject<USkyLightComponent>();
    AtmosphericFog = NewObject<USkyAtmosphereComponent>();
    PreviewScene->AddComponent(AtmosphericFog, FTransform::Identity);
    PreviewScene->DirectionalLight->SetMobility(EComponentMobility::Movable);
    PreviewScene->DirectionalLight->CastShadows = true;
    PreviewScene->DirectionalLight->CastStaticShadows = true;
    PreviewScene->DirectionalLight->CastDynamicShadows = true;
    PreviewScene->DirectionalLight->SetIntensity(3);
    PreviewScene->SetSkyBrightness(1.0f);
	PreviewScene->SetFloorVisibility(false);
}

STiledItemSetEditorViewport::~STiledItemSetEditorViewport()
{
	if (EditorViewportClient.IsValid())
	{
		EditorViewportClient->Viewport = nullptr;
		EditorViewportClient.Reset();
	}
}

void STiledItemSetEditorViewport::BindCommands()
{
	SEditorViewport::BindCommands();
}

TSharedRef<FEditorViewportClient> STiledItemSetEditorViewport::MakeEditorViewportClient()
{
	EditorViewportClient = MakeShareable(new FTiledItemSetEditorViewportClient(TiledItemSetEditorPtr, SharedThis(this), *PreviewScene, nullptr));
	EditorViewportClient->SetRealtime(true);
	return EditorViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> STiledItemSetEditorViewport::MakeViewportToolbar()
{
	return SNew(STiledItemSetEditorViewportToolbar)
	.EditorViewport(SharedThis(this))
	.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute()); // don't know what this is for...
}

EVisibility STiledItemSetEditorViewport::GetTransformToolbarVisibility() const
{
	return SEditorViewport::GetTransformToolbarVisibility();
}

void STiledItemSetEditorViewport::OnFocusViewportToSelection()
{
	SEditorViewport::OnFocusViewportToSelection();
}

TSharedRef<SEditorViewport> STiledItemSetEditorViewport::GetViewportWidget()
{
	return SharedThis(this);
}

TSharedPtr<FExtender> STiledItemSetEditorViewport::GetExtenders() const
{
	TSharedPtr<FExtender> Result(MakeShareable(new FExtender));
	return Result;
}

void STiledItemSetEditorViewport::OnFloatingButtonClicked()
{
}

void STiledItemSetEditorViewport::PopulateViewportOverlays(TSharedRef<SOverlay> Overlay)
{
	SEditorViewport::PopulateViewportOverlays(Overlay);

	Overlay->AddSlot()
	.VAlign(VAlign_Bottom)
	.HAlign(HAlign_Right)
	.Padding(30)
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		.BorderBackgroundColor(FLinearColor(0.5f, 0.5f, 0.5f, 0.5f))
		.Padding(FMargin(15, 10))
		.Visibility_Lambda([=]()
		{
			return TiledItemSetEditorPtr.Pin()->FixedItemsInfo.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SBox)
			.MinDesiredWidth(150)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString("Fixed Items:"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold",10))
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10, 10, 10, 0)
				[
					SNew(STextBlock)
					.Text_Lambda([=]()
					{
						FString TargetItemInfo(TEXT("Block:"));
						for (auto elem : TiledItemSetEditorPtr.Pin()->FixedItemsInfo)
						{
							if (elem.Key->PlacedType == EPlacedType::Block)
								TargetItemInfo.Appendf(TEXT(" %s (%d),"), *elem.Key->GetItemName(), elem.Value.Num());
						}
						TargetItemInfo.RemoveAt(TargetItemInfo.Len() - 1);
						return FText::FromString(TargetItemInfo);
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Regular",8))
					.WrapTextAt(600)
					.Visibility_Lambda([=]()
					{
						for (auto elem : TiledItemSetEditorPtr.Pin()->FixedItemsInfo)
						{
							if (elem.Key->PlacedType == EPlacedType::Block)
								return EVisibility::Visible;
						}
						return EVisibility::Collapsed;
					})
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10, 0)
				[
					SNew(STextBlock)
					.Text_Lambda([=]()
					{
						FString TargetItemInfo(TEXT("Floor:"));
						for (auto elem : TiledItemSetEditorPtr.Pin()->FixedItemsInfo)
						{
							if (elem.Key->PlacedType == EPlacedType::Floor)
								TargetItemInfo.Appendf(TEXT(" %s (%d),"), *elem.Key->GetItemName(), elem.Value.Num());
						}
						TargetItemInfo.RemoveAt(TargetItemInfo.Len() - 1);
						return FText::FromString(TargetItemInfo);
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Regular",8))
					.WrapTextAt(600)
					.Visibility_Lambda([=]()
					{
						for (auto elem : TiledItemSetEditorPtr.Pin()->FixedItemsInfo)
						{
							if (elem.Key->PlacedType == EPlacedType::Floor)
								return EVisibility::Visible;
						}
						return EVisibility::Collapsed;
					})
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10, 0)
				[
					SNew(STextBlock)
					.Text_Lambda([=]()
					{
						FString TargetItemInfo(TEXT("Wall:"));
						for (auto elem : TiledItemSetEditorPtr.Pin()->FixedItemsInfo)
						{
							if (elem.Key->PlacedType == EPlacedType::Wall)
								TargetItemInfo.Appendf(TEXT(" %s (%d),"), *elem.Key->GetItemName(), elem.Value.Num());
						}
						TargetItemInfo.RemoveAt(TargetItemInfo.Len() - 1);
						return FText::FromString(TargetItemInfo);
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Regular",8))
					.WrapTextAt(600)
					.Visibility_Lambda([=]()
					{
						for (auto elem : TiledItemSetEditorPtr.Pin()->FixedItemsInfo)
						{
							if (elem.Key->PlacedType == EPlacedType::Wall)
								return EVisibility::Visible;
						}
						return EVisibility::Collapsed;
					})
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10, 0)
				[
					SNew(STextBlock)
					.Text_Lambda([=]()
					{
						FString TargetItemInfo(TEXT("Edge:"));
						for (auto elem : TiledItemSetEditorPtr.Pin()->FixedItemsInfo)
						{
							if (elem.Key->PlacedType == EPlacedType::Edge)
								TargetItemInfo.Appendf(TEXT(" %s (%d),"), *elem.Key->GetItemName(), elem.Value.Num());
						}
						TargetItemInfo.RemoveAt(TargetItemInfo.Len() - 1);
						return FText::FromString(TargetItemInfo);
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Regular",8))
					.WrapTextAt(600)
					.Visibility_Lambda([=]()
					{
						for (auto elem : TiledItemSetEditorPtr.Pin()->FixedItemsInfo)
						{
							if (elem.Key->PlacedType == EPlacedType::Edge)
								return EVisibility::Visible;
						}
						return EVisibility::Collapsed;
					})
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10, 0)
				[
					SNew(STextBlock)
					.Text_Lambda([=]()
					{
						FString TargetItemInfo(TEXT("Pillar:"));
						for (auto elem : TiledItemSetEditorPtr.Pin()->FixedItemsInfo)
						{
							if (elem.Key->PlacedType == EPlacedType::Pillar)
								TargetItemInfo.Appendf(TEXT(" %s (%d),"), *elem.Key->GetItemName(), elem.Value.Num());
						}
						TargetItemInfo.RemoveAt(TargetItemInfo.Len() - 1);
						return FText::FromString(TargetItemInfo);
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Regular",8))
					.WrapTextAt(600)
					.Visibility_Lambda([=]()
					{
						for (auto elem : TiledItemSetEditorPtr.Pin()->FixedItemsInfo)
						{
							if (elem.Key->PlacedType == EPlacedType::Pillar)
								return EVisibility::Visible;
						}
						return EVisibility::Collapsed;
					})
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10, 0)
				[
					SNew(STextBlock)
					.Text_Lambda([=]()
					{
						FString TargetItemInfo(TEXT("Point:"));
						for (auto elem : TiledItemSetEditorPtr.Pin()->FixedItemsInfo)
						{
							if (elem.Key->PlacedType == EPlacedType::Point)
								TargetItemInfo.Appendf(TEXT(" %s (%d),"), *elem.Key->GetItemName(), elem.Value.Num());
						}
						TargetItemInfo.RemoveAt(TargetItemInfo.Len() - 1);
						return FText::FromString(TargetItemInfo);
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Regular",8))
					.WrapTextAt(600)
					.Visibility_Lambda([=]()
					{
						for (auto elem : TiledItemSetEditorPtr.Pin()->FixedItemsInfo)
						{
							if (elem.Key->PlacedType == EPlacedType::Point)
								return EVisibility::Visible;
						}
						return EVisibility::Collapsed;
					})
				]
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
