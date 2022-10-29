// Copyright 2022 PufStudio. All Rights Reserved.

#include "STiledLevelEditorViewport.h"
#include "TiledLevelEditorViewportClient.h"
#include "AdvancedPreviewScene.h"
#include "STiledLevelEditorViewportToolbar.h"
#include "TiledLevelEditor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"


STiledLevelEditorViewport::FArguments::FArguments()
{
}

void STiledLevelEditorViewport::Construct(const FArguments& InArgs, TSharedPtr<class FTiledLevelEditor> InEditor)
{
	TiledLevelEditorPtr = InEditor;
	{
		FAdvancedPreviewScene::ConstructionValues CVS;
		CVS.bCreatePhysicsScene = true; // this is a must for line trace to work!!??
		CVS.LightBrightness = 2;
		CVS.SkyBrightness = 1;
		PreviewScene = MakeShareable(new FAdvancedPreviewScene(CVS));
	}
	
	SEditorViewport::Construct(SEditorViewport::FArguments());
	
	
    Skylight = NewObject<USkyLightComponent>();
    AtmosphericFog = NewObject<USkyAtmosphereComponent>();
    PreviewScene->AddComponent(AtmosphericFog, FTransform::Identity);
    PreviewScene->DirectionalLight->SetMobility(EComponentMobility::Movable);
    PreviewScene->DirectionalLight->CastShadows = true;
    PreviewScene->DirectionalLight->CastStaticShadows = true;
    PreviewScene->DirectionalLight->CastDynamicShadows = true;
    PreviewScene->DirectionalLight->SetIntensity(3);
	PreviewScene->SetFloorVisibility(false);
    PreviewScene->SetSkyBrightness(1.0f);
}

void STiledLevelEditorViewport::BindCommands()
{
	SEditorViewport::BindCommands();
}

TSharedRef<FEditorViewportClient> STiledLevelEditorViewport::MakeEditorViewportClient()
{
	EditorViewportClient = MakeShareable(new FTiledLevelViewportClient(TiledLevelEditorPtr, SharedThis(this), *PreviewScene, TiledLevelEditorPtr.Pin().Get()->GetTiledLevelBeingEdited()));
	EditorViewportClient->SetRealtime(true);
	EditorViewportClient->PreviewSceneFloor = PreviewScene->GetFloorMeshComponent();
	
	return EditorViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> STiledLevelEditorViewport::MakeViewportToolbar()
{
	return SNew(STiledLevelEditorViewportToolbar)
	.EditorViewport(SharedThis(this))
	.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute());
}

EVisibility STiledLevelEditorViewport::GetTransformToolbarVisibility() const
{
	return EVisibility::Visible;
}

void STiledLevelEditorViewport::OnFocusViewportToSelection()
{
	SEditorViewport::OnFocusViewportToSelection();
}

TSharedRef<SEditorViewport> STiledLevelEditorViewport::GetViewportWidget()
{
	return SharedThis(this);
}

TSharedPtr<FExtender> STiledLevelEditorViewport::GetExtenders() const
{
	TSharedPtr<FExtender> Result(MakeShareable(new FExtender));
	return Result;
}

void STiledLevelEditorViewport::OnFloatingButtonClicked()
{
}

void STiledLevelEditorViewport::ActivateEdMode()
{
	EditorViewportClient->ActivateEdMode();
}

