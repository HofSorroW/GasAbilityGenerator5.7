// GasAbilityGeneratorRuntime - Runtime module for BP-callable bridge components
// v7.5.3: Split from Editor module to allow runtime Blueprint access

using UnrealBuildTool;

public class GasAbilityGeneratorRuntime : ModuleRules
{
	public GasAbilityGeneratorRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// RUNTIME-SAFE dependencies only - NO UnrealEd
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayAbilities",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			// Narrative Pro integration (runtime module)
			"NarrativeArsenal"
		});
	}
}
