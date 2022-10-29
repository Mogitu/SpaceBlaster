// Copyright 2022 PufStudio. All Rights Reserved.

#include "TiledLevelModule.h"
#include "TiledLevelEditorLog.h"
#include "AssetToolsModule.h"
#include "TiledLevelStyle.h"
#include "IAssetTools.h"
#include "ISettingsModule.h"
#include "LevelEditor.h"
#include "StaticTiledLevel.h"
#include "StaticTiledLevelDetailCustomization.h"
#include "TiledItemDetailCustomization.h"
#include "TiledItem_ThumbnailRenderer.h"
#include "TiledLevel.h"
#include "TiledLevelAsset.h"
#include "TiledLevelEditor/TiledLevelDetailCustomization.h"
#include "TiledLevelCommands.h"
#include "TiledLevelEditorUtility.h"
#include "TiledLevelEditor/TiledLevelEdMode.h"
#include "ToolMenus.h"
#include "TiledLevelItem.h"
#include "TiledLevelPropertyCustomizations.h"
#include "TiledLevelSettings.h"
#include "Engine/StaticMeshActor.h"
#include "TiledItemSetEditor/TiledItemSetDetailCustomization.h"
#include "TiledItemSetEditor/TiledItemSetTypeActions.h"
#include "TiledLevelEditor/TiledLevelEditor.h"
#include "TiledLevelEditor/TiledLevelAssetTypeActions.h"
#include "TiledLevelEditor/TiledLevelAsset_ThumbnailRenderer.h"

DEFINE_LOG_CATEGORY(LogTiledLevelDev);

#define LOCTEXT_NAMESPACE "TiledLevelModule"

void FTiledLevelModule::StartupModule()
{
	// register TiledLevel EdMode
	FEditorModeRegistry::Get().RegisterMode<FTiledLevelEdMode>(
		FTiledLevelEdMode::EM_TiledLevelEdModeId,
		LOCTEXT("TiledLevelEdModeName", "TiledLevelEdMode"),
		FSlateIcon(),
		false); // make invisible, and only enable when editing asset

	// StyleSet
	FTiledLevelStyle::Initialize();
	FTiledLevelStyle::ReloadTextures();
	
	// register detail customization
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomClassLayout(
		UTiledLevelAsset::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(FTiledLevelDetailCustomization::MakeInstance)
		);
	PropertyModule.RegisterCustomClassLayout(
		ATiledLevel::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(FTiledLevelDetailCustomization::MakeInstance)
		);
	PropertyModule.RegisterCustomClassLayout(
		UTiledLevelItem::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(FTiledItemDetailCustomization::MakeInstance)
		);
	PropertyModule.RegisterCustomClassLayout(
		UTiledItemSet::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(FTiledItemSetDetailCustomization::MakeInstance)
		);
	PropertyModule.RegisterCustomClassLayout(
		AStaticTiledLevel::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(FStaticTiledLevelDetailCustomization::MakeInstance)
		);

    // struct property customizations
    PropertyModule.RegisterCustomPropertyTypeLayout(
        FName(TEXT("TemplateAsset")),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTemplateAssetDetails::MakeInstance)
        );
    

	// register tiled level asset
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	EAssetTypeCategories::Type TiledLevelAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("TiledLevel")), LOCTEXT("TiledLevelAssetCategory", "TiledLevel"));
	TSharedRef<IAssetTypeActions> TiledLevelAssetActions = MakeShareable(new FTiledLevelAssetTypeActions(TiledLevelAssetCategory));
	TSharedRef<IAssetTypeActions> TiledItemSetActions = MakeShareable(new FTiledItemSetTypeActions(TiledLevelAssetCategory));
	AssetTools.RegisterAssetTypeActions(TiledLevelAssetActions);
	CreatedAssetTypeActions.Add(TiledLevelAssetActions);
	AssetTools.RegisterAssetTypeActions(TiledItemSetActions);
	CreatedAssetTypeActions.Add(TiledItemSetActions);


	// register thumbnail renderer
	UThumbnailManager::Get().RegisterCustomRenderer(UTiledLevelItem::StaticClass(), UTiledItem_ThumbnailRenderer::StaticClass());
	UThumbnailManager::Get().RegisterCustomRenderer(UTiledLevelAsset::StaticClass(), UTiledLevelAsset_ThumbnailRenderer::StaticClass());
	UThumbnailManager::Get().RegisterCustomRenderer(UTiledLevelTemplateItem::StaticClass(), UTiledLevelAsset_ThumbnailRenderer::StaticClass());
	FTiledLevelCommands::Register();

	// Register plugin setting
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings(
		    "Editor",
		    "Plugins",
		    "TiledLevel",
		    LOCTEXT("PluginSettingsLabel", "Tiled Level"),
		    LOCTEXT("PluginSettingsDescription", "Configure Tiled Level plugin"),
		    GetMutableDefault<UTiledLevelSettings>()
		);
	}

	// Add extra level viewport menu extender for tiled level actor
	AddLevelViewportMenuExtender();

}

