// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelEditorHelper.h"
#include "TiledLevelAsset.h"
#include "TiledLevelGrid.h"
#include "TiledLevelItem.h"
#include "TiledLevelUtility.h"
#include "TiledLevelRestrictionHelper.h"
#include "ProceduralMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "TiledLevel.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ATiledLevelEditorHelper::ATiledLevelEditorHelper(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	AreaHint = CreateDefaultSubobject<UTiledLevelGrid>(TEXT("AreaHint"));
	AreaHint->SetupAttachment(Root);

	FloorGrids = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FloorGrids"));
	FloorGrids->CastShadow = false;
	FloorGrids->SetupAttachment(Root);

	FillPreviewGrids = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FillPreviewGrids"));
	FillPreviewGrids->CastShadow = false;
	FillPreviewGrids->SetupAttachment(Root);

	CustomGizmo = CreateDefaultSubobject<UTiledLevelGrid>(TEXT("CustomGizmo"));
	CustomGizmo->SetBoxColor(FColor::Yellow);
	CustomGizmo->SetBoxExtent(FVector(3));
	CustomGizmo->SetupAttachment(Root);

	Brush = CreateDefaultSubobject<UTiledLevelGrid>(TEXT("Brush"));
	Brush->SetupAttachment(CustomGizmo);
	Brush->SetBoxExtent(FVector(100, 100, 100), false);
	Brush->SetBoxColor(FColor::Green);
	Brush->SetLineThickness(3.0f);
	Center = CreateDefaultSubobject<USceneComponent>(TEXT("Center"));
	Center->SetupAttachment(Brush);

	CenterHint = CreateDefaultSubobject<UTiledLevelGrid>(TEXT("CenterHint"));
	CenterHint->SetupAttachment(Center);
	CenterHint->SetBoxColor(FColor::Red);
	CenterHint->SetBoxExtent(FVector(3));

	PreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(Center);
	PreviewMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);
	
	CustomGizmo->SetRelativeRotation(FRotator(0));
	BrushRotationIndex = 0;

	// Load materials in constructor - fix v2.0.0 issues
	ConstructorHelpers::FObjectFinder<UMaterialInterface> Asset_M_FloorGrids(TEXT("/TiledLevel/Materials/M_FloorGrids"));
	ConstructorHelpers::FObjectFinder<UMaterialInterface> Asset_M_HelperFloor(TEXT("/TiledLevel/Materials/M_HelperFloor"));
	ConstructorHelpers::FObjectFinder<UMaterialInterface> Asset_M_FillPreview(TEXT("/TiledLevel/Materials/M_FillPreview"));
	ConstructorHelpers::FObjectFinder<UMaterialInterface> Asset_M_PreviewCanBuild(TEXT("/TiledLevel/Materials/MI_Preview_Normal"));
	ConstructorHelpers::FObjectFinder<UMaterialInterface> Asset_M_PreviewCanNotBuild(TEXT("/TiledLevel/Materials/MI_Preview_Eraser"));
	M_FloorGrids = Asset_M_FloorGrids.Object;
	M_HelperFloor = Asset_M_HelperFloor.Object;
	M_FillPreview = Asset_M_FillPreview.Object;
	CanBuildPreviewMaterial = Asset_M_PreviewCanBuild.Object;
	CanNotBuildPreviewMaterial =Asset_M_PreviewCanNotBuild.Object;
}

void ATiledLevelEditorHelper::BeginPlay()
{
	Super::BeginPlay();
	if (IsEditorHelper)
		Destroy();
}

void ATiledLevelEditorHelper::Destroyed()
{
	if (PreviewActor)
		PreviewActor->Destroy();
	Super::Destroyed();
}

void ATiledLevelEditorHelper::OnConstruction(const FTransform& Transform)
{
	SetActorLocation(FVector(0, 0, 0));
}

