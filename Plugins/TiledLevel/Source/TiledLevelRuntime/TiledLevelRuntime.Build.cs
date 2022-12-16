// Copyright 2022 PufStudio. All Rights Reserved.

using UnrealBuildTool;

public class TiledLevelRuntime : ModuleRules
{
	public TiledLevelRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"ProceduralMeshComponent",
				"MeshDescription",
				"StaticMeshDescription",
			}
			);
		
	}
}
