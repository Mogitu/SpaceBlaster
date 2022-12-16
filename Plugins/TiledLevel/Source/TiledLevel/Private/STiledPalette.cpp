// Copyright 2022 PufStudio. All Rights Reserved.

#include "STiledPalette.h"
#include "TiledItemSetEditor/TiledItemSetEditor.h"
#include "TiledLevelEditorLog.h"
#include "TiledPaletteItem.h"
#include "TiledLevelItem.h"
#include "TiledItemSet.h"
#include "TiledLevelSettings.h"
#include "STiledItemPicker.h"
#include "TiledLevelEditor/TiledLevelEdMode.h"
#include "Engine/StaticMeshActor.h"
#include "Dialogs/Dialogs.h"
#include "AssetSelection.h"
#include "IStructureDetailsView.h"
#include "SlateOptMacros.h"
#include "Framework/Commands/GenericCommands.h"
#include "GameFramework/Info.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetTextLibrary.h"
#include "Widgets//Layout/SScaleBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

#define LOCTEXT_NAMESPACE "TiledItemSetEditor"

// Copy from SRowEditor.cpp
class FStructFromDataTable : public FStructOnScope
{
	TWeakObjectPtr<UDataTable> DataTable;
	FName RowName;

public:
	FStructFromDataTable(UDataTable* InDataTable, FName InRowName)
		: FStructOnScope()
		  , DataTable(InDataTable)
		  , RowName(InRowName)
	{
	}

	virtual uint8* GetStructMemory() override
	{
		return (DataTable.IsValid() && !RowName.IsNone()) ? DataTable->FindRowUnchecked(RowName) : nullptr;
	}

	virtual const uint8* GetStructMemory() const override
	{
		return (DataTable.IsValid() && !RowName.IsNone()) ? DataTable->FindRowUnchecked(RowName) : nullptr;
	}

	virtual const UScriptStruct* GetStruct() const override
	{
		return DataTable.IsValid() ? DataTable->GetRowStruct() : nullptr;
	}

	virtual UPackage* GetPackage() const override
	{
		return DataTable.IsValid() ? DataTable->GetOutermost() : nullptr;
	}

	virtual void SetPackage(UPackage* InPackage) override
	{
	}

	virtual bool IsValid() const override
	{
		return !RowName.IsNone()
			&& DataTable.IsValid()
			&& DataTable->GetRowStruct()
			&& DataTable->FindRowUnchecked(RowName);
	}

	virtual void Destroy() override
	{
		DataTable = nullptr;
		RowName = NAME_None;
	}

	FName GetRowName() const
	{
		return RowName;
	}
};


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

class STiledItemDragDropHandler : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STiledItemDragDropHandler)
		{
		}

		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_EVENT(FOnDrop, OnDrop)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		bIsDragOn = false;
		OnDropDelegate = InArgs._OnDrop;

		this->ChildSlot
		[
			SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(this, &STiledItemDragDropHandler::GetBackgroundColor)
				.Padding(FMargin(30))
			[
				InArgs._Content.Widget
			]
		];
	}

	FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		bIsDragOn = false;
		if (OnDropDelegate.IsBound())
		{
			return OnDropDelegate.Execute(MyGeometry, DragDropEvent);
		}

		return FReply::Handled();
	}

	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		bIsDragOn = true;
	}

	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override
	{
		bIsDragOn = false;
	}

private:
	FSlateColor GetBackgroundColor() const
	{
		return bIsDragOn ? FLinearColor(1.0f, 0.6f, 0.1f, 0.9f) : FLinearColor(0.1f, 0.1f, 0.1f, 0.9f);
	}

	FOnDrop OnDropDelegate;
	bool bIsDragOn = false;
};


