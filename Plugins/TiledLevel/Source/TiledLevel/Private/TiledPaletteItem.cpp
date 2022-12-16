// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledPaletteItem.h"
#include "TiledLevelItem.h"
#include "STiledPalette.h"
#include "TiledLevelAsset.h"
#include "TiledLevelSettings.h"
#include "TiledLevelEditor/TiledLevelEdMode.h"
#include "Kismet/KismetTextLibrary.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/STextComboBox.h"

#define LOCTEXT_NAMESPACE "TiledLevel"

FTiledItemViewData::FTiledItemViewData(UTiledLevelItem* InItem, FTiledLevelEdMode* InEdMode,
	TSharedPtr<FAssetThumbnailPool> InThumbnailPool)
		:Item(InItem), ThumbnailPool(InThumbnailPool), EdMode(InEdMode)
{
	FAssetData AssetData = FAssetData(Item);

	int32 MaxThumbnailSize = 128;
	TSharedPtr<FAssetThumbnail> Thumbnail = MakeShareable(
		new FAssetThumbnail(AssetData, MaxThumbnailSize, MaxThumbnailSize, ThumbnailPool));

	FAssetThumbnailConfig ThumbnailConfig;
	UTiledLevelSettings* Settings = GetMutableDefault<UTiledLevelSettings>();

    if (Item->IsA<UTiledLevelRestrictionItem>() || Item->IsA<UTiledLevelTemplateItem>())
    {
        ThumbnailConfig.AssetTypeColorOverride = Settings->SpecialItemColor;
    }
	else if (Item->StructureType == ETLStructureType::Structure)
	{
		if (Item->SourceType == ETLSourceType::Actor)
			ThumbnailConfig.AssetTypeColorOverride = Settings->StructureActorItemColor;
		else
			ThumbnailConfig.AssetTypeColorOverride = Settings->StructureMeshItemColor;
	}
	else
	{
		if (Item->SourceType == ETLSourceType::Actor)
			ThumbnailConfig.AssetTypeColorOverride = Settings->PropActorItemColor;
		else
			ThumbnailConfig.AssetTypeColorOverride = Settings->PropMeshItemColor;
	}
	
	ThumbnailWidget = Thumbnail->MakeThumbnailWidget(ThumbnailConfig);

}

TSharedRef<SToolTip> FTiledItemViewData::CreateTooltipWidget()
{
    TSharedRef<SWidget> Content = SNullWidget::NullWidget;
    if (Item->IsA<UTiledLevelRestrictionItem>())
        Content = MakeRestrictionItemTooltipContent();
    else if (Item->IsA<UTiledLevelTemplateItem>())
        Content = MakeTemplateItemTooltipContent();
    else
        Content = MakeItemTooltipContent();
        
	return
		SNew(SToolTip)
		.TextMargin(1)
		.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ToolTipBorder"))
		[
			SNew(SBorder)
			.Padding(3.f)
			.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.NonContentBorder"))
			[
                SNew(SVerticalBox)
                + SVerticalBox::Slot() // name widget
                .AutoHeight()
                [
                    SNew(SBorder)
                    .Padding(FMargin(6.f))
                    .HAlign(HAlign_Left)
                    .BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromName(GetDisplayName()))
                        .Font(FAppStyle::GetFontStyle("ContentBrowser.TileViewTooltip.NameFont"))
                        .HighlightText(this, &FTiledItemViewData::GetPaletteSearchText)
                    ]
                ]
                + SVerticalBox::Slot() // detail content widget
                .AutoHeight() 
                .Padding(FMargin(0.f, 3.f, 0.f, 0.f))
                [
                    SNew(SBorder)
                    .Padding(6.f)
                    .BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
                    [
                         Content
                    ]
                ]
			]
		];
}

