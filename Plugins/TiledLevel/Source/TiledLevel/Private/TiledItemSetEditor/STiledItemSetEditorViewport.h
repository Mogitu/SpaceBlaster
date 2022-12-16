// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "SCommonEditorViewportToolbarBase.h"
#include "SEditorViewport.h"

class STiledItemSetEditorViewport : public SEditorViewport, public ICommonEditorViewportToolbarInfoProvider
{
public:
	SLATE_BEGIN_ARGS(STiledItemSetEditorViewport);
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, TSharedPtr<class FTiledItemSetEditor> InEditor);
	~STiledItemSetEditorViewport();

	// editor viewport interface
	virtual void BindCommands() override;
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() override;
	virtual EVisibility GetTransformToolbarVisibility() const override;
	virtual void OnFocusViewportToSelection() override;
	//end of editor viewport

	// common editor viewport toolbar
	virtual TSharedRef<SEditorViewport> GetViewportWidget() override;
	virtual TSharedPtr<FExtender> GetExtenders() const override;
	virtual void OnFloatingButtonClicked() override;
	// end of common editor viewport toolbar

	virtual void PopulateViewportOverlays(TSharedRef<SOverlay> Overlay) override;
	
	TSharedPtr<class FAdvancedPreviewScene> GetPreviewScene() const { return PreviewScene; }
	TWeakPtr<FTiledItemSetEditor> GetSetEditor() const { return TiledItemSetEditorPtr; }
private:
	TWeakPtr<FTiledItemSetEditor> TiledItemSetEditorPtr;

	// viewport client
	TSharedPtr<class FTiledItemSetEditorViewportClient> EditorViewportClient;

	// The preview scenes
    TSharedPtr<class FAdvancedPreviewScene> PreviewScene;
    class USkyLightComponent* Skylight = nullptr;
    class USkyAtmosphereComponent* AtmosphericFog = nullptr;
	
};
