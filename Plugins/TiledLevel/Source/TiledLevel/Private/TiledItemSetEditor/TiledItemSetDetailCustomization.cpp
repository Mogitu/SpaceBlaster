// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledItemSetDetailCustomization.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "TiledItemSet.h"
#include "TiledLevelSettings.h"
#include "Widgets/Input/STextComboBox.h"

#define LOCTEXT_NAMESPACE "TiledLevel"

FTiledItemSetDetailCustomization::FTiledItemSetDetailCustomization()
{
	BlockOptions.Empty();
	BlockOptions.Add(MakeShareable(new FString("Bottom")));
	BlockOptions.Add(MakeShareable(new FString("Center")));
	BlockOptions.Add(MakeShareable(new FString("Corner")));
	FloorOptions.Empty();
	FloorOptions.Add(MakeShareable(new FString("Bottom")));
	FloorOptions.Add(MakeShareable(new FString("Corner")));
	WallOptions.Empty();
	WallOptions.Add(MakeShareable(new FString("Bottom")));
	WallOptions.Add(MakeShareable(new FString("Center")));
	WallOptions.Add(MakeShareable(new FString("Side")));
	PillarOptions.Empty();
	PillarOptions.Add(MakeShareable(new FString("Bottom")));
	PillarOptions.Add(MakeShareable(new FString("Center")));
	EdgeOptions.Empty();
	EdgeOptions.Add(MakeShareable(new FString("Center")));
	EdgeOptions.Add(MakeShareable(new FString("Side")));
}

void FTiledItemSetDetailCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	Property_DefaultBlockPivotPosition = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledItemSet, DefaultBlockPivotPosition));
	Property_DefaultBlockPivotPosition->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([=]()
	{
		uint8 OutValue;
		Property_DefaultBlockPivotPosition->GetValue(OutValue);
		UTiledLevelSettings* Settings = GetMutableDefault<UTiledLevelSettings>();
		Settings->DefaultPlacedTypePivotPositionMap[EPlacedType::Block] = static_cast<EPivotPosition>(OutValue);
	}));
	Property_IsFloorIncluded = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledItemSet, IsFloorIncluded));
	Property_DefaultFloorPivotPosition = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledItemSet, DefaultFloorPivotPosition));
	Property_DefaultFloorPivotPosition->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([=]()
	{
		uint8 OutValue;
		Property_DefaultFloorPivotPosition->GetValue(OutValue);
		UTiledLevelSettings* Settings = GetMutableDefault<UTiledLevelSettings>();
		Settings->DefaultPlacedTypePivotPositionMap[EPlacedType::Floor] = static_cast<EPivotPosition>(OutValue);
	}));
	Property_IsWallIncluded = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledItemSet, IsWallIncluded));
	Property_DefaultWallPivotPosition = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledItemSet, DefaultWallPivotPosition));
	Property_DefaultWallPivotPosition->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([=]()
	{
		uint8 OutValue;
		Property_DefaultWallPivotPosition->GetValue(OutValue);
		UTiledLevelSettings* Settings = GetMutableDefault<UTiledLevelSettings>();
		Settings->DefaultPlacedTypePivotPositionMap[EPlacedType::Wall] = static_cast<EPivotPosition>(OutValue);
	}));
	Property_IsPillarIncluded = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledItemSet, IsPillarIncluded));
	Property_DefaultPillarPivotPosition = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledItemSet, DefaultPillarPivotPosition));
	Property_DefaultPillarPivotPosition->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([=]()
	{
		uint8 OutValue;
		Property_DefaultPillarPivotPosition->GetValue(OutValue);
		UTiledLevelSettings* Settings = GetMutableDefault<UTiledLevelSettings>();
		Settings->DefaultPlacedTypePivotPositionMap[EPlacedType::Pillar] = static_cast<EPivotPosition>(OutValue);
	}));
	Property_IsEdgeIncluded = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledItemSet, IsEdgeIncluded));
	Property_DefaultEdgePivotPosition = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledItemSet, DefaultEdgePivotPosition));
	Property_DefaultEdgePivotPosition->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([=]()
	{
		uint8 OutValue;
		Property_DefaultEdgePivotPosition->GetValue(OutValue);
		UTiledLevelSettings* Settings = GetMutableDefault<UTiledLevelSettings>();
		Settings->DefaultPlacedTypePivotPositionMap[EPlacedType::Edge] = static_cast<EPivotPosition>(OutValue);
	}));
	DetailBuilder.HideProperty(Property_DefaultBlockPivotPosition);
	DetailBuilder.HideProperty(Property_IsFloorIncluded);
	DetailBuilder.HideProperty(Property_DefaultFloorPivotPosition);
	DetailBuilder.HideProperty(Property_IsWallIncluded);
	DetailBuilder.HideProperty(Property_DefaultWallPivotPosition);
	DetailBuilder.HideProperty(Property_IsPillarIncluded);
	DetailBuilder.HideProperty(Property_DefaultPillarPivotPosition);
	DetailBuilder.HideProperty(Property_IsEdgeIncluded);
	DetailBuilder.HideProperty(Property_DefaultEdgePivotPosition);

	IDetailCategoryBuilder& NewItemCategory = DetailBuilder.EditCategory(
		"NewItem",FText::GetEmpty(), ECategoryPriority::Uncommon);
	
	NewItemCategory.AddCustomRow(FText::GetEmpty())
	.NameContent()
	[
		Property_DefaultBlockPivotPosition->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		CreatePivotPositionWidget(EPlacedType::Block)
	];
	
	NewItemCategory.AddCustomRow(FText::GetEmpty())
	.NameContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			Property_IsFloorIncluded->CreatePropertyValueWidget()
		]
		+ SHorizontalBox::Slot()
		.Padding(5, 0, 0, 0)
		[
			Property_IsFloorIncluded->CreatePropertyNameWidget()
		]
	]
	.ValueContent()
	[
		CreatePivotPositionWidget(EPlacedType::Floor)
	];
	
	NewItemCategory.AddCustomRow(FText::GetEmpty())
	.NameContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			Property_IsWallIncluded->CreatePropertyValueWidget()
		]
		+ SHorizontalBox::Slot()
		.Padding(5, 0, 0, 0)
		[
			Property_IsWallIncluded->CreatePropertyNameWidget()
		]
	]
	.ValueContent()
	[
		CreatePivotPositionWidget(EPlacedType::Wall)
	];
	
	NewItemCategory.AddCustomRow(FText::GetEmpty())
	.NameContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			Property_IsEdgeIncluded->CreatePropertyValueWidget()
		]
		+ SHorizontalBox::Slot()
		.Padding(5, 0, 0, 0)
		[
			Property_IsEdgeIncluded->CreatePropertyNameWidget()
		]
	]
	.ValueContent()
	[
		CreatePivotPositionWidget(EPlacedType::Edge)
	];
	
	NewItemCategory.AddCustomRow(FText::GetEmpty())
	.NameContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			Property_IsPillarIncluded->CreatePropertyValueWidget()
		]
		+ SHorizontalBox::Slot()
		.Padding(5, 0, 0, 0)
		[
			Property_IsPillarIncluded->CreatePropertyNameWidget()
		]
	]
	.ValueContent()
	[
		CreatePivotPositionWidget(EPlacedType::Pillar)
	];

	NewItemCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTiledItemSet, IsPointIncluded)));


	//
	TArray<TWeakObjectPtr<UObject>> Temp;
	DetailBuilder.GetObjectsBeingCustomized(Temp);
	UTiledItemSet* CustomizedItemSet = Cast<UTiledItemSet>(Temp[0].Get());
	UTiledLevelSettings* Settings = GetMutableDefault<UTiledLevelSettings>();
	Settings->DefaultPlacedTypePivotPositionMap[EPlacedType::Block] = CustomizedItemSet->DefaultBlockPivotPosition;
	Settings->DefaultPlacedTypePivotPositionMap[EPlacedType::Floor] = CustomizedItemSet->DefaultFloorPivotPosition;
	Settings->DefaultPlacedTypePivotPositionMap[EPlacedType::Wall] = CustomizedItemSet->DefaultWallPivotPosition;
	Settings->DefaultPlacedTypePivotPositionMap[EPlacedType::Pillar] = CustomizedItemSet->DefaultPillarPivotPosition;
	Settings->DefaultPlacedTypePivotPositionMap[EPlacedType::Edge] = CustomizedItemSet->DefaultEdgePivotPosition;
}

