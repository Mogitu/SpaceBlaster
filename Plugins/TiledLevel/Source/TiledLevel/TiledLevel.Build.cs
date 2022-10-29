// Copyright 2022 PufStudio. All Rights Reserved.

using UnrealBuildTool;

public class TiledLevel : ModuleRules
{
	public TiledLevel(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Slate",
				"SlateCore",
				"Engine",
				"UnrealEd",
				"PropertyEditor",
				"RenderCore",
				"RHI",
				"ProceduralMeshComponent",
				"MeshDescription",
				"StaticMeshDescription",
				"AssetTools",
				"AssetRegistry",
				"InputCore",
				"EditorStyle",
				"ToolMenus",
				"Projects",
				"ContentBrowser",
				"Kismet",
				"KismetWidgets",
				"AdvancedPreviewScene",
				"EditorFramework",
				"TiledLevelRuntime",
				// ... add other public dependencies that you statically link with here ...
			}
		);
	}

}