TSharedRef<SCheckBox> FTiledItemViewData::CreateAllowEraserCheckBox()
{
	return
		SNew(SCheckBox)
		.Padding(0.f)
		.OnCheckStateChanged(this, &FTiledItemViewData::HandleCheckStateChanged)
		.Visibility(this, &FTiledItemViewData::GetEraserCheckBoxVisibility)
		.IsChecked(this, &FTiledItemViewData::GetEraserCheckBoxState)
		.ToolTipText(LOCTEXT("TiledCheckboxTooltip", "Check to active erase this element"));
}

void FTiledItemViewData::HandleCheckStateChanged(const ECheckBoxState NewCheckedState)
{
	Item->bIsEraserAllowed = NewCheckedState==ECheckBoxState::Checked? true : false;
}



EVisibility FTiledItemViewData::GetEraserCheckBoxVisibility() const
{
	if (EdMode)
	{
		return EdMode->ActiveEditTool==Eraser && EdMode->GetPalettePtr()->AnyTileHovered()? EVisibility::Visible : EVisibility::Collapsed;
	}
	return EVisibility::Collapsed;
}

ECheckBoxState FTiledItemViewData::GetEraserCheckBoxState() const
{
	return Item->bIsEraserAllowed? ECheckBoxState::Checked: ECheckBoxState::Unchecked;
}

FText FTiledItemViewData::GetPaletteSearchText() const
{
	return GetMutableDefault<UTiledLevelSettings>()->PaletteSearchText;
}

EVisibility FTiledItemViewData::GetVisibilityBasedOnIsMesh() const
{
	return Item->SourceType == ETLSourceType::Actor? EVisibility::Collapsed : EVisibility::Visible;
}

FText FTiledItemViewData::GetSourceAssetTypeText() const
{
	if (Item->SourceType == ETLSourceType::Actor)
	{
		return LOCTEXT("BlueprintClassAsset", "Blueprint Actor");
	}
	return LOCTEXT("StaticMeshAsset", "Static Mesh");

}

FText FTiledItemViewData::GetInstanceCountText(bool bCurrentLayerOnly) const
{
	if (EdMode)
	{
		if (EdMode->PerItemInstanceCount.Contains(Item->ItemID))
			return FText::AsNumber(EdMode->PerItemInstanceCount[Item->ItemID]);
	}
	return FText::AsNumber(0);
}

FText FTiledItemViewData::GetMeshApproxSize() const
{
	if (UStaticMesh* Mesh = Item->TiledMesh)
	{
		return FText::Format(LOCTEXT("TileMeshApproxSize", "{0}x{1}x{2}"),
		FText::AsNumber(int32(Mesh->GetBounds().BoxExtent.X * 2.0f)),
		FText::AsNumber(int32(Mesh->GetBounds().BoxExtent.Y * 2.0f)),
		FText::AsNumber(int32(Mesh->GetBounds().BoxExtent.Z * 2.0f)));
	}
	return FText::GetEmpty();

}

FName FTiledItemViewData::GetDisplayName() const
{
	return FName(Item->GetItemName());
}

FText FTiledItemViewData::GetPlacedTypeText() const
{
	switch (Item->PlacedType)
	{
		case EPlacedType::Block: return FText::FromString("Block");
		case EPlacedType::Wall: return FText::FromString("Wall");
		case EPlacedType::Floor: return FText::FromString("Floor");
		case EPlacedType::Edge: return FText::FromString("Edge");
		case EPlacedType::Pillar: return FText::FromString("Pillar");
		case EPlacedType::Point: return FText::FromString("Point");
		default: break;
	}
	return FText::GetEmpty();
}

FText FTiledItemViewData::GetExtentText() const
{
	return FText::Format(LOCTEXT("ExtentText", "{0}x{1}x{2}"), Item->Extent.X, Item->Extent.Y, Item->Extent.Z);
}

FText FTiledItemViewData::GetStructureTypeText() const
{
	if (Item->StructureType == ETLStructureType::Structure)
		return FText::FromString("Structure");
	return FText::FromString("Prop");
}

FText FTiledItemViewData::GetRestrictionRuleText() const
{
    return UEnum::GetDisplayValueAsText(Cast<UTiledLevelRestrictionItem>(Item)->RestrictionType);
}

