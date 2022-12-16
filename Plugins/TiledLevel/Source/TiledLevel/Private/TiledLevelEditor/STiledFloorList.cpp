// Copyright 2022 PufStudio. All Rights Reserved.

#include "STiledFloorList.h"
#include "EditorModeManager.h"
#include "TiledLevelStyle.h"
#include "STiledFloor.h"
#include "TiledLevelCommands.h"
#include "TiledLevelEditorLog.h"
#include "Framework/Commands/GenericCommands.h"
#include "Widgets/Layout/SExpandableArea.h"

#define LOCTEXT_NAMESPACE "TiledLevel"

void STiledFloorList::Construct(const FArguments& InArgs, UTiledLevelAsset* InTiledLevel, FNotifyHook* InNotifyHook, TSharedPtr<class FUICommandList> InParentCommandList)
{
	TiledLevelAssetPtr = InTiledLevel;
	NotifyHook = InNotifyHook;
	OnResetInstances = InArgs._OnResetInstances;

	FTiledLevelCommands::Register();
	const FTiledLevelCommands& TiledLevelCommands = FTiledLevelCommands::Get();
	const FGenericCommands& GenericCommands = FGenericCommands::Get();
	
	CommandList = MakeShareable(new FUICommandList);
	InParentCommandList->Append(CommandList.ToSharedRef());

	CommandList->MapAction(
		TiledLevelCommands.AddNewFloorAbove,
		FExecuteAction::CreateSP(this, &STiledFloorList::AddNewFloorAbove));
	CommandList->MapAction(
		TiledLevelCommands.AddNewFloorBelow,
		FExecuteAction::CreateSP(this, &STiledFloorList::AddNewFloorBelow));
	CommandList->MapAction(
		TiledLevelCommands.MoveFloorUp,
		FExecuteAction::CreateSP(this, &STiledFloorList::MoveFloorUp, false),
		FCanExecuteAction::CreateSP(this, &STiledFloorList::CanExecuteActionNeedingFloorAbove));
	CommandList->MapAction(
		TiledLevelCommands.MoveFloorDown,
		FExecuteAction::CreateSP(this, &STiledFloorList::MoveFloorDown, false),
		FCanExecuteAction::CreateSP(this, &STiledFloorList::CanExecuteActionNeedingFloorBelow));
	CommandList->MapAction(
		TiledLevelCommands.MoveAllFloorsUp,
		FExecuteAction::CreateSP(this, &STiledFloorList::MoveAllFloors, true));
	CommandList->MapAction(
		TiledLevelCommands.MoveAllFloorsDown,
		FExecuteAction::CreateSP(this, &STiledFloorList::MoveAllFloors, false));
	CommandList->MapAction(
		TiledLevelCommands.MoveFloorToTop,
		FExecuteAction::CreateSP(this, &STiledFloorList::MoveFloorUp, true),
		FCanExecuteAction::CreateSP(this, &STiledFloorList::CanExecuteActionNeedingFloorAbove));
	CommandList->MapAction(
		TiledLevelCommands.MoveFloorToBottom,
		FExecuteAction::CreateSP(this, &STiledFloorList::MoveFloorDown, true),
		FCanExecuteAction::CreateSP(this, &STiledFloorList::CanExecuteActionNeedingFloorBelow));
	CommandList->MapAction(
		TiledLevelCommands.HideAllFloors,
		FExecuteAction::CreateSP(this, &STiledFloorList::HideAllFloors));
	CommandList->MapAction(
		TiledLevelCommands.UnhideAllFloors,
		FExecuteAction::CreateSP(this, &STiledFloorList::UnhideAllFloors));
	CommandList->MapAction(
		TiledLevelCommands.HideOtherFloors,
		FExecuteAction::CreateSP(this, &STiledFloorList::HideOtherFloors),
		FCanExecuteAction::CreateSP(this, &STiledFloorList::HasOtherFloors));
	CommandList->MapAction(
		TiledLevelCommands.UnhideOtherFloors,
		FExecuteAction::CreateSP(this, &STiledFloorList::UnhideOtherFloors),
		FCanExecuteAction::CreateSP(this, &STiledFloorList::HasOtherFloors));
	
	
	// Generic commands...
	CommandList->MapAction(
		GenericCommands.Duplicate,
		FExecuteAction::CreateSP(this, &STiledFloorList::DuplicateFloor),
		FCanExecuteAction::CreateSP(this, &STiledFloorList::CanExecuteActionNeedingSelectedFloor));
	
	CommandList->MapAction(
		GenericCommands.Delete,
		FExecuteAction::CreateSP(this, &STiledFloorList::DeleteFloor),
		FCanExecuteAction::CreateSP(this, &STiledFloorList::CanExecuteActionNeedingSelectedFloor));
	
	CommandList->MapAction(
		TiledLevelCommands.SelectFloorAbove,
		FExecuteAction::CreateSP(this, &STiledFloorList::SelectFloorAbove, false),
		FCanExecuteAction::CreateSP(this, &STiledFloorList::CanExecuteActionNeedingSelectedFloor));
	CommandList->MapAction(
		TiledLevelCommands.SelectFloorBelow,
		FExecuteAction::CreateSP(this, &STiledFloorList::SelectFloorBelow, false),
		FCanExecuteAction::CreateSP(this, &STiledFloorList::CanExecuteActionNeedingSelectedFloor));

	FToolBarBuilder ToolbarBuilder(CommandList, FMultiBoxCustomization("TiledLevelLayerBrowserToolbar"),
	                               TSharedPtr<FExtender>(), true);
	ToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);
	ToolbarBuilder.AddToolBarButton(TiledLevelCommands.AddNewFloorAbove);
	ToolbarBuilder.AddToolBarButton(TiledLevelCommands.AddNewFloorBelow);
	ToolbarBuilder.AddToolBarButton(TiledLevelCommands.MoveFloorUp);
	ToolbarBuilder.AddToolBarButton(TiledLevelCommands.MoveFloorDown);
	ToolbarBuilder.AddToolBarButton(TiledLevelCommands.MoveAllFloorsUp);
	ToolbarBuilder.AddToolBarButton(TiledLevelCommands.MoveAllFloorsDown);
	

	FSlateIcon DuplicateIcon(FTiledLevelStyle::GetStyleSetName(), "TiledLevel.DuplicateFloor"); 
	ToolbarBuilder.AddToolBarButton(GenericCommands.Duplicate, NAME_None, TAttribute<FText>(), LOCTEXT("DuplicateFloorTooltip", "Duplicate selected floor."), DuplicateIcon);
	FSlateIcon DeleteIcon(FTiledLevelStyle::GetStyleSetName(), "TiledLevel.DeleteFloor"); 
	ToolbarBuilder.AddToolBarButton(GenericCommands.Delete, NAME_None, TAttribute<FText>(), LOCTEXT("DeleteFloorTooltip", "Delete selected floor."), DeleteIcon);

	TSharedRef<SWidget> Toolbar = ToolbarBuilder.MakeWidget();

	ListViewWidget = SNew(STiledFloorListView)
	.SelectionMode(ESelectionMode::Single)
	.ClearSelectionOnClick(false)
	.ListItemsSource(&MirrorList)
	.OnSelectionChanged(this, &STiledFloorList::OnSelectionChanged)
	.OnGenerateRow(this, &STiledFloorList::OnGenerateLayerListRow)
	.OnContextMenuOpening(this, &STiledFloorList::OnConstructContextMenu);

	RefreshMirrorList();

	if (UTiledLevelAsset* TiledLevelAsset = TiledLevelAssetPtr.Get())
	{
		SetSelectedFloor(TiledLevelAsset->ActiveFloorPosition);
	}
	
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(SBox)
			.HeightOverride(135.0f)
			[
				ListViewWidget.ToSharedRef()
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			Toolbar
		]
	];

	GEditor->RegisterForUndo(this);
}

