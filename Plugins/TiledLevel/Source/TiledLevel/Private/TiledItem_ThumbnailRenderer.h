// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "ThumbnailHelpers.h"
#include "ThumbnailRendering/DefaultSizedThumbnailRenderer.h"
#include "TiledItem_ThumbnailRenderer.generated.h"

UCLASS(CustomConstructor, Config=Editor)
class TILEDLEVEL_API UTiledItem_ThumbnailRenderer : public UDefaultSizedThumbnailRenderer
{
	GENERATED_UCLASS_BODY()
	
	UTiledItem_ThumbnailRenderer(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer), StaticMeshThumbnailScene(nullptr)
	{}

	virtual bool CanVisualizeAsset(UObject* Object) override;
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Viewport,
		FCanvas* Canvas, bool bAdditionalViewFamily) override;
	virtual void BeginDestroy() override;
	virtual bool AllowsRealtimeThumbnails(UObject* Object) const override;
	
private:
	class  FStaticMeshThumbnailScene* StaticMeshThumbnailScene;
	TClassInstanceThumbnailScene<FBlueprintThumbnailScene, 100> BlueprintThumbnailScene;
	// TClassInstanceThumbnailScene<FClassThumbnailScene, 100> SpecialThumbnailScene;
};
