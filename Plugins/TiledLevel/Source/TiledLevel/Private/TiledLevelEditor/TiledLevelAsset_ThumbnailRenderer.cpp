// Copyright 2022 PufStudio. All Rights Reserved.


#include "TiledLevelAsset_ThumbnailRenderer.h"

#include "TiledLevel.h"
#include "TiledLevelAsset.h"
#include "TiledLevelItem.h"
#include "Kismet/GameplayStatics.h"
#include "ThumbnailRendering/SceneThumbnailInfo.h"


FTiledLevelAsset_ThumbnailScene::FTiledLevelAsset_ThumbnailScene()
    : FThumbnailPreviewScene()
{
    bForceAllUsedMipsResident = false;
}

void FTiledLevelAsset_ThumbnailScene::SetAsset(UTiledLevelAsset* InTiledLevelAsset)
{
    if (InTiledLevelAsset)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        SpawnParameters.bNoFail = true;
        SpawnParameters.ObjectFlags = RF_Transactional;
        PreviewActor = GetWorld()->SpawnActor<ATiledLevel>(SpawnParameters);
        PreviewActor->SetActiveAsset(InTiledLevelAsset);
        PreviewActor->ResetAllInstance(true);
        FVector Origin, Extent;
        PreviewActor->GetActorBounds(false, Origin, Extent, true);
        PreviewActor->SetActorLocation(-Origin + FVector(0, 0, Extent.Z + 1));
    }
    else
    {
        PreviewActor->RemoveAsset();
    }
}

void FTiledLevelAsset_ThumbnailScene::GetViewMatrixParameters(const float InFOVDegrees, FVector& OutOrigin,
    float& OutOrbitPitch, float& OutOrbitYaw, float& OutOrbitZoom) const
{
    if (!PreviewActor) return;
    FVector Origin, Extent;
    PreviewActor->GetActorBounds(false, Origin, Extent, true);
	const float HalfFOVRadians = FMath::DegreesToRadians<float>(InFOVDegrees) * 0.5f;
	const float HalfMeshSize = Extent.X; 
	const float BoundsZOffset = Extent.Z + 1;
	const float TargetDistance = HalfMeshSize / FMath::Tan(HalfFOVRadians);
	USceneThumbnailInfo* ThumbnailInfo = Cast<USceneThumbnailInfo>(PreviewActor->GetAsset()->ThumbnailInfo);
    if (ThumbnailInfo)
    {
        if (TargetDistance + ThumbnailInfo->OrbitZoom < 0)
        {
            ThumbnailInfo->OrbitZoom = - TargetDistance;
        }
    }
    else
    {
	    ThumbnailInfo = USceneThumbnailInfo::StaticClass()->GetDefaultObject<USceneThumbnailInfo>();
    }
	OutOrigin = -Origin + FVector(0, 0, BoundsZOffset);
	OutOrbitPitch = ThumbnailInfo->OrbitPitch;
	OutOrbitYaw = ThumbnailInfo->OrbitYaw;
	OutOrbitZoom = TargetDistance + ThumbnailInfo->OrbitZoom;
}

void UTiledLevelAsset_ThumbnailRenderer::BeginDestroy()
{
    if (ThumbnailScene != nullptr)
    {
        ThumbnailScene->GetPreviewActor()->Destroy();
        delete ThumbnailScene;
        ThumbnailScene = nullptr;
    }
    
    Super::BeginDestroy();
}

bool UTiledLevelAsset_ThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
    if (UTiledLevelTemplateItem* TemplateItem = Cast<UTiledLevelTemplateItem>(Object))
    {
        return IsValid(TemplateItem);
    }
    UTiledLevelAsset* TLA = Cast<UTiledLevelAsset>(Object);
    return IsValid(TLA);
}

void UTiledLevelAsset_ThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height,
    FRenderTarget* Viewport, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	UTiledLevelAsset* TLA;
    if (UTiledLevelTemplateItem* TemplateItem = Cast<UTiledLevelTemplateItem>(Object))
    {
        TLA = TemplateItem->GetAsset();
    }
    else
    {
        TLA = Cast<UTiledLevelAsset>(Object);
    }
	if (TLA)
	{
		if ( ThumbnailScene == nullptr )
		{
			ThumbnailScene = new FTiledLevelAsset_ThumbnailScene();
            ThumbnailScene->SetAsset(TLA);
		}
	    if (ThumbnailScene->GetPreviewActor()->GetAsset() != TLA)
	    {
	        ThumbnailScene->SetAsset(TLA);
	    }
	    // Fix unknown issue that tiled level actor is stacked in same scene...
	    if (UWorld* World = ThumbnailScene->GetPreviewActor()->GetWorld())
	    {
	        TArray<AActor*> OutActors;
            UGameplayStatics::GetAllActorsOfClass(World, ATiledLevel::StaticClass(), OutActors);
	        for (AActor* A : OutActors)
	        {
	            if (A != ThumbnailScene->GetPreviewActor())
	                A->Destroy();
	        }
	    }

		FSceneViewFamilyContext ViewFamily( FSceneViewFamily::ConstructionValues( Viewport, ThumbnailScene->GetScene(), FEngineShowFlags(ESFIM_Game) )
			.SetTime(UThumbnailRenderer::GetTime())
			.SetAdditionalViewFamily(bAdditionalViewFamily));

		ViewFamily.EngineShowFlags.DisableAdvancedFeatures();
		ViewFamily.EngineShowFlags.MotionBlur = 0;
		ViewFamily.EngineShowFlags.LOD = 0;

		
		RenderViewFamily(Canvas,&ViewFamily, ThumbnailScene->CreateView(&ViewFamily, X, Y, Width, Height));
	    
	}

}