void ATiledLevelEditorHelper::SetupPaintBrush(UTiledLevelItem* Item)
{
	ActiveItem = Item;
	TileExtent = ActiveItem->Extent;

	CustomGizmo->SetVisibility(true, true);
	Brush->SetVisibility(true);
	Brush->SetBoxColor(FColor::Green);
	Center->SetVisibility(true, true);

	// For Debug
	CenterHint->SetVisibility(true);
	
	if (ActiveItem->SourceType == ETLSourceType::Mesh)
	{
		if (!ActiveItem->TiledMesh) return;
		PreviewMesh->SetStaticMesh(ActiveItem->TiledMesh);
		for (int i = 0 ; i < ActiveItem->OverrideMaterials.Num(); i++)
		{
			 if (UMaterialInterface* M = ActiveItem->OverrideMaterials[i])
				  PreviewMesh->SetMaterial(i, M);
		}
		// use this function to handle all the placement settings...
		FTiledLevelUtility::ApplyPlacementSettings(TileSize, ActiveItem, PreviewMesh, Brush, Center);
		PreviewMeshInitLocation = PreviewMesh->GetRelativeLocation();
	}
	else
	{
		if (UTiledLevelRestrictionItem* RestrictionItem = Cast<UTiledLevelRestrictionItem>(Item))
		{
			PreviewActor = GetWorld()->SpawnActor(ATiledLevelRestrictionHelper::StaticClass());
			ATiledLevelRestrictionHelper* NewArh = Cast<ATiledLevelRestrictionHelper>(PreviewActor);
			NewArh->SourceItemSet = Cast<UTiledItemSet>(RestrictionItem->GetOuter());
			NewArh->SourceItemID = RestrictionItem->ItemID;
			NewArh->UpdatePreviewVisual();
		}
		else
		{
			PreviewActor = GetWorld()->SpawnActor<AActor>(Cast<UBlueprint>(ActiveItem->TiledActor)->GeneratedClass);
		}

		// force set movable...
		PreviewActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
		for (auto Com : PreviewActor->GetComponents())
		{
			if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Com))
				SMC->SetMobility(EComponentMobility::Movable);
		}
		
		PreviewActor->AttachToComponent(Center, FAttachmentTransformRules::KeepRelativeTransform);
		FTiledLevelUtility::ApplyPlacementSettings_TiledActor(TileSize, ActiveItem, Brush, Center);
		PreviewActor->SetActorEnableCollision(false);
		PreviewActor->SetActorRelativeTransform(ActiveItem->TransformAdjustment);
		PreviewActorRelativeLocation = Center->GetComponentTransform().InverseTransformPosition(PreviewActor->GetActorLocation());
		
	}
	CachedBrushLocation = Brush->GetRelativeLocation();
}

void ATiledLevelEditorHelper::SetupEraserBrush(FIntVector EraserExtent, EPlacedType SnapTarget)
{
	CustomGizmo->SetVisibility(true, false);
	Brush->SetVisibility(true);
	Brush->SetHiddenInGame(false);
	Center->SetVisibility(false, true);
	FVector BrushExtent = FVector(EraserExtent) * TileSize * 0.5;
	Brush->SetBoxExtent(BrushExtent);
	Brush->SetRelativeLocation(BrushExtent);
	Brush->SetBoxColor(FColor::Red);

	switch (SnapTarget)
	{
	case EPlacedType::Wall:
		BrushExtent = FVector(TileSize.X * EraserExtent.X, TileSize.Y * 0.25, TileSize.Z * EraserExtent.Z) * 0.5;
		Brush->SetBoxExtent(BrushExtent);
		Brush->SetRelativeLocation(BrushExtent * FVector(1, 0, 1));
		break;
	case EPlacedType::Edge:
		BrushExtent = FVector(TileSize.X * EraserExtent.X, TileSize.Y * 0.25, TileSize.Z * 0.25) * 0.5;
		Brush->SetBoxExtent(BrushExtent);
		Brush->SetRelativeLocation(BrushExtent * FVector(1, 0, 1));
		break;
	case EPlacedType::Pillar:
		BrushExtent = FVector(TileSize.X * 0.25, TileSize.Y * 0.25, TileSize.Z * EraserExtent.Z) * 0.5;
		Brush->SetBoxExtent(BrushExtent);
		Brush->SetRelativeLocation(BrushExtent * FVector(0, 0, 1));
		break;
	case EPlacedType::Point:
		BrushExtent = FVector(TileSize.X * 0.25, TileSize.Y * 0.25, TileSize.Z * 0.25) * 0.5;
		Brush->SetBoxExtent(BrushExtent);
		Brush->SetRelativeLocation(FVector(0));
		break;
	case EPlacedType::Any:
		Brush->SetBoxColor(FColor::Magenta);
		Brush->SetRelativeScale3D((BrushExtent + TileSize * 0.125)/ BrushExtent);
		break;
	default: ;
	}
}

