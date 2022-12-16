// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelDetailCustomization.h"
#include "AssetToolsModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EditorModeManager.h"
#include "IDetailTreeNode.h"
#include "IPropertyUtilities.h"
#include "TiledLevelAsset.h"
#include "STiledFloorList.h"
#include "TiledLevel.h"
#include "TiledLevelEditorHelper.h"
#include "TiledLevelEditorLog.h"
#include "TiledLevelEdMode.h"
#include "TiledLevelPromotionFactory.h"
#include "TiledLevelGametimeSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/Layout/SWrapBox.h"
#include "WorkflowOrientedApp/SContentReference.h"

#define LOCTEXT_NAMESPACE "TiledLevel"

void FTiledLevelDetailCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	const TArray<TWeakObjectPtr<UObject>>& SelectedObjects = DetailBuilder.GetSelectedObjects();
	MyDetailLayout = &DetailBuilder;

	FNotifyHook* NotifyHook = DetailBuilder.GetPropertyUtilities()->GetNotifyHook();

	bool bEditingActor = false;

	UTiledLevelAsset* TiledLevelAsset = nullptr;
	ATiledLevel* TiledLevelActor = nullptr;
	for (int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
	{
		UObject* TestObject = SelectedObjects[ObjectIndex].Get();
		if (ATiledLevel* TestActor = Cast<ATiledLevel>(TestObject))
		{
			TiledLevelActor = TestActor;
			bEditingActor = true;
			break;
		}

		if (UTiledLevelAsset* TestTiledLevel = Cast<UTiledLevelAsset>(TestObject))
		{
			TiledLevelAsset = TestTiledLevel;
			if (TiledLevelAsset->EdMode)
			{
				TiledLevelActor = TiledLevelAsset->EdMode->GetActiveLevelActor();
			}
			break;
		}
	}
	TiledLevelAssetPtr = TiledLevelAsset;
	TiledLevelActorPtr = TiledLevelActor;

	IDetailCategoryBuilder& TiledLevelCategory = DetailBuilder.EditCategory(
		"Tiled Level", FText::GetEmpty(), ECategoryPriority::Important);

	// Add the 'instanced' versus 'asset' indicator to the tile map header
	TiledLevelCategory.HeaderContent
	(
		SNew(SBox)
		.HAlign(HAlign_Right)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			  .Padding(FMargin(5.0f, 0.0f))
			  .AutoWidth()
			[
				SNew(STextBlock)
				.Font(FAppStyle::GetFontStyle("TinyText"))
				.Text_Lambda([this]
				                {
					                return IsInstanced()
						                       ? LOCTEXT("Instanced", "Instanced")
						                       : LOCTEXT("Asset", "Asset");
				                })
				.ToolTipText(LOCTEXT("InstancedVersusAssetTooltip",
				                     "Tiled level can either own a unique asset instance, or reference a shareable asset in content browser"))
			]
		]
	);


	// Buttons to convert instance to asset when edit on level
	TSharedRef<SWrapBox> ButtonBox = SNew(SWrapBox).UseAllottedSize(true);
	const float MinButtonSize = 120.0f;
	const FMargin ButtonPadding(0.0f, 2.0f, 2.0f, 0.0f);

	// Edit tiled level in asset button
	ButtonBox->AddSlot()
	         .Padding(ButtonPadding)
	[
		SNew(SBox)
		.MinDesiredWidth(MinButtonSize)
		[
			SNew(SButton)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.OnClicked(this, &FTiledLevelDetailCustomization::OnEditInAssetClicked)
			.Visibility(this, &FTiledLevelDetailCustomization::GetVisibilityBasedOnIsInstanced, true)
			.Text(LOCTEXT("EditAsset", "Edit"))
			.ToolTipText(LOCTEXT("EditAssetToolTip", "Edit this tiled level in asset editor"))
		]
	];

	// Promote to instance button
	ButtonBox->AddSlot()
	         .Padding(ButtonPadding)
	[
		SNew(SBox)
		.MinDesiredWidth(MinButtonSize)
		[
			SNew(SButton)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.OnClicked(this, &FTiledLevelDetailCustomization::OnPromoteToInstanceClicked)
			.Visibility(this, &FTiledLevelDetailCustomization::GetVisibilityBasedOnIsInstanced, true)
			.Text(LOCTEXT("ConvertToInstance", "Convert To Instance"))
			.ToolTipText(LOCTEXT("PromoteToInstanceToolTip",
			                     "Convert to unique object that can be edited directly in level editor"))
		]
	];

	// Edit tiled level as instance button
	ButtonBox->AddSlot()
	         .Padding(ButtonPadding)
	[
		SNew(SBox)
		.MinDesiredWidth(MinButtonSize)
		[
			SNew(SButton)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.OnClicked(this, &FTiledLevelDetailCustomization::OnEditInLevelClicked)
			.Visibility(this, &FTiledLevelDetailCustomization::GetVisibilityBasedOnIsInstanced, false)
			.Text(LOCTEXT("EditInstance", "Edit"))
			.ToolTipText(
				             LOCTEXT("EditAsInstaceTooltip",
				                     "Edit this tiled level instance directly in current viewport."))
		]
	];

	// Promote to asset button
	ButtonBox->AddSlot()
	         .Padding(ButtonPadding)
	[
		SNew(SBox)
		.MinDesiredWidth(MinButtonSize)
		[
			SNew(SButton)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.OnClicked(this, &FTiledLevelDetailCustomization::OnPromoteToAssetClicked)
			.Visibility(this, &FTiledLevelDetailCustomization::GetVisibilityBasedOnIsInstanced, false)
			.Text(LOCTEXT("PromoteToAsset", "Promote To Asset"))
			.ToolTipText(LOCTEXT("PromoteToAssetToolTip", "Save this tiled level instance as a reusable asset"))
		]
	];

	// Try to get the hosting command list from the details view
	TSharedPtr<FUICommandList> CommandList = DetailBuilder.GetDetailsView()->GetHostCommandList();
	if (!CommandList.IsValid())
	{
		CommandList = MakeShareable(new FUICommandList);
	}

	// Add the floor list widget
	if (IsInstanced() || !bEditingActor)
	{
		if (TiledLevelAsset == nullptr)
		{
			TiledLevelAsset = TiledLevelActor->GetAsset();
		}
		TiledLevelCategory.AddCustomRow(LOCTEXT("TiledFloorList", "Tiled Floor list"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(DetailBuilder.GetDetailFont())
				.Text(LOCTEXT("TiledFloorList", "Tiled Floor list"))
			]
			+ SVerticalBox::Slot()
			[
				SAssignNew(FloorListWidget, STiledFloorList, TiledLevelAsset, NotifyHook, CommandList)
				.OnResetInstances(this, &FTiledLevelDetailCustomization::OnResetInstances)
			]
		];
	}

	if (bEditingActor)
	{
		TiledLevelCategory
			.AddCustomRow(LOCTEXT("TiledLevelInstancingControlsSearchText", "Edit Level New Empty Promote Asset"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					ButtonBox
				]
			];
	}

	if (TiledLevelAsset != nullptr)
	{
		FTiledFloor* ActiveFloor = TiledLevelAsset->GetActiveFloor();

		IDetailCategoryBuilder& FloorsCategory = DetailBuilder.EditCategory(
			TEXT("ActiveFloor"), LOCTEXT("FloorCategoryHeading", "Active floor"), ECategoryPriority::Important);

		FloorsCategory.HeaderContent
		(
			SNew(SBox)
			.HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				  .Padding(FMargin(5.0f, 0.0f))
				  .AutoWidth()
				[
					SNew(STextBlock)
					.Font(FAppStyle::GetFontStyle("TinyText"))
					.Text(this, &FTiledLevelDetailCustomization::GetFloorSettingsHeadingText)
					.ToolTipText(LOCTEXT("FloorSettingsTooltip", "Properties specific to the currently selected Floor"))
				]
			]
		);
	}


	// Add all of the properties from the inline tiled level asset
	if (IsInstanced())
	{
		if (!TiledLevelActor->GetAsset()) return;
		IDetailCategoryBuilder& SetupCategory = DetailBuilder.EditCategory(
			"Setup", FText::GetEmpty(), ECategoryPriority::Important);
		TArray<UObject*> TiledLevelAssets;
		TiledLevelAssets.Add(TiledLevelActor->GetAsset());

		for (const FProperty* TestProperty : TFieldRange<FProperty>(TiledLevelActor->GetAsset()->GetClass()))
		{
			if (TestProperty->HasAnyPropertyFlags(CPF_Edit))
			{
				const bool bAdvancedDisplay = TestProperty->HasAnyPropertyFlags(CPF_AdvancedDisplay);
				const EPropertyLocation::Type PropertyLocation = bAdvancedDisplay
					                                                 ? EPropertyLocation::Advanced
					                                                 : EPropertyLocation::Common;
				const FName CategoryName(*TestProperty->GetMetaData(TEXT("Category")));
				IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(CategoryName);
				Category.AddExternalObjectProperty(TiledLevelAssets, TestProperty->GetFName(), PropertyLocation);
			}
		}

		if (IsInEditor())
		{
			if (TiledLevelActor->GetAsset() == nullptr)
			{
				return;
			}

			SetupCategory.AddCustomRow(FText::GetEmpty())
			[
				SNew(SWrapBox).UseAllottedSize(true)
				+ SWrapBox::Slot()
				.Padding(ButtonPadding)
				[
					SNew(SBox)
					.MinDesiredWidth(MinButtonSize)
					[
						SNew(SButton)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.OnClicked_Lambda([=]()
						{
							TiledLevelActor->GetAsset()->ConfirmTileSize();
					        return FReply::Handled();
						})
						.Text(LOCTEXT("ConfirmTileSize_Label", "ConfirmTileSize"))
					]
				]
				+ SWrapBox::Slot()
				.Padding(ButtonPadding)
				[
					SNew(SBox)
					.MinDesiredWidth(MinButtonSize)
					[
						SNew(SButton)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.OnClicked_Lambda([=]()
			            {
							TiledLevelActor->GetAsset()->ResetTileSize();
							return FReply::Handled();
			            })
						.Text(LOCTEXT("ResetTileSize_Label", "ResetTileSize"))
						.ToolTipText(LOCTEXT("ResetTileSize_ToolTip",
						                     "Warning! Reset tile size will clear all exsiting placements."))
					]
				]
			];
		}
	}


	if (bEditingActor)
	{
		if (FTiledLevelEdMode* EdMode = static_cast<FTiledLevelEdMode*>(GLevelEditorModeTools().GetActiveMode(
			FTiledLevelEdMode::EM_TiledLevelEdModeId)))
		{
			EdMode->FloorListWidget = FloorListWidget;
		}
		IDetailCategoryBuilder& GutilCategory = DetailBuilder.EditCategory("Gametime Utility", FText::GetEmpty(), ECategoryPriority::Important);
		GutilCategory.AddCustomRow(FText::GetEmpty())
		[
			SNew(SWrapBox).UseAllottedSize(true)
			+ SWrapBox::Slot()
			.Padding(ButtonPadding)
			[
				SNew(SBox)
				.MinDesiredWidth(MinButtonSize)
				[
					 SNew(SButton)
					 .VAlign(VAlign_Center)
					 .HAlign(HAlign_Center)
					 .OnClicked(this, &FTiledLevelDetailCustomization::OnApplyGametimeRequiredTransformClicked)
					 .Text(LOCTEXT("ApplyGametimeRequiredTransform_Label", "Apply gametime required transform"))
					 .ToolTipText(LOCTEXT("To gametime transform tooltip",
										  "Adjust tranformation to meet the required setup for gametime initialization"))
				]
			]
			+ SWrapBox::Slot()
			.Padding(ButtonPadding)
			[
				SNew(SBox)
				.MinDesiredWidth(MinButtonSize)
				[
					 SNew(SButton)
					 .VAlign(VAlign_Center)
					 .HAlign(HAlign_Center)
					 .OnClicked(this, &FTiledLevelDetailCustomization::OnCheckOverlapClicked)
					 .Text(LOCTEXT("Check overlap label", "Check overlap"))
					 .ToolTipText(LOCTEXT("CheckOverlap_ToolTip",
										  "Check if any tiled level overlapped with each other, which may fail gametime initialization"))
				]
			]
		];
	}
	else
	{
		if (TiledLevelAsset->EdMode)
		{
			TiledLevelAsset->EdMode->FloorListWidget = FloorListWidget;
		}
	}

}

