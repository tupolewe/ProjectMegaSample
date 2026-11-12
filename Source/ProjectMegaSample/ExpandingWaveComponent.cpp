#include "ExpandingWaveComponent.h"

#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

UExpandingWaveComponent::UExpandingWaveComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
    TargetObjectChannels.Add(ECC_Pawn);
}

bool UExpandingWaveComponent::StartWave(FVector GroundOrigin)
{
    if (bRunning || !GetWorld() || GroundOrigin.ContainsNaN()
        || !FMath::IsFinite(MaxRadius) || MaxRadius <= 0.f
        || !FMath::IsFinite(ExpansionSpeed) || ExpansionSpeed <= 0.f
        || !FMath::IsFinite(RingWidth) || RingWidth <= 0.f
        || !FMath::IsFinite(WaveHeight) || WaveHeight <= 0.f)
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

    Origin = GroundOrigin;
    ActiveMaxRadius = MaxRadius;
    ActiveSpeed = ExpansionSpeed;
    ActiveWidth = RingWidth;
    ActiveHeight = WaveHeight;
    CurrentRadius = 0.f;
    Detected.Reset();
    ++Generation;
    bRunning = true;
    SetComponentTickEnabled(true);
    return true;
}

void UExpandingWaveComponent::StopWave()
{
    ++Generation;
    bRunning = false;
    SetComponentTickEnabled(false);
    Detected.Reset();
}

void UExpandingWaveComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopWave();
    Super::EndPlay(EndPlayReason);
}

void UExpandingWaveComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bRunning || DeltaTime <= 0.f || !GetWorld())
    {
        return;
    }

    const uint64 TickGeneration = Generation;
    // Cover the entire radial distance travelled this frame, not just the new ring.
    const float InnerRadius = FMath::Max(0.f, CurrentRadius - ActiveWidth);
    CurrentRadius = FMath::Min(ActiveMaxRadius, CurrentRadius + ActiveSpeed * DeltaTime);
    const FVector QueryCenter = Origin + FVector(0, 0, ActiveHeight * 0.5f);
    FCollisionObjectQueryParams ObjectParams;
    for (const ECollisionChannel Channel : ActiveChannels)
    {
        ObjectParams.AddObjectTypesToQuery(Channel);
    }
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ExpandingWave), false, GetOwner());
    TArray<FOverlapResult> Overlaps;
    GetWorld()->OverlapMultiByObjectType(Overlaps, QueryCenter, FQuat::Identity, ObjectParams,
        FCollisionShape::MakeBox(FVector(CurrentRadius, CurrentRadius, ActiveHeight * 0.5f)), QueryParams);

    for (const FOverlapResult& Overlap : Overlaps)
    {
        ACharacter* Character = Cast<ACharacter>(Overlap.GetActor());
        if (!IsValid(Character) || Character == GetOwner() || Detected.Contains(Character))
        {
            continue;
        }
        const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
        if (!Capsule || Overlap.GetComponent() != Capsule)
        {
            continue;
        }
        // This detector models upright character capsules, not tilted/ragdoll bodies.
        if (FMath::Abs(Capsule->GetUpVector().Z) < 0.999f)
        {
            continue;
        }
        const FVector Center = Capsule->GetComponentLocation();
        const float Radius = Capsule->GetScaledCapsuleRadius();
        const float AxisHalfHeight = FMath::Max(0.f, Capsule->GetScaledCapsuleHalfHeight() - Radius);
        const double VerticalGap = FMath::Max(0.0, FMath::Max(
            Origin.Z - (Center.Z + AxisHalfHeight),
            (Center.Z - AxisHalfHeight) - (Origin.Z + ActiveHeight)));
        if (VerticalGap > Radius)
        {
            continue;
        }
        // Cross-section of the capsule within the wave's vertical band.
        const double HorizontalRadius = FMath::Sqrt(FMath::Max(0.0, Radius * Radius - VerticalGap * VerticalGap));
        const double Distance = FVector::Dist2D(Center, Origin);
        if (Distance - HorizontalRadius > CurrentRadius || Distance + HorizontalRadius < InnerRadius)
        {
            continue;
        }
        Detected.Add(Character);
        OnCharacterDetected.Broadcast(Character);
        // Blueprint callbacks can cancel, destroy or restart the wave.
        if (!IsValid(this) || !bRunning || Generation != TickGeneration)
        {
            return;
        }
    }

    if (bDrawDebug)
    {
        for (const float Height : {0.f, ActiveHeight})
        {
            const FVector DrawCenter = Origin + FVector(0, 0, Height);
            for (const float Radius : {CurrentRadius, FMath::Max(0.f, CurrentRadius - ActiveWidth)})
            {
                DrawDebugCircle(GetWorld(), DrawCenter, Radius, 64, FColor::Cyan, false, -1.f,
                    0, 1.f, FVector::ForwardVector, FVector::RightVector, false);
            }
        }
    }

    if (CurrentRadius >= ActiveMaxRadius)
    {
        StopWave();
        OnWaveFinished.Broadcast();
    }
}
