// Copyright 2022 PufStudio. All Rights Reserved.


#include "TiledLevelEditorUtility.h"
#include "TiledLevelAsset.h"
#include "TiledLevel.h"

#include "AssetToolsModule.h"
#include "KismetProceduralMeshLibrary.h"
#include "MeshDescription.h"
#include "ProceduralMeshComponent.h"
#include "StaticMeshAttributes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dialogs/DlgPickAssetPath.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "TiledLevel"

UStaticMesh* FTiledLevelEditorUtility::MergeTiledLevelAsset(UTiledLevelAsset* TargetAsset)
{
	// if it's empty asset, just stop here
	if (TargetAsset->GetNumOfAllPlacements() == 0)
		return nullptr;
	
	// Find first selected ProcMeshComp
	FString NewNameSuggestion = FString(TEXT("TiledLevelMesh"));
	FString PackageName = FString(TEXT("/Game/")) + NewNameSuggestion;
	FString Name;
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackageName, Name);

	TSharedPtr<SDlgPickAssetPath> PickAssetPathWidget =
		 SNew(SDlgPickAssetPath)
		 .Title(LOCTEXT("ConvertToStaticMeshPickName", "Choose New StaticMesh Location"))
		 .DefaultAssetPath(FText::FromString(PackageName));

	if (PickAssetPathWidget->ShowModal() == EAppReturnType::Ok)
	{
		 // Get the full name of where we want to create the physics asset.
		 FString UserPackageName = PickAssetPathWidget->GetFullAssetPath().ToString();
		 FName MeshName(*FPackageName::GetLongPackageAssetName(UserPackageName));

		 // Check if the user inputed a valid asset name, if they did not, give it the generated default name
		 if (MeshName == NAME_None)
		 {
			 // Use the defaults that were already generated.
			 UserPackageName = PackageName;
			 MeshName = *Name;
		 }
		int MaxLOD = 1;
		for ( UStaticMesh* SMPtr : TargetAsset->GetUsedStaticMeshSet())
		{
			MaxLOD = FMath::Max(SMPtr->GetNumLODs(), MaxLOD);
		}
		// Then find/create it.
		UPackage* Package = CreatePackage(*UserPackageName);
		check(Package);

		// Create StaticMesh object
		UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, MeshName, RF_Public | RF_Standalone);
		StaticMesh->InitResources();

		StaticMesh->SetLightingGuid(FGuid::NewGuid());
		for (int LOD = 0; LOD < MaxLOD; LOD ++)
		{
			UProceduralMeshComponent* ProcMeshComp = FTiledLevelUtility::ConvertTiledLevelAssetToProcMesh(TargetAsset, LOD);
			FMeshDescription MeshDescription = FTiledLevelUtility::BuildMeshDescription(ProcMeshComp);

			 // If we got some valid data.
			if (MeshDescription.Polygons().Num() == 0) continue;
			
			 // Add source to new StaticMesh
			 FStaticMeshSourceModel& SrcModel = StaticMesh->AddSourceModel();
			 SrcModel.BuildSettings.bRecomputeNormals = false;
			 SrcModel.BuildSettings.bRecomputeTangents = false;
			 SrcModel.BuildSettings.bRemoveDegenerates = false;
			 SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
			 SrcModel.BuildSettings.bUseFullPrecisionUVs = false;
			 SrcModel.BuildSettings.bGenerateLightmapUVs = true;
			 SrcModel.BuildSettings.SrcLightmapIndex = 0;
			 SrcModel.BuildSettings.DstLightmapIndex = 1;
			 StaticMesh->CreateMeshDescription(LOD, MoveTemp(MeshDescription));
			 StaticMesh->CommitMeshDescription(LOD);

			 //// SIMPLE COLLISION

			  StaticMesh->CreateBodySetup();
			  UBodySetup* NewBodySetup = StaticMesh->GetBodySetup();
			  NewBodySetup->BodySetupGuid = FGuid::NewGuid();
			  NewBodySetup->AggGeom.ConvexElems = ProcMeshComp->ProcMeshBodySetup->AggGeom.ConvexElems;
			  NewBodySetup->bGenerateMirroredCollision = false;
			  NewBodySetup->bDoubleSidedGeometry = true;
			  NewBodySetup->CollisionTraceFlag = CTF_UseDefault;
			  NewBodySetup->CreatePhysicsMeshes();

			 //// MATERIALS
			 TSet<UMaterialInterface*> UniqueMaterials;
			 const int32 NumSections = ProcMeshComp->GetNumSections();
			 for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
			 {
				 FProcMeshSection *ProcSection =
					  ProcMeshComp->GetProcMeshSection(SectionIdx);
				 UMaterialInterface *Material = ProcMeshComp->GetMaterial(SectionIdx);
				 UniqueMaterials.Add(Material);
			 }
			 // Copy materials to new mesh
			 for (auto* Material : UniqueMaterials)
			 {
				 StaticMesh->GetStaticMaterials().Add(FStaticMaterial(Material));
			 }

			 //Set the Imported version before calling the build
			 StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;
		}
		// Build mesh from source
		StaticMesh->Build(false);
		StaticMesh->PostEditChange();

		// Notify asset registry of new asset
		FAssetRegistryModule::AssetCreated(StaticMesh);
		return StaticMesh;
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE