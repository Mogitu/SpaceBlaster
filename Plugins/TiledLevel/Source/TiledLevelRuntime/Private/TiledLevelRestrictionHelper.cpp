// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelRestrictionHelper.h"
#include "KismetProceduralMeshLibrary.h"
#include "ProceduralMeshComponent.h"
#include "TiledItemSet.h"
#include "TiledLevelItem.h"
#include "Materials/MaterialInstanceDynamic.h"


// Sets default values
ATiledLevelRestrictionHelper::ATiledLevelRestrictionHelper()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	AreaHint = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("AreaHint"));
	AreaHint->SetupAttachment(Root);
}


void ATiledLevelRestrictionHelper::UpdatePreviewVisual()
{
	if (!GetSourceItem()) return;
	AreaHint->ClearAllMeshSections();
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	UKismetProceduralMeshLibrary::GenerateBoxMesh(SourceItemSet->GetTileSize()/2, Vertices, Triangles, Normals, UVs, Tangents);
	TArray<FColor> VertexColors; 
	AreaHint->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);
	// Set material to the color user specified...
	UMaterialInterface* M_Base = LoadObject<UMaterialInterface>(NULL, TEXT("/TiledLevel/Materials/M_UVOutline"), NULL, 0, NULL);
	M_AreaHint = UMaterialInstanceDynamic::Create(M_Base, this);
	M_AreaHint->SetVectorParameterValue("BorderColor", GetSourceItem()->BorderColor);
	M_AreaHint->SetScalarParameterValue("BorderWidth", GetSourceItem()->BorderWidth);
	AreaHint->SetMaterial(0, M_AreaHint);
	AreaHint->SetHiddenInGame(GetSourceItem()->bHiddenInGame);
}

bool ATiledLevelRestrictionHelper::IsInsideTargetedTilePositions(const TArray<FIntVector>& PositionsToCheck)
{
	for (const FIntVector& P : PositionsToCheck)
	{
		if (TargetedTilePositions.Contains(P))
			return true;
	}
	return false;
}

void ATiledLevelRestrictionHelper::AddTargetedPositions(FIntVector NewPoint)
{
	TargetedTilePositions.Add(NewPoint);
	UpdateVisual();
}

void ATiledLevelRestrictionHelper::RemoveTargetedPositions(FIntVector PointToRemove)
{
	TargetedTilePositions.Remove(PointToRemove);
	UpdateVisual();
}

// check out whether there are existing same class of this, then get that actor ptr, and add more cube on that actor, delete self then...
void ATiledLevelRestrictionHelper::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (TargetedTilePositions.Num() == 0 )
		UpdatePreviewVisual();
	else
		UpdateVisual();
}

UTiledLevelRestrictionItem* ATiledLevelRestrictionHelper::GetSourceItem() const
{
	if (!SourceItemSet) return nullptr;
	return Cast<UTiledLevelRestrictionItem>(SourceItemSet->GetItem(SourceItemID));
}