void STiledPalette::Construct(const FArguments& InArgs, TSharedPtr<FTiledItemSetEditor> InSetEditor,
                              FTiledLevelEdMode* InEdMode, UTiledItemSet* InItemSet)
{
	SetEditor = InSetEditor;
	EdMode = InEdMode;
	ItemSetPtr = InItemSet;

	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(1024));
	ItemFilter = MakeShareable(new ItemTextFilter(
		ItemTextFilter::FItemToStringArray::CreateSP(this, &STiledPalette::GetPaletteFilterString)
	));

	CommandList = MakeShareable(new FUICommandList);

	TileViewWidget = SNew(STileView<TSharedPtr<FTiledItemViewData>>)
	.ListItemsSource(&FilteredItems)
	.SelectionMode(this, &STiledPalette::GetSelectionMode)
	.OnGenerateTile(this, &STiledPalette::GenerateTile)
	.OnContextMenuOpening(this, &STiledPalette::ConstructItemContextMenu)
	.OnSelectionChanged(this, &STiledPalette::OnSelectionChanged)
	.ItemHeight(this, &STiledPalette::GetThumbnailSize)
	.ItemWidth(this, &STiledPalette::GetThumbnailSize)
	.ItemAlignment(EListItemAlignment::LeftAligned)
	.ClearSelectionOnClick(true)
	.OnMouseButtonDoubleClick(this, &STiledPalette::OnItemDoubleClicked);


	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs Args;
	Args.bUpdatesFromSelection = false;
	Args.bLockable = false;
	Args.bAllowSearch = false;
	Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	Args.bHideSelectionTip = true;
	
	DetailsWidget = PropertyModule.CreateDetailView(Args);
	FStructureDetailsViewArgs StructViewArgs;
	CustomDataDetailsWidget = PropertyModule.CreateStructureDetailView(Args, StructViewArgs, ItemCustomDataRow);
	CustomDataDetailsWidget->GetOnFinishedChangingPropertiesDelegate().AddRaw(this, &STiledPalette::OnCustomDataEdited);

	SAssignNew(FillDetailWidget, SBorder)
	.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
    .Padding(3.f);

	SAssignNew(FillDetailTreeWidget, STreeView<TSharedPtr<FTiledItemViewData>>)
	.TreeItemsSource(&SelectedFillItems)
	.SelectionMode(ESelectionMode::None)
	.OnGenerateRow(this, &STiledPalette::GenerateFillItemRow)
	.OnGetChildren(this, &STiledPalette::GetFillItemChildren)
	.Visibility(this, &STiledPalette::GetFillWidgetVisibility)
	.HeaderRow
   (
       SNew(SHeaderRow)
       // No label should be good enough~
       + SHeaderRow::Column(
           FillDetailTreeColumns::ColumnID_Thumbnail)
       .HeaderContentPadding(
           FMargin(0, 1, 0, 1))
       .DefaultLabel(FText::GetEmpty())
       .HAlignHeader(HAlign_Center)

       + SHeaderRow::Column(
           FillDetailTreeColumns::ColumnID_RandomRotation)
       .DefaultLabel(
           LOCTEXT(
               "FillRandomRotation_Label",
               "Random Rotation?"))
       .DefaultTooltip(
           LOCTEXT(
               "FillWithRotationTooltip",
               "Place filled item with random yaw rotation?"))
       .HeaderContentPadding(
           FMargin(10, 1, 0, 1))
       .HAlignHeader(HAlign_Center)
       .HAlignCell(HAlign_Center)

       + SHeaderRow::Column(
           FillDetailTreeColumns::ColumnID_AllowOverlay)
       .DefaultLabel(
           LOCTEXT("AllowOverlay_Label",
               "Allow Overlay?"))
       .DefaultTooltip(
           LOCTEXT("AllowOverlayTooltip",
               "Allow overaly with other tile? (Still only one structural item per unit)"))
       .HeaderContentPadding(
           FMargin(10, 1, 0, 1))
       .HAlignHeader(HAlign_Center)
       .HAlignCell(HAlign_Center)

       + SHeaderRow::Column(
           FillDetailTreeColumns::ColumnID_Coefficient)
       .DefaultLabel(
           LOCTEXT("FillCoefficient_Label",
               "Coefficient | Actual Ratio"))
       .DefaultTooltip(
           LOCTEXT("FillCofficientTooltip",
               "Determine relative area filled with this item"))
       .HeaderContentPadding(
           FMargin(10, 1, 0, 1))
       .HAlignHeader(HAlign_Center)
       .HAlignCell(HAlign_Center)
       .VAlignCell(VAlign_Center)
   );


	FillDetailWidget->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
			.Text(LOCTEXT("Fill Detail Label", "Fill Detail"))
		]
		+ SVerticalBox::Slot()
		  .FillHeight(1.0)
		  .VAlign(VAlign_Center)
		  .HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
			.Text(LOCTEXT("SelectFillItemsLabel", "Select items to fill"))
			.ShadowOffset(FVector2D(1.f, 1.f))
			.Visibility(this, &STiledPalette::GetFillWidgetTooltipVisibility)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			FillDetailTreeWidget.ToSharedRef()
		]
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .Padding(0, 20)
		[
			SNew(SHorizontalBox)
			.Visibility(this, &STiledPalette::GetFillWidgetVisibility)

			+ SHorizontalBox::Slot()
			  .FillWidth(1.0)
			  .HAlign(HAlign_Right)
			  .VAlign(VAlign_Center)
			  .Padding(3, 0)
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &STiledPalette::OnEnableFillGapChanged)
				.IsChecked(this, &STiledPalette::GetEnableInsertGap)
			]
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .HAlign(HAlign_Right)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AddFillGapLabel", "Add fill gap?"))
			]
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .HAlign(HAlign_Right)
			  .VAlign(VAlign_Center)
			  .Padding(20, 0, 5, 0)
			[
				SNew(SNumericEntryBox<float>)
			   .MinDesiredValueWidth(72.f)
			   .AllowSpin(true)
			   .MinValue(0.0f)
			   .MaxValue(1.0f)
			   .MinSliderValue(0.0f)
			   .MaxSliderValue(1.0f)
			   .IsEnabled(this, &STiledPalette::CanSetGapCoefficient)
			   .OnValueChanged(this, &STiledPalette::OnGapCoefficientChanged)
			   .OnValueCommitted(this, &STiledPalette::OnGapCoefficientCommitted)
			   .Value(this, &STiledPalette::GetGapCoefficient)
			   .ToolTipText(LOCTEXT("Gap Coefficient Label", "Gap Coefficient"))
			]
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .HAlign(HAlign_Right)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &STiledPalette::GetGapActualRatioText)
			]
		]
	);

	SAssignNew(PaletteDetailSwitcher, SWidgetSwitcher);
	PaletteDetailSwitcher->AddSlot(ETiledPaletteDetailViewMode::ItemDetail)[DetailsWidget.ToSharedRef()];
	PaletteDetailSwitcher->AddSlot(ETiledPaletteDetailViewMode::FillDetail)[FillDetailWidget.ToSharedRef()];
	PaletteDetailSwitcher->SetActiveWidgetIndex(ETiledPaletteDetailViewMode::ItemDetail);

	// try to separate Palette Content based on editor
	TSharedPtr<SVerticalBox> PaletteContent = SNew(SVerticalBox);

	PaletteContent->AddSlot()
	              .AutoHeight()
	              .HAlign(HAlign_Fill)
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
		.Padding(FMargin(6.f, 2.f))
		.BorderBackgroundColor(FLinearColor(.6f, .6f, .6f, 1.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			  .HAlign(HAlign_Left)
			  .VAlign(VAlign_Center)
			  .Padding(2)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PaletteLabel", "Tiled Item Palette"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
				.ShadowOffset(FVector2D(1.0f, 1.0f))
			]
			+ SHorizontalBox::Slot()
			  .HAlign(HAlign_Right)
			  .Padding(2)
			  .FillWidth(1.0)
			[
				SAssignNew(SearchBox, SSearchBox)
				.MinDesiredWidth(150.f)
				.HintText(LOCTEXT("SearchItemHint", "Search Item"))
				.OnTextChanged(this, &STiledPalette::OnSearchTextChanged)
			]
			+ SHorizontalBox::Slot()
			  .HAlign(HAlign_Right)
			  .AutoWidth()
			[
				SNew(SComboButton)
				.ContentPadding(0)
				.ForegroundColor(FSlateColor::UseForeground())
				.ButtonStyle(FAppStyle::Get(), "ToggleButton")
				.OnGetMenuContent(this, &STiledPalette::GetViewOptionMenuContent)
				.ButtonContent()
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("GenericViewButton"))
				]
			]
			+ SHorizontalBox::Slot()
			  .HAlign(HAlign_Right)
			  .AutoWidth()
			[
				SNew(SComboButton)
				.Visibility(this, &STiledPalette::GetAddSpecialItemsVisibility)
				.ToolTipText(LOCTEXT("Add_Special_Items_ToolTip",
				                     "Add special items for particular purpose! Only restriction rule item is provided for now."))
				.ContentPadding(0)
				.ForegroundColor(FSlateColor::UseForeground())
				.ButtonStyle(FAppStyle::Get(), "ToggleButton")
				.OnGetMenuContent(this, &STiledPalette::GetAddSpecialItemsMenuContent)
				.ButtonContent()
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Plus"))
				]
			]
		]
	];

	// in set editor
	if (EdMode == nullptr)
	{
		PaletteContent->AddSlot()
		[
			SNew(SSplitter)
			.Orientation(Orient_Vertical)
			.Style(FAppStyle::Get(), "FoliageEditMode.Splitter")

			+ SSplitter::Slot()
			.Value(0.6f)
			[
				SNew(SOverlay)
				// actual Palettes
				+ SOverlay::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					  .AutoHeight()
					  .Padding(FMargin(6.f, 3.f))
					[
						SNew(SBox)
						.Visibility(this, &STiledPalette::GetDragItemHintVisibility)
						.Padding(FMargin(30, 0))
						.MinDesiredHeight(30)
						[
							SNew(SScaleBox)
							.Stretch(EStretch::ScaleToFit)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("Item_DropStatic", "+ Drop Item Here"))
								.ToolTipText(LOCTEXT("Item_DropStatic_ToolTip",
								                     "Drag and drop static meshed or actor from content browser to add a new tiled item"))
							]
						]
					]

					+ SVerticalBox::Slot()
					[
						TileViewWidget.ToSharedRef()
					]

					+ SVerticalBox::Slot()
					  .Padding(FMargin(3.f, 0.f))
					  .VAlign(VAlign_Bottom)
					  .HAlign(HAlign_Right)
					  .AutoHeight()
					[
						SNew(STextBlock).Text(this, &STiledPalette::GetCurrentItemName)
					]
				]
				// drag drop zone
				+ SOverlay::Slot()
				  .HAlign(HAlign_Fill)
				  .VAlign(VAlign_Fill)
				[
					SNew(STiledItemDragDropHandler)
					.Visibility(this, &STiledPalette::GetItemDropTargetVisibility)
					.OnDrop(this, &STiledPalette::HandleItemDropped)
					[
						SNew(SScaleBox)
						.Stretch(EStretch::ScaleToFit)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("TiledItem_AddTiledItem", "+ Tiled Item"))
							.ShadowOffset(FVector2D(1.f, 1.f))
						]
					]
				]
			]

			+ SSplitter::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					DetailsWidget.ToSharedRef()
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					CustomDataDetailsWidget->GetWidget().ToSharedRef()
				]
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .Padding(20, 4, 0, 4)
				[
					SNew(SBox)
					.HAlign(HAlign_Left)
					[
						SNew(SButton)
						.Text(LOCTEXT("TiledItem_CustomDataButtonText", "Create Row For Custom Data"))
						.ToolTipText(LOCTEXT("TiledItem_CustomDataButtonTooltipText",
						                     "Create a new empty row in your custom data table for this item"))
						.Visibility(this, &STiledPalette::GetCreateCustomDataRowButtonVisibility)
						.OnClicked(this, &STiledPalette::OnCreateNewEmptyCustomDataRowClicked)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(ItemPicker, STiledItemPicker)
					.Visibility(TAttribute<EVisibility>(this, &STiledPalette::GetItemPickerVisibility))
				]
			]
		];
		
		
	}
	else // Not allow to add new content
	{
		PaletteContent->AddSlot()
		[
			SNew(SSplitter)
			.Orientation(Orient_Vertical)
			.Style(FAppStyle::Get(), "FoliageEditMode.Splitter")

			+ SSplitter::Slot()
			.Value(0.6f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				[
					TileViewWidget.ToSharedRef()
				]

				+ SVerticalBox::Slot()
				  .Padding(FMargin(3.f, 0.f))
				  .VAlign(VAlign_Bottom)
				  .AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					  .Padding(FMargin(3.0f))
					  .VAlign(VAlign_Bottom)
					[
						SNew(STextBlock).Text(this, &STiledPalette::GetCurrentItemName)
					]

					+ SHorizontalBox::Slot()
					  .HAlign(HAlign_Right)
					  .AutoWidth()
					[
						SNew(SButton)
						.ToolTipText(this, &STiledPalette::GetShowHideDetailsTooltipText)
						.ForegroundColor(FSlateColor::UseForeground())
						.ButtonStyle(FAppStyle::Get(), "ToggleButton")
						.OnClicked(this, &STiledPalette::OnShowHideDetailsClicked)
						.ContentPadding(FMargin(2.f))
						.Content()
						[
							SNew(SHorizontalBox)

							// Details icon
							+ SHorizontalBox::Slot()
							  .AutoWidth()
							  .HAlign(HAlign_Center)
							  .VAlign(VAlign_Center)
							[
								SNew(SImage)
								.Image(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
							]

							// Arrow
							+ SHorizontalBox::Slot()
							  .Padding(FMargin(3.f, 0.f))
							  .AutoWidth()
							  .HAlign(HAlign_Center)
							  .VAlign(VAlign_Center)
							[
								SNew(SImage)
								.Image(this, &STiledPalette::GetShowHideDetailsImage)
							]
						]

					]
				]
			]
			+ SSplitter::Slot()
			[
				PaletteDetailSwitcher.ToSharedRef()
			]
		];
	}

	ChildSlot
	[
		PaletteContent.ToSharedRef()
	];

	UpdatePalette(true);
}

