#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TraversalComponent.generated.h"

class ACharacter;
class AController;
class UAnimInstance;
class UAnimMontage;

UENUM(BlueprintType)
enum class ETraversalAction : uint8 { Mantle, Vault };

UENUM(BlueprintType)
enum class ETraversalResult : uint8 { Succeeded, Cancelled, Blocked, AnimationInterrupted, InvalidOwner, TimedOut };

USTRUCT(BlueprintType)
struct PROJECTMEGASAMPLE_API FTraversalTarget
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Traversal") ETraversalAction Action = ETraversalAction::Mantle;
    UPROPERTY(BlueprintReadOnly, Category="Traversal") FVector Start = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category="Traversal") FVector Lift = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category="Traversal") FVector Across = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category="Traversal") FVector Landing = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category="Traversal") float ObstacleHeight = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTraversalStarted, FTraversalTarget, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTraversalFinished, ETraversalResult, Result, FTraversalTarget, Target);

/** Local/standalone traversal with swept capsule movement. Blueprint owns combat/grapple state. */
UCLASS(ClassGroup=(Movement), meta=(BlueprintSpawnableComponent))
class PROJECTMEGASAMPLE_API UTraversalComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UTraversalComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    /** Read-only detection and full capsule path validation; does not start an action. */
    UFUNCTION(BlueprintCallable, Category="Traversal")
    bool FindTraversalTarget(FTraversalTarget& Target, FString& FailureReason) const;

    /** Returns false without acquiring locks when detection or prerequisites fail. */
    UFUNCTION(BlueprintCallable, Category="Traversal") bool TryTraverse();
    UFUNCTION(BlueprintCallable, Category="Traversal") void CancelTraversal();
    UFUNCTION(BlueprintPure, Category="Traversal") bool IsTraversing() const { return bActive; }

    UPROPERTY(BlueprintAssignable, Category="Traversal") FTraversalStarted OnTraversalStarted;
    UPROPERTY(BlueprintAssignable, Category="Traversal") FTraversalFinished OnTraversalFinished;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Traversal") FString LastFailureReason;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Traversal") FTraversalTarget CurrentTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Detection", meta=(ClampMin="1", Units="cm")) float Reach = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Detection", meta=(ClampMin="1", Units="cm")) float MinHeight = 30.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Detection", meta=(ClampMin="1", Units="cm")) float MaxHeight = 220.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Detection", meta=(ClampMin="1", Units="cm")) float MaxVaultHeight = 110.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Detection", meta=(ClampMin="1", Units="cm")) float MaxVaultDepth = 180.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Detection", meta=(ClampMin="1", Units="cm")) float MaxLandingDrop = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Detection", meta=(ClampMin="2", Units="cm")) float Clearance = 5.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Detection") TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

    /** Movement is driven by this component, not montage root motion. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Animation") TObjectPtr<UAnimMontage> MantleMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Animation") TObjectPtr<UAnimMontage> VaultMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Animation", meta=(ClampMin="0.1")) float MontagePlayRate = 1.f;
    /** Used when no montage is assigned; otherwise the accepted montage length is used. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Movement", meta=(ClampMin="0.1", Units="s")) float Duration = 0.8f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Movement") bool bLockLook = true;

private:
    UFUNCTION() void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    void Finish(ETraversalResult Result, bool bNotify=true);
    bool bActive=false;
    bool bTransitioning=false;
    uint32 ActionSerial=0;
    bool bLockedLook=false;
    bool bLockedMove=false;
    float Elapsed=0.f;
    float ActiveDuration=0.f;
    uint8 SavedRootMotionMode=0;
    TWeakObjectPtr<ACharacter> Character;
    TWeakObjectPtr<AController> LockedController;
    TWeakObjectPtr<UAnimInstance> Animation;
    UPROPERTY(Transient) TObjectPtr<UAnimMontage> ActiveMontage;
};
