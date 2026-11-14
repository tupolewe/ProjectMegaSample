#include "LineAreaAttackComponent.h"

#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

ULineAreaAttackComponent::ULineAreaAttackComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
    TargetObjectChannels.Add(ECC_Pawn);
}

bool ULineAreaAttackComponent::StartAttack(FVector GroundOrigin, FVector TargetLocation)
{
    if (bRunning || !GetWorld() || GroundOrigin.ContainsNaN() || TargetLocation.ContainsNaN()
        || AreaCount < 1 || AreaCount > 256
        || !FMath::IsFinite(AreaDiameter) || AreaDiameter <= 0.f
        || !FMath::IsFinite(AreaHeight) || AreaHeight <= 0.f
        || !FMath::IsFinite(StepDelay) || StepDelay < 0.01f
        || !FMath::IsFinite(WarningDuration) || WarningDuration < 0.f
        || !FMath::IsFinite(StartOffset) || StartOffset < 0.f)
    {
        return false;
    }
    FVector Direction = TargetLocation - GroundOrigin;
    Direction.Z = 0.0;
    if (!Direction.Normalize())
    {
        return false;
    }
    ActiveChannels.Reset();
    for (const ECollisionChannel Channel : TargetObjectChannels)
    {
        if (FCollisionObjectQueryParams::IsValidObjectQuery(Channel))
        {
            ActiveChannels.AddUnique(Channel);
        }
    }
    if (ActiveChannels.IsEmpty())
    {
        return false;
    }

    Areas.Reset(AreaCount);
    for (int32 Index = 0; Index < AreaCount; ++Index)
    {
        FLineAttackArea& Area = Areas.AddDefaulted_GetRef();
        Area.Index = Index;
        Area.Radius = AreaDiameter * 0.5f;
        Area.Height = AreaHeight;
        Area.WarningDuration = WarningDuration;
        Area.GroundCenter = GroundOrigin + Direction * (StartOffset + Area.Radius + double(Index) * AreaDiameter);
    }
    ActiveStepDelay = StepDelay;
    bActiveDetectOnce = bDetectEachCharacterOncePerAttack;
    Elapsed = 0.0;
    NextWarning = NextActivation = 0;
    Detected.Reset();
    ++Generation;
    bRunning = true;
    SetComponentTickEnabled(true);
    return true;
}

bool ULineAreaAttackComponent::IsCurrent(uint64 Run) const
{
    return IsValid(this) && bRunning && Generation == Run;
}

void ULineAreaAttackComponent::FinishAttack(bool bCancelled)
{
    bRunning = false;
    ++Generation;
    SetComponentTickEnabled(false);
    Areas.Reset();
    Detected.Reset();
    OnAttackEnded.Broadcast(bCancelled);
}

void ULineAreaAttackComponent::StopAttack()
{
    if (bRunning)
    {
        FinishAttack(true);
    }
}

void ULineAreaAttackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Do not invoke gameplay callbacks during world/owner teardown.
    bRunning = false;
    ++Generation;
    SetComponentTickEnabled(false);
    Areas.Reset();
    Detected.Reset();
    Super::EndPlay(EndPlayReason);
}

void ULineAreaAttackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bRunning || DeltaTime <= 0.f)
    {
        return;
    }
    const uint64 Run = Generation;
    Elapsed += DeltaTime;
    // Process overdue events chronologically, including multiple steps in a slow frame.
    while (IsCurrent(Run) && NextActivation < Areas.Num())
    {
        const double WarningTime = NextWarning < Areas.Num() ? NextWarning * ActiveStepDelay : TNumericLimits<double>::Max();
        const double ActivationTime = NextActivation * ActiveStepDelay + Areas[NextActivation].WarningDuration;
        if (FMath::Min(WarningTime, ActivationTime) > Elapsed)
        {
            break;
        }
        if (WarningTime <= ActivationTime)
        {
            const FLineAttackArea Area = Areas[NextWarning++];
            OnAreaWarning.Broadcast(Area);
        }
        else
        {
            // Copy: Blueprint callbacks may clear or replace the area array.
            const FLineAttackArea Area = Areas[NextActivation++];
            OnAreaActivated.Broadcast(Area);
            if (!IsCurrent(Run)) return;
            DetectArea(Area, Run);
        }
    }
    if (!IsCurrent(Run)) return;

    if (bDrawDebug)
    {
        for (int32 Index = NextActivation; Index < NextWarning; ++Index)
        {
            const FLineAttackArea& Area = Areas[Index];
            DrawDebugCylinder(GetWorld(), Area.GroundCenter, Area.GroundCenter + FVector(0, 0, Area.Height),
                Area.Radius, 48, FColor::Yellow, false, -1.f, 0, 1.f);
        }
    }
    if (NextActivation >= Areas.Num())
    {
        FinishAttack(false);
    }
}

void ULineAreaAttackComponent::DetectArea(const FLineAttackArea& Area, uint64 Run)
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (bDrawDebug)
    {
        DrawDebugCylinder(World, Area.GroundCenter, Area.GroundCenter + FVector(0, 0, Area.Height),
            Area.Radius, 48, FColor::Red, false, 0.2f, 0, 2.f);
    }
    FCollisionObjectQueryParams ObjectParams;
    for (const ECollisionChannel Channel : ActiveChannels)
    {
        ObjectParams.AddObjectTypesToQuery(Channel);
    }
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LineAreaAttack), false, GetOwner());
    TArray<FOverlapResult> Overlaps;
    World->OverlapMultiByObjectType(Overlaps, Area.GroundCenter + FVector(0, 0, Area.Height * 0.5f),
        FQuat::Identity, ObjectParams, FCollisionShape::MakeBox(FVector(Area.Radius, Area.Radius, Area.Height * 0.5f)), QueryParams);
    TSet<TWeakObjectPtr<ACharacter>> AreaDetected;
    for (const FOverlapResult& Overlap : Overlaps)
    {
        ACharacter* Character = Cast<ACharacter>(Overlap.GetActor());
        if (!IsValid(Character) || Character == GetOwner() || AreaDetected.Contains(Character)
            || (bActiveDetectOnce && Detected.Contains(Character))) continue;
        const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
        if (!Capsule || Overlap.GetComponent() != Capsule || FMath::Abs(Capsule->GetUpVector().Z) < 0.999f) continue;
        const FVector Center = Capsule->GetComponentLocation();
        const double Radius = Capsule->GetScaledCapsuleRadius();
        const double AxisHalfHeight = FMath::Max(0.0, Capsule->GetScaledCapsuleHalfHeight() - Radius);
        const double VerticalGap = FMath::Max(0.0, FMath::Max(
            Area.GroundCenter.Z - (Center.Z + AxisHalfHeight),
            Center.Z - AxisHalfHeight - (Area.GroundCenter.Z + Area.Height)));
        if (VerticalGap > Radius) continue;
        const double CrossSectionRadius = FMath::Sqrt(FMath::Max(0.0, Radius * Radius - VerticalGap * VerticalGap));
        if (FVector::Dist2D(Center, Area.GroundCenter) > Area.Radius + CrossSectionRadius) continue;
        AreaDetected.Add(Character);
        Detected.Add(Character);
        OnCharacterDetected.Broadcast(Character, Area);
        if (!IsCurrent(Run)) return;
    }
}
