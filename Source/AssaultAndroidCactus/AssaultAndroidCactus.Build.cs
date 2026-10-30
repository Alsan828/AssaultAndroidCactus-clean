using UnrealBuildTool;

public class AssaultAndroidCactus : ModuleRules
{
    public AssaultAndroidCactus(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicIncludePaths.Add(ModuleDirectory);
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule", "NavigationSystem", "UMG", "Niagara"
        });
        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore" });
    }
}
