using UnrealBuildTool;

public class AssaultAndroidCactusEditorTarget : TargetRules
{
    public AssaultAndroidCactusEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("AssaultAndroidCactus");
    }
}