void ATiledLevelEditorHelper::ResetBrush()
{
	PreviewMesh->SetStaticMesh(nullptr);
	PreviewMesh->ResetRelativeTransform();

	if (PreviewActor)
	{
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}

	Brush->SetHiddenInGame(true);
	Brush->SetVisibility(false);
	Center->SetVisibility(false, true);
	CustomGizmo->SetVisibility(false, true);
	Brush->SetRelativeScale3D(FVector(1));
	FillPreviewGrids->ClearAllMeshSections();
	
	// // Reset rotation
	BrushRotationIndex = 0;
	CustomGizmo->SetRelativeRotation(FRotator(0));
}

void ATiledLevelEditorHelper::ToggleQuickEraserBrush(bool IsSet)
{
	if (IsSet)
	{
		CustomGizmo->SetVisibility(true, false);
		Brush->SetVisibility(true);
		Center->SetVisibility(false, true);
		Brush->SetBoxExtent(Brush->GetUnscaledBoxExtent());
		Brush->SetBoxColor(FColor::Red);
		CachedCenterScale = Center->GetRelativeScale3D();
		Center->SetRelativeScale3D(CachedCenterScale * FVector(1.2));
	}
	else
	{
		CustomGizmo->SetVisibility(true, true);
		Brush->SetVisibility(true);
		Brush->SetBoxExtent(Brush->GetUnscaledBoxExtent()); // must set extent again so that set color may work...
		Brush->SetBoxColor(FColor::Green);
		Center->SetVisibility(true, true);
		Center->SetRelativeScale3D(CachedCenterScale);
	}
}

void ATiledLevelEditorHelper::ToggleValidBrush(bool IsValid)
{
	if (IsBrushValid == IsValid) return;
	IsBrushValid = IsValid;
	if (IsValid)
	{
		PreviewMesh->SetVisibility(true);
		if (PreviewActor) PreviewActor->SetActorRelativeLocation(PreviewActorRelativeLocation);
	} else
	{
		PreviewMesh->SetVisibility(false);
		if (PreviewActor) PreviewActor->SetActorRelativeLocation(FVector(100000,0,-100000));
	}
}

void ATiledLevelEditorHelper::MoveBrush(FIntVector TilePosition, bool IsSnapEnabled)
{
	CustomGizmo->SetRelativeLocation(FVector(TilePosition) * TileSize);
	if (IsSnapEnabled)
	{
		FTiledLevelUtility::TrySnapPropToFloor(PreviewMeshInitLocation, BrushRotationIndex, TileSize, ActiveItem, PreviewMesh);
		PreviewMeshInitLocation = PreviewMesh->GetRelativeLocation();
	}
}

void ATiledLevelEditorHelper::MoveBrush(FTiledLevelEdge TileEdge, bool IsSnapEnabled)
{
	CustomGizmo->SetRelativeLocation(FVector(TileEdge.X, TileEdge.Y, TileEdge.Z) * TileSize);
	if (IsSnapEnabled)
	{
		const float Z_Offset = FTiledLevelUtility::TrySnapPropToFloor(PreviewMeshInitLocation, BrushRotationIndex, TileSize, ActiveItem, PreviewMesh);
		FTiledLevelUtility::TrySnapPropToWall(PreviewMeshInitLocation, BrushRotationIndex, TileSize, ActiveItem, PreviewMesh, Z_Offset);
	}
}

