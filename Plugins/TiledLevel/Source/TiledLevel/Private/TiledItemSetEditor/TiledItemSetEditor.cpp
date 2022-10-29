// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledItemSetEditor.h"
#include "AdvancedPreviewScene.h"
#include "ContentBrowserModule.h"
#include "EdMode.h"
#include "IContentBrowserSingleton.h"
#include "SAdvancedPreviewDetailsTab.h"
#include "TiledItemSet.h"
#include "STiledItemSetEditorViewport.h"
#include "SSingleObjectDetailsPanel.h"
#include "STiledPalette.h"
#include "TiledLevelCommands.h"
#include "TiledLevelEditorLog.h"
#include "TiledLevelGrid.h"
#include "TiledLevelItem.h"
#include "TiledLevelRestrictionHelper.h"
#include "TiledLevelSelectHelper.h"
#include "TiledLevelSettings.h"
#include "TiledLevelStyle.h"
#include "TiledLevelUtility.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/KismetGuidLibrary.h"
#include "Slate/SGameLayerManager.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SGridPanel.h"

#define LOCTEXT_NAMESPACE "TiledItemSetEditor"


//////////////////////////////////////////////

const FName TiledItemSetEditorAppName = FName(TEXT("TiledItemSetEditorApp"));

///////////////////////////////////////////////////////
struct FTiledItemSetEditorTabs
{
	static const FName PaletteID;
	static const FName PreviewID;
	static const FName DetailsID;
	static const FName ContentBrowserID;
	static const FName PreviewSettingsID;
};

const FName FTiledItemSetEditorTabs::PaletteID(TEXT("Palette"));
const FName FTiledItemSetEditorTabs::PreviewID(TEXT("Preview"));
const FName FTiledItemSetEditorTabs::DetailsID(TEXT("Details"));
const FName FTiledItemSetEditorTabs::ContentBrowserID(TEXT("ContentBrowser"));
const FName FTiledItemSetEditorTabs::PreviewSettingsID(TEXT("PreviewSettings"));
//////////////////////////////////////////////////////


/////////////////////////////////////////////////////
// STileMapPropertiesTabBody

class STiledItemSetPropertiesTabBody : public SSingleObjectDetailsPanel
{
public:
	SLATE_BEGIN_ARGS(STiledItemSetPropertiesTabBody)
		{
		}

	SLATE_END_ARGS()

private:
	TWeakPtr<FTiledItemSetEditor> EditorPtr;

public:
	void Construct(const FArguments& InArgs, TSharedPtr<FTiledItemSetEditor> InEditor)
	{
		EditorPtr = InEditor;

		SSingleObjectDetailsPanel::Construct(
			SSingleObjectDetailsPanel::FArguments().HostCommandList(EditorPtr.Pin()->GetToolkitCommands()).
			                                        HostTabManager(EditorPtr.Pin()->GetTabManager()));
	}

	// SSingleObjectDetailsPanel interface
	virtual UObject* GetObjectToObserve() const override
	{
		return EditorPtr.Pin()->GetTiledItemSetBeingEdited();
	}

	virtual TSharedRef<SWidget> PopulateSlot(TSharedRef<SWidget> PropertyEditorWidget) override
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1)
			[
				PropertyEditorWidget
			];
	}

	// End of SSingleObjectDetailsPanel interface
};

//////////////////////////////////////////////////
///
FTiledItemSetEditor::FTiledItemSetEditor()
	: TiledItemSetBeingEdited(nullptr), ActiveItem(nullptr), PreviewSceneActor(nullptr), PreviewMesh(nullptr),
	  PreviewBrush(nullptr), PreviewItemActor(nullptr)
{
	// Register to be notified when properties are edited
	FCoreUObjectDelegates::FOnObjectPropertyChanged::FDelegate OnPropertyChangedDelegate =
		FCoreUObjectDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(
			this, &FTiledItemSetEditor::OnPropertyChanged);
	OnPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.Add(OnPropertyChangedDelegate);
	
	UnfixedButton.Normal.TintColor = FSlateColor(FColorList::LightGrey);
	UnfixedButton.Hovered.TintColor = FSlateColor(FColorList::Grey);
	UnfixedButton.Pressed.TintColor = FSlateColor(FColorList::DimGrey);
	
	FixedButton.Normal.TintColor = FSlateColor(FColorList::BlueViolet);
	FixedButton.Hovered.TintColor = FSlateColor(FColorList::Grey);
	FixedButton.Pressed.TintColor = FSlateColor(FColorList::DimGrey);

	DisabledButton.Normal.TintColor = FSlateColor(FLinearColor::Black);
	
	PositionButtons.Init(SNew(SButton), 9);
	
}

FTiledItemSetEditor::~FTiledItemSetEditor()
{
	// Fix deletion crash error...
	for (UTiledLevelItem* Item : GetTiledItemSetBeingEdited()->GetItemSet())
	{
		Item->IsEditInSet = false;
	}
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(OnPropertyChangedHandle);
}

void FTiledItemSetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(
		LOCTEXT("WorkspaceMenu_TiledElementSetEditor", "Tiled Element Set Editor"));
	TSharedRef<FWorkspaceItem> WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
	InTabManager->RegisterTabSpawner(FTiledItemSetEditorTabs::PaletteID,
	                                 FOnSpawnTab::CreateSP(this, &FTiledItemSetEditor::SpawnTab_Palette))
	            .SetDisplayName(LOCTEXT("PaletteTab_Description", "Item Palette"))
	            .SetTooltipText(LOCTEXT("PaletteTab_Tooltip", "Display the Item Palette"))
	            .SetGroup(WorkspaceMenuCategoryRef)
	            .SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.PlacementBrowser"));
	InTabManager->RegisterTabSpawner(FTiledItemSetEditorTabs::PreviewID,
	                                 FOnSpawnTab::CreateSP(this, &FTiledItemSetEditor::SpawnTab_Preview))
	            .SetDisplayName(LOCTEXT("PreviewTab_Description", "Preview Viewport"))
	            .SetTooltipText(LOCTEXT("PreviewTab_Tooltip", "Preview Item Placement"))
	            .SetGroup(WorkspaceMenuCategoryRef)
	            .SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"));
	InTabManager->RegisterTabSpawner(FTiledItemSetEditorTabs::DetailsID,
	                                 FOnSpawnTab::CreateSP(this, &FTiledItemSetEditor::SpawnTab_Details))
	            .SetDisplayName(LOCTEXT("DetailTab_Description", "Item Detail"))
	            .SetGroup(WorkspaceMenuCategoryRef)
	            .SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));
	InTabManager->RegisterTabSpawner(FTiledItemSetEditorTabs::ContentBrowserID,
	                                 FOnSpawnTab::CreateSP(this, &FTiledItemSetEditor::SpawnTab_ContentBrowser))
	            .SetDisplayName(LOCTEXT("ContentBrowserTab_Description", "Content Browser"))
	            .SetGroup(WorkspaceMenuCategoryRef)
	            .SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.ContentBrowser"));
	InTabManager->RegisterTabSpawner(FTiledItemSetEditorTabs::PreviewSettingsID,
	                                 FOnSpawnTab::CreateSP(this, &FTiledItemSetEditor::SpawnTab_PreviewSettings))
	            .SetDisplayName(LOCTEXT("PropertiesTabLabel", "Preview Settings"))
	            .SetGroup(WorkspaceMenuCategory.ToSharedRef())
	            .SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FTiledItemSetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(FTiledItemSetEditorTabs::PaletteID);
	InTabManager->UnregisterTabSpawner(FTiledItemSetEditorTabs::PreviewID);
	InTabManager->UnregisterTabSpawner(FTiledItemSetEditorTabs::DetailsID);
	InTabManager->UnregisterTabSpawner(FTiledItemSetEditorTabs::ContentBrowserID);
}

FName FTiledItemSetEditor::GetToolkitFName() const
{
	return FName("TiledItemSetEditor");
}

FText FTiledItemSetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("TiledItemSetEditorAppLabelBase", "Tiled Item Set Editor");
}

FText FTiledItemSetEditor::GetToolkitName() const
{
	const bool bDirtyState = TiledItemSetBeingEdited->GetOutermost()->IsDirty();
	return FText::Format(
		LOCTEXT("TiledItemSetEditorAppLabel", "{0}{1}"), FText::FromString(TiledItemSetBeingEdited->GetName()),
		bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
}

FText FTiledItemSetEditor::GetToolkitToolTipText() const
{
	return GetToolTipTextForObject(TiledItemSetBeingEdited);
}

FLinearColor FTiledItemSetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

FString FTiledItemSetEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("TiledElementSetEditor");
}

void FTiledItemSetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(TiledItemSetBeingEdited);
}

