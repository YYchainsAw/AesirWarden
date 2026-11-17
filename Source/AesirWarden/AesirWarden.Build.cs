// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class AesirWarden : ModuleRules
{
	public AesirWarden(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore",
			"GameplayTags",
			"GameplayAbilities",
			"GameplayTasks",
			"MotionWarping"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"UnrealEd", "UMGEditor", "KismetCompiler", "AssetRegistry"
			});
		}

		PrivateDependencyModuleNames.AddRange(new string[] {
			"LevelSequence",
			"MovieScene",
			"HTTP",
			"Json",
			"JsonUtilities",
			"Sockets"
		});

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicSystemLibraries.Add("bcrypt.lib");

			if (!Target.bBuildEditor)
			{
				string BossPolicyDirectory = Path.GetFullPath(Path.Combine(
					ModuleDirectory, "..", "..", "ThirdParty", "BossPolicy", "Windows"));
				// Re-evaluate the receipt when a runtime is restored after an earlier BT-only build.
				ExternalDependencies.Add(Path.Combine(BossPolicyDirectory, "release-manifest.json"));
				if (Directory.Exists(BossPolicyDirectory))
				{
					foreach (string SourceFile in Directory.EnumerateFiles(
						BossPolicyDirectory, "*", SearchOption.AllDirectories))
					{
						string RelativePath = Path.GetRelativePath(BossPolicyDirectory, SourceFile).Replace('\\', '/');
						RuntimeDependencies.Add("$(TargetOutputDir)/BossPolicy/" + RelativePath,
							SourceFile, StagedFileType.NonUFS);
						ExternalDependencies.Add(SourceFile);
					}
				}
			}
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