TSharedRef<IDetailCustomization> FTiledItemSetDetailCustomization::MakeInstance()
{
	return MakeShareable(new FTiledItemSetDetailCustomization);
}

TSharedRef<SWidget> FTiledItemSetDetailCustomization::CreatePivotPositionWidget(EPlacedType TargetPlacedType)
{
	TArray<TSharedPtr<FString>>* Options = nullptr;
	switch (TargetPlacedType) {
		case EPlacedType::Block:
			Options = &BlockOptions;
			break;
		case EPlacedType::Floor:
			Options = &FloorOptions;
			break;
		case EPlacedType::Wall:
			Options = &WallOptions;
			break;
		case EPlacedType::Pillar:
			Options = &PillarOptions;
			break;
		case EPlacedType::Edge:
			Options = &EdgeOptions;
			break;
		default: ;
	}

	return SNew(STextComboBox)
	.OptionsSource(Options)
	.ToolTipText(LOCTEXT("DefaultPivotPosition_Tooltip", "Default pivot position for this placed type"))
	.IsEnabled_Lambda([=]()
	{
		bool Out = false;
		switch (TargetPlacedType)
		{
		case EPlacedType::Block:
			Out = true;
			break;
		case EPlacedType::Floor:
			Property_IsFloorIncluded->GetValue(Out);
			break;
		case EPlacedType::Wall:
			Property_IsWallIncluded->GetValue(Out);
			break;
		case EPlacedType::Pillar:
			Property_IsPillarIncluded->GetValue(Out);
			break;
		case EPlacedType::Edge:
			Property_IsEdgeIncluded->GetValue(Out);
			break;
		default: ;
		}
		return Out;
	})
	.InitiallySelectedItem((*Options)[0])
	.OnSelectionChanged_Lambda([=](TSharedPtr<FString> NewPivotPosition, ESelectInfo::Type SelectInfo)
	{
		uint8 PivotPositionValue = 0;
		if (*NewPivotPosition.Get() == FString("Center"))
			PivotPositionValue = 1;
		else if (*NewPivotPosition.Get() == FString("Corner"))
			PivotPositionValue = 2;
		else if (*NewPivotPosition.Get() == FString("Side"))
			PivotPositionValue = 3;
			
		switch (TargetPlacedType) {
			case EPlacedType::Block:
				Property_DefaultBlockPivotPosition->SetValue(PivotPositionValue);
				break;
			case EPlacedType::Floor:
				Property_DefaultFloorPivotPosition->SetValue(PivotPositionValue);
				break;
			case EPlacedType::Wall:
				Property_DefaultWallPivotPosition->SetValue(PivotPositionValue);
				break;
			case EPlacedType::Pillar:
				Property_DefaultPillarPivotPosition->SetValue(PivotPositionValue);
				break;
			case EPlacedType::Edge:
				Property_DefaultEdgePivotPosition->SetValue(PivotPositionValue);
				break;
			default: ;
		}
	})
	;
}

#undef LOCTEXT_NAMESPACE