void FTiledLevelModule::ShutdownModule()
{
	FEditorModeRegistry::Get().UnregisterMode(FTiledLevelEdMode::EM_TiledLevelEdModeId);
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(UTiledLevelAsset::StaticClass()->GetFName());
		PropertyModule.UnregisterCustomClassLayout(ATiledLevel::StaticClass()->GetFName());
		PropertyModule.UnregisterCustomClassLayout(UTiledLevelItem::StaticClass()->GetFName());
		PropertyModule.NotifyCustomizationModuleChanged();
	}
	RemoveLevelViewportMenuExtender();
	
	FTiledLevelStyle::Shutdown();
	FTiledLevelCommands::Unregister();
}

TSharedRef<FTiledLevelEditor> FTiledLevelModule::CreateTiledLevelEditor(const EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost> InitToolkitHost, UTiledLevelAsset* TiledLevel)
{
	TSharedRef<FTiledLevelEditor> NewTiledLevelEditor(new FTiledLevelEditor());
	NewTiledLevelEditor->InitTileLevelEditor(Mode, InitToolkitHost, TiledLevel);
	return NewTiledLevelEditor;
}

void FTiledLevelModule::AddLevelViewportMenuExtender()
{
	// Try To add level viewport menu extender
	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	auto& MenuExtenders = LevelEditorModule.GetAllLevelViewportContextMenuExtenders();
	MenuExtenders.Add(FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors::CreateLambda([=](const TSharedRef<FUICommandList> CommandList, const TArray<AActor*> InActors)
	{
		TSharedRef<FExtender> Extender = MakeShareable(new FExtender);
		if (InActors.Num() == 1)
		{
			if (ATiledLevel* ATL = Cast<ATiledLevel>(InActors[0]))
			{
				FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
				TSharedRef<FUICommandList> LevelEditorCommandBindings = LevelEditor.GetGlobalLevelEditorActions();
				Extender->AddMenuExtension("ActorControl", EExtensionHook::After, LevelEditorCommandBindings, FMenuExtensionDelegate::CreateLambda(
					[=](FMenuBuilder& MenuBuilder) {

					MenuBuilder.AddMenuEntry(
						LOCTEXT("ConvertSelectedTiledLevelToStaticMeshes", "Break Tiled Level"),
						LOCTEXT("ConvertSelectedTiledLevelToStaticMeshesTooltip", "Break the selected tiled level into separate actors! Enable fine-tuning detailed transformation"),
						FSlateIcon(FTiledLevelStyle::GetStyleSetName(), "TiledLevel.BreakAsset"),
						FUIAction(FExecuteAction::CreateRaw(this, &FTiledLevelModule::BreakTiledLevel, ATL))
					);
					MenuBuilder.AddMenuEntry(
						LOCTEXT("MergeTiledLevel", "Merge Tiled Level"),
						LOCTEXT("MergeTiledLevelTooltip", "Merge tiled level into single static mesh asset"),
						FSlateIcon(FTiledLevelStyle::GetStyleSetName(), "TiledLevel.MergeAsset"),
						FUIAction(FExecuteAction::CreateRaw(this, &FTiledLevelModule::MergeTiledLevel, ATL))
					);
					MenuBuilder.AddMenuEntry(
						LOCTEXT("MergeAndReplaceTiledLevel", "Merge and Replace"),
						LOCTEXT("MergeAndReplaceTiledLevelTooltip", "Merge tiled level into single static mesh asset and replace it here with current transform"),
						FSlateIcon(FTiledLevelStyle::GetStyleSetName(), "TiledLevel.MergeAsset"),
						FUIAction(FExecuteAction::CreateRaw(this, &FTiledLevelModule::MergeTiledLevelAndReplace, ATL))
					);
				})
				);
			}
			if (AStaticTiledLevel* ASL = Cast<AStaticTiledLevel>(InActors[0]))
			{
				FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
				TSharedRef<FUICommandList> LevelEditorCommandBindings = LevelEditor.GetGlobalLevelEditorActions();
				Extender->AddMenuExtension("ActorControl", EExtensionHook::After, LevelEditorCommandBindings, FMenuExtensionDelegate::CreateLambda(
					[=](FMenuBuilder& MenuBuilder)
					{
						MenuBuilder.AddMenuEntry(
							LOCTEXT("RevertTiledLevel", "Revert to tiled level"),
							LOCTEXT("RevertTiledLevelTooltip", "Make this static version of tiled level editable again!"),
							FSlateIcon(FTiledLevelStyle::GetStyleSetName(), "ClassIcon.TiledLevelAsset"),
							FUIAction(FExecuteAction::CreateRaw(this, &FTiledLevelModule::RevertStaticTiledLevel, ASL))
						);
					}
					));
			}
		}
		return Extender;
	}));
	LevelViewportExtenderHandle = MenuExtenders.Last().GetHandle();
	
}

