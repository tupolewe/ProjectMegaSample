#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AttackSelectionLibrary.generated.h"

/** Stateless weighted selection, compatible with existing Blueprint attack structs. */
UCLASS()
class PROJECTMEGASAMPLE_API UAttackSelectionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** Connect AvailableAttacks (S_MaleeAttackData[]) directly. Requires numeric fields
     * MinRange, MaxRange and Weight. Invalid ranges/weights are skipped.
     * Returns false and index -1 if no candidate exists. Does not modify CurrentAttack.
     * Callable (not pure) so Blueprint evaluates the random choice exactly once. */
    UFUNCTION(BlueprintCallable, CustomThunk, Category="Combat|Attack Selection",
        meta=(ArrayParm="AvailableAttacks", DisplayName="Choose Melee Attack (Weighted)"))
    static bool ChooseMeleeAttackWeighted(const TArray<int32>& AvailableAttacks, float AttackDistance, int32& SelectedIndex);

    DECLARE_FUNCTION(execChooseMeleeAttackWeighted);

    /** Native implementation; UnitSample must be in [0,1). Exposed for deterministic tests. */
    static bool SelectAttack(const void* Array, const FArrayProperty* ArrayProperty,
        double Distance, double UnitSample, int32& SelectedIndex);
};