void STiledFloorList::PostUndo(bool bSuccess)
{
	RefreshMirrorList();
}

void STiledFloorList::PostRedo(bool bSuccess)
{
	RefreshMirrorList();
}

STiledFloorList::~STiledFloorList()
{
}

void STiledFloorList::RefreshMirrorList()
{
	MirrorList.Empty();
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		TiledLevelAsset->TiledFloors.Sort();
		
		for (FTiledFloor& F : TiledLevelAsset->TiledFloors)
		{
			TSharedPtr<int32> NewEntry = MakeShareable(new int32);
			*NewEntry = F.FloorPosition;
			MirrorList.Add(NewEntry);
		}
	}
	ListViewWidget->RequestListRefresh();
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		SetSelectedFloor(TiledLevelAsset->ActiveFloorPosition);
	}
}

int32 STiledFloorList::GetNumFloors() const
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		return TiledLevelAsset->TiledFloors.Num();
	}
	return 0;
}

int32 STiledFloorList::GetMaxFloorPosition() const
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		// the operator< overload is in reversed way... so use "min to get max"
		return FMath::Min(TiledLevelAsset->TiledFloors).FloorPosition;
	}
	return INDEX_NONE;
}

int32 STiledFloorList::GetMinFloorPosition() const
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		// the operator< overload is in reversed way... so use "min to get max"
		return FMath::Max(TiledLevelAsset->TiledFloors).FloorPosition;
	}
	return INDEX_NONE;
}

