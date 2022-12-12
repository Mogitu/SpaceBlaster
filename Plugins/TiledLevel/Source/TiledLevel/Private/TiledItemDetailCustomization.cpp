// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledItemDetailCustomization.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IAssetTools.h"
#include "IDetailGroup.h"
#include "TiledLevelAsset.h"
#include "TiledLevelEditor/TiledLevelEdMode.h"
#include "TiledLevelItem.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "PropertyCustomizationHelpers.h"
#include "TiledItemSet.h"
#include "TiledLevelEditorLog.h"
#include "TiledLevelRestrictionHelper.h"
#include "TiledLevelSettings.h"
#include "Factories/BlueprintFactory.h"
#include "GameFramework/Info.h"

#define LOCTEXT_NAMESPACE "TiledLevel"

UTiledLevelItem* FTiledItemDetailCustomization::CustomizedItem;
TArray<UTiledLevelItem*> FTiledItemDetailCustomization::CustomizedItems;

void FTiledItemDetailCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	CustomizedItem = nullptr;
	CustomizedItems.Empty(); 
	TArray<TWeakObjectPtr<UObject>> Temp;
	DetailBuilder.GetObjectsBeingCustomized( Temp);
	for (auto T : Temp)
		CustomizedItems.Add(Cast<UTiledLevelItem>(T.Get()));
	if (CustomizedItems.IsValidIndex(0))
		CustomizedItem = CustomizedItems[0];
	MyDetailLayout = &DetailBuilder;
    
	if (CustomizedItem->IsA<UTiledLevelRestrictionItem>())
	{
		CustomizeRestrictionItem(DetailBuilder, Cast<UTiledLevelRestrictionItem>(CustomizedItem));
		return;
	}
    if (CustomizedItem->IsA<UTiledLevelTemplateItem>())
    {
		CustomizeTemplateItem(DetailBuilder, Cast<UTiledLevelTemplateItem>(CustomizedItem));
        return;
    }
        
	DetailBuilder.EditCategory("Actor");
	DetailBuilder.EditCategory("Mesh");
	IDetailCategoryBuilder& TiledItemCategory = DetailBuilder.EditCategory("TiledItem");
	IDetailCategoryBuilder& PlacementCategory = DetailBuilder.EditCategory("Placement");
	Property_PlacedType = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, PlacedType));
	Property_StructureType = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, StructureType));
	Property_Extent = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, Extent));
	DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, PivotPosition)));
	UnavailableSourceNames.Empty();

	TAttribute<EVisibility> PivotOptionsVisibility  = TAttribute<EVisibility>::Create(
		TAttribute<EVisibility>::FGetter::CreateLambda([]()
		{
			if (CustomizedItems.Num() == 0 || CustomizedItem == nullptr) return EVisibility::Collapsed;
			bool SamePlacedType = true;
			for (UTiledLevelItem* Item : CustomizedItems)
			{
				if (CustomizedItem->PlacedType != Item->PlacedType)
				{
					SamePlacedType = false;
					break;
				}
			}
			return SamePlacedType? EVisibility::Visible : EVisibility::Collapsed;
		}));
	
	TSharedRef<SHorizontalBox> PivotOptions = SNew(SHorizontalBox)
	.Visibility(PivotOptionsVisibility)
	+ SHorizontalBox::Slot()
	.FillWidth(1.0)
	[
		SNew(SCheckBox)
		.Style(FAppStyle::Get(), "RadioButton")
		.Visibility(this, &FTiledItemDetailCustomization::GetPivotOptionVisibility, EPivotPosition::Bottom)
		.IsChecked(this, &FTiledItemDetailCustomization::GetPivotOptionCheckState, EPivotPosition::Bottom)
		.OnCheckStateChanged(this, &FTiledItemDetailCustomization::OnPivotOptionCheckStateChanged,EPivotPosition::Bottom)
		.ToolTipText(LOCTEXT("AutoPivotTooltip_Bottom", "Set pivot point to bottom"))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("CheckboxLabel_Bottom", "Bottom"))
		]
	]
	+ SHorizontalBox::Slot()
	.FillWidth(1.0)
	[
		SNew(SCheckBox)
		.Style(FAppStyle::Get(), "RadioButton")
		.Visibility(this, &FTiledItemDetailCustomization::GetPivotOptionVisibility, EPivotPosition::Center)
		.IsChecked(this, &FTiledItemDetailCustomization::GetPivotOptionCheckState, EPivotPosition::Center)
		.OnCheckStateChanged(this, &FTiledItemDetailCustomization::OnPivotOptionCheckStateChanged,EPivotPosition::Center)
		.ToolTipText(LOCTEXT("AutoPivotTooltip_center","Set pivot point to center"))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("CheckboxLabel_Center", "Center"))
		]
	]
	+ SHorizontalBox::Slot()
	.FillWidth(1.0)
	[
		SNew(SCheckBox)
		.Style(FAppStyle::Get(), "RadioButton")
		.Visibility(this, &FTiledItemDetailCustomization::GetPivotOptionVisibility, EPivotPosition::Corner)
		.IsChecked(this, &FTiledItemDetailCustomization::GetPivotOptionCheckState, EPivotPosition::Corner)
		.OnCheckStateChanged(this, &FTiledItemDetailCustomization::OnPivotOptionCheckStateChanged,EPivotPosition::Corner)
		.ToolTipText(LOCTEXT("AutoPivotTooltip_Corner", "Set pivot point to corner"))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("CheckboxLabel_Corner", "Corner"))
		]
	]
	+ SHorizontalBox::Slot()
	.FillWidth(1.0)
	[
		SNew(SCheckBox)
		.Style(FAppStyle::Get(), "RadioButton")
		.Visibility(this, &FTiledItemDetailCustomization::GetPivotOptionVisibility, EPivotPosition::Side)
		.IsChecked(this, &FTiledItemDetailCustomization::GetPivotOptionCheckState, EPivotPosition::Side)
		.OnCheckStateChanged(this, &FTiledItemDetailCustomization::OnPivotOptionCheckStateChanged,EPivotPosition::Side)
		.ToolTipText(LOCTEXT("AutoPivotTooltip_Side", "Set pivot point to Side"))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("CheckboxLabel_Side", "Side"))
		]
	]
	+ SHorizontalBox::Slot()
	.FillWidth(1.0)
	[
		SNew(SCheckBox)
		.Style(FAppStyle::Get(), "RadioButton")
		.Visibility(this, &FTiledItemDetailCustomization::GetPivotOptionVisibility, EPivotPosition::Fit)
		.IsChecked(this, &FTiledItemDetailCustomization::GetPivotOptionCheckState, EPivotPosition::Fit)
		.OnCheckStateChanged(this, &FTiledItemDetailCustomization::OnPivotOptionCheckStateChanged,EPivotPosition::Fit)
		.ToolTipText(LOCTEXT("AutoPivotTooltip_Fit", "Fit the mesh to tile extent as possible!"))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("CheckboxLabel_Fit", "Fit"))
		]
	];

	Property_StructureType->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([this]()
	{
		UTiledLevelSettings* Settings = GetMutableDefault<UTiledLevelSettings>();
		for (const auto Item : CustomizedItems)
		{
			Item->PivotPosition = Settings->DefaultPlacedTypePivotPositionMap[CustomizedItem->PlacedType];
		}
		MyDetailLayout->ForceRefreshDetails();
	}));
	Property_PlacedType->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([this]()
	{
		UTiledLevelSettings* Settings = GetMutableDefault<UTiledLevelSettings>();
		for (const auto Item : CustomizedItems)
		{
			Item->PivotPosition = Settings->DefaultPlacedTypePivotPositionMap[CustomizedItem->PlacedType];
		}
		MyDetailLayout->ForceRefreshDetails();
	}));
	

	bool IsSourceMixed = false;
	for (UTiledLevelItem* Item : CustomizedItems)
	{
		if (CustomizedItem->SourceType != Item->SourceType)
		{
			IsSourceMixed = true;
			break;
		}
	}

	if (IsSourceMixed)
	{
		DetailBuilder.HideCategory(FName(TEXT("Mesh")));
		DetailBuilder.HideCategory(FName(TEXT("Actor")));
		DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, bAutoPlacement)));
		DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, bHeightOverride)));
		DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, NewHeight)));
		DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, bThicknessOverride)));
		DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, NewThickness)));
		DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, bSnapToFloor)));
		DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, bSnapToWall)));

		PlacementCategory.AddCustomRow(FText::GetEmpty())
		[
			SNew(SBox).Padding(FMargin(30, 0))
			[
				PivotOptions
			]
		];
	}
	else if (CustomizedItem->SourceType == ETLSourceType::Actor)
	{
		DetailBuilder.HideCategory(FName(TEXT("Mesh")));

		// Customized Actor input widget
		if (CustomizedItems.Num() > 1 || !CustomizedItem->IsEditInSet)
		{
			DetailBuilder.HideCategory(FName("Actor"));
		}
		else
		{
			if (UTiledItemSet* SourceSet = Cast<UTiledItemSet>(CustomizedItem->GetOuter()))
			{
				for (UObject* ActorObject : SourceSet->GetAllItemBlueprintObject())
				{
					if (IsValid(ActorObject))
						UnavailableSourceNames.Add(ActorObject->GetFName());
				}
			}

			TSharedRef<IPropertyHandle> ActorPropertyHandle = DetailBuilder.GetProperty(
				GET_MEMBER_NAME_CHECKED(UTiledLevelItem, TiledActor));
			IDetailPropertyRow& ActorRow = DetailBuilder.EditCategory("Actor").AddProperty(ActorPropertyHandle);

			TSharedPtr<SWidget> NameWidget;
			TSharedPtr<SWidget> ValueWidget;
			FDetailWidgetRow Row;
			ActorRow.GetDefaultWidgets(NameWidget, ValueWidget, Row);

			ActorRow.CustomWidget()
					.NameContent()
					.MinDesiredWidth(Row.NameWidget.MinWidth)
					.MaxDesiredWidth(Row.NameWidget.MaxWidth)
				[
					NameWidget.ToSharedRef()
				]
				.ValueContent()
				.MinDesiredWidth(Row.ValueWidget.MinWidth)
				.MaxDesiredWidth(Row.ValueWidget.MaxWidth)
				[
					// ObjectPropertyEntryBox treat "Actor" as special case..., that I can not get what I what
					// So, I finally change the property type from TSubclass<AActor> to UObject*, and the problem is solved
					SNew(SObjectPropertyEntryBox)
				.AllowedClass(UBlueprint::StaticClass())
				.PropertyHandle(ActorPropertyHandle)
				.ThumbnailPool(DetailBuilder.GetThumbnailPool())
				.AllowClear(false)
				.DisplayUseSelected(true)
				.DisplayBrowse(true) // enable this could open weird tab..., its not a big deal compares to its merit?
				.NewAssetFactories(TArray<UFactory*>{}) // inconvenient but the most safe way, not allowed to create new asset here
				.OnShouldFilterAsset(this, &FTiledItemDetailCustomization::OnShouldFilterActorAsset)
				];
		}
		
		DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, bAutoPlacement)));
		DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, bHeightOverride)));
		DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, NewHeight)));
		DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, bThicknessOverride)));
		DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, NewThickness)));
		DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, bSnapToFloor)));
		DetailBuilder.HideProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledLevelItem, bSnapToWall)));
		
		PlacementCategory.AddCustomRow(FText::GetEmpty())
		[
			SNew(SBox).Padding(FMargin(30, 0))
			[
				PivotOptions
			]
		];
	}
	else
	{
		DetailBuilder.HideCategory(FName(TEXT("Actor")));

		if (CustomizedItems.Num() > 1)
		{
			DetailBuilder.HideCategory(FName(TEXT("Mesh")));
		}
		else
		{
			// Customized Mesh input widget
			if (UTiledItemSet* SourceSet = Cast<UTiledItemSet>(CustomizedItem->GetOuter()))
			{
				for (UStaticMesh* UsedMesh : SourceSet->GetAllItemMeshes())
				{
					if (UsedMesh)
						  UnavailableSourceNames.Add(UsedMesh->GetFName());
				}
			}

			TSharedRef<IPropertyHandle> MeshPropertyHandle = DetailBuilder.GetProperty(
				GET_MEMBER_NAME_CHECKED(UTiledLevelItem, TiledMesh));
			IDetailPropertyRow& MeshRow = DetailBuilder.EditCategory("Mesh").AddProperty(MeshPropertyHandle);

			TSharedPtr<SWidget> NameWidget;
			TSharedPtr<SWidget> ValueWidget;
			FDetailWidgetRow Row;
			MeshRow.GetDefaultWidgets(NameWidget, ValueWidget, Row);

			MeshRow.CustomWidget()
				   .NameContent()
				   .MinDesiredWidth(Row.NameWidget.MinWidth)
				   .MaxDesiredWidth(Row.NameWidget.MaxWidth)
				[
					NameWidget.ToSharedRef()
				]
				.ValueContent()
				.MinDesiredWidth(Row.ValueWidget.MinWidth)
				.MaxDesiredWidth(Row.ValueWidget.MaxWidth)
				[
					SNew(SObjectPropertyEntryBox)
				.AllowedClass(UStaticMesh::StaticClass())
				.PropertyHandle(MeshPropertyHandle)
				.ThumbnailPool(DetailBuilder.GetThumbnailPool())
				.AllowClear(false)
				.DisplayUseSelected(true)
				.DisplayBrowse(true) // enable this could open weird tab... , its not a big deal compares to its merit?
				.OnShouldFilterAsset(this, &FTiledItemDetailCustomization::OnShouldFilterMeshAsset)
				];
		}
		TSharedPtr<IPropertyHandle> PropertyHandle_bAutoPlacement = DetailBuilder.GetProperty(
			GET_MEMBER_NAME_CHECKED(UTiledLevelItem, bAutoPlacement));
		PropertyHandle_bAutoPlacement->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([this]()
		{
			UTiledLevelSettings* Settings = GetMutableDefault<UTiledLevelSettings>();
			if (CustomizedItem->PivotPosition == EPivotPosition::Fit)
			{
				for (const auto Item : CustomizedItems)
				{
					 Item->PivotPosition = Settings->DefaultPlacedTypePivotPositionMap[CustomizedItem->PlacedType];
				}
				MyDetailLayout->ForceRefreshDetails();
			}
		}));
			
		PlacementCategory.AddProperty(PropertyHandle_bAutoPlacement);
		PlacementCategory.AddCustomRow(FText::GetEmpty())
		[
			SNew(SBox).Padding(FMargin(30, 0))
			[
				PivotOptions
			]
		];

		// fit bound overrides
		TSharedPtr<IPropertyHandle> Property_HeightOverride = DetailBuilder.GetProperty(
			GET_MEMBER_NAME_CHECKED(UTiledLevelItem, bHeightOverride));
		TSharedPtr<IPropertyHandle> Property_NewHeight = DetailBuilder.GetProperty(
			GET_MEMBER_NAME_CHECKED(UTiledLevelItem, NewHeight));
		TSharedPtr<IPropertyHandle> Property_ThicknessOverride = DetailBuilder.GetProperty(
			GET_MEMBER_NAME_CHECKED(UTiledLevelItem, bThicknessOverride));
		TSharedPtr<IPropertyHandle> Property_NewThickness = DetailBuilder.GetProperty(
			GET_MEMBER_NAME_CHECKED(UTiledLevelItem, NewThickness));

		bool IsAllFit = true;
		bool IsAllFloor = true;
		bool IsAllWall = true;
		for (UTiledLevelItem* Item : CustomizedItems)
		{
			if (Item->PivotPosition != EPivotPosition::Fit)
			{
				IsAllFit = false;
				break;
			}
		}
		for (UTiledLevelItem* Item : CustomizedItems)
		{
			if (Item->PlacedType != EPlacedType::Floor)
			{
				IsAllFloor = false;
				break;
			}
		}
		for (UTiledLevelItem* Item : CustomizedItems)
		{
			if (Item->PlacedType != EPlacedType::Wall)
			{
				IsAllWall = false;
				break;
			}
		}
		if (IsAllFit)
		{
			if (IsAllFloor)
			{
				PlacementCategory.AddCustomRow(FText::GetEmpty())
				.NameContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						Property_HeightOverride->CreatePropertyValueWidget()
					]
					+ SHorizontalBox::Slot()
					.Padding(5, 0, 0, 0)
					[
						Property_HeightOverride->CreatePropertyNameWidget()
					]
				]
				.ValueContent()
				[
					Property_NewHeight->CreatePropertyValueWidget()
				];
			}
			else if (IsAllWall) // height override, floor only
			{
				PlacementCategory.AddCustomRow(FText::GetEmpty())
				.NameContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						Property_ThicknessOverride->CreatePropertyValueWidget()
					]
					+ SHorizontalBox::Slot()
					.Padding(5, 0, 0, 0)
					[
						Property_ThicknessOverride->CreatePropertyNameWidget()
					]
				]
				.ValueContent()
				[
					Property_NewThickness->CreatePropertyValueWidget()
				];
			}
		}

		// hide these properties, since I've create custom widget...
		DetailBuilder.HideProperty(Property_HeightOverride);
		DetailBuilder.HideProperty(Property_NewHeight);
		DetailBuilder.HideProperty(Property_ThicknessOverride);
		DetailBuilder.HideProperty(Property_NewThickness);

		// Auto Snap
		TSharedPtr<IPropertyHandle> Property_bSnapToFloor = DetailBuilder.GetProperty(
			GET_MEMBER_NAME_CHECKED(UTiledLevelItem, bSnapToFloor));
		TSharedPtr<IPropertyHandle> Property_bSnapToWall = DetailBuilder.GetProperty(
			GET_MEMBER_NAME_CHECKED(UTiledLevelItem, bSnapToWall));
		IDetailGroup& AutoSnap_Group = PlacementCategory.AddGroup(FName("AutoSnap"),
			FText::FromString("AutoSnap"),false, true);
			
		AutoSnap_Group.AddPropertyRow(Property_bSnapToFloor.ToSharedRef());
		AutoSnap_Group.AddWidgetRow()
		.NameContent()
		[
			Property_bSnapToWall->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.IsEnabled_Lambda([this]()
			{
				for (UTiledLevelItem* Item : CustomizedItems)
				{
					if (Item->PlacedType != EPlacedType::Wall || Item->StructureType != ETLStructureType::Prop)
						return false;
				}
				return true;
			})
			.IsChecked_Lambda([=]()
			{
				bool V;
				Property_bSnapToWall->GetValue(V);
				return V? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([=](ECheckBoxState InCheckBoxState)
			{
				InCheckBoxState == ECheckBoxState::Checked?
					Property_bSnapToWall->SetValue(true) : Property_bSnapToWall->SetValue(false);
			})
		];
		DetailBuilder.HideProperty(Property_bSnapToWall);
	}

	bool IsAllSamePlacedType = true;
	for (UTiledLevelItem* Item : CustomizedItems)
	{
		if (Item->PlacedType != CustomizedItem->PlacedType)
		{
			IsAllSamePlacedType = false;
			break;
		}
	}

	if (IsAllSamePlacedType)
	{
		IDetailPropertyRow& ExtentRow = TiledItemCategory.AddProperty(Property_Extent);
		TSharedRef<SWidget> X_Label = SNumericEntryBox<int>::BuildLabel(FText::FromString("X"), FLinearColor::White,
																		SNumericEntryBox<int>::RedLabelBackgroundColor);
		TSharedRef<SWidget> Y_Label = SNumericEntryBox<int>::BuildLabel(FText::FromString("Y"), FLinearColor::White,
																		SNumericEntryBox<int>::GreenLabelBackgroundColor);
		TSharedRef<SWidget> Z_Label = SNumericEntryBox<int>::BuildLabel(FText::FromString("Z"), FLinearColor::White,
																		SNumericEntryBox<int>::BlueLabelBackgroundColor);

		TSharedRef<SNumericEntryBox<int>> Extent_XInput = SNew(SNumericEntryBox<int>)
			.AllowSpin(true)
			.MinValue(0)
			.MaxValue(16)
			.MinSliderValue(1)
			.MaxSliderValue(16)
			.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
			.Value(this, &FTiledItemDetailCustomization::GetExtentValue, EAxis::X)
			.OnValueChanged(this, &FTiledItemDetailCustomization::OnExtentChanged, EAxis::X)
			.OnValueCommitted(this, &FTiledItemDetailCustomization::OnExtentCommitted, EAxis::X)
			.Label()[X_Label]
			.LabelPadding(0);

		TSharedRef<SNumericEntryBox<int>> Extent_YInput = SNew(SNumericEntryBox<int>)
			.AllowSpin(true)
			.MinValue(0)
			.MaxValue(16)
			.MinSliderValue(1)
			.MaxSliderValue(16)
			.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
			.Value(this, &FTiledItemDetailCustomization::GetExtentValue, EAxis::Y)
			.OnValueChanged(this, &FTiledItemDetailCustomization::OnExtentChanged, EAxis::Y)
			.OnValueCommitted(this, &FTiledItemDetailCustomization::OnExtentCommitted, EAxis::Y)
			.Label()[Y_Label]
			.LabelPadding(0);
		
		TSharedRef<SNumericEntryBox<int>> Extent_ZInput = SNew(SNumericEntryBox<int>)
			.AllowSpin(true)
			.MinValue(0)
			.MaxValue(16)
			.MinSliderValue(1)
			.MaxSliderValue(16)
			.UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
			.Value(this, &FTiledItemDetailCustomization::GetExtentValue, EAxis::Z)
			.OnValueChanged(this, &FTiledItemDetailCustomization::OnExtentChanged, EAxis::Z)
			.OnValueCommitted(this, &FTiledItemDetailCustomization::OnExtentCommitted, EAxis::Z)
			.Label()[Z_Label]
			.LabelPadding(0);
		
		TSharedRef<SHorizontalBox> ExtentInputs = SNew(SHorizontalBox);
		
		switch (CustomizedItem->PlacedType)
		{
		case EPlacedType::Block:
			ExtentInputs->AddSlot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			.Padding(0.0f, 1.0f, 4.0f, 1.0f)
			[
				Extent_XInput
			];
			ExtentInputs->AddSlot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			.Padding(0.0f, 1.0f, 4.0f, 1.0f)
			[
				Extent_YInput
			];
			ExtentInputs->AddSlot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			.Padding(0.0f, 1.0f, 4.0f, 1.0f)
			[
				Extent_ZInput
			];
			break;
			
		case EPlacedType::Floor:
			ExtentInputs->AddSlot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			.Padding(0.0f, 1.0f, 4.0f, 1.0f)
			[
				Extent_XInput
			];
			ExtentInputs->AddSlot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			.Padding(0.0f, 1.0f, 4.0f, 1.0f)
			[
				Extent_YInput
			];
			break;
		case EPlacedType::Wall:
			ExtentInputs->AddSlot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			.Padding(0.0f, 1.0f, 4.0f, 1.0f)
			[
				Extent_XInput
			];

			ExtentInputs->AddSlot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			.Padding(0.0f, 1.0f, 4.0f, 1.0f)
			[
				Extent_ZInput
			];
			break;
		case EPlacedType::Pillar:
			ExtentInputs->AddSlot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			.Padding(0.0f, 1.0f, 4.0f, 1.0f)
			[
				Extent_ZInput
			];
			break;
		case EPlacedType::Edge:
			ExtentInputs->AddSlot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			.Padding(0.0f, 1.0f, 4.0f, 1.0f)
			[
				Extent_XInput
			];
			break;
		default: ;
		}
		
		ExtentRow.CustomWidget()
				 .NameContent()
			[
				Property_Extent->CreatePropertyNameWidget()
			]
			.ValueContent()
			.MinDesiredWidth(400.0f)
			.MinDesiredWidth(400.0f)
			[
				ExtentInputs
			];
	}
	else
	{
		DetailBuilder.HideProperty(Property_Extent);
	}

	if (!CustomizedItem->IsEditInSet)
	{
		DetailBuilder.HideCategory("TiledItem");
		DetailBuilder.HideCategory("Mesh");
		DetailBuilder.HideCategory("Actor");
	}
}

