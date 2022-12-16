// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Framework/Commands/UICommandList.h"
#include "Widgets/SCompoundWidget.h"
#include "EditorUndoClient.h"
#include "TiledLevelAsset.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"

class FNotifyHook;
template <typename ItemType> class SListView;

class STiledFloorList : public SCompoundWidget, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(STiledFloorList) {}
		SLATE_EVENT(FSimpleDelegate, OnResetInstances)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UTiledLevelAsset* InTiledLevel, FNotifyHook* InNotifyHook, TSharedPtr<class FUICommandList> InParentCommandList );
	
	// FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	// End of FEditorUndoClient

	~STiledFloorList();
	
	void RefreshMirrorList();
	int32 GetNumFloors() const;
	int32 GetMaxFloorPosition() const;
	int32 GetMinFloorPosition() const;
	void SelectFloorAbove(bool bTopmost);
	void SelectFloorBelow(bool bBottommost);
	struct FTiledFloor* GetActiveFloor() const;
	
private:
	typedef TSharedPtr<int32> FMirrorEntry;
	TArray<FMirrorEntry> MirrorList;
	typedef SListView<FMirrorEntry> STiledFloorListView;
	TSharedPtr<STiledFloorListView> ListViewWidget;
	TSharedPtr<class FUICommandList> CommandList;
	TWeakObjectPtr<UTiledLevelAsset> TiledLevelAssetPtr;
	FNotifyHook* NotifyHook = nullptr;
	FSimpleDelegate OnResetInstances;
	TSharedPtr<class SExpandableArea> FloorListArea;
	
	int32 GetSelectionPosition() const;	
	TSharedRef<ITableRow> OnGenerateLayerListRow(FMirrorEntry Item, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedPtr<SWidget> OnConstructContextMenu();
	
	void AddFloor(int32 PositionIndex = INDEX_NONE);
	void SetSelectedFloor(int32 PositionIndex);
	void ChangeFloorOrdering(int32 OldPosition, int32 NewPosition);
	void AddNewFloorAbove();
	void AddNewFloorBelow();
	void MoveFloorUp(bool bForceToTop);
	void MoveFloorDown(bool bForceToBottom);
	void MoveAllFloors(bool Up);
	void DuplicateFloor();
	void DeleteFloor();
	void HideAllFloors();
	void UnhideAllFloors();
	void HideOtherFloors();
	void UnhideOtherFloors();
	void DeleteSelectedFloorWithNoTransaction();
	bool CanExecuteActionNeedingFloorAbove() const;
	bool CanExecuteActionNeedingFloorBelow() const;
	bool CanExecuteActionNeedingSelectedFloor() const;
	bool HasOtherFloors() const;
	void OnSelectionChanged(FMirrorEntry ItemChangingState, ESelectInfo::Type SelectInfo);
	void ClearFloorContent() const;
	void PostEditNotifications(bool ShouldResetInstances = false); // Called after edits are finished

};