void FTiledItemSetEditor::InitTiledItemSetEditor(const EToolkitMode::Type Mode,
                                                 const TSharedPtr<IToolkitHost>& InitToolkitHost,
                                                 UTiledItemSet* InItemSet)
{
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseOtherEditors(InItemSet, this);
	TiledItemSetBeingEdited = InItemSet;
	TiledItemSetBeingEdited->ItemSetPostUndo.AddLambda([this]()
	{
		PalettePtr->UpdatePalette(true);
	});
	BindCommands();
	ViewportPtr = SNew(STiledItemSetEditorViewport, SharedThis(this));

	// Layout
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout(
			"Standalone_TiledElementSetEditor_Dev011")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			// ->Split
			// (
			// 	FTabManager::NewStack()
			// 	->SetSizeCoefficient(0.1)
			// 	->SetHideTabWell(true)
			// 	->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
			// )
			// ->Split
			// (
			// 	FTabManager::NewSplitter()
			// 	->SetOrientation(Orient_Horizontal)
			// 	->SetSizeCoefficient(0.9)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.4)
					->SetHideTabWell(true)
					->AddTab(FTiledItemSetEditorTabs::PaletteID, ETabState::OpenedTab) // palette (comes from edmode)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.6)
					->Split
					(
						FTabManager::NewSplitter()
						->SetOrientation(Orient_Horizontal)
						->Split
						(
							FTabManager::NewStack()
							->SetSizeCoefficient(0.6)
							->SetHideTabWell(true)
							->AddTab(FTiledItemSetEditorTabs::PreviewID, ETabState::OpenedTab) // viewport
						)
						->Split
						(
							FTabManager::NewStack()
							->SetSizeCoefficient(0.4)
							->SetHideTabWell(true)
							->AddTab(FTiledItemSetEditorTabs::PreviewSettingsID, ETabState::OpenedTab)
							// preview settings
							->AddTab(FTiledItemSetEditorTabs::DetailsID, ETabState::OpenedTab) // detail
						)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.4)
						->SetHideTabWell(true)
						->AddTab(FTiledItemSetEditorTabs::ContentBrowserID, ETabState::OpenedTab) // content browser
					)
				)
			// )
		);

	InitAssetEditor(
		Mode,
		InitToolkitHost,
		TiledItemSetEditorAppName,
		StandaloneDefaultLayout,
		true,
		true,
		InItemSet
	);

	ExtendToolbar();
	ExtendMenu();
	RegenerateMenusAndToolbars();

	FixedItemsInfo.Empty();
	
	// turn off all palette item filter
	GetMutableDefault<UTiledLevelSettings>()->ResetTiledPaletteSettings();

	// Spawned required actor and register its core components in this scene world
	if (ViewportPtr.IsValid())
	{
		UWorld* World = ViewportPtr->GetPreviewScene()->GetWorld();
		PreviewSceneActor = World->SpawnActor<AActor>(AActor::StaticClass());
		PreviewCenter = NewObject<UTiledLevelGrid>(PreviewSceneActor, UTiledLevelGrid::StaticClass(),
		                                           FName(TEXT("Center")));
		PreviewBrush = NewObject<UTiledLevelGrid>(PreviewSceneActor, UTiledLevelGrid::StaticClass(),
		                                          FName(TEXT("Brush")));
		PreviewMesh = NewObject<UStaticMeshComponent>(PreviewSceneActor, UStaticMeshComponent::StaticClass(),
		                                              FName(TEXT("Mesh")));

		PreviewBrush->RegisterComponentWithWorld(World);
		PreviewSceneActor->AddComponent(FName(TEXT("Brush")), false, FTransform(), PreviewBrush);
		PreviewSceneActor->SetRootComponent(PreviewBrush);
		PreviewBrush->SetLineThickness(5);

		PreviewCenter->RegisterComponentWithWorld(World);
		PreviewSceneActor->AddComponent(FName(TEXT("Center")), false, FTransform(), PreviewCenter);
		PreviewCenter->AttachToComponent(PreviewBrush, FAttachmentTransformRules::KeepRelativeTransform);
		PreviewCenter->SetRelativeLocation(FVector(0));
		PreviewCenter->SetBoxExtent(FVector(3));
		PreviewCenter->SetLineThickness(5);

		PreviewMesh->RegisterComponentWithWorld(World);
		PreviewSceneActor->AddComponent(FName(TEXT("Mesh")), false, FTransform(), PreviewMesh);
		PreviewMesh->AttachToComponent(PreviewCenter, FAttachmentTransformRules::KeepRelativeTransform);

		UpdatePreviewScene(nullptr);
	}
}

