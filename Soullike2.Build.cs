// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Soullike2 : ModuleRules
{
    public Soullike2(ReadOnlyTargetRules Target) : base(Target)
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
            "Slate"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "AnimGraphRuntime"
        });

        PublicIncludePaths.AddRange(new string[] {
            "Soullike2",
            "Soullike2/Variant_Platforming",
            "Soullike2/Variant_Platforming/Animation",
            "Soullike2/Variant_Combat",
            "Soullike2/Variant_Combat/AI",
            "Soullike2/Variant_Combat/Animation",
            "Soullike2/Variant_Combat/Gameplay",
            "Soullike2/Variant_Combat/Interfaces",
            "Soullike2/Variant_Combat/UI",
            "Soullike2/Variant_SideScrolling",
            "Soullike2/Variant_SideScrolling/AI",
            "Soullike2/Variant_SideScrolling/Gameplay",
            "Soullike2/Variant_SideScrolling/Interfaces",
            "Soullike2/Variant_SideScrolling/UI"
        });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}

