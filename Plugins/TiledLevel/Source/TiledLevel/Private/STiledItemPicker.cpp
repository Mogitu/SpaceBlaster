// Copyright 2022 PufStudio. All Rights Reserved.


#include "STiledItemPicker.h"
#include "TiledLevelItem.h"
#include "PropertyCustomizationHelpers.h"
#include "SlateOptMacros.h"
#include "STiledPalette.h"
#include "TiledItemSet.h"
#include "TiledLevelEditorLog.h"
#include "Widgets/Layout/SScrollBox.h"

#define LOCTEXT_NAMESPACE "TiledItemPicker"

void STiledItemPicker::Construct(const FArguments& InArgs)
{
    
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .VAlign(VAlign_Fill)
        [
            SAssignNew(Content, SVerticalBox)
        ]
    ];
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

// add drag drop?
void STiledItemPicker::SetContent(TArray<FGuid>* InPickedItemIDs, const TArray<UTiledLevelItem*>& InItemOptions,
    const TSharedPtr<FAssetThumbnailPool>& InThumbnailPool)
{
    PickedItemIDs = InPickedItemIDs;
    ItemOptions = InItemOptions;
    ThumbnailPool = InThumbnailPool;
    RefreshContent();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

const FSlateBrush* STiledItemPicker::GetExpanderImage() const
{

	FName ResourceName;
	if (!IsCollapsed)
	{
		if ( ExpanderArrow->IsHovered() )
		{
			static FName ExpandedHoveredName = "TreeArrow_Expanded_Hovered";
			ResourceName = ExpandedHoveredName;
		}
		else
		{
			static FName ExpandedName = "TreeArrow_Expanded";
			ResourceName = ExpandedName;
		}
	}
	else
	{
		if ( ExpanderArrow->IsHovered() )
		{
			static FName CollapsedHoveredName = "TreeArrow_Collapsed_Hovered";
			ResourceName = CollapsedHoveredName;
		}
		else
		{
			static FName CollapsedName = "TreeArrow_Collapsed";
			ResourceName = CollapsedName;
		}
	}

	return FEditorStyle::Get().GetBrush(ResourceName);
}

FReply STiledItemPicker::OnExpanderArrowClicked()
{
    IsCollapsed = IsCollapsed? false : true;
    return FReply::Handled();
}

EVisibility STiledItemPicker::GetChildVisibility() const
{
    return IsCollapsed? EVisibility::Collapsed : EVisibility::Visible;
}

FText STiledItemPicker::GetTargetItemName(int Nth) const
{
    if (Nth == INDEX_NONE)
        return FText::FromString("None");
    return FText::FromString(ItemOptions[Nth]->GetItemName());
}

TSharedRef<SWidget> STiledItemPicker::MakeAddItemMenu(int Nth)
{
    FMenuBuilder MenuBuilder(true, nullptr, nullptr, true);
    MenuBuilder.BeginSection("Item List", LOCTEXT("ItemlistLabel", "Item List"));
    {
        for (UTiledLevelItem* Item : ItemOptions)
        {
            if (PickedItemIDs->Contains(Item->ItemID))
                continue;
            FAssetData AssetData = FAssetData(Item);
            TSharedPtr<FAssetThumbnail> Thumbnail = MakeShareable(
                new FAssetThumbnail(AssetData, 64, 64, ThumbnailPool));
            
            MenuBuilder.AddWidget(
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SBox)
                    .WidthOverride(64.f)
                    .HeightOverride(64.f)
                    [
                        Thumbnail->MakeThumbnailWidget()
                    ]
                ]
                + SHorizontalBox::Slot()
                .VAlign(VAlign_Fill)
                .HAlign(HAlign_Fill)
                [
                    SNew(SButton)
                    .ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
                    .ForegroundColor(FSlateColor::UseForeground())
                    .VAlign(VAlign_Center)
                    .HAlign(HAlign_Left)
                    .ContentPadding(FMargin(6,0))
                    .OnClicked_Lambda([=]()
                    {
                        const FScopedTransaction Transaction(LOCTEXT("TiledLevel_SetTargetItemTransaction", "Set target Item"));
                        Cast<UTiledItemSet>(ItemOptions[0]->GetOuter())->Modify();
                        (*PickedItemIDs)[Nth] = Item->ItemID;
                        RefreshContent();
                        return FReply::Handled();
                    })
                    [
                        SNew(STextBlock).Text(FText::FromString(Item->GetItemName()))
                    ]
                ],
                FText::GetEmpty()
                );
        }
    }
    MenuBuilder.EndSection();
    
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .MaxHeight(600.f)
        [
            MenuBuilder.MakeWidget()
        ];
}