void FTiledItemSetEditor::UpdatePreviewScene(UTiledLevelItem* TargetItem)
{
	if (TargetItem)
	{
		ActiveItem = TargetItem;
		
		// Spawn everything
		UWorld* World = ViewportPtr->GetPreviewScene()->GetWorld();
		if (TargetItem->SourceType == ETLSourceType::Mesh)
		{
			if (PreviewItemActor)
			{
				PreviewItemActor->Destroy();
				PreviewItemActor = nullptr;
			}
			TargetItem->UpdatePreviewMesh.BindLambda([=]()
			{
				if (!TiledItemSetBeingEdited) return;
				UpdatePreviewMesh(TargetItem);
		
				FTiledLevelUtility::ApplyPlacementSettings(TiledItemSetBeingEdited->GetTileSize(), TargetItem,
				                                           PreviewMesh, PreviewBrush, PreviewCenter);
				FVector InitPreviewMeshLocation = PreviewMesh->GetRelativeLocation();
				float Offset = FTiledLevelUtility::TrySnapPropToFloor(InitPreviewMeshLocation, 0,
				                                                      TiledItemSetBeingEdited->GetTileSize(),
				                                                      ActiveItem, PreviewMesh);
				FTiledLevelUtility::TrySnapPropToWall(InitPreviewMeshLocation, 0,
				                                      TiledItemSetBeingEdited->GetTileSize(), ActiveItem, PreviewMesh,
				                                      Offset);
			});
			UpdatePreviewMesh(TargetItem);
			FTiledLevelUtility::ApplyPlacementSettings(TiledItemSetBeingEdited->GetTileSize(), TargetItem, PreviewMesh,
			                                           PreviewBrush, PreviewCenter);
			FVector InitPreviewMeshLocation = PreviewMesh->GetRelativeLocation();
			float Offset = FTiledLevelUtility::TrySnapPropToFloor(InitPreviewMeshLocation, 0,
			                                                      TiledItemSetBeingEdited->GetTileSize(), ActiveItem,
			                                                      PreviewMesh);
			FTiledLevelUtility::TrySnapPropToWall(InitPreviewMeshLocation, 0, TiledItemSetBeingEdited->GetTileSize(),
			                                      ActiveItem, PreviewMesh, Offset);
		}
		else
		{
			PreviewMesh->SetStaticMesh(nullptr);
			if (PreviewItemActor) PreviewItemActor->Destroy();
			if (!IsValid(ActiveItem->TiledActor)) return;
			FTiledLevelUtility::ApplyPlacementSettings_TiledActor(TiledItemSetBeingEdited->GetTileSize(), ActiveItem,
				PreviewBrush, PreviewCenter);
			if (UTiledLevelRestrictionItem* RestrictionItem = Cast<UTiledLevelRestrictionItem>(ActiveItem))
			{
				PreviewItemActor = World->SpawnActor(ATiledLevelRestrictionHelper::StaticClass());
				ATiledLevelRestrictionHelper* Arh = Cast<ATiledLevelRestrictionHelper>(PreviewItemActor);
				Arh->SourceItemSet = GetTiledItemSetBeingEdited();
				Arh->SourceItemID = RestrictionItem->ItemID;
				Arh->UpdatePreviewVisual();
				Cast<UTiledLevelRestrictionItem>(ActiveItem)->UpdatePreviewRestriction.BindLambda([=]()
				{
					Arh->UpdatePreviewVisual();
				});
			}
		    else if (UTiledLevelTemplateItem* TemplateItem = Cast<UTiledLevelTemplateItem>(ActiveItem))
            {
                PreviewItemActor = World->SpawnActor(ATiledLevelSelectHelper::StaticClass());
		        if (UTiledLevelAsset* Asset = TemplateItem->GetAsset())
		        {
                    TArray<FTilePlacement> TemplateTiles;
                    TArray<FEdgePlacement> TemplateEdges;
                    TArray<FPointPlacement> TemplatePoints;
                    TemplateTiles.Append(Asset->GetAllBlockPlacements());
                    TemplateTiles.Append(Asset->GetAllFloorPlacements());
                    TemplateEdges.Append(Asset->GetAllEdgePlacements());
                    TemplateEdges.Append(Asset->GetAllWallPlacements());
                    TemplatePoints.Append(Asset->GetAllPillarPlacements());
                    TemplatePoints.Append(Asset->GetAllPointPlacements());
                    ATiledLevelSelectHelper* Ash = Cast<ATiledLevelSelectHelper>(PreviewItemActor);
		            Ash->SyncToSelection(Asset->GetTileSize(), TemplateTiles, TemplateEdges, TemplatePoints, true);
		            Ash->PopulateCopied(true);
		            ActiveItem->Extent = FVector(Ash->GetCopiedExtent(true));
                    FTiledLevelUtility::ApplyPlacementSettings_TiledActor(TiledItemSetBeingEdited->GetTileSize(), ActiveItem,
                         PreviewBrush, PreviewCenter);
		        }    
            }
		    else
			{
				PreviewItemActor = World->SpawnActor(Cast<UBlueprint>(ActiveItem->TiledActor)->GeneratedClass);
				ActiveItem->UpdatePreviewActor.BindLambda([=]()
				{
					 FTiledLevelUtility::ApplyPlacementSettings_TiledActor(TiledItemSetBeingEdited->GetTileSize(), ActiveItem,
						  PreviewBrush, PreviewCenter);
					 PreviewItemActor->SetActorTransform(PreviewCenter->GetComponentTransform());
					 PreviewItemActor->AddActorWorldTransform(ActiveItem->TransformAdjustment);
					 PreviewItemActor->SetActorScale3D(ActiveItem->TransformAdjustment.GetScale3D());
				});
			}
			PreviewItemActor->SetActorTransform(PreviewCenter->GetComponentTransform());
			PreviewItemActor->AddActorWorldTransform(ActiveItem->TransformAdjustment);
			// AddActorWorldTransform did not handle scale...
			PreviewItemActor->SetActorScale3D(ActiveItem->TransformAdjustment.GetScale3D());
		}
		UpdateFixedItemsStartPoint(TargetItem->Extent);
	}
	else
	{
		ActiveItem = nullptr;
		PreviewMesh->SetStaticMesh(nullptr);
		PreviewBrush->SetBoxExtent(TiledItemSetBeingEdited->GetTileSize() * 0.5);
		PreviewBrush->SetRelativeLocation(TiledItemSetBeingEdited->GetTileSize() * 0.5);
		PreviewBrush->SetBoxColor(FColor::Green);

		if (PreviewItemActor)
			PreviewItemActor->Destroy();
		PreviewItemActor = nullptr;
		UpdateFixedItemsStartPoint(FVector(1));
	}
}