void STiledFloorList::SelectFloorAbove(bool bTopmost)
{
	const int32 SelectedIndex = GetSelectionPosition();
	const int32 NewIndex = bTopmost ? GetMaxFloorPosition() : SelectedIndex + 1;
	SetSelectedFloor(NewIndex);
	PostEditNotifications();
}

void STiledFloorList::SelectFloorBelow(bool bBottommost)
{
	const int32 SelectedIndex = GetSelectionPosition();
	const int32 NewIndex = bBottommost ? GetMinFloorPosition() : SelectedIndex - 1;
	SetSelectedFloor(NewIndex);
	PostEditNotifications();
}

FTiledFloor* STiledFloorList::GetActiveFloor() const
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		if (ListViewWidget->GetNumItemsSelected() > 0)
		{
			FMirrorEntry SelectedItem = ListViewWidget->GetSelectedItems()[0];
			const int32 SelectedIndex = *SelectedItem;
			return TiledLevelAsset->GetFloorFromPosition(SelectedIndex);
		}
	}
	return nullptr;
}

int32 STiledFloorList::GetSelectionPosition() const
{
	int32 PositionIndex = -999;
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		if (ListViewWidget->GetNumItemsSelected() > 0)
		{
			FMirrorEntry SelectedItem = ListViewWidget->GetSelectedItems()[0];
			PositionIndex = *SelectedItem;
		}
	}
	return PositionIndex;
}

TSharedRef<ITableRow> STiledFloorList::OnGenerateLayerListRow(FMirrorEntry Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	using RowType = STableRow<FMirrorEntry>;

	TSharedRef<RowType> NewRow = SNew(RowType, OwnerTable)
		.Style(&FTiledLevelStyle::Get().GetWidgetStyle<FTableRowStyle>("TiledLevel.LayerBrowser.TableViewRow"));
	FIsSelected IsSelectedDelegate = FIsSelected::CreateSP(NewRow, &RowType::IsSelectedExclusively);
	NewRow->SetContent(SNew(STiledFloor, *Item, TiledLevelAssetPtr.Get(), IsSelectedDelegate).OnResetInstances(OnResetInstances));
	return NewRow;
}