TSharedRef<IDetailCustomization> FTiledItemDetailCustomization::MakeInstance()
{
	return MakeShareable(new FTiledItemDetailCustomization);
}

void FTiledItemDetailCustomization::CustomizeRestrictionItem(IDetailLayoutBuilder& DetailBuilder,
	UTiledLevelRestrictionItem* RestrictionItem)
{
	DetailBuilder.HideCategory(FName(TEXT("TiledItem")));
	DetailBuilder.HideCategory(FName(TEXT("Mesh")));
	DetailBuilder.HideCategory(FName(TEXT("Actor")));
	DetailBuilder.HideCategory(FName(TEXT("Placement")));

	if (!CustomizedItem->IsEditInSet)
	{
		DetailBuilder.HideCategory(FName(TEXT("RestrictionItem")));
	}
}

void FTiledItemDetailCustomization::CustomizeTemplateItem(IDetailLayoutBuilder& DetailBuilder,
    UTiledLevelTemplateItem* TemplateItem)
{
	DetailBuilder.HideCategory(FName(TEXT("TiledItem")));
	DetailBuilder.HideCategory(FName(TEXT("Mesh")));
	DetailBuilder.HideCategory(FName(TEXT("Actor")));
	DetailBuilder.HideCategory(FName(TEXT("Placement")));

    /*
     * Can not get access to any property in subclass of "item", so customization will just fail...
     */

}