void STiledItemPicker::RefreshContent()
{
    Content->ClearChildren();

    // the first header row
    Content->AddSlot()
    .AutoHeight()
    [
        SNew(SBorder)
        .Padding(6.f)
        .BorderImage(FEditorStyle::GetBrush("DetailsView.CategoryTop"))
        .BorderBackgroundColor(FLinearColor(.6f, .6f, .6f, 1.0f))
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .Padding(4.f, 0.f)
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                // copy from SExpanderArrow.cpp 
                SAssignNew(ExpanderArrow, SButton)
                .ButtonStyle( FCoreStyle::Get(), "NoBorder" )
                .VAlign(VAlign_Center)
                .HAlign(HAlign_Center)
                .ClickMethod(EButtonClickMethod::MouseDown)
                .OnClicked(this, &STiledItemPicker::OnExpanderArrowClicked)
                .ContentPadding(0.f)
                .ForegroundColor( FSlateColor::UseForeground() )
                .IsFocusable( false )
                .Content()
                [
                    SNew(SImage)
                    .Image(this, &STiledItemPicker::GetExpanderImage)
                    .ColorAndOpacity(FSlateColor::UseForeground())
                ]
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(SBox)
                .WidthOverride(300.f)
                [
                    SNew(STextBlock).Text(FText::FromString("Targeted Items"))
                ]
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d items picked"), PickedItemIDs->Num())))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                PropertyCustomizationHelpers::MakeAddButton(
                    FSimpleDelegate::CreateLambda([this]()
                    {
                        const FScopedTransaction Transaction(LOCTEXT("TiledLevel_AddTargetItemTransaction", "Add target Item"));
                        Cast<UTiledItemSet>(ItemOptions[0]->GetOuter())->Modify();
                        PickedItemIDs->Add(FGuid::NewGuid());
                        RefreshContent();
                    }),
                    LOCTEXT("AddNewTargetItemTooltip", "Add new target item.")
                )
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                PropertyCustomizationHelpers::MakeEmptyButton(
                    FSimpleDelegate::CreateLambda([this]()
                    {
                        const FScopedTransaction Transaction(LOCTEXT("TiledLevel_EmptyTargetItemsTransaction", "Empty target Items"));
                        Cast<UTiledItemSet>(ItemOptions[0]->GetOuter())->Modify();
                        PickedItemIDs->Empty();
                        RefreshContent();
                    }),
                    LOCTEXT("RemoveTargetItemsTooltip", "Remove all target items.")
                )
            ]
        ]
    ];

    
    Content->AddSlot()
    .FillHeight(1.0f)
    .MaxHeight(450.f)
    [
        SNew(SScrollBox)
        +SScrollBox::Slot()
        [
            SAssignNew(OptionRows, SVerticalBox)
        ]
    ];

    for (int i = 0; i < PickedItemIDs->Num(); i++)
    {
        const int FoundIndex = ItemOptions.IndexOfByPredicate([=](UTiledLevelItem* Item)
        {
           return Item->ItemID == (*PickedItemIDs)[i];
        });
        FAssetData AssetData;
        if (FoundIndex != INDEX_NONE)
            AssetData = FAssetData(ItemOptions[FoundIndex]);
        TSharedPtr<FAssetThumbnail> Thumbnail = MakeShareable(
            new FAssetThumbnail(AssetData, 64, 64, ThumbnailPool));
        TSharedPtr<SWidget> ThumbnailWidget = Thumbnail->MakeThumbnailWidget();
        
        OptionRows->AddSlot()
        .Padding(24.f, 6.f, 6.f, 6.f)
        .AutoHeight()
        [
            SNew(SHorizontalBox)
            .Visibility(this, &STiledItemPicker::GetChildVisibility)
            + SHorizontalBox::Slot()
            .VAlign(VAlign_Center)
            .AutoWidth()
            .Padding(0, 0, 120, 0)
            [
                SNew(STextBlock).Text(FText::FromString(FString::FromInt(i)))
            ]
            + SHorizontalBox::Slot()
            .VAlign(VAlign_Center)
            .AutoWidth()
            [
                SNew(SBox)
                .WidthOverride(64.f)
                .HeightOverride(64.f)
                [
                    ThumbnailWidget.ToSharedRef()
                ]
            ]
            + SHorizontalBox::Slot()
            .VAlign(VAlign_Center)
            .AutoWidth()
            .Padding(8, 0)
            [
                SNew(SBox)
                .WidthOverride(240.f)
                [
                    SNew(SComboButton)
                    .HasDownArrow(true)
                    .ContentPadding(FMargin(6.f, 2.f))
                    .OnGetMenuContent(this, &STiledItemPicker::MakeAddItemMenu, i)
                    .ButtonContent()
                    [
                        SNew(STextBlock).Text(this, &STiledItemPicker::GetTargetItemName, FoundIndex)
                    ]
                ]
            ]
            + SHorizontalBox::Slot()
            .VAlign(VAlign_Center)
            .AutoWidth()
            [
                PropertyCustomizationHelpers::MakeRemoveButton(
                    FSimpleDelegate::CreateLambda([=]()
                    {
                        const FScopedTransaction Transaction(LOCTEXT("TiledLevel_RemoveTargetItemTransaction", "remove target Item"));
                        Cast<UTiledItemSet>(ItemOptions[0]->GetOuter())->Modify();
                        PickedItemIDs->RemoveAt(i);
                        RefreshContent();
                    }),
                    LOCTEXT("RemoveTargetItemTooltip", "Remove target item")
                    )
            ]
        ];
    }
}


#undef LOCTEXT_NAMESPACE