FReply STiledPalette::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (CommandList->ProcessCommandBindings(InKeyEvent)) return FReply::Handled();
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void STiledPalette::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);

	// make sure changes in set will reflect to level editor
	if (UTiledItemSet* ItemSet = ItemSetPtr.Get())
	{
		if (TempSetVersionNumber < ItemSet->VersionNumber)
			UpdatePalette(true);
	}
}

FReply STiledPalette::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsControlDown())
	{
		GetMutableDefault<UTiledLevelSettings>()->ThumbnailScale =
			FMath::Clamp(GetMutableDefault<UTiledLevelSettings>()->ThumbnailScale + 0.05f * MouseEvent.GetWheelDelta(),
			             0.0f, 1.0f);
		return FReply::Handled();
	}
	return SCompoundWidget::OnMouseWheel(MyGeometry, MouseEvent);
}

void STiledPalette::ChangeSet(UTiledItemSet* NewSet)
{
	ItemSetPtr = NewSet;
	UpdatePalette(true);
}

void STiledPalette::UpdatePalette(bool bRebuildItem)
{
	UTiledItemSet* ItemSet = ItemSetPtr.Get();
	if (!ItemSet)
	{
		TileViewWidget->ClearSelection();
		PaletteItems.Empty();
		FilteredItems.Empty();
		TileViewWidget->RequestListRefresh();
		return;
	}
	TempSetVersionNumber = ItemSet->VersionNumber;
	if (bRebuildItem)
	{
		auto PreviousSelectedItems = TileViewWidget->GetSelectedItems();
		TileViewWidget->ClearSelection();

		PaletteItems.Empty();
		for (UTiledLevelItem* Item : ItemSet->GetItemSet())
		{
			PaletteItems.Add(MakeShareable(new FTiledItemViewData(Item, EdMode, ThumbnailPool)));
			Item->RequestForRefreshPalette.BindSP(this, &STiledPalette::UpdatePalette, true);
		}

		if (PreviousSelectedItems.IsValidIndex(0))
		{
			for (auto Item : PaletteItems)
			{
				if (Item->GetDisplayName().ToString() == PreviousSelectedItems[0]->GetDisplayName().ToString())
				{
					TileViewWidget->SetItemSelection(Item, true);
					break;
				}
			}
		}
	}

	FilteredItems.Empty();
	for (auto& Item : PaletteItems)
	{
		if (ItemFilter->PassesFilter(Item) && PassesOptionFilter(Item->Item))
		{
			FilteredItems.Add(Item);
		}
	}

	auto SortFunction = [=](const TSharedPtr<FTiledItemViewData>& A, const TSharedPtr<FTiledItemViewData>& B)
	{
		UTiledLevelItem* ItemA = A->Item;
		UTiledLevelItem* ItemB = B->Item;
		if (ItemA->PlacedType != ItemB->PlacedType)
			return ItemA->PlacedType < ItemB->PlacedType;
		if (ItemA->SourceType != ItemB->SourceType)
			return ItemA->SourceType == ETLSourceType::Mesh;
		if (ItemA->StructureType != ItemB->StructureType)
			return ItemA->StructureType == ETLStructureType::Structure;
		return ItemA->GetItemName().Compare(ItemB->GetItemName()) < 0;
	};
	FilteredItems.Sort(SortFunction);

	TileViewWidget->RequestListRefresh();
}

void STiledPalette::SelectItem(UTiledLevelItem* NewItem)
{
	for (auto Data : FilteredItems)
	{
		if (Data->Item == NewItem)
		{
			TileViewWidget->ClearSelection();
			TileViewWidget->SetSelection(Data);
			return;
		}
	}
}

void STiledPalette::ClearItemSelection()
{
	TileViewWidget->ClearSelection();
	DetailsWidget->SetObject(nullptr);
}

void STiledPalette::SetThumbnailScale(float NewScale)
{
	GetMutableDefault<UTiledLevelSettings>()->ThumbnailScale = NewScale;
}

float STiledPalette::GetThumbnailScale() const
{
	return GetMutableDefault<UTiledLevelSettings>()->ThumbnailScale;
}

float STiledPalette::GetThumbnailSize() const
{
	const FInt32Interval SizeRange(32, 128);
	return SizeRange.Min + SizeRange.Size() * GetThumbnailScale();
}

bool STiledPalette::AnyTileHovered() const
{
	for (auto Item : FilteredItems)
	{
		TSharedPtr<ITableRow> Tile = TileViewWidget->WidgetFromItem(Item);
		if (Tile.IsValid() && Tile->AsWidget()->IsHovered())
		{
			return true;
		}
	}
	return false;
}

void STiledPalette::DisplayFillDetail(bool bShouldDisplay) const
{
	if (bShouldDisplay)
	{
		PaletteDetailSwitcher->SetActiveWidgetIndex(ETiledPaletteDetailViewMode::FillDetail);
	}
	else
	{
		PaletteDetailSwitcher->SetActiveWidgetIndex(ETiledPaletteDetailViewMode::ItemDetail);
	}
}