FText FTiledItemViewData::GetRestrictionTargetNumText() const
{
    UTiledLevelRestrictionItem* R = Cast<UTiledLevelRestrictionItem>(Item);
    if (R->bTargetAllItems)
        return FText::FromString("All");
    return FText::AsNumber(R->TargetItems.Num());
}

FText FTiledItemViewData::GetTemplateAssetInfoText(int WhichInfo) const
{
    UTiledLevelAsset* Asset = Cast<UTiledLevelTemplateItem>(Item)->GetAsset();
    if (!Asset) return FText::FromString(TEXT("Unset"));
    if (WhichInfo == 0)
    {
        return FText::FromString(Asset->GetName());
    }
    if (WhichInfo == 1)
    {
        return FText::FromString(
            FString::Printf(TEXT("%d x %d x %d"),
                FMath::RoundToInt(Asset->TileSizeX),
                FMath::RoundToInt(Asset->TileSizeY),
                FMath::RoundToInt(Asset->TileSizeZ)));
    }
    if (WhichInfo == 2)
    {
        return FText::FromString(FString::Printf(TEXT("%d x %d x %d"),
            Asset->X_Num, Asset->Y_Num, Asset->TiledFloors.Num()));
    }
    if (WhichInfo == 3)
    {
        return FText::AsNumber(Asset->GetNumOfAllPlacements());
    }
    return FText::GetEmpty();
}

TSharedRef<SWidget> FTiledItemViewData::GetThumbnailWidget()
{
	return ThumbnailWidget.ToSharedRef();
}

TSharedRef<SWidget> FTiledItemViewData::MakeItemTooltipContent()
{
    return SNew(SVerticalBox)
    + SVerticalBox::Slot() // Asset type
    .Padding(-1, 1)
    .AutoHeight()
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot()
        .AutoWidth()
        [
        SNew(STextBlock)
        .Text(LOCTEXT("SourceAssetTypeHeading", "Source Asset Type: "))
        .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(this, &FTiledItemViewData::GetSourceAssetTypeText)
        ]
    ]
    + SVerticalBox::Slot() // Item extent
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(LOCTEXT("TiledExtentHeading", "Extent: "))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(this, &FTiledItemViewData::GetExtentText)
        ]
    ]
    + SVerticalBox::Slot() // Item PlacedType
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(LOCTEXT("PlacedTypeHeading", "Placed Type: "))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(this, &FTiledItemViewData::GetPlacedTypeText)
        ]
    ]

    + SVerticalBox::Slot() // structure type
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(LOCTEXT("StructureTypeHeading", "Structure Type: "))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(this, &FTiledItemViewData::GetStructureTypeText)
        ]
    ]
    + SVerticalBox::Slot() // Item mesh size
    .Padding(-1, 1)
    .AutoHeight()
    [
        SNew(SHorizontalBox)
        .Visibility(this, &FTiledItemViewData::GetVisibilityBasedOnIsMesh)
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(LOCTEXT("TileMeshApproxSizeHeading", "Approx Size: "))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(this, &FTiledItemViewData::GetMeshApproxSize)
        ]
    ];
}

TSharedRef<SWidget> FTiledItemViewData::MakeRestrictionItemTooltipContent()
{
    return SNew(SVerticalBox)
    + SVerticalBox::Slot()
    .Padding(-1, 1)
    .AutoHeight()
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(LOCTEXT("RestrictionTypeHeading", "Restriction Type: "))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(this, &FTiledItemViewData::GetRestrictionRuleText)
        ]
    ]
    + SVerticalBox::Slot()
    .Padding(-1, 1)
    .AutoHeight()
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(LOCTEXT("TargetNumHeading", "Number of targeted items: "))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(this, &FTiledItemViewData::GetRestrictionTargetNumText)
        ]
    ];
}

