// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

class FTiledLevelViewportClient : public FEditorViewportClient
{
public:
	FTiledLevelViewportClient(
		TWeakPtr<class FTiledLevelEditor> InLevelEditor,
		const TWeakPtr<class SEditorViewport>& InEditorViewportWidget,
		FPreviewScene& InPreviewScene,
		class UTiledLevelAsset* InDataSource
	);
	
	virtual ~FTiledLevelViewportClient() override;
	
    virtual void Tick(float DeltaSeconds) override;
    virtual void ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	
	void ActivateEdMode();
	const UStaticMeshComponent* PreviewSceneFloor;

private:
	TWeakPtr<class FTiledLevelEditor> LevelEditorPtr;
	TWeakObjectPtr<UTiledLevelAsset> DataSource;
	class FTiledLevelEdMode* EdMode = nullptr;
};