void FTiledItemSetEditor::RefreshPalette()
{
	PalettePtr->UpdatePalette(false);
}

void FTiledItemSetEditor::OnPropertyChanged(UObject* ObjectBeingModified,
                                            FPropertyChangedEvent& PropertyChangedEvent)
{
	if (ObjectBeingModified == TiledItemSetBeingEdited)
	{
		ClearAllFixedItem();
		UpdatePreviewScene(nullptr);
		PalettePtr->ClearItemSelection();
	}
}

void FTiledItemSetEditor::BindCommands()
{
	const TSharedRef<FUICommandList>& CommandList = GetToolkitCommands();
	const FTiledLevelCommands& Commands = FTiledLevelCommands::Get();

	CommandList->MapAction(
		Commands.ClearFixedItem,
		FExecuteAction::CreateSP(this, &FTiledItemSetEditor::ClearAllFixedItem)
	);
}

void FTiledItemSetEditor::ExtendMenu()
{
}

void FTiledItemSetEditor::ExtendToolbar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder, FTiledItemSetEditor* ThisEditor)
		{
			ToolbarBuilder.BeginSection("FixedItems");
			{
				ToolbarBuilder.AddComboButton(
					FUIAction(
						FExecuteAction(),
						FCanExecuteAction()),
						FOnGetContent::CreateRaw(ThisEditor, &FTiledItemSetEditor::CreateFixedItemWidget),
						LOCTEXT("FixItem_Label", "FixItem"),
						LOCTEXT("FixItem_Tooltip", "Fix item on preview viewport for referencing"),
						FSlateIcon(FTiledLevelStyle::GetStyleSetName(), "TiledLevel.FixItem")
					),
				ToolbarBuilder.AddToolBarButton(FTiledLevelCommands::Get().ClearFixedItem);
			}
			ToolbarBuilder.EndSection();
		}
	};

	FTiledItemSetEditor* ThisEditor = this;
	
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		ToolkitCommands,
		FToolBarExtensionDelegate::CreateStatic(Local::FillToolbar, ThisEditor)
	);
	AddToolbarExtender(ToolbarExtender);
}

TSharedRef<SWidget> FTiledItemSetEditor::GeneratePositionButton(int BtnIndex)
{
	return SNew(SBox)
	.HeightOverride(36)
	.WidthOverride(36)
	.Padding(1)
	[
		SAssignNew(PositionButtons[BtnIndex], SButton)
		.ButtonStyle(&UnfixedButton)
		.OnClicked_Lambda([=]()
		{
			if (FixedItemsInfo.Contains(ActiveItem))
			{
				bool Found = false;
				for (FIntPoint P : FixedItemsInfo[ActiveItem])
				{
					if (P.X == FixedFloor && P.Y == BtnIndex)
					{
						Found = true;
						break;
					}
				}
				if (Found)
				{
					PositionButtons[BtnIndex]->SetButtonStyle(&UnfixedButton);
					FixedItemsInfo[ActiveItem].Remove(FIntPoint(FixedFloor, BtnIndex));
					UnfixActiveItem(FIntPoint(FixedFloor, BtnIndex));
				} else
				{
					 FixedItemsInfo[ActiveItem].Add(FIntPoint(FixedFloor, BtnIndex));
					 FixActiveItem(FIntPoint(FixedFloor, BtnIndex));
					 PositionButtons[BtnIndex]->SetButtonStyle(&FixedButton);
				}
			} else
			{
				FixedItemsInfo.Add(ActiveItem, TArray<FIntPoint>{FIntPoint(FixedFloor, BtnIndex)});
				FixActiveItem(FIntPoint(FixedFloor, BtnIndex));
				PositionButtons[BtnIndex]->SetButtonStyle(&FixedButton);
			}
				
			return FReply::Handled();
		})
    ];
}