TSharedRef<SWidget> FTiledItemViewData::MakeTemplateItemTooltipContent()
{
    return SNew(SVerticalBox)
    + SVerticalBox::Slot()
    .Padding(-1, 1)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(LOCTEXT("AssetNameHeading", "Asset: "))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(this, &FTiledItemViewData::GetTemplateAssetInfoText, 0)
        ]
    ]
    + SVerticalBox::Slot()
    .AutoHeight()
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(FText::FromString("Tile Size: "))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(this, &FTiledItemViewData::GetTemplateAssetInfoText, 1)
        ]
    ]
    + SVerticalBox::Slot()
    .AutoHeight()
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(FText::FromString("Tile Amount: "))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(this, &FTiledItemViewData::GetTemplateAssetInfoText, 2)
        ]
    ]
    + SVerticalBox::Slot()
    .Padding(-1, 1)
    .AutoHeight()
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(FText::FromString("Total Placements: "))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(STextBlock)
            .Text(this, &FTiledItemViewData::GetTemplateAssetInfoText, 3)
        ]
    ];
        
}

void STiledPaletteItem::Construct(const FArguments& InArgs, TSharedPtr<FTiledItemViewData> InData,
                                  const TSharedRef<STableViewBase>& InOwnerTableView,
                                  class FTiledLevelEdMode* InEdMode)
{
	Data = InData;
	EdMode = InEdMode;
	MyCheckBox = Data->CreateAllowEraserCheckBox();

	STableRow<TSharedPtr<FTiledItemViewData>>::Construct(
		STableRow<TSharedPtr<FTiledItemViewData>>::FArguments()
		.Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow")
		.Padding(1.f)
		.Content()
		[
			SNew(SOverlay)
			.ToolTip(Data->CreateTooltipWidget())

			// Thumbnail
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.Padding(2.f)
				.BorderImage(FAppStyle::GetBrush("ContentBrowser.ThumbnailShadow"))
				.ForegroundColor(FLinearColor::White)
				.ColorAndOpacity(this, &STiledPaletteItem::GetTileColorAndOpacity)
				[
					Data->GetThumbnailWidget()
				]
			]

			// Checkbox
			+ SOverlay::Slot()
			  .HAlign(HAlign_Left)
			  .VAlign(VAlign_Top)
			  .Padding(FMargin(3.f))
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ContentBrowser.ThumbnailShadow"))
				.Padding(3.f)
				[
					MyCheckBox.ToSharedRef()
				]
			]

			// Instance Count
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			.Padding(FMargin(6.f, 8.f))
			[
				SNew(STextBlock)
				.Visibility(this, &STiledPaletteItem::GetInstanceCountVisibility)
				.Text(Data.ToSharedRef(), &FTiledItemViewData::GetInstanceCountText, true)
				.ShadowOffset(FVector2D(1.f, 1.f))
				.ColorAndOpacity(FLinearColor(.85f, .85f, .85f, 1.f))
			]
		], InOwnerTableView);
}

FLinearColor STiledPaletteItem::GetTileColorAndOpacity() const
{
	if (EdMode)
	{
		if (Data->Item->bIsEraserAllowed && EdMode->ActiveEditTool == Eraser)
			return FLinearColor(1.0, 0.6, 0.6, 1);
		if (EdMode->ActiveEditTool == ETiledLevelEditTool::Paint)
		{
			if (Data->Item == EdMode->ActiveItem)
				return FLinearColor::White;
		}
		if (EdMode->ActiveEditTool == ETiledLevelEditTool::Fill)
		{
			if (EdMode->SelectedItems.Contains(Data->Item))
				return FLinearColor::White;
		}
		return FLinearColor(0.5, 0.5, 0.5, 1);
	}
	return FLinearColor::White;
}

EVisibility STiledPaletteItem::GetCheckBoxVisibility() const
{
	if (EdMode)
		return (EdMode->ActiveEditTool == Eraser && EdMode->GetPalettePtr()->AnyTileHovered())? EVisibility::Visible : EVisibility::Collapsed;
	return EVisibility::Collapsed;
}

EVisibility STiledPaletteItem::GetInstanceCountVisibility() const
{
	if (!EdMode) return EVisibility::Collapsed;
	return GetMutableDefault<UTiledLevelSettings>()->ThumbnailScale > 0.2? EVisibility::Visible : EVisibility::Collapsed;
}