void FTiledLevelModule::RemoveLevelViewportMenuExtender()
{
	if (LevelViewportExtenderHandle.IsValid())
	{
		FLevelEditorModule* LevelEditorModule = FModuleManager::Get().GetModulePtr<FLevelEditorModule>("LevelEditor");
		if (LevelEditorModule)
		{
			typedef FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors DelegateType;
			LevelEditorModule->GetAllLevelViewportContextMenuExtenders().RemoveAll([=](const DelegateType& In) { return In.GetHandle() == LevelViewportExtenderHandle; });
		}
	}
}

// TODO: transaction ??? I did not implement it??
void FTiledLevelModule::BreakTiledLevel(ATiledLevel* TargetTiledLevel)
{
	FScopedTransaction Transaction(LOCTEXT("BreakTransaction", "Break tiled level"));
	TargetTiledLevel->Break();
}

void FTiledLevelModule::MergeTiledLevel(ATiledLevel* TargetTiledLevel)
{
	FScopedTransaction Transaction(LOCTEXT("MergeTransaction", "Merge tiled level"));
	if (!TargetTiledLevel->GetAsset()->HostLevel)
		TargetTiledLevel->GetAsset()->HostLevel = TargetTiledLevel;
	FTiledLevelEditorUtility::MergeTiledLevelAsset(TargetTiledLevel->GetAsset());
}

void FTiledLevelModule::MergeTiledLevelAndReplace(ATiledLevel* TargetTiledLevel)
{
	FScopedTransaction Transaction(LOCTEXT("MergeInplaceTransaction", "Merge and replace"));
	if (!TargetTiledLevel->GetAsset()->HostLevel)
		TargetTiledLevel->GetAsset()->HostLevel = TargetTiledLevel;
	if (UStaticMesh* NewMeshAsset = FTiledLevelEditorUtility::MergeTiledLevelAsset(TargetTiledLevel->GetAsset()))
	{
		FActorSpawnParameters Params;
		Params.bNoFail = 1;
		
		AStaticTiledLevel* STA = TargetTiledLevel->GetWorld()->SpawnActor<AStaticTiledLevel>(Params);
		STA->SetSourceAsset(TargetTiledLevel->GetAsset());
		STA->GetStaticMeshComponent()->SetStaticMesh(NewMeshAsset);
		STA->SetActorTransform(TargetTiledLevel->GetActorTransform());
		TArray<AActor*> Attached;
		TargetTiledLevel->GetAttachedActors(Attached);
		for (AActor* A : Attached)
			A->Destroy();
		TargetTiledLevel->Destroy();
	}
}

void FTiledLevelModule::RevertStaticTiledLevel(AStaticTiledLevel* TargetStaticTiledLevel)
{
	FScopedTransaction Transaction(LOCTEXT("RevertSTLTransaction", "Revert static tiled level"));
	TargetStaticTiledLevel->RevertToTiledLevel();
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTiledLevelModule, TiledLevel)