using UnrealBuildTool;

public class AssaultAndroidCactusTarget : TargetRules
{
    public AssaultAndroidCactusTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("AssaultAndroidCactus");
    }
}