void STiledPaletteItem::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	STableRow<TSharedPtr<FTiledItemViewData, ESPMode::ThreadSafe>>::OnMouseEnter(MyGeometry, MouseEvent);
	if (MouseEvent.IsMouseButtonDown(FKey(EKeys::LeftMouseButton)))
	{
		if (GetCheckBoxVisibility() == EVisibility::Visible)
			MyCheckBox->ToggleCheckedState();
	}
}

void SNewItemDropConfirmItemRow::Construct(const FArguments& InArgs, TSharedPtr<FString> InAssetName,
                                           const TSharedRef<STableViewBase>& InOwnerTableView, EPlacedType* InTargetPlacedType, ETLStructureType* InTargetStructure, 
                                           FVector* InTargetExtent)
{
	AssetName = InAssetName;
	TargetPlacedType = InTargetPlacedType;
	TargetStructureType = InTargetStructure;
	TargetExtent = InTargetExtent;
	PlacedTypeOptions.Add(MakeShareable(new FString("Block")));
	PlacedTypeOptions.Add(MakeShareable(new FString("Floor")));
	PlacedTypeOptions.Add(MakeShareable(new FString("Wall")));
	PlacedTypeOptions.Add(MakeShareable(new FString("Edge")));
	PlacedTypeOptions.Add(MakeShareable(new FString("Pillar")));
	PlacedTypeOptions.Add(MakeShareable(new FString("Point")));
	StructureTypeOptions.Add(MakeShareable(new FString("Structure")));
	StructureTypeOptions.Add(MakeShareable(new FString("Prop")));
	SMultiColumnTableRow<TSharedPtr<FString>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

TSharedRef<SWidget> SNewItemDropConfirmItemRow::GenerateWidgetForColumn(const FName& InColumnName)
{
	TSharedPtr<SWidget> RowWidget = SNullWidget::NullWidget;
	const auto InitialPlacedTypeOption = PlacedTypeOptions[static_cast<uint8>(*TargetPlacedType)];
	const auto InitialStructureTypeOption = *TargetStructureType == ETLStructureType::Structure? StructureTypeOptions[0] : StructureTypeOptions[1];
	if (InColumnName == FName("Name"))
	{
		RowWidget = SNew(SBox).VAlign(VAlign_Center).HAlign(HAlign_Left).Padding(FMargin(10, 1, 0,1))
		[
			SNew(STextBlock).Text(FText::FromString(*AssetName.Get()))
		];
	}
	else if (InColumnName == FName("PlacedType"))
	{
		RowWidget = SNew(SBox).VAlign(VAlign_Center).HAlign(HAlign_Left).Padding(FMargin(10, 1, 0,1))
		[
			SNew(STextComboBox)
			.OptionsSource(&PlacedTypeOptions)
			.InitiallySelectedItem(InitialPlacedTypeOption)
			.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewPlacedType, ESelectInfo::Type SelectInfo)
			{
				const float MaxXY = FMath::Max(TargetExtent->X, TargetExtent->Y);
				uint8 PlacedTypeIndex = 0;
				if (*NewPlacedType.Get() == FString("Block"))
				{
					PlacedTypeIndex = 0;
				}
				else if (*NewPlacedType.Get() == FString("Floor"))
				{
					TargetExtent->Z = 1;
					PlacedTypeIndex = 1;
				}
				else if (*NewPlacedType.Get() == FString("Wall"))
				{
					TargetExtent->X = MaxXY;
					TargetExtent->Y = MaxXY;
					PlacedTypeIndex = 2;
				}
				else if (*NewPlacedType.Get() == FString("Edge"))
				{
					TargetExtent->X = MaxXY;
					TargetExtent->Y = MaxXY;
					TargetExtent->Z = 1;
					PlacedTypeIndex = 3;
				}
				else if (*NewPlacedType.Get() == FString("Pillar"))
				{
					TargetExtent->X = 1;
					TargetExtent->Y = 1;
					PlacedTypeIndex = 4;
				}
				else if (*NewPlacedType.Get() == FString("Point"))
				{
					*TargetExtent = FVector(1);
					PlacedTypeIndex = 5;
				}
				*TargetPlacedType = static_cast<EPlacedType>(PlacedTypeIndex);
			})
		];
	}
	else if (InColumnName == FName("StructureType"))
	{
		RowWidget = SNew(SBox).VAlign(VAlign_Center).HAlign(HAlign_Left).Padding(FMargin(10, 1, 0,1))
		[
			SNew(STextComboBox)
			.OptionsSource(&StructureTypeOptions)
			.InitiallySelectedItem(InitialStructureTypeOption)
			.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewStructureType, ESelectInfo::Type SelectInfo)
			{
				uint8 i = *NewStructureType.Get() == FString("Structure")? 0 : 1;
				*TargetStructureType = static_cast<ETLStructureType>(i);
			})
		];
	}
	else if (InColumnName == FName("Extent"))
	{
		TSharedRef<SWidget> X_Label = SNumericEntryBox<int>::BuildLabel(FText::FromString("X"), FLinearColor::White, SNumericEntryBox<int>::RedLabelBackgroundColor);
		TSharedRef<SWidget> Y_Label = SNumericEntryBox<int>::BuildLabel(FText::FromString("Y"), FLinearColor::White, SNumericEntryBox<int>::GreenLabelBackgroundColor);
		TSharedRef<SWidget> Z_Label = SNumericEntryBox<int>::BuildLabel(FText::FromString("Z"), FLinearColor::White, SNumericEntryBox<int>::BlueLabelBackgroundColor);
		
		RowWidget = SNew(SBox)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Left)
		.Padding(FMargin(10, 1, 0,1))
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SNumericEntryBox<int>)
				.AllowSpin(true)
				.MinValue(0)
				.MaxValue(16)
				.MinSliderValue(1)
				.MaxSliderValue(16)
				.Visibility(this, &SNewItemDropConfirmItemRow::GetExtentInputVisibility, EAxis::X)
				.Value(this, &SNewItemDropConfirmItemRow::GetExtentValue, EAxis::X)
				.OnValueChanged(this, &SNewItemDropConfirmItemRow::OnExtentChanged, EAxis::X)
				.OnValueCommitted(this, &SNewItemDropConfirmItemRow::OnExtentCommitted, EAxis::X)
				.Label()[X_Label]
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SNumericEntryBox<int>)
				.AllowSpin(true)
				.MinValue(0)
				.MaxValue(16)
				.MinSliderValue(1)
				.MaxSliderValue(16)
				.Visibility(this, &SNewItemDropConfirmItemRow::GetExtentInputVisibility, EAxis::Y)
				.Value(this, &SNewItemDropConfirmItemRow::GetExtentValue, EAxis::Y)
				.OnValueChanged(this, &SNewItemDropConfirmItemRow::OnExtentChanged, EAxis::Y)
				.OnValueCommitted(this, &SNewItemDropConfirmItemRow::OnExtentCommitted, EAxis::Y)
				.Label()[Y_Label]
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SNumericEntryBox<int>)
				.AllowSpin(true)
				.MinValue(0)
				.MaxValue(16)
				.MinSliderValue(1)
				.MaxSliderValue(16)
				.Visibility(this, &SNewItemDropConfirmItemRow::GetExtentInputVisibility, EAxis::Z)
				.Value(this, &SNewItemDropConfirmItemRow::GetExtentValue, EAxis::Z)
				.OnValueChanged(this, &SNewItemDropConfirmItemRow::OnExtentChanged, EAxis::Z)
				.OnValueCommitted(this, &SNewItemDropConfirmItemRow::OnExtentCommitted, EAxis::Z)
				.Label()[Z_Label]
			]
		];
	}

	return RowWidget.ToSharedRef();
}