void STiledPalette::GetPaletteFilterString(TSharedPtr<FTiledItemViewData> Item, TArray<FString>& OutArray) const
{
	OutArray.Add(Item->GetDisplayName().ToString());
}

void STiledPalette::OnSearchTextChanged(const FText& InFilterText)
{
	ItemFilter->SetRawFilterText(InFilterText);
	SearchBox->SetError(ItemFilter->GetFilterErrorText());
	GetMutableDefault<UTiledLevelSettings>()->PaletteSearchText = InFilterText;
	UpdatePalette(false);
}

bool STiledPalette::PassesOptionFilter(UTiledLevelItem* InItem)
{
	TSet<EPlacedType> PlacedTypePassesOptions;
	TSet<ETLStructureType> StructureTypePassesOptions;
	TSet<ETLSourceType> SourceTypePassesOptions;

	UTiledLevelSettings* Settings = GetMutableDefault<UTiledLevelSettings>();
	if (Settings->IsItemsVisible_Block) PlacedTypePassesOptions.Add(EPlacedType::Block);
	if (Settings->IsItemsVisible_Floor) PlacedTypePassesOptions.Add(EPlacedType::Floor);
	if (Settings->IsItemsVisible_Wall) PlacedTypePassesOptions.Add(EPlacedType::Wall);
	if (Settings->IsItemsVisible_Edge) PlacedTypePassesOptions.Add(EPlacedType::Edge);
	if (Settings->IsItemsVisible_Pillar) PlacedTypePassesOptions.Add(EPlacedType::Pillar);
	if (Settings->IsItemsVisible_Point) PlacedTypePassesOptions.Add(EPlacedType::Point);
	if (Settings->IsItemsVisible_Structure) StructureTypePassesOptions.Add(ETLStructureType::Structure);
	if (Settings->IsItemsVisible_Prop) StructureTypePassesOptions.Add(ETLStructureType::Prop);
	if (Settings->IsItemsVisible_Actor) SourceTypePassesOptions.Add(ETLSourceType::Actor);
	if (Settings->IsItemsVisible_Mesh) SourceTypePassesOptions.Add(ETLSourceType::Mesh);

	bool PassEraserFilter = true;
	bool PassFillFilter = true;
	if (EdMode)
	{
		if (EdMode->ActiveEditTool == ETiledLevelEditTool::Eraser)
		{
			if (EdMode->EraserTarget != EPlacedType::Any)
				PassEraserFilter = InItem->PlacedType == EdMode->EraserTarget;
		}
		else if (EdMode->ActiveEditTool == ETiledLevelEditTool::Fill)
		{
			PassFillFilter = EdMode->IsFillTiles
				                 ? (InItem->PlacedType == EPlacedType::Block || InItem->PlacedType ==
					                 EPlacedType::Floor)
				                 : (InItem->PlacedType == EPlacedType::Edge || InItem->PlacedType == EPlacedType::Wall);
		}
	}

	return
		PlacedTypePassesOptions.Contains(InItem->PlacedType) &&
		StructureTypePassesOptions.Contains(InItem->StructureType) &&
		SourceTypePassesOptions.Contains(InItem->SourceType) &&
		PassEraserFilter && PassFillFilter;
}

FText STiledPalette::GetCurrentItemName() const
{
	if (TileViewWidget->GetSelectedItems().Num() == 1)
	{
		UTiledLevelItem* SelectedItem = TileViewWidget->GetSelectedItems()[0]->Item;
		return FText::FromString(SelectedItem->GetItemName());
	}
	else if (TileViewWidget->GetSelectedItems().Num() > 1)
	{
		return FText::Format(
			LOCTEXT("CurrentItemNameLabel", "{0} Items Selected"),
			FText::AsNumber(TileViewWidget->GetSelectedItems().Num()));
	}
	return FText::GetEmpty();
}

TSharedRef<SWidget> STiledPalette::GetViewOptionMenuContent()
{
	FMenuBuilder MenuBuilder(true, CommandList);
	MenuBuilder.BeginSection("TiledItemFilterOption", LOCTEXT("FilterOptionHeading", "Filter Option"));
	{
		MenuBuilder.AddWidget(
			SNew(SBox)
			.Padding(FMargin(15, 0, 0, 0))
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(1, 3))
				+ SUniformGridPanel::Slot(0, 0)
				  .VAlign(VAlign_Center)
				  .HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PlacedTypeFilterOptionLabel_Block", "Block"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				]
				+ SUniformGridPanel::Slot(1, 0)
				  .VAlign(VAlign_Center)
				  .HAlign(HAlign_Left)
				[
					SNew(SCheckBox)
					.IsChecked_Raw(this, &STiledPalette::GetFilterOptionState_Block)
					.OnCheckStateChanged_Raw(this, &STiledPalette::SetFilterOptionState_Block)
				]
				+ SUniformGridPanel::Slot(0, 1)
				  .VAlign(VAlign_Center)
				  .HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PlacedTypeFilterOptionLabel_Floor", "Floor"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				]
				+ SUniformGridPanel::Slot(1, 1)
				  .VAlign(VAlign_Center)
				  .HAlign(HAlign_Left)
				[
					SNew(SCheckBox)
					.IsChecked_Raw(this, &STiledPalette::GetFilterOptionState_Floor)
					.OnCheckStateChanged_Raw(this, &STiledPalette::SetFilterOptionState_Floor)
				]
				+ SUniformGridPanel::Slot(2, 0)
				  .VAlign(VAlign_Center)
				  .HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PlacedTypeFilterOptionLabel_Wall", "Wall"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				]
				+ SUniformGridPanel::Slot(3, 0)
				  .VAlign(VAlign_Center)
				  .HAlign(HAlign_Left)
				[
					SNew(SCheckBox)
					.IsChecked_Raw(this, &STiledPalette::GetFilterOptionState_Wall)
					.OnCheckStateChanged_Raw(this, &STiledPalette::SetFilterOptionState_Wall)
				]
				+ SUniformGridPanel::Slot(2, 1)
				  .HAlign(HAlign_Right)
				  .VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PlacedTypeFilterOptionLabel_Edge", "Edge"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				]
				+ SUniformGridPanel::Slot(3, 1)
				  .HAlign(HAlign_Left)
				  .VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.IsChecked_Raw(this, &STiledPalette::GetFilterOptionState_Edge)
					.OnCheckStateChanged_Raw(this, &STiledPalette::SetFilterOptionState_Edge)
				]
				+ SUniformGridPanel::Slot(4, 0)
				  .HAlign(HAlign_Right)
				  .VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PlacedTypeFilterOptionLabel_Pillar", "Pillar"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				]
				+ SUniformGridPanel::Slot(5, 0)
				  .HAlign(HAlign_Left)
				  .VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.IsChecked_Raw(this, &STiledPalette::GetFilterOptionState_Pillar)
					.OnCheckStateChanged_Raw(this, &STiledPalette::SetFilterOptionState_Pillar)
				]
				+ SUniformGridPanel::Slot(4, 1)
				  .HAlign(HAlign_Right)
				  .VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PlacedTypeFilterOptionLabel_Point", "Point"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				]
				+ SUniformGridPanel::Slot(5, 1)
				  .HAlign(HAlign_Left)
				  .VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.IsChecked_Raw(this, &STiledPalette::GetFilterOptionState_Point)
					.OnCheckStateChanged_Raw(this, &STiledPalette::SetFilterOptionState_Point)
				]
			],
			LOCTEXT("PlacedTypeFilterLabel", "     Placed Type:")
		);

		MenuBuilder.AddWidget(
			SNew(SUniformGridPanel)
			.SlotPadding(1)
			+ SUniformGridPanel::Slot(0, 0)
			  .VAlign(VAlign_Center)
			  .HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("StructureTypeFilterOptionLabel_Structure", "Struct"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			]
			+ SUniformGridPanel::Slot(1, 0)
			  .VAlign(VAlign_Center)
			  .HAlign(HAlign_Left)
			[
				SNew(SCheckBox)
				.IsChecked_Raw(this, &STiledPalette::GetFilterOptionState_Structure)
				.OnCheckStateChanged_Raw(this, &STiledPalette::SetFilterOptionState_Structure)
			]
			+ SUniformGridPanel::Slot(2, 0)
			  .VAlign(VAlign_Center)
			  .HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("StructureTypeFilterOptionLabel_Prop", "Prop"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			]
			+ SUniformGridPanel::Slot(3, 0)
			  .VAlign(VAlign_Center)
			  .HAlign(HAlign_Left)
			[
				SNew(SCheckBox)
				.IsChecked_Raw(this, &STiledPalette::GetFilterOptionState_Prop)
				.OnCheckStateChanged_Raw(this, &STiledPalette::SetFilterOptionState_Prop)
			]
			,
			LOCTEXT("IsPropFilterLabel", "Structure Type:")
		);

		MenuBuilder.AddWidget(
			SNew(SUniformGridPanel)
			.SlotPadding(1)
			+ SUniformGridPanel::Slot(0, 0)
			  .VAlign(VAlign_Center)
			  .HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SourceTypeFilterOptionLabel_Actor", " Actor"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			]
			+ SUniformGridPanel::Slot(1, 0)
			  .VAlign(VAlign_Center)
			  .HAlign(HAlign_Left)
			[
				SNew(SCheckBox)
				.IsChecked_Raw(this, &STiledPalette::GetFilterOptionState_Actor)
				.OnCheckStateChanged_Raw(this, &STiledPalette::SetFilterOptionState_Actor)
			]
			+ SUniformGridPanel::Slot(2, 0)
			  .VAlign(VAlign_Center)
			  .HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SourceTypeFilterOptionLabel_Mesh", "Mesh"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			]
			+ SUniformGridPanel::Slot(3, 0)
			  .VAlign(VAlign_Center)
			  .HAlign(HAlign_Left)
			[
				SNew(SCheckBox)
				.IsChecked_Raw(this, &STiledPalette::GetFilterOptionState_Mesh)
				.OnCheckStateChanged_Raw(this, &STiledPalette::SetFilterOptionState_Mesh)
			]
			,
			LOCTEXT("IsActorFilterLabel", "    Source Type:")
		);
	}
	MenuBuilder.EndSection();


	MenuBuilder.BeginSection("TiledItemViewOption", LOCTEXT("ViewOptionHeading", "View Option"));
	{
		MenuBuilder.AddWidget(
			SNew(SBox)
			.WidthOverride(150.0f)
			[
				SNew(SSlider)
				.ToolTipText(LOCTEXT("ThumbnailScaleToolTip", "Adjust the size of thumbnails."))
				.Value(this, &STiledPalette::GetThumbnailScale)
				.OnValueChanged(this, &STiledPalette::SetThumbnailScale)
			],
			LOCTEXT("ThumbnailSizeLabel", "Thumbnail Size"),
			true
		);
	}
	MenuBuilder.EndSection();
	return MenuBuilder.MakeWidget();
}