TSharedRef<IDetailCustomization> FTiledLevelDetailCustomization::MakeInstance()
{
	return MakeShareable(new FTiledLevelDetailCustomization);
}

void FTiledLevelDetailCustomization::OnResetInstances()
{
	if (ATiledLevel* TiledLevelActor = TiledLevelActorPtr.Get())
	{
		TiledLevelActor->ResetAllInstance(true);
	}
}

FText FTiledLevelDetailCustomization::GetFloorSettingsHeadingText() const
{
	if (FloorListWidget.IsValid())
	{
		return FText::FromName(FloorListWidget->GetActiveFloor()->FloorName);
	}
	return FText::GetEmpty();
}

bool FTiledLevelDetailCustomization::IsInstanced() const
{
	if (ATiledLevel* TiledLevelActor = TiledLevelActorPtr.Get())
	{
		return TiledLevelActor->IsInstance;
	}
	return false;
}

bool FTiledLevelDetailCustomization::IsInEditor() const
{
	if (ATiledLevel* TiledLevelActor = TiledLevelActorPtr.Get())
	{
		return TiledLevelActor->IsInTiledLevelEditor;
	}
	return false;
}

EVisibility FTiledLevelDetailCustomization::GetVisibilityBasedOnIsActor(bool ShouldReverse) const
{
	if (ShouldReverse)
	{
		return IsValid(TiledLevelActorPtr.Get()) ? EVisibility::Collapsed : EVisibility::Visible;
	}
	return IsValid(TiledLevelActorPtr.Get()) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FTiledLevelDetailCustomization::GetVisibilityBasedOnIsInstanced(bool ShouldReverse) const
{
	if (ShouldReverse)
	{
		return IsInstanced() ? EVisibility::Collapsed : EVisibility::Visible;
	}
	return IsInstanced() ? EVisibility::Visible : EVisibility::Collapsed;
}

FReply FTiledLevelDetailCustomization::OnEditInAssetClicked()
{
	if (GEditor->IsPlayingSessionInEditor())
	{
		return FReply::Unhandled();
	}
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(TiledLevelActorPtr->GetAsset());
	return FReply::Handled();
}

FReply FTiledLevelDetailCustomization::OnPromoteToInstanceClicked()
{
	if (GEditor->IsPlayingSessionInEditor())
	{
		return FReply::Unhandled();
	}
	const FScopedTransaction Transaction(LOCTEXT("ConvertToInstanceTransaction",
	                                             "Convert Tiled Level asset to unique instance"));
	TiledLevelActorPtr.Get()->MakeEditable();
	MyDetailLayout->ForceRefreshDetails();
	return FReply::Handled();
}

FReply FTiledLevelDetailCustomization::OnEditInLevelClicked()
{
	if (GEditor->IsPlayingSessionInEditor())
	{
		return FReply::Unhandled();
	}

	// check if its already in edmode... prevent re-enter
	if (static_cast<FTiledLevelEdMode*>(GLevelEditorModeTools().
		GetActiveMode(FTiledLevelEdMode::EM_TiledLevelEdModeId)))
	{
		return FReply::Handled();
	}

	GLevelEditorModeTools().ActivateMode(FTiledLevelEdMode::EM_TiledLevelEdModeId);
	FTiledLevelEdMode* EdMode = static_cast<FTiledLevelEdMode*>(GLevelEditorModeTools().GetActiveMode(
		FTiledLevelEdMode::EM_TiledLevelEdModeId));
	ATiledLevelEditorHelper* Helper = EdMode->GetWorld()->SpawnActor<ATiledLevelEditorHelper>(
		ATiledLevelEditorHelper::StaticClass());
	Helper->IsEditorHelper = true;
	EdMode->SetupData(TiledLevelActorPtr.Get()->GetAsset(), TiledLevelActorPtr.Get(), Helper, nullptr);
	MyDetailLayout->ForceRefreshDetails();
	return FReply::Handled();
}

FReply FTiledLevelDetailCustomization::OnPromoteToAssetClicked()
{
	if (GEditor->IsPlayingSessionInEditor())
	{
		return FReply::Unhandled();
	}
	GLevelEditorModeTools().ActivateDefaultMode();
	{
		const FScopedTransaction Transaction(LOCTEXT("PromoteToAssetTrancaction",
		                                             "Convert Tiled Level instance to an asset"));

		UTiledLevelPromotionFactory* PromotionFactory = NewObject<UTiledLevelPromotionFactory>();
		PromotionFactory->AssetToRename = TiledLevelActorPtr.Get()->GetAsset();

		FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();
		UObject* NewAsset = AssetToolsModule.Get().CreateAssetWithDialog(
			PromotionFactory->GetSupportedClass(), PromotionFactory);

		TArray<UObject*> ObjectsToSync;
		ObjectsToSync.Add(NewAsset);
		GEditor->SyncBrowserToObjects(ObjectsToSync);
	}
	TiledLevelActorPtr.Get()->IsInstance = false;
	MyDetailLayout->ForceRefreshDetails();
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(TiledLevelActorPtr->GetAsset());
	return FReply::Handled();
}

FReply FTiledLevelDetailCustomization::OnApplyGametimeRequiredTransformClicked()
{
	FScopedTransaction Transaction(LOCTEXT("Apply Runtime Transformation", "Apply runtime required transformation"));
	TiledLevelActorPtr->Modify();
	FTiledLevelUtility::ApplyGametimeRequiredTransform(TiledLevelActorPtr.Get(), TiledLevelActorPtr->GetAsset()->GetTileSize());
	return FReply::Handled();
}

FReply FTiledLevelDetailCustomization::OnCheckOverlapClicked()
{
	const UWorld* World = TiledLevelActorPtr->GetWorld();
	TArray<AActor*> Temp;
	UGameplayStatics::GetAllActorsOfClass(World, ATiledLevel::StaticClass(), Temp);
	TMap<ATiledLevel*, FTransform> OldTransformMap;
	TArray<ATiledLevel*> FoundTl;
	for (AActor* A: Temp)
	{
		ATiledLevel* Atl = Cast<ATiledLevel>(A);
		FoundTl.Add(Atl);
		OldTransformMap.Add(Atl, A->GetActorTransform());
		FTiledLevelUtility::ApplyGametimeRequiredTransform(Atl, Atl->GetAsset()->GetTileSize());
	}
	TArray<ATiledLevel*> PairedTl = FoundTl;
	FString OutMessages = TEXT("Check Overlap Result:\n\n");
	bool AnyOverlapped = false;
	for (ATiledLevel* Atl1 : FoundTl)
	{
		for (ATiledLevel* Atl2 : PairedTl)
		{
			if (Atl1 == Atl2) continue;
			if (FTiledLevelUtility::CheckOverlappedTiledLevels(Atl1, Atl2))
			{
				OutMessages.Append(FString::Printf(TEXT("    %s is overlapped with %s.\n"), *Atl1->GetName(), *Atl2->GetName()));
				AnyOverlapped = true;
			}
		}
		PairedTl.Remove(Atl1);
	}
	if (!AnyOverlapped)
		OutMessages.Append(TEXT("    No overlaps detected!\n"));
	
	for (auto elem : OldTransformMap)
		elem.Key->SetActorTransform(elem.Value);
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(OutMessages));
	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