EVisibility SNewItemDropConfirmItemRow::GetExtentInputVisibility(EAxis::Type Axis) const
{
	if (!TargetPlacedType) return EVisibility::Visible;
	if (*TargetPlacedType == EPlacedType::Floor && Axis== EAxis::Z) return EVisibility::Collapsed;
	if (*TargetPlacedType == EPlacedType::Wall && Axis== EAxis::Y) return EVisibility::Collapsed;
	if (*TargetPlacedType == EPlacedType::Pillar && (Axis == EAxis::X || Axis == EAxis::Y)) return EVisibility::Collapsed;
	if (*TargetPlacedType == EPlacedType::Edge && (Axis== EAxis::Y || Axis == EAxis::Z)) return EVisibility::Collapsed;
	if (*TargetPlacedType == EPlacedType::Point) return EVisibility::Collapsed;
	return EVisibility::Visible;
}

TOptional<int> SNewItemDropConfirmItemRow::GetExtentValue(EAxis::Type Axis) const
{
	if (TargetExtent)
 	{
 		switch (Axis) {
 		case EAxis::X: return TargetExtent->X;
 		case EAxis::Y: return TargetExtent->Y;
 		case EAxis::Z: return TargetExtent->Z;
 		default: ;
 		}
 	}
 	return 1;
}

