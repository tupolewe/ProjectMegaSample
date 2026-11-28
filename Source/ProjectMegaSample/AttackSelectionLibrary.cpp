#include "AttackSelectionLibrary.h"
#include "UObject/Stack.h"
#include "UObject/UnrealType.h"

bool UAttackSelectionLibrary::ChooseMeleeAttackWeighted(const TArray<int32>&, float, int32& SelectedIndex)
{
    // Blueprint calls the custom thunk, never this placeholder array signature.
    SelectedIndex = INDEX_NONE;
    return false;
}

DEFINE_FUNCTION(UAttackSelectionLibrary::execChooseMeleeAttackWeighted)
{
    Stack.MostRecentProperty = nullptr;
    Stack.StepCompiledIn<FArrayProperty>(nullptr);
    const void* Array = Stack.MostRecentPropertyAddress;
    const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
    if (!ArrayProperty)
    {
        Stack.bArrayContextFailed = true;
        return;
    }
    P_GET_PROPERTY(FFloatProperty, AttackDistance);
    P_GET_PROPERTY_REF(FIntProperty, SelectedIndex);
    P_FINISH;
    P_NATIVE_BEGIN;
    // RandHelper uses an exclusive upper bound.
    const double Sample = double(FMath::RandHelper(16777216)) / 16777216.0;
    *(bool*)RESULT_PARAM = SelectAttack(Array, ArrayProperty, AttackDistance, Sample, SelectedIndex);
    P_NATIVE_END;
}

bool UAttackSelectionLibrary::SelectAttack(const void* Array, const FArrayProperty* ArrayProperty,
    double Distance, double UnitSample, int32& SelectedIndex)
{
    SelectedIndex = INDEX_NONE;
    const FStructProperty* Element = ArrayProperty ? CastField<FStructProperty>(ArrayProperty->Inner) : nullptr;
    if (!Array || !Element || !FMath::IsFinite(Distance) || Distance < 0.0
        || !FMath::IsFinite(UnitSample) || UnitSample < 0.0 || UnitSample >= 1.0) return false;

    auto FindNumber = [Element](const TCHAR* Name) -> const FNumericProperty*
    {
        for (TFieldIterator<FProperty> It(Element->Struct); It; ++It)
        {
            if (It->GetAuthoredName().TrimStartAndEnd().Equals(Name, ESearchCase::IgnoreCase))
                return CastField<FNumericProperty>(*It);
        }
        return nullptr;
    };
    const FNumericProperty* Min = FindNumber(TEXT("MinRange"));
    const FNumericProperty* Max = FindNumber(TEXT("MaxRange"));
    const FNumericProperty* Weight = FindNumber(TEXT("Weight"));
    if (!Min || !Max || !Weight) return false;

    auto Read = [](const FNumericProperty* Property, const void* Data) -> double
    {
        const void* Value = Property->ContainerPtrToValuePtr<void>(Data);
        return Property->IsFloatingPoint() ? Property->GetFloatingPointPropertyValue(Value)
            : Property->GetSignedIntPropertyValue(Value);
    };
    FScriptArrayHelper Helper(ArrayProperty, Array);
    struct FCandidate { int32 Index; double Weight; };
    TArray<FCandidate, TInlineAllocator<16>> Candidates;
    double LargestWeight = 0.0;
    for (int32 Index = 0; Index < Helper.Num(); ++Index)
    {
        const void* Data = Helper.GetRawPtr(Index);
        const double Low = Read(Min, Data), High = Read(Max, Data), W = Read(Weight, Data);
        if (!FMath::IsFinite(Low) || !FMath::IsFinite(High) || !FMath::IsFinite(W)
            || Low < 0.0 || High < Low || W <= 0.0 || Distance < Low || Distance > High) continue;
        Candidates.Add({Index, W});
        LargestWeight = FMath::Max(LargestWeight, W);
    }
    if (Candidates.IsEmpty()) return false;
    // Scale weights to avoid overflowing the total for large authored values.
    double Total = 0.0;
    for (const FCandidate& Candidate : Candidates) Total += Candidate.Weight / LargestWeight;
    const double Threshold = UnitSample * Total;
    double Running = 0.0;
    for (const FCandidate& Candidate : Candidates)
    {
        Running += Candidate.Weight / LargestWeight;
        if (Threshold < Running)
        {
            SelectedIndex = Candidate.Index;
            return true;
        }
    }
    // Floating point rounding at the upper boundary.
    SelectedIndex = Candidates.Last().Index;
    return true;
}
