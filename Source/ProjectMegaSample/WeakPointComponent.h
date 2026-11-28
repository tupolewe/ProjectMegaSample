#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "WeakPointComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FWeakPointHitEvent, const FHitResult&, Hit,
    float, BaseDamage, float, ModifiedDamage, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWeakPointStateEvent, bool, bEnabled);

/** Query-only hitbox. Weapon code must explicitly call ProcessWeakPointHit.
 * This component never applies damage and does not subscribe to physics hit events. */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class PROJECTMEGASAMPLE_API UWeakPointComponent : public USphereComponent
{
    GENERATED_BODY()

public:
    UWeakPointComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Weak Point")
    FName WeakPointId = TEXT("Core");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Weak Point", meta=(ClampMin="1.0"))
    float DamageMultiplier = 2.5f;

    /** Vulnerability only: disabled points still block the configured weapon trace.
     * This preserves cover; a disabled point should receive ordinary base damage. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Weak Point")
    bool bWeakPointEnabled = true;

    /** Validates the Hit Component and calculates damage. Returns true only for an
     * enabled weak-point hit. On false, ModifiedDamage contains safe base damage.
     * Caller applies damage ONCE using ModifiedDamage, regardless of return value.
     * Call once per confirmed weapon hit, on the server for networked games. */
    UFUNCTION(BlueprintCallable, Category="Combat|Weak Point")
    bool ProcessWeakPointHit(const FHitResult& Hit, float BaseDamage, AActor* DamageCauser,
        float& ModifiedDamage);

    UFUNCTION(BlueprintCallable, Category="Combat|Weak Point")
    void SetWeakPointEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category="Combat|Weak Point")
    bool IsWeakPointEnabled() const { return bWeakPointEnabled; }

    /** Animation, VFX, sound, stagger etc. Do not apply damage here AND in the weapon. */
    UPROPERTY(BlueprintAssignable, Category="Combat|Weak Point")
    FWeakPointHitEvent OnWeakPointHit;

    UPROPERTY(BlueprintAssignable, Category="Combat|Weak Point")
    FWeakPointStateEvent OnWeakPointStateChanged;
};