TSharedPtr<SWidget> STiledFloorList::OnConstructContextMenu()
{
	const FTiledLevelCommands& TiledLevelCommands = FTiledLevelCommands::Get();
	const FGenericCommands& GenericCommands = FGenericCommands::Get();
	FMenuBuilder MenuBuilder(true, CommandList);

	FSlateIcon DummyIcon(NAME_None, NAME_None);

	MenuBuilder.BeginSection("InstanceOperations", LOCTEXT("InstaceOperationsHeader", "Instance actions"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ClearInstace", "Clear Instance"),
			LOCTEXT("ClearInstanceTooltip", "Clear all instance in this floor"),
			DummyIcon,
			FUIAction(FExecuteAction::CreateRaw(this, &STiledFloorList::ClearFloorContent))
			);
	}
	MenuBuilder.EndSection();
	
	MenuBuilder.BeginSection("BasicOperations", LOCTEXT("BasicOperationsHeader", "Floor actions"));
 	{
		MenuBuilder.AddMenuEntry(GenericCommands.Duplicate, NAME_None, TAttribute<FText>(), TAttribute<FText>(), DummyIcon);
		MenuBuilder.AddMenuEntry(GenericCommands.Delete, NAME_None, TAttribute<FText>(), TAttribute<FText>(), DummyIcon);
		MenuBuilder.AddMenuSeparator();
		MenuBuilder.AddMenuEntry(TiledLevelCommands.SelectFloorAbove, NAME_None, TAttribute<FText>(), TAttribute<FText>(), DummyIcon);
		MenuBuilder.AddMenuEntry(TiledLevelCommands.SelectFloorBelow, NAME_None, TAttribute<FText>(), TAttribute<FText>(), DummyIcon);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("OrderingOperations", LOCTEXT("OrderingOperationsHeader", "Order actions"));
	{
		MenuBuilder.AddMenuEntry(TiledLevelCommands.MoveFloorToTop, NAME_None, TAttribute<FText>(), TAttribute<FText>(), DummyIcon);
		MenuBuilder.AddMenuEntry(TiledLevelCommands.MoveFloorUp, NAME_None, TAttribute<FText>(), TAttribute<FText>(), DummyIcon);
		MenuBuilder.AddMenuEntry(TiledLevelCommands.MoveFloorDown, NAME_None, TAttribute<FText>(), TAttribute<FText>(), DummyIcon);
		MenuBuilder.AddMenuEntry(TiledLevelCommands.MoveFloorToBottom, NAME_None, TAttribute<FText>(), TAttribute<FText>(), DummyIcon);
	}
	MenuBuilder.EndSection();
	
	MenuBuilder.BeginSection("DisplayOperations", LOCTEXT("DisplayOperationsHeader", "Display actions"));
	{
		MenuBuilder.AddMenuEntry(TiledLevelCommands.HideAllFloors, NAME_None, TAttribute<FText>(), TAttribute<FText>(), DummyIcon);
		MenuBuilder.AddMenuEntry(TiledLevelCommands.HideOtherFloors, NAME_None, TAttribute<FText>(), TAttribute<FText>(), DummyIcon);
		MenuBuilder.AddMenuEntry(TiledLevelCommands.UnhideAllFloors, NAME_None, TAttribute<FText>(), TAttribute<FText>(), DummyIcon);
		MenuBuilder.AddMenuEntry(TiledLevelCommands.UnhideOtherFloors, NAME_None, TAttribute<FText>(), TAttribute<FText>(), DummyIcon);
	}
	MenuBuilder.EndSection();
	
	return MenuBuilder.MakeWidget();
}

void STiledFloorList::AddFloor(int32 PositionIndex)
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("TiledLevelAddFloorTransaction", "Add New Floor"));
		TiledLevelAsset->SetFlags(RF_Transactional);
		TiledLevelAsset->Modify();

		int NewFloorPosition = TiledLevelAsset->AddNewFloor(PositionIndex);

		// Change the selection set to select it
		PostEditNotifications(true);
		SetSelectedFloor(NewFloorPosition);
		TiledLevelAsset->VersionNumber += 1;
	}
}

void STiledFloorList::SetSelectedFloor(int32 PositionIndex)
{
	for (auto M : MirrorList)
	{
		if (PositionIndex == *M.Get())
		{
			ListViewWidget->SetSelection(M);
			return;
		}
	}
}

void STiledFloorList::ChangeFloorOrdering(int32 OldPosition, int32 NewPosition)
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		if (TiledLevelAsset->IsFloorExists(OldPosition) && TiledLevelAsset->IsFloorExists(NewPosition))
		{

			for (auto& F : TiledLevelAsset->TiledFloors)
			{
				if (F.FloorPosition == OldPosition)
				{
					F.UpdatePosition(NewPosition, TiledLevelAsset->TileSizeZ);
				}
				else if (F.FloorPosition == NewPosition)
				{
					F.UpdatePosition(OldPosition, TiledLevelAsset->TileSizeZ);
				}
			}
			PostEditNotifications(true);
		}
		TiledLevelAsset->VersionNumber += 1;
	}
}

