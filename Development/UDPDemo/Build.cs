// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UdptestClient : ModuleRules
{
	public UdptestClient(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "NavigationSystem", "AIModule", "Networking", "Sockets", "InputCore" });
	}
}
