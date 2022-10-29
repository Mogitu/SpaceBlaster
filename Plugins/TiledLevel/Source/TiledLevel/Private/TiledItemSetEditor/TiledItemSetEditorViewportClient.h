// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

class FTiledItemSetEditorViewportClient : public FEditorViewportClient
{
public:
    FTiledItemSetEditorViewportClient(
    	TWeakPtr<class FTiledItemSetEditor> InSetEditor,
    	const TWeakPtr<class SEditorViewport>& InEditorViewportWidget,
    	FPreviewScene& InPreviewScene,
    	class UTiledItemSet* InDataSource
    );
	
    virtual void ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX,
	    uint32 HitY) override;
    virtual void Tick(float DeltaSeconds) override;

private:
	TWeakPtr<class FTiledItemSetEditor> TiledItemSetEditor;
};