TSharedPtr<SWidget> STiledPalette::ConstructItemContextMenu()
{
	FMenuBuilder MenuBuilder(true, CommandList);
	const FGenericCommands& GenericCommands = FGenericCommands::Get();
	CommandList->MapAction(GenericCommands.Delete, FExecuteAction::CreateRaw(this, &STiledPalette::RemoveTiledItem));
	FSlateIcon DummyIcon(NAME_None, NAME_None);

	if (TileViewWidget->GetSelectedItems().Num() > 0)
	{
		MenuBuilder.BeginSection("InstanceActionOptions", LOCTEXT("InstanceActionOptionsHeader", "Action"));
		{
			if (SetEditor.IsValid())
			{
				MenuBuilder.AddMenuEntry(
					GenericCommands.Delete,
					NAME_None,
					LOCTEXT("DeleteItemAction", "Delete Item"),
					LOCTEXT("DeleteItemActionTooltip", "Completely delete this tiled item and remove all its instance"),
					DummyIcon);
			}
			else
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("RemoveInThisFloorAction", "Remove in this floor"),
					LOCTEXT("RemoveInThisfloorTooltip", "Remove instances of this item in selected floor"),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateRaw(EdMode, &FTiledLevelEdMode::ClearSelectedItemInstancesInActiveFloor),
						FCanExecuteAction::CreateRaw(
							EdMode, &FTiledLevelEdMode::CanClearSelectedItemInstancesInActiveFloor)
					),
					NAME_None
				);

				MenuBuilder.AddMenuEntry(
					LOCTEXT("ClearItemInstanceAction", "Clear all"),
					LOCTEXT("ClearItemInstanceActionTooltip", "Clear all instances of this item"),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateRaw(EdMode, &FTiledLevelEdMode::ClearSelectedItemInstances)
					),
					NAME_None
				);
			}
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection("EraserActionOptions", LOCTEXT("EraserActionOptionsHeader", "Eraser Options"));
		{
			if (!SetEditor.IsValid())
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("AllowedToErase_Lable", "Allowed To Erase"),
					LOCTEXT("AllowedToErase_Tooltip", "Set allowed to erase this item"),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateLambda([this]()
						{
							for (auto& Data : TileViewWidget->GetSelectedItems())
							{
								Data->Item->bIsEraserAllowed = true;
							}
						}),
						FCanExecuteAction::CreateLambda([this]()
						{
							return EdMode->ActiveEditTool == ETiledLevelEditTool::Eraser;
						})
					),
					NAME_None
				);

				MenuBuilder.AddMenuEntry(
					LOCTEXT("NotAllowedToErae_Lable", "Not Allowed To Erase"),
					LOCTEXT("NotAllowedToErase_Tooltip", "Set not allowed to erase this item"),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateLambda([this]()
						{
							for (auto& Data : TileViewWidget->GetSelectedItems())
							{
								Data->Item->bIsEraserAllowed = false;
							}
						}),
						FCanExecuteAction::CreateLambda([this]()
						{
							return EdMode->ActiveEditTool == ETiledLevelEditTool::Eraser;
						})
					),
					NAME_None
				);
			}
		}
		MenuBuilder.EndSection();
	}
	return MenuBuilder.MakeWidget();
}

FText STiledPalette::GetShowHideDetailsTooltipText() const
{
	return DetailsWidget->GetVisibility() != EVisibility::Collapsed
		       ? LOCTEXT("HideDetails_Tooltip", "Hide details for the selected item.")
		       : LOCTEXT("ShowDetails_Tooltip", "Show details for the selected item.");
}

FReply STiledPalette::OnShowHideDetailsClicked() const
{
	const bool DetailWidgetCurrentlyVisible = PaletteDetailSwitcher->GetVisibility() != EVisibility::Collapsed;
	PaletteDetailSwitcher->SetVisibility(DetailWidgetCurrentlyVisible
		                                     ? EVisibility::Collapsed
		                                     : EVisibility::SelfHitTestInvisible);
	return FReply::Handled();
}

