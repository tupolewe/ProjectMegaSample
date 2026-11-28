#include "WeakPointComponent.h"

UWeakPointComponent::UWeakPointComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    InitSphereRadius(20.f);
    SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SetCollisionObjectType(ECC_WorldDynamic);
    SetCollisionResponseToAllChannels(ECR_Ignore);
    SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    SetGenerateOverlapEvents(false);
    SetCanEverAffectNavigation(false);
    SetHiddenInGame(true);
}

bool UWeakPointComponent::ProcessWeakPointHit(const FHitResult& Hit, float BaseDamage,
    AActor* DamageCauser, float& ModifiedDamage)
{
    ModifiedDamage = FMath::IsFinite(BaseDamage) ? FMath::Max(0.f, BaseDamage) : 0.f;
    if (!bWeakPointEnabled || Hit.GetComponent() != this || !FMath::IsFinite(BaseDamage)
        || BaseDamage <= 0.f || !FMath::IsFinite(DamageMultiplier) || DamageMultiplier < 1.f)
    {
        return false;
    }

    const double Result = double(BaseDamage) * double(DamageMultiplier);
    ModifiedDamage = float(FMath::Min(Result, double(TNumericLimits<float>::Max())));
    OnWeakPointHit.Broadcast(Hit, BaseDamage, ModifiedDamage, DamageCauser);
    return true;
}

void UWeakPointComponent::SetWeakPointEnabled(bool bEnabled)
{
    if (bWeakPointEnabled != bEnabled)
    {
        bWeakPointEnabled = bEnabled;
        OnWeakPointStateChanged.Broadcast(bEnabled);
    }
}