void ATiledLevelEditorHelper::RotateBrush(bool IsCW, EPlacedType PlacedType, bool& ShouldRotateExtent)
{
	FVector TempExtent = FVector(TileExtent.Y, TileExtent.X, TileExtent.Z);
	if (IsCW)
	{
		BrushRotationIndex == 3 ? BrushRotationIndex = 0 : BrushRotationIndex ++;
	}
	else
	{
		BrushRotationIndex == 0 ? BrushRotationIndex = 3 : BrushRotationIndex --;
	}
	
	FTransform F = GetActorTransform();
	F.SetTranslation(FVector(0));
	switch (BrushRotationIndex)
	{
	case 0:
		CustomGizmo->SetRelativeRotation(FRotator(0));
		Brush->SetRelativeLocation(CachedBrushLocation);
		ShouldRotateExtent = false;
		break;
	case 1:
		CustomGizmo->SetRelativeRotation(FRotator(0, 90, 0));
		Brush->SetRelativeLocation(CachedBrushLocation);
		switch (PlacedType)
		{
			case EPlacedType::Block:
				Brush->AddWorldOffset(
				F.TransformPosition(TileSize * TempExtent * FVector(1, 0, 0))
				);
				break;
			case EPlacedType::Floor:
				Brush->AddWorldOffset(
				F.TransformPosition(TileSize * TempExtent * FVector(1, 0, 0))
				);
				break;
			case EPlacedType::Wall:
				break;
			case EPlacedType::Pillar:
				break;
			case EPlacedType::Edge:
				break;
			default: ;
		}
		ShouldRotateExtent = true;
		break;
	case 2:
		CustomGizmo->SetRelativeRotation(FRotator(0, 180, 0));
		Brush->SetRelativeLocation(CachedBrushLocation);
		switch (PlacedType)
		{
			case EPlacedType::Block:
				Brush->AddWorldOffset(
				F.TransformPosition(TileSize * TileExtent * FVector(1, 1, 0))
				);
				break;
			case EPlacedType::Floor:
				Brush->AddWorldOffset(
				F.TransformPosition(TileSize * TileExtent * FVector(1, 1, 0))
				);
				break;
			case EPlacedType::Wall:
				Brush->AddWorldOffset(
				F.TransformPosition(TileSize * TileExtent * FVector(1, 0, 0))
				);
				break;
			case EPlacedType::Pillar:
				break;
			case EPlacedType::Edge:
				Brush->AddWorldOffset(
				F.TransformPosition(TileSize * TileExtent * FVector(1, 0, 0))
				);
				break;
			default: ;
		}
		ShouldRotateExtent = false;
		break;
	case 3:
		CustomGizmo->SetRelativeRotation(FRotator(0, 270, 0));
		Brush->SetRelativeLocation(CachedBrushLocation);
		switch (PlacedType)
		{
			case EPlacedType::Block:
				Brush->AddWorldOffset(
				F.TransformPosition(TileSize * TempExtent * FVector(0, 1, 0))
				);
				break;
			case EPlacedType::Floor:
				Brush->AddWorldOffset(
				F.TransformPosition(TileSize * TempExtent * FVector(0, 1, 0))
				);
				break;
			case EPlacedType::Wall:
				Brush->AddWorldOffset(
				F.TransformPosition(TileSize * TileExtent * FVector(0, 1, 0))
				);
				break;
			case EPlacedType::Pillar:
				break;
			case EPlacedType::Edge:
				Brush->AddWorldOffset(
				F.TransformPosition(TileSize * TileExtent * FVector(0, 1, 0))
				);
				break;
			default: ;
		}
		ShouldRotateExtent = true;
		break;
	default: ;
	}

}

void ATiledLevelEditorHelper::RotateEraserBrush(bool IsCW, bool&  ShouldEraseVertical)
{
	if (IsCW)
	{
		BrushRotationIndex == 3 ? BrushRotationIndex = 0 : BrushRotationIndex ++;
	}
	else
	{
		BrushRotationIndex == 0 ? BrushRotationIndex = 3 : BrushRotationIndex --;
	}
	if (BrushRotationIndex == 1 || BrushRotationIndex == 3)
	{
		CustomGizmo->SetRelativeRotation(FRotator(0, 90, 0));
		ShouldEraseVertical = true;
	} else
	{
		CustomGizmo->SetRelativeRotation(FRotator(0));
		ShouldEraseVertical = false;
	}
}

bool ATiledLevelEditorHelper::IsBrushExtentChanged()
{
	if (ActiveItem)
	{
		return (ActiveItem->PlacedType == EPlacedType::Block || ActiveItem->PlacedType == EPlacedType::Floor) &&
			ActiveItem->Extent.X != ActiveItem->Extent.Y &&
			(BrushRotationIndex == 1 || BrushRotationIndex == 3);
	}
	return false;
}

FTransform ATiledLevelEditorHelper::GetPreviewPlacementWorldTransform()
{
	if (ActiveItem->SourceType == ETLSourceType::Actor)
		return PreviewActor->GetActorTransform();
	return PreviewMesh->GetComponentTransform();
}

