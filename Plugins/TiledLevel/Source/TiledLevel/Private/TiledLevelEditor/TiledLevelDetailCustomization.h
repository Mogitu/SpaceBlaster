// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class TILEDLEVEL_API FTiledLevelDetailCustomization : public IDetailCustomization
{
public:
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	static TSharedRef<IDetailCustomization> MakeInstance();

private:
	TWeakObjectPtr<class UTiledLevelAsset>  TiledLevelAssetPtr;
	TWeakObjectPtr<class ATiledLevel>  TiledLevelActorPtr;
	IDetailLayoutBuilder* MyDetailLayout = nullptr;
	TSharedPtr<class STiledFloorList> FloorListWidget;
	
	void OnResetInstances();
	FText GetFloorSettingsHeadingText() const;
	bool IsInstanced() const;
	bool IsInEditor() const;
	EVisibility GetVisibilityBasedOnIsActor(bool ShouldReverse = false) const;
	EVisibility GetVisibilityBasedOnIsInstanced(bool ShouldReverse = false) const;
	FReply OnEditInAssetClicked();
	FReply OnPromoteToInstanceClicked();
	FReply OnEditInLevelClicked();
	FReply OnPromoteToAssetClicked();
	FReply OnApplyGametimeRequiredTransformClicked();
	FReply OnCheckOverlapClicked();
	// FReply OnRegenerateGameIdClicked();

	// UObject* GetSystemControllingThisLevel() const;
};