TSharedRef<SWidget> FTiledItemSetEditor::CreateFixedItemWidget()
{
	TSharedRef<SWidget> Out = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.AutoHeight()
	.VAlign(VAlign_Fill)
	.Padding(3)
	[
		SNew(STextBlock).Text(LOCTEXT("FixedItemWidget_Label", "Select fixed position"))
	]
	+SVerticalBox::Slot()
	.Padding(5)
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SGridPanel)
		+ SGridPanel::Slot(0, 0)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			GeneratePositionButton(0)
		]
		+ SGridPanel::Slot(1, 0)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			GeneratePositionButton(1)
		]
		+ SGridPanel::Slot(2, 0)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			GeneratePositionButton(2)
		]
		+ SGridPanel::Slot(0, 1)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			GeneratePositionButton(3)
		]
		+ SGridPanel::Slot(1, 1)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			GeneratePositionButton(4)
		]
		+ SGridPanel::Slot(2, 1)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			GeneratePositionButton(5)
		]
		+ SGridPanel::Slot(0, 2)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			GeneratePositionButton(6)
		]
		+ SGridPanel::Slot(1, 2)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			GeneratePositionButton(7)
		]
		+ SGridPanel::Slot(2, 2)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			GeneratePositionButton(8)
		]
	]
	+ SVerticalBox::Slot()
	.Padding(5)
	.AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text_Lambda([this]()
			{
				FText FloorText;
				switch (FixedFloor)
				{
				case -1:
					FloorText = FText::FromString("B1");
					break;
				case 0:
					FloorText = FText::FromString("1F");
					break;
				case 1:
					FloorText = FText::FromString("2F");
					break;
				}
				return FText::Format(LOCTEXT("FixedItemFloor_Label", "At {0}"), FloorText);
			})
		]
		+ SHorizontalBox::Slot()
		[
			 SNew(SSlider)
			 .StepSize(1)
			 .MinValue(-1)
			 .MaxValue(1)
			 .Value(this, &FTiledItemSetEditor::GetFixedFloor)
			 .OnValueChanged(this, &FTiledItemSetEditor::SetFixedFloor)
		]
	];
	
	SetupButtonState();
	
	return Out;
}

void FTiledItemSetEditor::SetFixedFloor(float NewFloor)
{
	FixedFloor = NewFloor;
	SetupButtonState();
}

void FTiledItemSetEditor::SetupButtonState()
{
	if (ActiveItem)
	{
		for (auto Btn : PositionButtons)
		{
			Btn->SetButtonStyle(&UnfixedButton);
		}
		if (FixedItemsInfo.Contains(ActiveItem))
		{
			for (FIntPoint P : FixedItemsInfo[ActiveItem])
			{
				if (P.X == FixedFloor)
					PositionButtons[P.Y]->SetButtonStyle(&FixedButton);
			}
		}
		TArray<int> DisabledBtnIndices;
		if (ActiveItem->PlacedType == EPlacedType::Edge || ActiveItem->PlacedType == EPlacedType::Wall)
		{
			DisabledBtnIndices = {0, 2, 4, 6, 8};
		}
		else if (ActiveItem->PlacedType == EPlacedType::Pillar || ActiveItem->PlacedType == EPlacedType::Point)
		{
			DisabledBtnIndices = {1, 3, 4, 5, 7};
		}
		for (int I : DisabledBtnIndices)
		{
			PositionButtons[I]->SetEnabled(false);
			PositionButtons[I]->SetButtonStyle(&DisabledButton);
		}
	} else
	{
		for (auto Btn : PositionButtons)
		{
			Btn->SetEnabled(false);
			Btn->SetButtonStyle(&DisabledButton);
		}
	}
}

