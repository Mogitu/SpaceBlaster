// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelEditorViewportClient.h"
#include "AssetEditorModeManager.h"
#include "EditorModeManager.h"
#include "Selection.h"
#include "TiledLevel.h"
#include "TiledLevelEditor.h"
#include "TiledLevelEditorHelper.h"
#include "TiledLevelEditorLog.h"
#include "TiledLevelEdMode.h"
#include "UnrealWidget.h"


FTiledLevelViewportClient::FTiledLevelViewportClient(TWeakPtr<class FTiledLevelEditor> InLevelEditor,
                                                     const TWeakPtr<SEditorViewport>& InEditorViewportWidget, FPreviewScene& InPreviewScene,
                                                     UTiledLevelAsset* InDataSource)
	:FEditorViewportClient(nullptr, &InPreviewScene, InEditorViewportWidget)
{

	// in UE 5.0
	Widget->SetUsesEditorModeTools(ModeTools.Get());
	PreviewScene = &InPreviewScene;
	((FAssetEditorModeManager*)ModeTools.Get())->SetPreviewScene(PreviewScene);
	
	LevelEditorPtr = InLevelEditor;
	DataSource = InDataSource;

	SetRealtime(true);
	
	// Setup defaults for the common draw helper.
     DrawHelper.bDrawPivot = false;
     DrawHelper.bDrawWorldBox = false;
     DrawHelper.bDrawKillZ = false;
     DrawHelper.bDrawGrid = false;
     DrawHelper.GridColorAxis = FColor(160, 160, 160);
     DrawHelper.GridColorMajor = FColor(144, 144, 144);
     DrawHelper.GridColorMinor = FColor(128, 128, 128);
     DrawHelper.PerspectiveGridSize = 250;
     DrawHelper.NumCells = DrawHelper.PerspectiveGridSize / (32 * 2);

     SetViewMode(VMI_Lit);
 
     EngineShowFlags.SetSnap(false);
     EngineShowFlags.CompositeEditorPrimitives = true;
     OverrideNearClipPlane(1.0f);
     bUsingOrbitCamera = true;

     // Set the initial camera position
     FRotator OrbitRotation(-60, 180, 0);
     SetCameraSetup(
         FVector::ZeroVector,
         OrbitRotation,
         FVector(0.0f, 0, 0.f),
         FVector::ZeroVector,
         FVector(0, 0, 0),
         FRotator(0, 0, 0)
     );
     SetViewLocation(FVector(0, 0, 3000));

	ModeTools->GetSelectedObjects()->Select(LevelEditorPtr.Pin().Get()->GetTiledLevelBeingEdited());

}

void FTiledLevelViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);
	if (!GIntraFrameDebuggingGameThread)
	{
		PreviewScene->GetWorld()->Tick(LEVELTICK_ViewportsOnly, DeltaSeconds);
	}
}

void FTiledLevelViewportClient::ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event,
	uint32 HitX, uint32 HitY)
{
	FEditorViewportClient::ProcessClick(View, HitProxy, Key, Event, HitX, HitY);
}

void FTiledLevelViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	// FEditorViewportClient::Draw(View, PDI);
	DrawHelper.Draw(View, PDI);
}

void FTiledLevelViewportClient::ActivateEdMode()
{
	if (ModeTools)
	{
		ModeTools->SetToolkitHost(LevelEditorPtr.Pin()->GetToolkitHost()); // this is removed in UE5.0, but I can not remove it....
		ModeTools->SetDefaultMode(FTiledLevelEdMode::EM_TiledLevelEdModeId);
		ModeTools->ActivateDefaultMode();
		ATiledLevel* LevelActor = PreviewScene->GetWorld()->SpawnActor<ATiledLevel>(ATiledLevel::StaticClass());
		LevelActor->IsInTiledLevelEditor = true;
		ATiledLevelEditorHelper* Helper = PreviewScene->GetWorld()->SpawnActor<ATiledLevelEditorHelper>(ATiledLevelEditorHelper::StaticClass());
		Helper->IsEditorHelper = true;
		EdMode = static_cast<FTiledLevelEdMode*>(ModeTools->GetActiveMode(FTiledLevelEdMode::EM_TiledLevelEdModeId));
		if (EdMode)
		{
			DataSource.Get()->EdMode = EdMode;
			EdMode->SetupData(DataSource.Get(), LevelActor, Helper, PreviewSceneFloor);

			// TODO: Dirty fix... Should remove it if I find real reason behind it...
			LevelEditorPtr.Pin()->ToolboxPtr->SetContent(EdMode->GetToolkit()->GetInlineContent().ToSharedRef());
		}
	}
}

FTiledLevelViewportClient::~FTiledLevelViewportClient()
{
	if (EdMode)
	{
		EdMode->Exit();
	}
}

