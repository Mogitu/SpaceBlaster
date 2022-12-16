// Copyright 2022 PufStudio. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "ThumbnailHelpers.h"
#include "ThumbnailRendering/DefaultSizedThumbnailRenderer.h"
#include "TiledLevelAsset_ThumbnailRenderer.generated.h"


class FTiledLevelAsset_ThumbnailScene : public FThumbnailPreviewScene
{
public:
    FTiledLevelAsset_ThumbnailScene();

    void SetAsset(class UTiledLevelAsset* InTiledLevelAsset);

    class ATiledLevel* GetPreviewActor() const { return PreviewActor; }
    
    
protected:
    virtual void GetViewMatrixParameters(const float InFOVDegrees, FVector& OutOrigin, float& OutOrbitPitch,
        float& OutOrbitYaw, float& OutOrbitZoom) const override;
    
private:
    class ATiledLevel* PreviewActor;
};


UCLASS(CustomConstructor, Config=Editor)
class TILEDLEVEL_API UTiledLevelAsset_ThumbnailRenderer : public UDefaultSizedThumbnailRenderer
{
    GENERATED_UCLASS_BODY()

    UTiledLevelAsset_ThumbnailRenderer(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer), ThumbnailScene(nullptr)
    {}

    virtual void BeginDestroy() override;
    virtual bool CanVisualizeAsset(UObject* Object) override;
    virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Viewport,
        FCanvas* Canvas, bool bAdditionalViewFamily) override;
    virtual bool AllowsRealtimeThumbnails(UObject* Object) const override { return true; }

protected:
    FTiledLevelAsset_ThumbnailScene* ThumbnailScene;
    
};
