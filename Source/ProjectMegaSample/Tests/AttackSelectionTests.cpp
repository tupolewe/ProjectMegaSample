#if WITH_DEV_AUTOMATION_TESTS
#include "../AttackSelectionLibrary.h"
#include "Misc/AutomationTest.h"
#include "Engine/Blueprint.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackSelectionTest, "ProjectMegaSample.Combat.WeightedAttackSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAttackSelectionTest::RunTest(const FString&)
{
    UBlueprint* BP = LoadObject<UBlueprint>(nullptr, TEXT("/Game/Blueprints/Enemy/BP_Enemy_Heavy.BP_Enemy_Heavy"));
    if (!TestNotNull(TEXT("Heavy Blueprint"), BP) || !BP->GeneratedClass) return false;
    const FArrayProperty* Property = FindFProperty<FArrayProperty>(BP->GeneratedClass, TEXT("AvailableAttacks"));
    if (!TestNotNull(TEXT("Existing attack array"), Property)) return false;
    const FStructProperty* Inner = CastField<FStructProperty>(Property->Inner);
    if (!TestNotNull(TEXT("Blueprint attack structure"), Inner)) return false;
    FScriptArray Array;
    FScriptArrayHelper Helper(Property, &Array);
    Helper.AddValues(3);
    auto Set = [&](int32 Index, const TCHAR* Name, double Value)
    {
        for (TFieldIterator<FProperty> It(Inner->Struct); It; ++It)
        {
            if (It->GetAuthoredName().TrimStartAndEnd() == Name)
            {
                FNumericProperty* Number = CastField<FNumericProperty>(*It);
                if (!Number) return false;
                void* Address = Number->ContainerPtrToValuePtr<void>(Helper.GetRawPtr(Index));
                if (Number->IsFloatingPoint()) Number->SetFloatingPointPropertyValue(Address, Value);
                else Number->SetIntPropertyValue(Address, int64(Value));
                return true;
            }
        }
        return false;
    };
    for (int32 I = 0; I < 3; ++I)
    {
        TestTrue(TEXT("MinRange field"), Set(I, TEXT("MinRange"), 100));
        TestTrue(TEXT("MaxRange field"), Set(I, TEXT("MaxRange"), 200));
        TestTrue(TEXT("Weight field"), Set(I, TEXT("Weight"), I == 0 ? 1 : I == 1 ? 3 : 0));
    }
    int32 Selected = 99;
    int32 Counts[3] = {};
    for (int32 Sample = 0; Sample < 100; ++Sample)
    {
        TestTrue(TEXT("Selection succeeds"), UAttackSelectionLibrary::SelectAttack(&Array, Property, 150, Sample / 100.0, Selected));
        if (Selected >= 0 && Selected < 3) ++Counts[Selected];
    }
    TestEqual(TEXT("First candidate receives 25%"), Counts[0], 25);
    TestEqual(TEXT("Second candidate receives 75%"), Counts[1], 75);
    TestEqual(TEXT("Zero weight never selected"), Counts[2], 0);
    TestTrue(TEXT("Inclusive minimum"), UAttackSelectionLibrary::SelectAttack(&Array, Property, 100, 0, Selected));
    TestTrue(TEXT("Inclusive maximum"), UAttackSelectionLibrary::SelectAttack(&Array, Property, 200, 0, Selected));
    TestFalse(TEXT("Out of range"), UAttackSelectionLibrary::SelectAttack(&Array, Property, 201, 0, Selected));
    TestEqual(TEXT("Failure resets index"), Selected, INDEX_NONE);
    Set(0, TEXT("Weight"), -1);
    Set(1, TEXT("Weight"), 0);
    TestFalse(TEXT("No positive weights"), UAttackSelectionLibrary::SelectAttack(&Array, Property, 150, 0, Selected));
    Helper.EmptyValues();
    TestFalse(TEXT("Empty array"), UAttackSelectionLibrary::SelectAttack(&Array, Property, 150, 0, Selected));
    return true;
}
#endif