EVisibility FTiledItemDetailCustomization::GetPivotOptionVisibility(EPivotPosition TargetValue) const
{
	if (CustomizedItem)
	{		
		switch (TargetValue) {
		case EPivotPosition::Bottom:
				return (CustomizedItem->PlacedType != EPlacedType::Edge || CustomizedItem->PlacedType != EPlacedType::Point)?
					EVisibility::Visible : EVisibility::Collapsed;
			case EPivotPosition::Center:
				return CustomizedItem->PlacedType == EPlacedType::Floor? EVisibility::Collapsed : EVisibility::Visible;
			case EPivotPosition::Corner:
				return (CustomizedItem->PlacedType == EPlacedType::Block || CustomizedItem->PlacedType == EPlacedType::Floor)?
					EVisibility::Visible : EVisibility::Collapsed;
			case EPivotPosition::Side:
				return (CustomizedItem->PlacedType == EPlacedType::Wall || CustomizedItem->PlacedType == EPlacedType::Edge)?
					EVisibility::Visible : EVisibility::Collapsed;
			case EPivotPosition::Fit:
				return CustomizedItem->bAutoPlacement &&
					CustomizedItem->SourceType == ETLSourceType::Mesh &&
					CustomizedItem->StructureType == ETLStructureType::Structure &&
					CustomizedItem->PlacedType != EPlacedType::Point?
						EVisibility::Visible: EVisibility::Collapsed;
			default:
				return EVisibility::Collapsed;
		}
	}
	return EVisibility::Collapsed;
}