void SNewItemDropConfirmItemRow::OnExtentChanged(int NewValue, EAxis::Type Axis)
{
	if (NewValue < 1) NewValue = 1;
 	if (NewValue > 16) NewValue = 16;
 	if (TargetExtent && TargetPlacedType)
 	{
 		switch (Axis) {
 		case EAxis::X:
			TargetExtent->X = NewValue;
 			if (*TargetPlacedType == EPlacedType::Edge || *TargetPlacedType == EPlacedType::Wall)
 				TargetExtent->Y = NewValue;
 			break;
 		case EAxis::Y:
			TargetExtent->Y = NewValue;
 			break;
 		case EAxis::Z:
			TargetExtent->Z = NewValue;
 			break;
 		default: ;
 		}
 	}
}

void SNewItemDropConfirmItemRow::OnExtentCommitted(int NewValue, ETextCommit::Type CommitType, EAxis::Type Axis)
{
	if (NewValue < 1) NewValue = 1;
 	if (NewValue > 16) NewValue = 16;
 	if (TargetExtent && TargetPlacedType)
 	{
 		switch (Axis) {
 		case EAxis::X:
			TargetExtent->X = NewValue;
 			if (*TargetPlacedType == EPlacedType::Edge || *TargetPlacedType == EPlacedType::Wall)
 				TargetExtent->Y = NewValue;
 			break;
 		case EAxis::Y:
			TargetExtent->Y = NewValue;
 			break;
 		case EAxis::Z:
			TargetExtent->Z = NewValue;
 			break;
 		default: ;
 		}
 	}
}

void STiledPaletteFillDetailRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView,
	TSharedPtr<FTiledItemViewData> InData, FTiledLevelEdMode* InEdMode)
{
	MyData = InData;
	EdMode = InEdMode;
	SMultiColumnTableRow<TSharedPtr<FTiledItemViewData>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	SetToolTip(MyData->CreateTooltipWidget());
}