const FSlateBrush* STiledPalette::GetShowHideDetailsImage() const
{
	const bool bDetailsCurrentlyVisible = DetailsWidget->GetVisibility() != EVisibility::Collapsed;
	return FAppStyle::Get().GetBrush(bDetailsCurrentlyVisible ? "Symbols.DoubleDownArrow" : "Symbols.DoubleUpArrow");
}

void STiledPalette::RemoveTiledItem()
{
	if (ItemSetPtr.Get())
	{
		const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "TiledLevel_RemoveItemTransaction", "Remove Item"));
		ItemSetPtr->Modify();
		for (auto SelectedItem : TileViewWidget->GetSelectedItems())
			ItemSetPtr->RemoveItem(SelectedItem->Item);
		UpdatePalette(true);
	}
}

ESelectionMode::Type STiledPalette::GetSelectionMode() const
{
	if (!EdMode) return ESelectionMode::Multi;
	switch (EdMode->ActiveEditTool)
	{
	case Select:
		return ESelectionMode::None;
	case Eraser:
		return ESelectionMode::Multi;
	case Fill:
		return ESelectionMode::Multi;
	default:
		return ESelectionMode::Single;
	}
}

TSharedRef<ITableRow> STiledPalette::GenerateTile(TSharedPtr<FTiledItemViewData> InData,
                                                  const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STiledPaletteItem, InData, OwnerTable, EdMode);
}

void STiledPalette::OnSelectionChanged(TSharedPtr<FTiledItemViewData> InData, ESelectInfo::Type SelectInfo)
{
	if (EdMode)
	{
		EdMode->SelectedItems.Empty();
		SelectedFillItems.Empty();
		DetailsWidget->SetObject(nullptr);
		CustomDataDetailsWidget->SetStructureData(nullptr);
		EdMode->ActiveItem = nullptr;
		if (TileViewWidget->GetNumItemsSelected() == 1)
		{
			EdMode->ActiveItem = InData->Item;
			InData->Item->IsEditInSet = false;
			DetailsWidget->SetObject(InData->Item);
		}
		for (auto& SelectedItem : TileViewWidget->GetSelectedItems())
		{
			EdMode->SelectedItems.Add(SelectedItem->Item);
			SelectedFillItems.Add(SelectedItem);
		}
		FillDetailTreeWidget->RequestTreeRefresh();
	}
	// In tiled item set editor (no edmode)
	else
	{
		if (TileViewWidget->GetNumItemsSelected() == 0)
		{
			DetailsWidget->SetObject(nullptr);
			CustomDataDetailsWidget->SetStructureData(nullptr);
			SetEditor.Pin().Get()->UpdatePreviewScene(nullptr);
		}
		else if (TileViewWidget->GetNumItemsSelected() == 1)
		{
			InData->Item->IsEditInSet = true;
			DetailsWidget->SetObject(InData->Item);
		    // TODO: no custom datatable for? special items?
			if (ItemSetPtr->CustomDataTable)
			{
				ItemCustomDataRow = MakeShareable(
					new FStructFromDataTable(ItemSetPtr->CustomDataTable, FName(InData->Item->ItemID.ToString())));
				CustomDataDetailsWidget->SetStructureData(ItemCustomDataRow);
			}
			SetEditor.Pin().Get()->UpdatePreviewScene(InData->Item);
		    if (UTiledLevelRestrictionItem* RestrictionItem = Cast<UTiledLevelRestrictionItem>(InData->Item))
		    {
		        ItemPicker->SetContent(&RestrictionItem->TargetItems, ItemSetPtr->GetItemSet(), ThumbnailPool);
		    }
		}
		else
		{
			TArray<UObject*> SelectedTiledItems;
			for (auto Data : TileViewWidget->GetSelectedItems())
			{
				Data->Item->IsEditInSet = true;
				SelectedTiledItems.Add(Data->Item);
			}
			DetailsWidget->SetObjects(SelectedTiledItems);
			CustomDataDetailsWidget->SetStructureData(nullptr);
		}
	}
}

void STiledPalette::OnItemDoubleClicked(TSharedPtr<FTiledItemViewData> InData) const
{
	UTiledLevelItem* Item = InData->Item;
    if (Item->IsA<UTiledLevelRestrictionItem>()) return;
    if (UTiledLevelTemplateItem* TemplateItem = Cast<UTiledLevelTemplateItem>(Item))
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(TemplateItem->GetAsset());
	else if (Item->SourceType == ETLSourceType::Actor)
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Item->TiledActor);
	else
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Item->TiledMesh);
}

#define DEFINE_FILTER_FUNCTIONS(Target) \
ECheckBoxState STiledPalette::GetFilterOptionState_##Target() const\
{\
	return GetMutableDefault<UTiledLevelSettings>()->IsItemsVisible_##Target? ECheckBoxState::Checked : ECheckBoxState::Unchecked;\
}\
void STiledPalette::SetFilterOptionState_##Target(ECheckBoxState NewState)\
{\
	GetMutableDefault<UTiledLevelSettings>()->IsItemsVisible_##Target = NewState == ECheckBoxState::Checked? true : false;\
	UpdatePalette(false);\
}

DEFINE_FILTER_FUNCTIONS(Block)
DEFINE_FILTER_FUNCTIONS(Floor)
DEFINE_FILTER_FUNCTIONS(Wall)
DEFINE_FILTER_FUNCTIONS(Edge)
DEFINE_FILTER_FUNCTIONS(Pillar)
DEFINE_FILTER_FUNCTIONS(Point)
DEFINE_FILTER_FUNCTIONS(Structure)
DEFINE_FILTER_FUNCTIONS(Prop)
DEFINE_FILTER_FUNCTIONS(Actor)
DEFINE_FILTER_FUNCTIONS(Mesh)

EVisibility STiledPalette::GetDragItemHintVisibility() const
{
	return FilteredItems.Num() == 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

TSharedRef<SWidget> STiledPalette::CreateDropConfirmDialogContent()
{
	TSharedRef<STreeView<TSharedPtr<FString>>> NewElementPropertyTable = SNew(STreeView<TSharedPtr<FString>>)
		.TreeItemsSource(&NewItemData_SourceName)
		.SelectionMode(ESelectionMode::None)
		.OnGenerateRow_Lambda([=](TSharedPtr<FString> InData, const TSharedRef<STableViewBase>& OutTable)
		{
			int i = 0;
			for (TSharedPtr<FString> Name : NewItemData_SourceName)
			{
				if (*InData.Get() == *Name.Get()) break;
				i++;
			}
			return SNew(SNewItemDropConfirmItemRow, InData, OutTable, &NewItemData_PlacedType[i],
			            &NewItemData_StructureType[i], &NewItemData_Extent[i]);
		})
		.OnGetChildren_Lambda([=](TSharedPtr<FString> InData, TArray<TSharedPtr<FString>>& OutChildren)
		{
		})
		.HeaderRow
		(
			// Toggle Active
			SAssignNew(DropConfirmHeaderRow, SHeaderRow)

			// ID
			+ SHeaderRow::Column(FName("Name"))
			  .HeaderContentPadding(FMargin(10, 1, 0, 1))
			  .DefaultLabel(LOCTEXT("NewItem_AssetName", "AssetName"))
			  .FillWidth(1.f)

			// Placed Type
			+ SHeaderRow::Column(FName("PlacedType"))
			  .HeaderContentPadding(FMargin(10, 1, 0, 1))
			  .DefaultLabel(LOCTEXT("NewItem_PlacedType", "Placed Type"))
			  .FillWidth(1.f)

			// IsProp
			+ SHeaderRow::Column(FName("StructureType"))
			  .HeaderContentPadding(FMargin(10, 1, 0, 1))
			  .DefaultLabel(LOCTEXT("NewItem_StructureType", "Structure Type"))
			  .FillWidth(1.f)

			+ SHeaderRow::Column(FName("Extent"))
			  .HeaderContentPadding(FMargin(10, 1, 0, 1))
			  .DefaultLabel(LOCTEXT("NewItem_Extent", "Extent"))
			  .FillWidth(1.f)

		);

	return SNew(SBox)
	.Padding(8)
	.MinDesiredWidth(720)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .Padding(50, 5, 50, 10)
		[
			SNew(STextBlock).Text(FText::Format(
				                LOCTEXT("InitAddNewElement", "Add {0} elements?"),
				                FText::AsNumber(NewItemData_SourceName.Num())))
			                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			NewElementPropertyTable
		]
	];
}