void STiledFloorList::AddNewFloorAbove()
{
	AddFloor(GetSelectionPosition()+1);
}

void STiledFloorList::AddNewFloorBelow()
{
	AddFloor(GetSelectionPosition()-1);
}

void STiledFloorList::MoveFloorUp(bool bForceToTop)
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("TiledLevelReorderLayerTransaction", "Reorder Layer"));
		TiledLevelAsset->SetFlags(RF_Transactional);
		TiledLevelAsset->Modify();
		const int32 SelectedIndex = GetSelectionPosition();
		const int32 MaxPosition = GetMaxFloorPosition();
		if (bForceToTop)
		{
			for (auto& F : TiledLevelAsset->TiledFloors)
			{
				if (F.FloorPosition == SelectedIndex)
				{
					F.UpdatePosition(MaxPosition, TiledLevelAsset->TileSizeZ);
				} else if (F.FloorPosition > SelectedIndex)
				{
					F.UpdatePosition(F.FloorPosition -1, TiledLevelAsset->TileSizeZ);
				}
			}
			PostEditNotifications(true);
		}
		else
		{
			ChangeFloorOrdering(SelectedIndex, SelectedIndex+1);
		}
		SelectFloorAbove(bForceToTop);
	}
}

void STiledFloorList::MoveFloorDown(bool bForceToBottom)
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("TiledLevelReorderLayerTransaction", "Reorder Layer"));
		TiledLevelAsset->SetFlags(RF_Transactional);
		TiledLevelAsset->Modify();
		const int32 SelectedIndex = GetSelectionPosition();
		const int32 MinPosition = GetMinFloorPosition();
		if (bForceToBottom)
		{
			for (auto& F : TiledLevelAsset->TiledFloors)
			{
				if (F.FloorPosition == SelectedIndex)
				{
					F.UpdatePosition(MinPosition, TiledLevelAsset->TileSizeZ);
				} else if (F.FloorPosition < SelectedIndex)
				{
					F.UpdatePosition(F.FloorPosition +1, TiledLevelAsset->TileSizeZ);
				}
			}
			PostEditNotifications(true);
		}
		else
		{
			ChangeFloorOrdering(SelectedIndex, SelectedIndex-1);
		}
		SelectFloorBelow(bForceToBottom);
	}
}

void STiledFloorList::MoveAllFloors(bool Up)
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		TiledLevelAsset->MoveAllFloors(Up);
		if (TiledLevelAsset->TiledFloors.Num() > 1)
		{
			int SelectedIndex = GetSelectionPosition();
			SelectedIndex = Up? SelectedIndex + 1 : SelectedIndex -1;
			SetSelectedFloor(SelectedIndex);
		} else
		{
			TiledLevelAsset->ActiveFloorPosition = GetMinFloorPosition();
			SetSelectedFloor(GetMinFloorPosition());
		}
		PostEditNotifications(true);
	}
}

void STiledFloorList::DuplicateFloor()
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("TiledLevelDuplicateLayerTransaction", "Duplicate Layer"));
		TiledLevelAsset->SetFlags(RF_Transactional);
		TiledLevelAsset->Modify();
		const int P = GetSelectionPosition();
		TiledLevelAsset->DuplicatedFloor(P);
		PostEditNotifications(true);
		// Select the duplicated layer
		SetSelectedFloor(P + 1);
	}
}

void STiledFloorList::DeleteFloor()
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		if (GetNumFloors() <= 1) return;
		bool bProceed = true;
		const int N_Placement = GetActiveFloor()->GetItemPlacements().Num();
		if ( N_Placement > 0)
		{
		 	FText Message = FText::Format(NSLOCTEXT("UnrealEd", "TiledLevelEditor_DeleteFloor", "Are you sure you want to remove this floor with {0} placements ?"), N_Placement);
		 	bProceed = (FMessageDialog::Open(EAppMsgType::YesNo, Message) == EAppReturnType::Yes);
		}
		if (bProceed)
		{
			const FScopedTransaction Transaction(LOCTEXT("TiledLevelDeleteFloorTransaction", "Delete Floor"));
			TiledLevelAsset->SetFlags(RF_Transactional);
			TiledLevelAsset->Modify();
			DeleteSelectedFloorWithNoTransaction();
		}
	}
}

