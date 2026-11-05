using UnrealBuildTool;

public class ProjectMegaSample : ModuleRules
{
    public ProjectMegaSample(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "AIModule", "GameplayTasks"
        });
    }
}