EVisibility STiledPalette::GetItemDropTargetVisibility() const
{
	if (FSlateApplication::Get().IsDragDropping())
	{
		TArray<FAssetData> DraggedAssets = AssetUtil::ExtractAssetDataFromDrag(
			FSlateApplication::Get().GetDragDroppingContent());
		int N_Pass = 0;
		for (const FAssetData& AssetData : DraggedAssets)
		{
			if (AssetData.IsValid() && AssetData.GetClass()->IsChildOf(UStaticMesh::StaticClass()))
			{
				N_Pass += 1;
			}

			if (AssetData.IsValid())
			{
				if (UBlueprint* BP = Cast<UBlueprint>(AssetData.GetAsset()))
				{
					
					if (BP->ParentClass->IsChildOf(AActor::StaticClass()))
					{
						// TODO: add more is not child of ??
						if ( BP->ParentClass->IsChildOf(AController::StaticClass()) || BP->ParentClass->IsChildOf(AInfo::StaticClass()))
							  N_Pass += 0;
						else
							N_Pass += 1;
					}
				}
			}
		}
		if (N_Pass == DraggedAssets.Num()) return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}

FReply STiledPalette::HandleItemDropped(const FGeometry& DropZoneGeometry, const FDragDropEvent& DragDropEvent)
{
	UTiledItemSet* CurrentSet = ItemSetPtr.Get();

	TArray<FAssetData> ValidAssetData = AssetUtil::ExtractAssetDataFromDrag(DragDropEvent).FilterByPredicate(
		[=](FAssetData AssetData)
		{
			if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset()))
			{
				return !CurrentSet->GetAllItemMeshes().Contains(StaticMesh);
			}
			if (UBlueprint* BP = Cast<UBlueprint>(AssetData.GetAsset()))
			{
				return !CurrentSet->GetAllItemBlueprintObject().Array().Contains(BP->GeneratedClass);
			}
			return false;
		});

	const int N = ValidAssetData.Num();
	const float PredictionThreshold = CurrentSet->AutoExtentPredictionThreshold;
	const FVector TileSize = CurrentSet->GetTileSize();
	FVector TempExtent;
	if (N > 0)
	{
		NewItemData_SourceName.Empty();
		NewItemData_PlacedType.Empty();
		NewItemData_Extent.Empty();

		for (auto& AssetData : ValidAssetData)
		{
			NewItemData_SourceName.Emplace(MakeShareable(new FString(AssetData.GetAsset()->GetName())));
			if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset()))
			{
				FVector MeshBoundExtent = StaticMesh->GetBounds().BoxExtent * 2;

				if (MeshBoundExtent.X < TileSize.X * 0.40 && MeshBoundExtent.Y < TileSize.Y * 0.40 && MeshBoundExtent.Z
					< TileSize.Y * 0.40 && CurrentSet->IsPointIncluded)
				{
					NewItemData_PlacedType.Emplace(EPlacedType::Point);
					TempExtent = FVector(1, 1, 1);
				}
				// predict for pillar
				else if (MeshBoundExtent.X < TileSize.X * 0.40 && MeshBoundExtent.Y < TileSize.Y * 0.40 && CurrentSet->
					IsPillarIncluded)
				{
					NewItemData_PlacedType.Emplace(EPlacedType::Pillar);
					TempExtent = FVector(
						1, 1, int((MeshBoundExtent.Z - (PredictionThreshold * TileSize.Z)) / TileSize.Z + 1));
				}
				// predict for edge
				else if (MeshBoundExtent.Z < TileSize.Z * 0.40 &&
					((MeshBoundExtent.X > TileSize.X * 0.80 && MeshBoundExtent.Y < TileSize.Y * 0.40) ||
						(MeshBoundExtent.X < TileSize.X * 0.40 && MeshBoundExtent.Y > TileSize.Y * 0.80)) &&
					CurrentSet->IsEdgeIncluded
				)
				{
					NewItemData_PlacedType.Emplace(EPlacedType::Edge);
					int MaxWidth = FMath::Max(
						(MeshBoundExtent.X - (PredictionThreshold * TileSize.X)) / TileSize.X + 1,
						(MeshBoundExtent.Y - (PredictionThreshold * TileSize.Y)) / TileSize.Y + 1
					);
					TempExtent = FVector(MaxWidth, MaxWidth, 1);
				}
				// predict for wall
				else if ((MeshBoundExtent.X / MeshBoundExtent.Y > 2.f || MeshBoundExtent.X / MeshBoundExtent.Y < 0.5f)
					&& CurrentSet->IsWallIncluded)
				{
					NewItemData_PlacedType.Emplace(EPlacedType::Wall);
					int MaxWidth = FMath::Max(
						(MeshBoundExtent.X - (PredictionThreshold * TileSize.X)) / TileSize.X + 1,
						(MeshBoundExtent.Y - (PredictionThreshold * TileSize.Y)) / TileSize.Y + 1
					);
					TempExtent = FVector(
						MaxWidth,
						MaxWidth,
						int((MeshBoundExtent.Z - (PredictionThreshold * TileSize.Z)) / TileSize.Z + 1)
					);
				}
				// predict for floor
				else if (MeshBoundExtent.X > TileSize.X * 0.80 && MeshBoundExtent.Y > TileSize.Y * 0.80 &&
					MeshBoundExtent.Z < TileSize.Z * 0.5
					&& CurrentSet->IsFloorIncluded)
				{
					NewItemData_PlacedType.Emplace(EPlacedType::Floor);
					TempExtent = FVector(
						int((MeshBoundExtent.X - (PredictionThreshold * TileSize.X)) / TileSize.X + 1),
						int((MeshBoundExtent.Y - (PredictionThreshold * TileSize.Y)) / TileSize.Y + 1),
						1
					);
				}
				else // block
				{
					NewItemData_PlacedType.Emplace(EPlacedType::Block);
					TempExtent = FVector(
						int((MeshBoundExtent.X - (PredictionThreshold * TileSize.X)) / TileSize.X + 1),
						int((MeshBoundExtent.Y - (PredictionThreshold * TileSize.Y)) / TileSize.Y + 1),
						int((MeshBoundExtent.Z - (PredictionThreshold * TileSize.Z)) / TileSize.Z + 1)
					);
				}
				NewItemData_Extent.Emplace(
					FVector(FMath::Clamp<int>(TempExtent.X, 1, 16),
					        FMath::Clamp<int>(TempExtent.Y, 1, 16),
					        FMath::Clamp<int>(TempExtent.Z, 1, 16)));
			}
			else // For actor-type
			{
				NewItemData_PlacedType.Emplace(EPlacedType::Block);
				NewItemData_Extent.Emplace(FVector(1));
			}
		}
		NewItemData_StructureType.Init(CurrentSet->DefaultStructureType, N);


		SGenericDialogWidget::FArguments DialogArguments;
		DialogArguments.OnOkPressed_Lambda([=]()
		{
			// Treat the entire drop as a transaction (in case multiples types are being added)
			const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "TiledLevelMode_DragDropTypesTransaction",
			                                               "Drag-drop Items"));
			ItemSetPtr->Modify();
			int i = 0;
			for (auto& AssetData : ValidAssetData)
			{
				if (UStaticMesh* NewMesh = Cast<UStaticMesh>(AssetData.GetAsset()))
				{
					CurrentSet->AddNewItem(NewMesh, NewItemData_PlacedType[i], NewItemData_StructureType[i],
					                       NewItemData_Extent[i]);
				}
				else
				{
					CurrentSet->AddNewItem(AssetData.GetAsset(), NewItemData_PlacedType[i],
					                       NewItemData_StructureType[i],
					                       NewItemData_Extent[i]);
				}
				i++;
			}
			ItemSetPtr->VersionNumber += 10;
			UpdatePalette(true);
		});
		SGenericDialogWidget::OpenDialog(FText::FromString("Add New Items"), CreateDropConfirmDialogContent(), DialogArguments, true);

	}
	return FReply::Unhandled();
}