void FTiledItemSetEditor::FixActiveItem(FIntPoint PreviewPosition)
{
	UWorld* World = ViewportPtr->GetPreviewScene()->GetWorld();
	
	if (ActiveItem->SourceType == ETLSourceType::Mesh)
	{
		AStaticMeshActor* NewSMA = World->SpawnActor<AStaticMeshActor>();
		NewSMA->Tags.Add(FName(ActiveItem->ItemID.ToString()));
		NewSMA->Tags.Add(FName(PreviewPosition.ToString()));
		NewSMA->GetStaticMeshComponent()->SetStaticMesh(ActiveItem->TiledMesh);
		FTransform RefTransform;
		if (ActiveItem->PlacedType ==EPlacedType::Edge || ActiveItem->PlacedType == EPlacedType::Wall)
		{
			if (PreviewPosition.Y == 3)
			{
                PreviewSceneActor->SetActorRotation(FRotator(0, 90, 0));
			}
			else if	(PreviewPosition.Y == 5)
			{
                PreviewSceneActor->SetActorRotation(FRotator(0, -90, 0));
			}
			NewSMA->SetActorTransform(PreviewMesh->GetComponentToWorld());
			PreviewSceneActor->SetActorRotation(FRotator(0));
	 		NewSMA->AddActorWorldOffset(FVector(-(ActiveItem->Extent.X - 1)/2, 0.5, 0) * TiledItemSetBeingEdited->GetTileSize());
			NewSMA->Tags.Add(FName(NewSMA->GetActorTransform().ToString()));
		}
		else if (ActiveItem->PlacedType == EPlacedType::Pillar || ActiveItem->PlacedType == EPlacedType::Point)
		{
			NewSMA->SetActorTransform(PreviewMesh->GetComponentToWorld());
			NewSMA->Tags.Add(FName(NewSMA->GetActorTransform().ToString()));
		}
		else
		{
			RefTransform = PreviewMesh->GetComponentToWorld();
			NewSMA->SetActorTransform(RefTransform);
			RefTransform.AddToTranslation(-ActiveItem->Extent * TiledItemSetBeingEdited->GetTileSize() * FVector(0.5, 0.5, 0));
			NewSMA->Tags.Add(FName(RefTransform.ToString()));
		}
		SpawnedFixedActors.Add(NewSMA);
	}
}

void FTiledItemSetEditor::UnfixActiveItem(FIntPoint PreviewPosition)
{
	const int IndexToRemove = SpawnedFixedActors.IndexOfByPredicate([=](AActor* Actor)
	{
		FGuid Uid;
		bool ParseResult;
		UKismetGuidLibrary::Parse_StringToGuid(Actor->Tags[0].ToString(), Uid, ParseResult);
		FVector2D V2D;
		V2D.InitFromString(Actor->Tags[1].ToString());
		return (ActiveItem->ItemID == Uid && FVector2D(PreviewPosition) == V2D);
	});
	if (IndexToRemove != INDEX_NONE)
	{
		SpawnedFixedActors[IndexToRemove]->Destroy();
		SpawnedFixedActors.RemoveAt(IndexToRemove);
	}
}

void FTiledItemSetEditor::UpdateFixedItemsStartPoint(FVector SurroundExtent)
{
	for (AActor* Actor : SpawnedFixedActors)
	{
		FGuid Uid;
		bool ParseResult;
		UKismetGuidLibrary::Parse_StringToGuid(Actor->Tags[0].ToString(), Uid, ParseResult);
		FVector2D V2D;
		V2D.InitFromString(Actor->Tags[1].ToString());
		UTiledLevelItem* Item = TiledItemSetBeingEdited->GetItem(Uid);
		FTransform RefTransform;
		RefTransform.InitFromString(Actor->Tags[2].ToString());

		if (Item->PlacedType == EPlacedType::Edge || Item->PlacedType == EPlacedType::Wall)
		{
			RefTransform.AddToTranslation(
				ExtractGridPosition(V2D.IntPoint()) * TiledItemSetBeingEdited->GetTileSize() * FVector(0.5, 0.5, 1) * SurroundExtent
				+ FVector(SurroundExtent.X  - 1 , SurroundExtent.Y - 1, 0) * 0.5 * TiledItemSetBeingEdited->GetTileSize() )
			;
		} else if (Item->PlacedType == EPlacedType::Pillar || Item->PlacedType == EPlacedType::Point)
		{
			RefTransform.AddToTranslation(
				ExtractGridPosition(V2D.IntPoint()) * SurroundExtent * TiledItemSetBeingEdited->GetTileSize() * FVector(0.5, 0.5, 1)
				+ SurroundExtent * FVector(0.5, 0.5, 0) * TiledItemSetBeingEdited->GetTileSize()
			);
		} else
		{
			RefTransform.AddToTranslation(
				ExtractGridPosition(V2D.IntPoint()) * (0.5 * SurroundExtent + 0.5 * Item->Extent) * TiledItemSetBeingEdited->GetTileSize()
				+ SurroundExtent * FVector(0.5, 0.5, 0) * TiledItemSetBeingEdited->GetTileSize()
			);
		}
		Actor->SetActorTransform(RefTransform);
	}
}

