// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledItem_ThumbnailRenderer.h"
#include "ThumbnailHelpers.h"
#include "TiledLevelEditorLog.h"
#include "TiledLevelEditor/TiledLevelEdMode.h"
#include "TiledLevelItem.h"
#include "CanvasTypes.h"
#include "TiledLevelRestrictionHelper.h"


bool UTiledItem_ThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	UTiledLevelItem* Item = Cast<UTiledLevelItem>(Object);
    if (Item->IsA<UTiledLevelTemplateItem>())
        return false;
	return IsValid(Item);
}

void UTiledItem_ThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height,
	FRenderTarget* Viewport, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	UTiledLevelItem* Item = Cast<UTiledLevelItem>(Object);
	
	if (Cast<UTiledLevelRestrictionItem>(Object))
	{
        Canvas->DrawTile(
             (float)X,
             (float)Y,
             (float)Width,
             (float)Height,
             0.0f,
             0.0f,
             4.0f,
             4.0f,
             FLinearColor(0.05, 0.05, 0.05, 1.0f));
	    FString Str1 = TEXT("Restriction");
	    FString Str2 = TEXT("Rule");
	    int EXL1, EYL1, EXL2, EYL2;
	    const UFont* UsedFont = GEngine->GetLargeFont();
	    StringSize(UsedFont, EXL1, EYL1, *Str1);
	    StringSize(UsedFont, EXL2, EYL2, *Str2);
	    Canvas->DrawShadowedString((Width - EXL1)/2 , (Height - EYL1)/2 - 8.f, *Str1, UsedFont, FLinearColor::White); 
	    Canvas->DrawShadowedString((Width - EXL2)/2 , (Height - EYL2)/2 + 8.f, *Str2, UsedFont, FLinearColor::White);
	}
	else if (Item->TiledMesh)
	{
		if (StaticMeshThumbnailScene == nullptr)
		{
			StaticMeshThumbnailScene = new FStaticMeshThumbnailScene();
		}
		
		StaticMeshThumbnailScene->SetStaticMesh(Item->TiledMesh);
		
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(Viewport, StaticMeshThumbnailScene->GetScene(), FEngineShowFlags(ESFIM_Game))
			.SetTime(UThumbnailRenderer::GetTime())
			.SetAdditionalViewFamily(bAdditionalViewFamily));
		
		ViewFamily.EngineShowFlags.DisableAdvancedFeatures();
		ViewFamily.EngineShowFlags.MotionBlur = 0;
		ViewFamily.EngineShowFlags.LOD = 0;
		
		RenderViewFamily(Canvas, &ViewFamily, StaticMeshThumbnailScene->CreateView(&ViewFamily, X, Y, Width, Height));
		StaticMeshThumbnailScene->SetStaticMesh(nullptr);
		StaticMeshThumbnailScene->SetOverrideMaterials(TArray<class UMaterialInterface*>());
	}
	else if (Item->TiledActor)
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(Item->TiledActor);
		
		// Strict validation - it may hopefully fix UE-35705.
		const bool bIsBlueprintValid = IsValid(Blueprint)
			&& IsValid(Blueprint->GeneratedClass)
			&& Blueprint->bHasBeenRegenerated
			//&& Blueprint->IsUpToDate() - This condition makes the thumbnail blank whenever the BP is dirty. It seems too strict.
			&& !Blueprint->bBeingCompiled
			&& !Blueprint->HasAnyFlags(RF_Transient);
		if (bIsBlueprintValid)
		{
			TSharedRef<FBlueprintThumbnailScene> ThumbnailScene = BlueprintThumbnailScene.EnsureThumbnailScene(Blueprint->GeneratedClass);
		
			ThumbnailScene->SetBlueprint(Blueprint);
			FSceneViewFamilyContext ViewFamily( FSceneViewFamily::ConstructionValues( Viewport, ThumbnailScene->GetScene(), FEngineShowFlags(ESFIM_Game) )
				.SetTime(UThumbnailRenderer::GetTime())
				.SetAdditionalViewFamily(bAdditionalViewFamily));
		
			ViewFamily.EngineShowFlags.DisableAdvancedFeatures();
			ViewFamily.EngineShowFlags.MotionBlur = 0;
		
			RenderViewFamily(Canvas,&ViewFamily, ThumbnailScene->CreateView(&ViewFamily, X, Y, Width, Height));
		}
	}

}

void UTiledItem_ThumbnailRenderer::BeginDestroy()
{
	if (StaticMeshThumbnailScene != nullptr)
	{
		delete StaticMeshThumbnailScene;
		StaticMeshThumbnailScene = nullptr;
	}
	BlueprintThumbnailScene.Clear();

	Super::BeginDestroy();
}

bool UTiledItem_ThumbnailRenderer::AllowsRealtimeThumbnails(UObject* Object) const
{
	return false;
}
