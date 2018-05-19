// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AppleARKitFaceSupport : ModuleRules
{
	public AppleARKitFaceSupport(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(
			new string[] {
				"../../../../Plugins/Runtime/AppleARKit/Source/AppleARKit/Private",
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
                "IOSRuntimeSettings",
                "HeadMountedDisplay",
                "AugmentedReality",
                "ProceduralMeshComponent",
                "LiveLink",
                "LiveLinkInterface",
//                "OnlineSubsystem",
                "Sockets",
                "AppleARKit",
                "AppleImageUtils"
				// ... add private dependencies that you statically link with here ...
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

		if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			PublicFrameworks.Add( "ARKit" );
		}
	}
}