void ATiledLevelEditorHelper::ResizeArea(float X, float Y, float Z, int Num_X, int Num_Y, int Num_Floors,
	int LowestFloor)
{
	TileSize = FVector(X, Y, Z);
	
	AreaHint->SetBoxExtent(FVector(X * Num_X, Y * Num_Y, Z* Num_Floors)/2);
	AreaHint->SetRelativeLocation(FVector( X * Num_X * 0.5, Y * Num_Y * 0.5, Z * (Num_Floors + LowestFloor * 2) * 0.5));

	TArray<FVector> XVerticesTemplate =
	{
		FVector(1, 0, 2),
		FVector(-1, 0, 2),		
		FVector(1, 1, 2),		
		FVector(-1, 1, 2),		
	};
	
	TArray<FVector> YVerticesTemplate =
	{
		FVector(0, 1, 2),		
		FVector(0, -1, 2),		
		FVector(1, 1, 2),		
		FVector(1, -1, 2),		
	};
	
	TArray<int32> TrianglesTemplate =
		{ 0, 1, 2, 2, 1, 3, 5, 6, 7, 6, 5, 4};
	
	FloorGrids->ClearAllMeshSections();
	const float HalfGridWidth = 2;
	float GridMaxX = X * Num_X;
	float GridMaxY = Y * Num_Y;
	TArray<FVector> FloorGridVertices;
	TArray<int32> FloorGridTriangles;
	
	for (int i = 0; i <= Num_X; i++)
	{
		for (FVector V: XVerticesTemplate)
			FloorGridVertices.Add(V * FVector(HalfGridWidth, GridMaxY, 1) + FVector(X * i, 0, 0));
		for (int32 T: TrianglesTemplate)
			FloorGridTriangles.Add(T + 4 * i);
	}
	for (int i = 0; i <= Num_Y; i++)
	{
		for (FVector V: YVerticesTemplate)
			FloorGridVertices.Add(V * FVector(GridMaxX, HalfGridWidth, 1) + FVector(0, Y * i, 0));
		for (int32 T: TrianglesTemplate)
			FloorGridTriangles.Add(T + 4 * (i + Num_X));
	}
	FloorGrids->CreateMeshSection(0, FloorGridVertices, FloorGridTriangles, TArray<FVector>{}, TArray<FVector2D>{}, TArray<FColor>{}, TArray<FProcMeshTangent>{}, false);
	FloorGrids->SetMaterial(0, M_FloorGrids);
	TArray<FVector> PlaneVertices =
	{
		FVector(GridMaxX, 0, 1),
		FVector(0, 0, 1),
		FVector(GridMaxX, GridMaxY, 1),
		FVector(0, GridMaxY, 1),
	};
	TArray<int32> PlaneTriangles = { 0, 1, 2, 2, 1, 3 };
	FloorGrids->CreateMeshSection(1, PlaneVertices, PlaneTriangles, TArray<FVector>{}, TArray<FVector2D>{}, TArray<FColor>{}, TArray<FProcMeshTangent>{}, false);
	FloorGrids->SetMaterial(1, M_HelperFloor);
	
}

void ATiledLevelEditorHelper::MoveFloorGrids(int TargetFloorIndex)
{
	FloorGrids->SetRelativeLocation(FVector(0, 0, TargetFloorIndex * TileSize.Z));
}

void ATiledLevelEditorHelper::SetHelperGridsVisibility(bool IsVisible)
{
	FloorGrids->SetVisibility(IsVisible, true);
	AreaHint->SetVisibility(IsVisible, true);
}

