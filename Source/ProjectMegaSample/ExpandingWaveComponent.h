#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExpandingWaveComponent.generated.h"

class ACharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaveCharacterDetected, ACharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWaveFinished);

/** Horizontal expanding ring. Detects upright character capsules; never applies damage. */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class PROJECTMEGASAMPLE_API UExpandingWaveComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UExpandingWaveComponent();

    /** World-space point on the ground, NOT the character's capsule centre.
     * Returns false for invalid settings or while another wave is running.
     * Detection begins on the next tick, allowing Blueprint to bind events first. */
    UFUNCTION(BlueprintCallable, Category="Combat|Wave")
    bool StartWave(FVector GroundOrigin);

    /** Cancels silently. Does not broadcast OnWaveFinished. */
    UFUNCTION(BlueprintCallable, Category="Combat|Wave")
    void StopWave();

    UFUNCTION(BlueprintPure, Category="Combat|Wave")
    bool IsWaveRunning() const { return bRunning; }

    UFUNCTION(BlueprintPure, Category="Combat|Wave")
    float GetCurrentRadius() const { return CurrentRadius; }

    UFUNCTION(BlueprintPure, Category="Combat|Wave")
    FVector GetWaveOrigin() const { return Origin; }

    /** Called once per character per wave. Apply team filtering and damage in Blueprint. */
    UPROPERTY(BlueprintAssignable, Category="Combat|Wave")
    FWaveCharacterDetected OnCharacterDetected;

    /** Natural completion only. */
    UPROPERTY(BlueprintAssignable, Category="Combat|Wave")
    FWaveFinished OnWaveFinished;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Wave", meta=(ClampMin="1", Units="cm"))
    float MaxRadius = 1200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Wave", meta=(ClampMin="1", Units="cm/s"))
    float ExpansionSpeed = 600.f;

    /** Ring extends backwards from its leading edge by this distance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Wave", meta=(ClampMin="1", Units="cm"))
    float RingWidth = 60.f;

    /** Vertical band from GroundOrigin.Z to GroundOrigin.Z + WaveHeight. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Wave", meta=(ClampMin="1", Units="cm"))
    float WaveHeight = 40.f;

    /** Collision object channels used by target capsules. Defaults to Pawn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Wave")
    TArray<TEnumAsByte<ECollisionChannel>> TargetObjectChannels;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Wave")
    bool bDrawDebug = false;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    bool bRunning = false;
    uint64 Generation = 0;
    float CurrentRadius = 0.f;
    float ActiveMaxRadius = 0.f;
    float ActiveSpeed = 0.f;
    float ActiveWidth = 0.f;
    float ActiveHeight = 0.f;
    FVector Origin = FVector::ZeroVector;
    TArray<TEnumAsByte<ECollisionChannel>> ActiveChannels;
    TSet<TWeakObjectPtr<ACharacter>> Detected;
};