TSharedRef<SWidget> STiledPaletteFillDetailRow::GenerateWidgetForColumn(const FName& InColumnName)
{
	TSharedPtr<SWidget> TableRowWidget = SNullWidget::NullWidget;
	if (InColumnName == FillDetailTreeColumns::ColumnID_Thumbnail)
	{
		TableRowWidget = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(32.f)
			.HeightOverride(32.f)
			[
				MyData->GetThumbnailWidget()
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(FText::FromString(MyData->Item->GetItemName()))
		];
	}
	else if (InColumnName == FillDetailTreeColumns::ColumnID_RandomRotation)
	{
		TableRowWidget = SNew(SCheckBox)
		.IsChecked(this, &STiledPaletteFillDetailRow::GetAllowRandomRotation)
		.OnCheckStateChanged(this, &STiledPaletteFillDetailRow::OnAllowRandomRotationChanged);
	}
	else if (InColumnName == FillDetailTreeColumns::ColumnID_AllowOverlay) 
	{
		TableRowWidget = SNew(SCheckBox)
		.IsChecked(this, &STiledPaletteFillDetailRow::GetAllowOverlay)
		.OnCheckStateChanged(this, &STiledPaletteFillDetailRow::OnAllowOverlayChanged);
	}
	else if (InColumnName == FillDetailTreeColumns::ColumnID_Coefficient)
	{
		TableRowWidget = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			 SNew(SNumericEntryBox<float>)
			 .MinDesiredValueWidth(72.f)
			 .AllowSpin(true)
			 .MinValue(0.0f)
			 .MaxValue(1.0f)
			 .MinSliderValue(0.0f)
			 .MaxSliderValue(1.0f)
			 .Value(this, &STiledPaletteFillDetailRow::GetCoefficient)
			 .OnValueChanged(this , &STiledPaletteFillDetailRow::OnCoefficientChanged)
			 .OnValueCommitted(this, &STiledPaletteFillDetailRow::OnCoefficientCommitted)
		]
		+ SHorizontalBox::Slot()
		.Padding(5, 0, 0, 0)
		[
			SNew(STextBlock)
			.Text(this, &STiledPaletteFillDetailRow::GetActualRatioText)
		];
	}
	return TableRowWidget.ToSharedRef();
}

TOptional<float> STiledPaletteFillDetailRow::GetCoefficient() const
{
	return MyData->Item->FillCoefficient;
}

void STiledPaletteFillDetailRow::OnCoefficientChanged(float NewValue)
{
	MyData->Item->FillCoefficient = NewValue;
}

void STiledPaletteFillDetailRow::OnCoefficientCommitted(float NewValue, ETextCommit::Type CommitType)
{
	MyData->Item->FillCoefficient = NewValue;
}

FText STiledPaletteFillDetailRow::GetActualRatioText() const
{
	float TotalWeight = 0.00001f;
	for (auto item : EdMode->SelectedItems)
	{
		if (item->bAllowOverlay) continue;
		TotalWeight += item->FillCoefficient;
	}
	if (MyData->Item->bAllowOverlay)
	{
		float Ratio = MyData->Item->FillCoefficient;
		if (EdMode->EnableFillGap)
			Ratio *= 1 - (EdMode->GapCoefficient / (EdMode->GapCoefficient + TotalWeight));
		FText RatioText = UKismetTextLibrary::Conv_DoubleToText(Ratio * 100, ERoundingMode::HalfToZero, false, true, 1, 3, 2, 2);
		return FText::Format(LOCTEXT("Actual Fill Ratio", "({0} %)"), RatioText);
	}
	if (EdMode->EnableFillGap)
		TotalWeight += EdMode->GapCoefficient;
	float MyRatio = MyData->Item->FillCoefficient / TotalWeight;
	FText RatioText = UKismetTextLibrary::Conv_DoubleToText(MyRatio * 100, ERoundingMode::HalfToZero, false, true, 1, 3, 2, 2);
	return FText::Format(LOCTEXT("Actual Fill Ratio", "({0} %)"), RatioText);
}

ECheckBoxState STiledPaletteFillDetailRow::GetAllowRandomRotation() const
{
	return MyData->Item->bAllowRandomRotation? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void STiledPaletteFillDetailRow::OnAllowRandomRotationChanged(ECheckBoxState NewCheckBoxState)
{
	MyData->Item->bAllowRandomRotation = NewCheckBoxState == ECheckBoxState::Checked;
}

ECheckBoxState STiledPaletteFillDetailRow::GetAllowOverlay() const
{
	return MyData->Item->bAllowOverlay? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void STiledPaletteFillDetailRow::OnAllowOverlayChanged(ECheckBoxState NewCheckBoxState)
{
	MyData->Item->bAllowOverlay = NewCheckBoxState == ECheckBoxState::Checked;
}

#undef LOCTEXT_NAMESPACE