FVector FTiledItemSetEditor::ExtractGridPosition(FIntPoint Point)
{
	switch (Point.Y)
	{
	case 0:
		return FVector(-1, -1, Point.X);
	case 1:
		return FVector(0, -1, Point.X);
	case 2:
		return FVector(1, -1, Point.X);
	case 3:
		return FVector(-1, 0, Point.X);
	case 4:
		return FVector(0, 0, Point.X);
	case 5:
		return FVector(1, 0, Point.X);
	case 6:
		return FVector(-1, 1, Point.X);
	case 7:
		return FVector(0, 1, Point.X);
	case 8:
		return FVector(1, 1, Point.X);
	}
	return FVector();
}

void FTiledItemSetEditor::ClearAllFixedItem()
{
	for (AActor* Actor : SpawnedFixedActors)
	{
		if (Actor)
			Actor->Destroy();
	}
	SpawnedFixedActors.Empty();
	FixedItemsInfo.Empty();
}

TSharedRef<SDockTab> FTiledItemSetEditor::SpawnTab_Palette(const FSpawnTabArgs& Args)
{
	TSharedPtr<FTiledItemSetEditor> EditorPtr = SharedThis(this);

	return SNew(SDockTab)
		// .Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.PlacementBrowser"))
		.Label(LOCTEXT("PaletteTab_Title", "Palette"))
	[
		SAssignNew(PalettePtr, STiledPalette, EditorPtr, nullptr, GetTiledItemSetBeingEdited())
	];
}

TSharedRef<SDockTab> FTiledItemSetEditor::SpawnTab_Preview(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("PreviewTab_Title", "Preview"))
		[
			ViewportPtr.ToSharedRef()
		];
}

TSharedRef<SDockTab> FTiledItemSetEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	TSharedPtr<FTiledItemSetEditor> EditorPtr = SharedThis(this);

	return SNew(SDockTab)
		// .Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
		.Label(LOCTEXT("DetailsTab_Title", "Details"))
	[
		SNew(STiledItemSetPropertiesTabBody, EditorPtr)
	];
}

TSharedRef<SDockTab> FTiledItemSetEditor::SpawnTab_ContentBrowser(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		.Label(LOCTEXT("ContentBrowser_Title", "Content Browser"))
		.TabColorScale(GetTabColorScale())
		[
			SNullWidget::NullWidget
		];

	// spent sometime to figure out how to set filter here... failed...

	IContentBrowserSingleton& ContentBrowserSingleton = FModuleManager::LoadModuleChecked<
		FContentBrowserModule>("ContentBrowser").Get();
	const FName ContentBrowserID = *("TiledItemSetEditor_ContentBrowser_" + FGuid::NewGuid().ToString());
	FContentBrowserConfig ContentBrowserConfig;
	ContentBrowserConfig.bCanSetAsPrimaryBrowser = true;
	TSharedRef<SWidget> ContentBrowser = ContentBrowserSingleton.CreateContentBrowser(
		ContentBrowserID, SpawnedTab, &ContentBrowserConfig);
	SpawnedTab->SetContent(ContentBrowser);
	return SpawnedTab;
}

TSharedRef<SDockTab> FTiledItemSetEditor::SpawnTab_PreviewSettings(const FSpawnTabArgs& Args)
{
	TSharedPtr<FAdvancedPreviewScene> PreviewScene;
	TSharedPtr<SWidget> SettingsWidget = SNullWidget::NullWidget;
	if (ViewportPtr.IsValid())
	{
		PreviewScene = ViewportPtr->GetPreviewScene();
	}
	if (PreviewScene.IsValid())
	{
		TSharedPtr<SAdvancedPreviewDetailsTab> PreviewSettingsWidget = SNew(
			SAdvancedPreviewDetailsTab, PreviewScene.ToSharedRef());
		PreviewSettingsWidget->Refresh();
		SettingsWidget = PreviewSettingsWidget;
	}

	return SNew(SDockTab)
	// .Icon(FEditorStyle::GetBrush("Kismet.Tabs.Palette"))
	.Label(LOCTEXT("PreviewSettings_Title", "Preview Settings"))
	[
		SettingsWidget.ToSharedRef()
	];
}

void FTiledItemSetEditor::UpdatePreviewMesh(UTiledLevelItem* Item)
{
	if (!IsValid(Item->TiledMesh)) return;
	PreviewMesh->SetStaticMesh(Item->TiledMesh);
	if (Item->OverrideMaterials.Num() > 0)
	{
		for (int i = 0; i < Item->OverrideMaterials.Num(); i++)
		{
		if (UMaterialInterface* M = Item->OverrideMaterials[i])
			PreviewMesh->SetMaterial(i, M);
		}
	}
	else
	{
		for (int i = 0; i < PreviewMesh->GetNumMaterials(); i++)
		{
			PreviewMesh->SetMaterial(i, Item->TiledMesh->GetMaterial(i));
		}
	}
}


#undef LOCTEXT_NAMESPACE