ECheckBoxState FTiledItemDetailCustomization::GetPivotOptionCheckState(EPivotPosition TargetValue) const
{
	if (CustomizedItem)
	{
		for (UTiledLevelItem* Item : CustomizedItems)
			if (CustomizedItem->PivotPosition != Item->PivotPosition) return ECheckBoxState::Unchecked;
		return CustomizedItem->PivotPosition == TargetValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	return ECheckBoxState::Undetermined;
}

void FTiledItemDetailCustomization::OnPivotOptionCheckStateChanged(ECheckBoxState NewState, EPivotPosition TargetValue)
{
	if (NewState == ECheckBoxState::Unchecked) return;
	if (CustomizedItem)
	{
		bool ShouldRefreshDetails = false;
		// Active item unset fit
		if (CustomizedItem->PivotPosition == EPivotPosition::Fit)
		{
			ShouldRefreshDetails = true;
		}
		for (UTiledLevelItem* Item : CustomizedItems)
		{
			Item->PivotPosition = TargetValue;
			Item->PostEditChange();
		}
		// Active item set fit
		if (CustomizedItem->PivotPosition == EPivotPosition::Fit)
		{
			ShouldRefreshDetails = true;
		}
		if (ShouldRefreshDetails)
		{
			MyDetailLayout->ForceRefreshDetails();
		}
	}
}


bool FTiledItemDetailCustomization::OnShouldFilterActorAsset(const FAssetData& AssetData) const
{
    if (UBlueprint* BP = Cast<UBlueprint>(AssetData.GetAsset()))
    {
        if (!BP->ParentClass->IsChildOf(AActor::StaticClass()))
            return true;
        if ( BP->ParentClass->IsChildOf(AController::StaticClass()) || BP->ParentClass->IsChildOf(AInfo::StaticClass()))
            return true;
    }
    return UnavailableSourceNames.Contains(AssetData.AssetName);
}

bool FTiledItemDetailCustomization::OnShouldFilterMeshAsset(const FAssetData& AssetData) const
{
	if (AssetData.GetClass() == UStaticMesh::StaticClass())
	{
		return UnavailableSourceNames.Contains(AssetData.AssetName);
	}
	return false;
}

TOptional<int> FTiledItemDetailCustomization::GetExtentValue(EAxis::Type Axis) const
{
	FVector Extent;
	FPropertyAccess::Result R =  Property_Extent->GetValue(Extent);
	TOptional<int> Unset;
	switch (Axis)
	{
	case EAxis::X:
		if (Extent.X < 1.0f || Extent.X > 16.0f)
		{
			return Unset;
		}
		return Extent.X;
	case EAxis::Y:
		if (Extent.Y < 1.0f || Extent.Y > 16.0f)
		{
			return Unset;
		}
		return Extent.Y;
	case EAxis::Z:
		if (Extent.Z < 1.0f || Extent.Z > 16.0f)
		{
			return Unset;
		}
		return Extent.Z;
	default: 
		return Unset;
	}
}

/*
 * multi value will become 0, just leave it as an engine default feature?
 */
void FTiledItemDetailCustomization::OnExtentChanged(int InValue, EAxis::Type Axis)
{
	if (InValue < 1) InValue = 1;
	if (InValue > 16) InValue = 16;
	uint8 PlacedTypeIndex;
	Property_PlacedType->GetValue(PlacedTypeIndex);
	FVector Extent;
	Property_Extent->GetValue(Extent);
	switch (Axis)
	{
	case EAxis::X:
		Extent.X = InValue;
		switch (static_cast<EPlacedType>(PlacedTypeIndex)) {
			case EPlacedType::Wall:
				Extent.Y = InValue;
				break;
			case EPlacedType::Edge:
				Extent.Y = InValue;
				break;
			default: ;
		}
		Property_Extent->SetValue(Extent);
		break;
	case EAxis::Y:
		Extent.Y = InValue;
		Property_Extent->SetValue(Extent);
		break;
	case EAxis::Z:
		Extent.Z = InValue;
		Property_Extent->SetValue(Extent);
		break;
	default: ;
	}

}

void FTiledItemDetailCustomization::OnExtentCommitted(int InValue, ETextCommit::Type CommitType, EAxis::Type Axis)
{
	if (InValue < 1) InValue = 1;
	if (InValue > 16) InValue = 16;
	uint8 PlacedTypeIndex;
	Property_PlacedType->GetValue(PlacedTypeIndex);
	FVector Extent;
	Property_Extent->GetValue(Extent);
	switch (Axis)
	{
	case EAxis::X:
		Extent.X = InValue;
		switch (static_cast<EPlacedType>(PlacedTypeIndex)) {
			case EPlacedType::Wall:
				Extent.Y = InValue;
				break;
			case EPlacedType::Edge:
				Extent.Y = InValue;
				break;
			default: ;
		}
		Property_Extent->SetValue(Extent);
		break;
	case EAxis::Y:
		Extent.Y = InValue;
		Property_Extent->SetValue(Extent);
		break;
	case EAxis::Z:
		Extent.Z = InValue;
		Property_Extent->SetValue(Extent);
		break;
	default: ;
	}
}

#undef LOCTEXT_NAMESPACE