void STiledFloorList::HideAllFloors()
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("TiledLevelHideAllFloorsTransaction", "Hide all floors"));
		TiledLevelAsset->Modify();
		
		for (FTiledFloor& Floor : TiledLevelAsset->TiledFloors)
		{
			Floor.ShouldRenderInEditor = false;
		}
		PostEditNotifications(true);
	}
}

void STiledFloorList::UnhideAllFloors()
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("TiledLevelUnhideAllFloorsTransaction", "Unhide all floors"));
		TiledLevelAsset->Modify();
		
		for (FTiledFloor& Floor : TiledLevelAsset->TiledFloors)
		{
			Floor.ShouldRenderInEditor = true;
		}
		PostEditNotifications(true);
	}
}

void STiledFloorList::HideOtherFloors()
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("TiledLevelHideOtherFloorsTransaction", "Hide other floors"));
		TiledLevelAsset->Modify();
		
		for (FTiledFloor& Floor : TiledLevelAsset->TiledFloors)
		{
			if (Floor.FloorPosition != GetActiveFloor()->FloorPosition)
				Floor.ShouldRenderInEditor = false;
		}
		PostEditNotifications(true);
	}
}

void STiledFloorList::UnhideOtherFloors()
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("TiledLevelUnhideOtherFloorsTransaction", "Unhide other floors"));
		TiledLevelAsset->Modify();
		
		for (FTiledFloor& Floor : TiledLevelAsset->TiledFloors)
		{
			if (Floor.FloorPosition != GetActiveFloor()->FloorPosition)
				Floor.ShouldRenderInEditor = true;
		}
		PostEditNotifications(true);
	}
}

bool STiledFloorList::HasOtherFloors() const
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		return TiledLevelAsset->TiledFloors.Num() > 1;
	}
	return false;
}

void STiledFloorList::DeleteSelectedFloorWithNoTransaction()
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		const int32 DeleteIndex = GetSelectionPosition();
		TiledLevelAsset->RemoveFloor(DeleteIndex);
		SetSelectedFloor(GetMinFloorPosition());
		PostEditNotifications(true);
	}
}

bool STiledFloorList::CanExecuteActionNeedingFloorAbove() const
{
	return GetSelectionPosition() < GetMaxFloorPosition();
}

bool STiledFloorList::CanExecuteActionNeedingFloorBelow() const
{
	return GetSelectionPosition() > GetMinFloorPosition();
}

bool STiledFloorList::CanExecuteActionNeedingSelectedFloor() const
{
	return GetSelectionPosition() != -999;
}

void STiledFloorList::OnSelectionChanged(FMirrorEntry ItemChangingState, ESelectInfo::Type SelectInfo)
{
	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		TiledLevelAsset->ActiveFloorPosition = GetSelectionPosition();
		TiledLevelAsset->MoveHelperFloorGrids.ExecuteIfBound(GetActiveFloor()->FloorPosition);
		ListViewWidget->RequestScrollIntoView(ItemChangingState);
	}
}

void STiledFloorList::ClearFloorContent() const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "TiledLevelMode_ClearInstanceTransaction", "Clear Element Instance in layer"));
	TiledLevelAssetPtr.Get()->Modify();
	GetActiveFloor()->BlockPlacements.Empty();
	GetActiveFloor()->FloorPlacements.Empty();
	GetActiveFloor()->WallPlacements.Empty();
	GetActiveFloor()->EdgePlacements.Empty();
	GetActiveFloor()->PillarPlacements.Empty();
	GetActiveFloor()->PointPlacements.Empty();
	OnResetInstances.Execute();
}

void STiledFloorList::PostEditNotifications(bool ShouldResetInstances)
{
	RefreshMirrorList();

	if (UTiledLevelAsset* TiledLevelAsset =  TiledLevelAssetPtr.Get())
	{
		TiledLevelAsset->PostEditChange();
	}
	if (ShouldResetInstances)
		OnResetInstances.Execute();
}

#undef LOCTEXT_NAMESPACE
