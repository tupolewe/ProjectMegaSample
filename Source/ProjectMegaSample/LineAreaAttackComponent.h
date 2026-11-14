#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LineAreaAttackComponent.generated.h"

class ACharacter;

USTRUCT(BlueprintType)
struct FLineAttackArea
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Attack")
    int32 Index = 0;

    UPROPERTY(BlueprintReadOnly, Category="Attack")
    FVector GroundCenter = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="Attack")
    float Radius = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="Attack")
    float Height = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="Attack")
    float WarningDuration = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLineAreaEvent, FLineAttackArea, Area);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLineAreaHitEvent, ACharacter*, Character, FLineAttackArea, Area);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLineAttackEndEvent, bool, bCancelled);

/** Sequential, instantaneous circular strikes. Blueprint handles visuals, teams and damage. */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class PROJECTMEGASAMPLE_API ULineAreaAttackComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULineAreaAttackComponent();

    /** Captures a horizontal direction towards TargetLocation. Neither endpoint is tracked.
     * GroundOrigin must be on the floor. First circle's near edge is StartOffset away.
     * Returns false while running, for invalid settings, or for zero horizontal direction. */
    UFUNCTION(BlueprintCallable, Category="Combat|Line Attack")
    bool StartAttack(FVector GroundOrigin, FVector TargetLocation);

    /** Cancels pending areas and broadcasts OnAttackEnded(true). */
    UFUNCTION(BlueprintCallable, Category="Combat|Line Attack")
    void StopAttack();

    UFUNCTION(BlueprintPure, Category="Combat|Line Attack")
    bool IsAttackRunning() const { return bRunning; }

    /** Spawn a warning marker here. Area.Index identifies it within the current attack. */
    UPROPERTY(BlueprintAssignable, Category="Combat|Line Attack")
    FLineAreaEvent OnAreaWarning;

    /** Remove the warning and play the impact effect. Detection follows this event. */
    UPROPERTY(BlueprintAssignable, Category="Combat|Line Attack")
    FLineAreaEvent OnAreaActivated;

    UPROPERTY(BlueprintAssignable, Category="Combat|Line Attack")
    FLineAreaHitEvent OnCharacterDetected;

    /** Also use this to clean up any remaining warning markers. */
    UPROPERTY(BlueprintAssignable, Category="Combat|Line Attack")
    FLineAttackEndEvent OnAttackEnded;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Line Attack", meta=(ClampMin="1", ClampMax="256"))
    int32 AreaCount = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Line Attack", meta=(ClampMin="1", Units="cm"))
    float AreaDiameter = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Line Attack", meta=(ClampMin="1", Units="cm"))
    float AreaHeight = 150.f;

    /** Delay between consecutive warnings, and between consecutive strikes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Line Attack", meta=(ClampMin="0.01", Units="s"))
    float StepDelay = 0.3f;

    /** Time from each area's warning to its instantaneous detection pulse. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Line Attack", meta=(ClampMin="0", Units="s"))
    float WarningDuration = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Line Attack", meta=(ClampMin="0", Units="cm"))
    float StartOffset = 0.f;

    /** True: once across all areas. False: once per area, allowing repeated damage in Blueprint. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Line Attack")
    bool bDetectEachCharacterOncePerAttack = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Line Attack")
    TArray<TEnumAsByte<ECollisionChannel>> TargetObjectChannels;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Line Attack")
    bool bDrawDebug = false;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    bool IsCurrent(uint64 Run) const;
    void DetectArea(const FLineAttackArea& Area, uint64 Run);
    void FinishAttack(bool bCancelled);

    bool bRunning = false;
    bool bActiveDetectOnce = true;
    uint64 Generation = 0;
    double Elapsed = 0.0;
    double ActiveStepDelay = 0.0;
    int32 NextWarning = 0;
    int32 NextActivation = 0;
    TArray<FLineAttackArea> Areas;
    TArray<TEnumAsByte<ECollisionChannel>> ActiveChannels;
    TSet<TWeakObjectPtr<ACharacter>> Detected;
};