TSharedRef<ITableRow> STiledPalette::GenerateFillItemRow(TSharedPtr<FTiledItemViewData> InData,
                                                         const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STiledPaletteFillDetailRow, OwnerTable, InData, EdMode);
}

void STiledPalette::GetFillItemChildren(TSharedPtr<FTiledItemViewData> InData,
                                        TArray<TSharedPtr<FTiledItemViewData>>& OutChildren)
{
}

EVisibility STiledPalette::GetFillWidgetVisibility() const
{
	return TileViewWidget->GetNumItemsSelected() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility STiledPalette::GetFillWidgetTooltipVisibility() const
{
	return TileViewWidget->GetNumItemsSelected() == 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

void STiledPalette::OnEnableFillGapChanged(ECheckBoxState NewState)
{
	if (EdMode)
		EdMode->EnableFillGap = NewState == ECheckBoxState::Checked;
}

ECheckBoxState STiledPalette::GetEnableInsertGap() const
{
	if (EdMode)
		return EdMode->EnableFillGap ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	return ECheckBoxState::Undetermined;
}

bool STiledPalette::CanSetGapCoefficient() const
{
	if (EdMode)
		return EdMode->EnableFillGap;
	return false;
}

void STiledPalette::OnGapCoefficientChanged(float NewValue)
{
	if (EdMode)
		EdMode->GapCoefficient = NewValue;
}

void STiledPalette::OnGapCoefficientCommitted(float NewValue, ETextCommit::Type CommitType)
{
	if (EdMode)
		EdMode->GapCoefficient = NewValue;
}

TOptional<float> STiledPalette::GetGapCoefficient() const
{
	if (EdMode)
		return EdMode->GapCoefficient;
	return 0.f;
}

FText STiledPalette::GetGapActualRatioText() const
{
	if (!EdMode->EnableFillGap) return FText::GetEmpty();
	float TotalWeight = 0.000001f;
	for (auto item : EdMode->SelectedItems)
	{
		if (item->bAllowOverlay) continue;
		TotalWeight += item->FillCoefficient;
	}
	TotalWeight += EdMode->GapCoefficient;
	float MyRatio = EdMode->GapCoefficient / TotalWeight;
	FText RatioText = UKismetTextLibrary::Conv_DoubleToText(MyRatio * 100, ERoundingMode::HalfToZero, false, true, 1, 3, 2, 2);
	return FText::Format(LOCTEXT("Actual Fill Ratio", "({0} %)"), RatioText);
}

EVisibility STiledPalette::GetCreateCustomDataRowButtonVisibility() const
{
	auto SelectedItems = TileViewWidget->GetSelectedItems();
	if (ItemSetPtr->CustomDataTable && SelectedItems.Num() == 1)
	{
		if (SelectedItems[0]->Item->IsA<UTiledLevelRestrictionItem>())
			return EVisibility::Collapsed;
		if (SelectedItems[0]->Item->IsA<UTiledLevelTemplateItem>())
			return EVisibility::Collapsed;
		FName ItemID = FName(SelectedItems[0]->Item->ItemID.ToString());
		if (!ItemSetPtr->CustomDataTable->FindRowUnchecked(ItemID))
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

FReply STiledPalette::OnCreateNewEmptyCustomDataRowClicked()
{
	FString ItemID = TileViewWidget->GetSelectedItems()[0]->Item->ItemID.ToString();
	FString RawCSVData = ItemSetPtr->CustomDataTable->GetTableAsCSV();
	RawCSVData.Append("\n");
	RawCSVData.Append(ItemID);
	const UScriptStruct* RowStruct = ItemSetPtr->CustomDataTable->GetRowStruct();
	int NumProperties = 0;
	for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
	{
		NumProperties += 1;
	}
	for (int n = 0; n < NumProperties; n++)
	{
		RawCSVData.Append(",");
		RawCSVData.Append("\"\"");
	}
	ItemSetPtr->CustomDataTable->CreateTableFromCSVString(RawCSVData);
	ItemSetPtr->CustomDataTable->MarkPackageDirty();
	ItemCustomDataRow = MakeShareable(new FStructFromDataTable(ItemSetPtr->CustomDataTable, FName(ItemID)));
	CustomDataDetailsWidget->SetStructureData(ItemCustomDataRow);
	return FReply::Handled();
}

void STiledPalette::OnCustomDataEdited(const FPropertyChangedEvent& PropertyChangedEvent)
{
	ItemSetPtr->CustomDataTable->MarkPackageDirty();
}

EVisibility STiledPalette::GetAddSpecialItemsVisibility() const
{
	if (EdMode) return EVisibility::Collapsed;
	return EVisibility::Visible;
}

TSharedRef<SWidget> STiledPalette::GetAddSpecialItemsMenuContent()
{
	FMenuBuilder MenuBuilder(true, CommandList);

	MenuBuilder.BeginSection("InstanceActionOptions", LOCTEXT("InstanceActionOptionsHeader", "Action"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("AddGametimeRestrictionRuleAction", "Add restriction rules"),
			LOCTEXT("AddGametimeRestrictionRuleTooltip",
			        "Restriction rules allow you to define areas where allow or disallow to build or remove item during game time."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateRaw(this, &STiledPalette::AddRestrictionItem),
				FCanExecuteAction()
			),
			NAME_None
		);
		MenuBuilder.AddMenuEntry(
			LOCTEXT("AddTemplateAction", "Add Templates"),
			LOCTEXT("AddTemplatesTooltip",
			        "Add template item to reuse existing tiled level asset."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateRaw(this, &STiledPalette::AddTemplateItem),
				FCanExecuteAction()
			),
			NAME_None
		);
	}
	MenuBuilder.EndSection();
	return MenuBuilder.MakeWidget();
}

void STiledPalette::AddRestrictionItem()
{
	UTiledItemSet* CurrentSet = ItemSetPtr.Get();
	CurrentSet->AddSpecialItem_Restriction();
	ItemSetPtr->VersionNumber += 5;
	UpdatePalette(true);
}

void STiledPalette::AddTemplateItem()
{
	UTiledItemSet* CurrentSet = ItemSetPtr.Get();
	CurrentSet->AddSpecialItem_Template();
	ItemSetPtr->VersionNumber += 5;
	UpdatePalette(true);
}


EVisibility STiledPalette::GetItemPickerVisibility() const
{
	if (TileViewWidget->GetNumItemsSelected() == 1)
	{
		if (UTiledLevelRestrictionItem* Restriction = Cast<UTiledLevelRestrictionItem>(TileViewWidget->GetSelectedItems()[0]->Item))
		{
			if (!Restriction->bTargetAllItems) return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
#undef LOCTEXT_NAMESPACE