void ATiledLevelRestrictionHelper::UpdateVisual()
{
	if (!GetSourceItem()) return;
	if (!GetAttachParentActor()) return;

	SetActorTransform(GetAttachParentActor()->GetActorTransform()); // make sure transform is the same as parent tiled level actor
	AreaHint->ClearAllMeshSections();
	
	struct FCubeFace
	{
		FIntVector P0, P1, P2, P3;
		FVector2D UV0, UV1, UV2, UV3;
		FVector FaceNormal;
		
		FCubeFace(const FIntVector& InP0, const FIntVector& InP1, const FIntVector& InP2, const FIntVector& InP3, FVector InFaceNormal )
			: P0(InP0), P1(InP1), P2(InP2), P3(InP3), FaceNormal(InFaceNormal)
		{}

		void SetUV(FVector2D InUV0, FVector2D InUV1, FVector2D InUV2, FVector2D InUV3)
		{
			UV0 = InUV0;
			UV1 = InUV1;
			UV2 = InUV2;
			UV3 = InUV3;
		}
		
		bool operator==(const FCubeFace Other) const
		{
			TArray<FIntVector> Ps = {P0, P1, P2, P3};
			return Ps.Contains(Other.P0) && Ps.Contains(Other.P1) && Ps.Contains(Other.P2) && Ps.Contains(Other.P3);
		}
	};

	TArray<FCubeFace> CubeFaces;
	const FIntVector Xp1 = {1, 0, 0};
	const FIntVector Yp1 = {0, 1, 0};
	const FIntVector Zp1 = {0, 0, 1};
	const FIntVector XYp1 = {1, 1, 0};
	const FIntVector YZp1 = {0, 1, 1};
	const FIntVector XZp1 = {1, 0, 1};
	const FIntVector XYZp1 = {1, 1, 1};
	for (FIntVector Position : TargetedTilePositions)
	{
		// The six faces
		//front
		FCubeFace FrontFace(Position, Position+Yp1,Position+Zp1, Position+YZp1, {1.f, 0.f, 0.f});
		FrontFace.SetUV({0.f, 0.f}, {1.f, 0.f}, {0.f, 1.f}, {1.f, 1.f});
		if (CubeFaces.Contains(FrontFace))
		{
			CubeFaces.Remove(FrontFace);
		}
		else
		{
			CubeFaces.Add(FrontFace);
		}
		
		//back
		FCubeFace BackFace(Position+Xp1, Position+XYp1,Position+XZp1, Position+XYZp1, {-1.f, 0.f, 0.f});
		BackFace.SetUV({0.f, 0.f}, {1.f, 0.f}, {0.f, 1.f}, {1.f, 1.f});
		if (CubeFaces.Contains(BackFace))
		{
			CubeFaces.Remove(BackFace);
		}
		else
		{
			CubeFaces.Add(BackFace);
		}
		//right
		FCubeFace RightFace(Position+Yp1, Position+XYp1,Position+YZp1, Position+XYZp1, {0.f, 1.f, 0.f});
		RightFace.SetUV({0.f, 0.f}, {1.f, 0.f}, {0.f, 1.f}, {1.f, 1.f});
		if (CubeFaces.Contains(RightFace))
		{
			CubeFaces.Remove(RightFace);
		}
		else
		{
			CubeFaces.Add(RightFace);
		}
		//left
		FCubeFace LeftFace(Position, Position+Xp1,Position+Zp1, Position+XZp1, {0.f, -1.f, 0.f});
		LeftFace.SetUV({0.f, 0.f}, {1.f, 0.f}, {0.f, 1.f}, {1.f, 1.f});
		if (CubeFaces.Contains(LeftFace))
		{
			CubeFaces.Remove(LeftFace);
		}
		else
		{
			CubeFaces.Add(LeftFace);
		}
		//top
		FCubeFace TopFace(Position+Zp1, Position+XZp1,Position+YZp1, Position+XYZp1, {0.f, 0.f, 1.f});
		TopFace.SetUV({0.f, 0.f}, {1.f, 0.f}, {0.f, 1.f}, {1.f, 1.f});
		if (CubeFaces.Contains(TopFace))
		{
			CubeFaces.Remove(TopFace);
		}
		else
		{
			CubeFaces.Add(TopFace);
		}
		//bottom
		FCubeFace BottomFace(Position, Position+Xp1,Position+Yp1, Position+XYp1, {0.f, 0.f, -1.f});
		BottomFace.SetUV({0.f, 0.f}, {1.f, 0.f}, {0.f, 1.f}, {1.f, 1.f});
		if (CubeFaces.Contains(BottomFace))
		{
			CubeFaces.Remove(BottomFace);
		}
		else
		{
			CubeFaces.Add(BottomFace);
		}
	}
	// now, there is no duplicate face, let create triangles and normals...
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	TArray<FColor> VertexColors;
	const FVector TileSize = SourceItemSet->GetTileSize();
	int FaceCount = 0;
	for (FCubeFace Face : CubeFaces)
	{
		Vertices.Add(FVector(Face.P0)*TileSize);
		Vertices.Add(FVector(Face.P1)*TileSize);
		Vertices.Add(FVector(Face.P2)*TileSize);
		Vertices.Add(FVector(Face.P3)*TileSize);
		Triangles.Add(FaceCount * 4 + 0);
		Triangles.Add(FaceCount * 4 + 1);
		Triangles.Add(FaceCount * 4 + 2);
		Triangles.Add(FaceCount * 4 + 2);
		Triangles.Add(FaceCount * 4 + 1);
		Triangles.Add(FaceCount * 4 + 3);
		Normals.Add(Face.FaceNormal);
		Normals.Add(Face.FaceNormal);
		Normals.Add(Face.FaceNormal);
		Normals.Add(Face.FaceNormal);
		UVs.Add(Face.UV0);
		UVs.Add(Face.UV1);
		UVs.Add(Face.UV2);
		UVs.Add(Face.UV3);
		FaceCount++;
	}
      
	// TODO: options to add padding? 
	AreaHint->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);
	UMaterialInterface* M_Base = LoadObject<UMaterialInterface>(NULL, TEXT("/TiledLevel/Materials/M_UVOutline"), NULL, 0, NULL);
	M_AreaHint = UMaterialInstanceDynamic::Create(M_Base, this);
	M_AreaHint->SetVectorParameterValue("BorderColor", GetSourceItem()->BorderColor);
	M_AreaHint->SetScalarParameterValue("BorderWidth", GetSourceItem()->BorderWidth);
	AreaHint->SetMaterial(0, M_AreaHint);
}