void ATiledLevelEditorHelper::UpdateFillPreviewGrids(TArray<FIntPoint> InFillBoard,int InFillFloorPosition, int InFillHeight)
{
	if (InFillBoard == CacheFillTiles) return;
	CacheFillTiles = InFillBoard;
	FillPreviewGrids->ClearAllMeshSections();
	
	TArray<FVector> PreviewVertices;
	TArray<int32> PreviewTriangles;
	TArray<FVector> PreviewNormals;
	TArray<FVector2D> PreviewUVs;
	TArray<FProcMeshTangent> PreviewTangents;
	
	TArray<FVector> BoxVertices;
	TArray<int32> BoxTriangles;
	TArray<FVector> BoxNormals;
	TArray<FVector2D> BoxUVs;
	TArray<FProcMeshTangent> BoxTangents;
	
	UKismetProceduralMeshLibrary::GenerateBoxMesh(TileSize * FVector(0.5, 0.5, 0.5*InFillHeight), BoxVertices, BoxTriangles, BoxNormals, BoxUVs, BoxTangents);

	int N_Box = 0;
	FVector Offset = FVector(0.5, 0.5, InFillHeight * 0.5) * TileSize;
	
	for (auto Tile : InFillBoard)
	{
		for (FVector v : BoxVertices)
			PreviewVertices.Add(v + FVector(Tile.X, Tile.Y, InFillFloorPosition) * TileSize + Offset); 
		for (int32 t : BoxTriangles)
			PreviewTriangles.Add(t + N_Box * BoxVertices.Num());
		PreviewNormals.Append(BoxNormals);
		PreviewUVs.Append(BoxUVs);
		PreviewTangents.Append(BoxTangents);
		N_Box++;
	}
	
	FillPreviewGrids->CreateMeshSection(0, PreviewVertices, PreviewTriangles, PreviewNormals, PreviewUVs, TArray<FColor>{}, PreviewTangents, false);
	FillPreviewGrids->SetMaterial(0, M_FillPreview);
}

void ATiledLevelEditorHelper::UpdateFillPreviewEdges(TArray<FTiledLevelEdge> InEdgePoints, int InFillHeight)
{
	if (InEdgePoints == CacheFillEdges) return;
	CacheFillEdges = InEdgePoints;
	FillPreviewGrids->ClearAllMeshSections();

	TArray<FVector> PreviewVertices;
	TArray<int32> PreviewTriangles;
	TArray<FVector> PreviewNormals;
	TArray<FVector2D> PreviewUVs;
	TArray<FProcMeshTangent> PreviewTangents;
	
	TArray<FVector> HBoxVertices;
	TArray<FVector> VBoxVertices;
	TArray<int32> BoxTriangles;
	TArray<FVector> BoxNormals;
	TArray<FVector2D> BoxUVs;
	TArray<FProcMeshTangent> BoxTangents;
	
	UKismetProceduralMeshLibrary::GenerateBoxMesh(TileSize * FVector(0.5, 0.05, 0.5 * InFillHeight), HBoxVertices, BoxTriangles, BoxNormals, BoxUVs, BoxTangents);
	UKismetProceduralMeshLibrary::GenerateBoxMesh(TileSize * FVector(0.05, 0.5, 0.5 * InFillHeight), VBoxVertices, BoxTriangles, BoxNormals, BoxUVs, BoxTangents);
	
	int N_Box = 0;
	FVector HOffset = FVector(0.5, 0.025, 0.5 * InFillHeight) * TileSize;
	FVector VOffset = FVector(0.025, 0.5, 0.5 * InFillHeight) * TileSize;
	
	for (const auto Edge : InEdgePoints)
	{
		if (Edge.EdgeType == EEdgeType::Horizontal)
		{
			for (FVector v : HBoxVertices)
			{
				PreviewVertices.Add((v + FVector(Edge.X, Edge.Y, Edge.Z) * TileSize) + HOffset);
			}
		}
		else
		{
			for (FVector v : VBoxVertices)
			{
				PreviewVertices.Add((v + FVector(Edge.X, Edge.Y, Edge.Z) * TileSize) + VOffset);
			}
		}
		
		for (int32 t : BoxTriangles)
			PreviewTriangles.Add(t + N_Box * HBoxVertices.Num());
		PreviewNormals.Append(BoxNormals);
		PreviewUVs.Append(BoxUVs);
		PreviewTangents.Append(BoxTangents);
		N_Box++;
	}
	
	FillPreviewGrids->CreateMeshSection(0, PreviewVertices, PreviewTriangles, PreviewNormals, PreviewUVs, TArray<FColor>{}, PreviewTangents, false);
	FillPreviewGrids->SetMaterial(0, M_FillPreview);
	
}

