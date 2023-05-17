// Copyright (c) RITMPS, Rochester Institute of Technology, 2022

using UnrealBuildTool;

public class LiveLinkDragon : ModuleRules
{
	public LiveLinkDragon(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"LiveLinkInterface"
			});


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Json",
				"Networking",
				"Sockets"
			});
	}
}
