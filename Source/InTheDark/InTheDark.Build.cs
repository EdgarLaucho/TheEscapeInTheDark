using UnrealBuildTool;

public class InTheDark : ModuleRules
{
	public InTheDark(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Niagara", "GameplayTags", "AIModule", "NavigationSystem", "Slate" });
	}
}