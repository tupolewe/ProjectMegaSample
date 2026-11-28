#if WITH_DEV_AUTOMATION_TESTS
#include "../WeakPointComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeakPointComponentTest, "ProjectMegaSample.Combat.WeakPoint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeakPointComponentTest::RunTest(const FString&)
{
    UWeakPointComponent* Point = NewObject<UWeakPointComponent>();
    UWeakPointComponent* Other = NewObject<UWeakPointComponent>();
    FHitResult Hit;
    Hit.Component = Point;
    float Damage = -1.f;
    TestTrue(TEXT("Matching enabled hit accepted"), Point->ProcessWeakPointHit(Hit, 20.f, nullptr, Damage));
    TestEqual(TEXT("Damage multiplied"), Damage, 50.f);
    Point->SetWeakPointEnabled(false);
    TestFalse(TEXT("Disabled point"), Point->ProcessWeakPointHit(Hit, 20.f, nullptr, Damage));
    TestEqual(TEXT("Disabled retains base damage"), Damage, 20.f);
    TestEqual(TEXT("Disabling preserves trace blocking"), Point->GetCollisionResponseToChannel(ECC_Visibility), ECR_Block);
    Point->SetWeakPointEnabled(true);
    Hit.Component = Other;
    TestFalse(TEXT("Another point rejected"), Point->ProcessWeakPointHit(Hit, 20.f, nullptr, Damage));
    TestEqual(TEXT("Foreign hit retains base damage"), Damage, 20.f);
    Hit.Component = Point;
    TestFalse(TEXT("Negative damage rejected"), Point->ProcessWeakPointHit(Hit, -20.f, nullptr, Damage));
    TestEqual(TEXT("No negative damage"), Damage, 0.f);
    Point->DamageMultiplier = 0.f;
    TestFalse(TEXT("Invalid multiplier rejected"), Point->ProcessWeakPointHit(Hit, 20.f, nullptr, Damage));
    TestEqual(TEXT("Invalid multiplier retains base damage"), Damage, 20.f);
    Point->DamageMultiplier = 3.f;
    TestTrue(TEXT("Reenabled point works"), Point->ProcessWeakPointHit(Hit, 20.f, nullptr, Damage));
    TestEqual(TEXT("Updated multiplier"), Damage, 60.f);
    TestEqual(TEXT("Query-only collision"), Point->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
    TestEqual(TEXT("Does not block movement"), Point->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
    return true;
}
#endif
