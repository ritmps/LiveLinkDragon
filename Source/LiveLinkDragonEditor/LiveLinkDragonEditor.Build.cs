// Copyright (c) RITMPS, Rochester Institute of Technology, 2022

using UnrealBuildTool;

public class LiveLinkDragonEditor : ModuleRules
{
	public LiveLinkDragonEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
			});
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"LiveLinkDragon",
				"PropertyEditor",
				"Slate",
				"SlateCore"
			});
	}
}