void ATiledLevelEditorHelper::SetupPreviewBrushInGame(UTiledLevelItem* Item)
{
	ActiveItem = Item;
	TileExtent = ActiveItem->Extent;
	if (PreviewActor)
	{
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}
	PreviewMesh->SetStaticMesh(nullptr);

	if (ActiveItem->SourceType == ETLSourceType::Mesh)
	{
		if (!ActiveItem->TiledMesh) return;
		PreviewMesh->SetStaticMesh(ActiveItem->TiledMesh);
		PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (ShouldUsePreviewMaterial)
		{
			 for (int i = 0 ; i < PreviewMesh->GetNumMaterials(); i++)
			 {
				 PreviewMesh->SetMaterial(i, CanNotBuildPreviewMaterial);
			 }
		}
		// use this function to handle all the placement settings...
		FTiledLevelUtility::ApplyPlacementSettings(TileSize, ActiveItem, PreviewMesh, Brush, Center);
		PreviewMeshInitLocation = PreviewMesh->GetRelativeLocation();
		PreviewMesh->SetVisibility(true);
	}
	else
	{
		PreviewActor = GetWorld()->SpawnActor<AActor>(Cast<UBlueprint>(ActiveItem->TiledActor)->GeneratedClass);
		PreviewActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
		for (auto Com : PreviewActor->GetComponents())
		{
			if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Com))
			{
				SMC->SetMobility(EComponentMobility::Movable);
				if (ShouldUsePreviewMaterial)
				{
					for (int i = 0 ; i < SMC->GetNumMaterials(); i++)
					{
						SMC->SetMaterial(i, CanNotBuildPreviewMaterial);
					}
				}
			}
		}
		PreviewActor->AttachToComponent(Center, FAttachmentTransformRules::KeepRelativeTransform);
		FTiledLevelUtility::ApplyPlacementSettings_TiledActor(TileSize, ActiveItem, Brush, Center);
		PreviewActor->SetActorEnableCollision(false);
		PreviewActor->SetActorRelativeTransform(ActiveItem->TransformAdjustment);
		PreviewActorRelativeLocation = Center->GetComponentTransform().InverseTransformPosition(PreviewActor->GetActorLocation());
	}
	CachedBrushLocation = Brush->GetRelativeLocation();
}

void ATiledLevelEditorHelper::UpdatePreviewHint(bool CanBuild)
{
	if (ShouldUsePreviewMaterial)
	{
		TArray<UActorComponent*> PreviewActorComponents;
		if (CanBuild)
		{
			if (ActiveItem->SourceType == ETLSourceType::Mesh)
			{
				for (int i = 0 ; i < PreviewMesh->GetNumMaterials(); i++)
					 PreviewMesh->SetMaterial(i, CanBuildPreviewMaterial);
			}
			else
			{
				PreviewActor->GetComponents(PreviewActorComponents);
				for (UActorComponent* AC : PreviewActorComponents)
				{
					if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(AC))
					{
						  for (int i = 0 ; i < SMC->GetNumMaterials(); i++)
								SMC->SetMaterial(i, CanBuildPreviewMaterial);
					}
				}
			}
		}
		else
		{
			if (ActiveItem->SourceType == ETLSourceType::Mesh)
			{
				for (int i = 0 ; i < PreviewMesh->GetNumMaterials(); i++)
				{
					 PreviewMesh->SetMaterial(i, CanNotBuildPreviewMaterial);
				}
			}
			else
			{
				PreviewActor->GetComponents(PreviewActorComponents);
				for (UActorComponent* AC : PreviewActorComponents)
				{
					if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(AC))
					{
						  for (int i = 0 ; i < SMC->GetNumMaterials(); i++)
								SMC->SetMaterial(i, CanNotBuildPreviewMaterial);
					}
				}
			}
		}
	}
	else
	{
		if (CanBuild)
		{
			if (ActiveItem->SourceType == ETLSourceType::Mesh)
				PreviewMesh->SetVisibility(true);
			else
				PreviewActor->SetHidden(false);
		}
		else
		{
			if (ActiveItem->SourceType == ETLSourceType::Mesh)
				PreviewMesh->SetVisibility(false);
			else
				PreviewActor->SetHidden(false);
		}
		
	}
}

// TODO: leave for next update...
/*void ATiledLevelEditorHelper::SpawnGametimeLevel_Implementation()
{
	if (HasAuthority())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.bNoFail = true; // this stuck me so long... force it spawn... no matter the collision...
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		SpawnParams.Instigator = PlayerController->GetInstigator();
		SpawnParams.Owner = PlayerController;
		ATiledLevel* SpawnedLevel = GetWorld()->SpawnActor<ATiledLevel>(FVector(0, 0, 0), FRotator(0), SpawnParams);
		SpawnedLevel->Tags.Add(FName(TEXT("GametimeLevel")));
	}
}*/
