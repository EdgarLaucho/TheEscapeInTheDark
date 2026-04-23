using UnrealBuildTool;
using System.Collections.Generic;

public class InTheDarkEditorTarget : TargetRules
{
	public InTheDarkEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.AddRange(new string[] { "InTheDark" });
	}
}
