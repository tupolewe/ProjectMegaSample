#include "TraversalComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"

UTraversalComponent::UTraversalComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.bStartWithTickEnabled=false;
    PrimaryComponentTick.TickGroup=TG_PrePhysics;
}

bool UTraversalComponent::FindTraversalTarget(FTraversalTarget& Target, FString& Reason) const
{
    Target=FTraversalTarget(); Reason.Reset();
    auto Fail=[&Reason](const TCHAR* Text) { Reason=Text; return false; };
    const ACharacter* C=Cast<ACharacter>(GetOwner());
    if (!IsValid(C) || !GetWorld()) return Fail(TEXT("Owner must be a valid Character."));
    const UCapsuleComponent* Capsule=C->GetCapsuleComponent();
    const UCharacterMovementComponent* Movement=C->GetCharacterMovement();
    if (!Capsule || !Movement || !Capsule->IsQueryCollisionEnabled()) return Fail(TEXT("Character needs an enabled capsule and CharacterMovement."));
    if (FVector::DotProduct(C->GetActorUpVector(),FVector::UpVector)<0.99) return Fail(TEXT("Only upright characters with world-Z gravity are supported."));
    if (!FMath::IsFinite(Reach) || !FMath::IsFinite(MinHeight) || !FMath::IsFinite(MaxHeight)
        || !FMath::IsFinite(MaxVaultDepth) || !FMath::IsFinite(MaxVaultHeight) || !FMath::IsFinite(MaxLandingDrop)
        || !FMath::IsFinite(Clearance) || Reach<=0 || MinHeight<=0 || MaxHeight<MinHeight || MaxVaultDepth<=0
        || MaxVaultHeight<=0 || MaxLandingDrop<=0 || Clearance<2)
        return Fail(TEXT("Invalid traversal detection settings."));
    const float Radius=Capsule->GetScaledCapsuleRadius(), Half=Capsule->GetScaledCapsuleHalfHeight();
    const FVector Start=C->GetActorLocation();
    const FVector Feet=Start-FVector(0,0,Half);
    const FVector Forward=C->GetActorForwardVector().GetSafeNormal2D();
    const FCollisionQueryParams Query(SCENE_QUERY_STAT(TraversalDetection),false,C);
    FHitResult Wall;
    const FVector WallStart=Feet+FVector(0,0,MinHeight);
    if (!GetWorld()->LineTraceSingleByChannel(Wall,WallStart,WallStart+Forward*(Radius+Reach),TraceChannel,Query))
        return Fail(TEXT("No obstacle ahead."));
    if (Wall.bStartPenetrating || FMath::Abs(Wall.ImpactNormal.Z)>0.5) return Fail(TEXT("Surface is not a suitable wall."));
    const FVector IntoWall=-Wall.ImpactNormal.GetSafeNormal2D();
    if (FVector::DotProduct(IntoWall,Forward)<0.5) return Fail(TEXT("Face the wall more directly."));
    auto GroundAt=[&](const FVector& XY,FHitResult& Hit)
    {
        return GetWorld()->LineTraceSingleByChannel(Hit,FVector(XY.X,XY.Y,Feet.Z+MaxHeight+Clearance),
            FVector(XY.X,XY.Y,Feet.Z-MaxLandingDrop),TraceChannel,Query) && !Hit.bStartPenetrating && Movement->IsWalkable(Hit);
    };
    // Find the actual top near the front edge; narrow vaultable walls need not support a full capsule.
    FHitResult Top;
    if (!GroundAt(Wall.ImpactPoint+IntoWall*Clearance,Top)) return Fail(TEXT("No walkable obstacle top."));
    const float Height=Top.ImpactPoint.Z-Feet.Z;
    if (Height<MinHeight || Height>MaxHeight) return Fail(TEXT("Obstacle height outside configured range."));
    FTraversalTarget Candidate;
    Candidate.Start=Start; Candidate.ObstacleHeight=Height;
    const FCollisionShape Shape=FCollisionShape::MakeCapsule(Radius,Half);
    const FCollisionResponseParams Responses(Capsule->GetCollisionResponseToChannels());
    auto ClearAt=[&](const FVector& Point)
    {
        return !GetWorld()->OverlapBlockingTestByChannel(Point,FQuat::Identity,Capsule->GetCollisionObjectType(),Shape,Query,Responses);
    };
    auto PathClear=[&](const FVector& Landing)
    {
        Candidate.Landing=Landing;
        const double LiftZ=FMath::Max3(Start.Z,Top.ImpactPoint.Z+Half+Clearance,Landing.Z);
        Candidate.Lift=FVector(Start.X,Start.Y,LiftZ);
        Candidate.Across=FVector(Landing.X,Landing.Y,LiftZ);
        if (!ClearAt(Landing)) return false;
        const FVector Points[]={Start,Candidate.Lift,Candidate.Across,Landing};
        for (int32 I=0; I<3; ++I)
        {
            if (Points[I].Equals(Points[I+1],0.01)) continue;
            FHitResult Hit;
            if (GetWorld()->SweepSingleByChannel(Hit,Points[I],Points[I+1],FQuat::Identity,Capsule->GetCollisionObjectType(),Shape,Query,Responses)) return false;
        }
        return true;
    };
    if (Height<=MaxVaultHeight)
    {
        // Bounded search beyond a low obstacle, with enough room for the whole capsule.
        const float Step=FMath::Max(10.f,Radius*0.5f);
        const int32 Samples=FMath::Min(128,FMath::CeilToInt(MaxVaultDepth/Step));
        for (int32 I=0; I<=Samples; ++I)
        {
            const float Depth=Radius+Clearance+MaxVaultDepth*float(I)/FMath::Max(1,Samples);
            FHitResult Ground;
            if (!GroundAt(Wall.ImpactPoint+IntoWall*Depth,Ground) || Ground.ImpactPoint.Z>Top.ImpactPoint.Z-10) continue;
            if (PathClear(Ground.ImpactPoint+FVector(0,0,Half+Clearance)))
            { Candidate.Action=ETraversalAction::Vault; Target=Candidate; return true; }
        }
    }
    FHitResult Platform;
    if (GroundAt(Wall.ImpactPoint+IntoWall*(Radius+Clearance),Platform)
        && FMath::Abs(Platform.ImpactPoint.Z-Top.ImpactPoint.Z)<=10
        && PathClear(Platform.ImpactPoint+FVector(0,0,Half+Clearance)))
    { Candidate.Action=ETraversalAction::Mantle; Target=Candidate; return true; }
    return Fail(TEXT("No free landing or capsule path (check ceiling, wall depth and landing space)."));
}

bool UTraversalComponent::TryTraverse()
{
    if (bActive || bTransitioning) { LastFailureReason=TEXT("Traversal already active or changing state."); return false; }
    ACharacter* C=Cast<ACharacter>(GetOwner());
    if (!IsValid(C) || C->GetNetMode()!=NM_Standalone)
    { LastFailureReason=TEXT("This version requires a standalone Character; multiplayer prediction is not implemented."); return false; }
    UCharacterMovementComponent* Movement=C->GetCharacterMovement();
    if (!Movement || !(Movement->IsMovingOnGround() || Movement->IsFalling() || Movement->MovementMode==MOVE_Flying))
    { LastFailureReason=TEXT("Unsupported movement mode."); return false; }
    if (!FMath::IsFinite(Duration) || Duration<0.1 || !FMath::IsFinite(MontagePlayRate) || MontagePlayRate<0.1)
    { LastFailureReason=TEXT("Invalid duration or montage play rate."); return false; }
    FTraversalTarget Target;
    if (!FindTraversalTarget(Target,LastFailureReason)) return false;
    UAnimMontage* Montage=Target.Action==ETraversalAction::Vault ? VaultMontage : MantleMontage;
    if (Montage && (!FMath::IsFinite(Montage->RateScale) || Montage->RateScale<=0))
    { LastFailureReason=TEXT("Traversal montage must have a positive rate scale."); return false; }
    UAnimInstance* Anim=C->GetMesh()?C->GetMesh()->GetAnimInstance():nullptr;
    if (Montage && (!Anim || Anim->IsAnyMontagePlaying()))
    { LastFailureReason=TEXT("Animation instance unavailable or another montage is already playing."); return false; }
    float PlayDuration=Duration;
    bTransitioning=true;
    if (Montage)
    {
        PlayDuration=Anim->Montage_Play(Montage,MontagePlayRate,EMontagePlayReturnType::Duration,0,false);
        if (PlayDuration<=0 || !FMath::IsFinite(PlayDuration)) { bTransitioning=false; LastFailureReason=TEXT("Could not play traversal montage."); return false; }
        Animation=Anim; ActiveMontage=Montage;
        SavedRootMotionMode=static_cast<uint8>(Anim->RootMotionMode.GetValue());
        Anim->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
        Anim->OnMontageEnded.AddDynamic(this,&UTraversalComponent::HandleMontageEnded);
    }
    Character=C; CurrentTarget=Target; ActiveDuration=PlayDuration; Elapsed=0; bActive=true;
    Movement->StopMovementImmediately(); Movement->ClearAccumulatedForces();
    Movement->SetMovementMode(MOVE_Flying);
    // Execute after CharacterMovement; it must not fight our swept placement.
    AddTickPrerequisiteComponent(Movement);
    if (AController* Controller=C->GetController())
    {
        LockedController=Controller; Controller->SetIgnoreMoveInput(true); bLockedMove=true;
        bLockedLook=bLockLook; if (bLockedLook) Controller->SetIgnoreLookInput(true);
    }
    SetComponentTickEnabled(true);
    ++ActionSerial;
    bTransitioning=false;
    OnTraversalStarted.Broadcast(CurrentTarget);
    return true;
}

void UTraversalComponent::TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Dt,TickType,Function);
    if (!bActive) return;
    ACharacter* C=Character.Get();
    if (!IsValid(C) || !C->GetCharacterMovement() || !C->GetCapsuleComponent()->IsQueryCollisionEnabled())
    { Finish(ETraversalResult::InvalidOwner); return; }
    if (C->GetCharacterMovement()->MovementMode!=MOVE_Flying || (Animation.IsValid() && C->GetMesh()->GetAnimInstance()!=Animation.Get()))
    { Finish(ETraversalResult::Cancelled); return; }
    if (!FMath::IsFinite(Dt) || Dt<0) { Finish(ETraversalResult::TimedOut); return; }
    const float Previous=Elapsed; Elapsed+=Dt;
    const uint32 Serial=ActionSerial;
    C->GetCharacterMovement()->StopMovementImmediately(); C->GetCharacterMovement()->ClearAccumulatedForces();
    const FVector Points[]={CurrentTarget.Start,CurrentTarget.Lift,CurrentTarget.Across,CurrentTarget.Landing};
    const float Boundaries[]={0.f,0.3f,0.75f,1.f};
    // Visit each crossed corner, including during a hitch; never cut diagonally through the obstacle.
    for (int32 I=0; I<3; ++I)
    {
        if (Previous/ActiveDuration>=Boundaries[I+1] || Elapsed/ActiveDuration<Boundaries[I]) continue;
        const float Alpha=FMath::Clamp((Elapsed/ActiveDuration-Boundaries[I])/(Boundaries[I+1]-Boundaries[I]),0.f,1.f);
        FHitResult Hit;
        C->SetActorLocation(FMath::Lerp(Points[I],Points[I+1],Alpha),true,&Hit,ETeleportType::None);
        // Hit/overlap callbacks may cancel this action or start another one.
        if (!bActive || ActionSerial!=Serial) return;
        if (Hit.bBlockingHit || Hit.bStartPenetrating) { Finish(ETraversalResult::Blocked); return; }
    }
    if (Elapsed>=ActiveDuration) Finish(ETraversalResult::Succeeded);
}

void UTraversalComponent::HandleMontageEnded(UAnimMontage* Montage,bool bInterrupted)
{
    if (bActive && Montage==ActiveMontage && bInterrupted) Finish(ETraversalResult::AnimationInterrupted);
}
void UTraversalComponent::CancelTraversal() { Finish(ETraversalResult::Cancelled); }
void UTraversalComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    Finish(ETraversalResult::Cancelled,false);
    Super::EndPlay(Reason);
}
void UTraversalComponent::Finish(ETraversalResult Result,bool bNotify)
{
    if (!bActive) return;
    bActive=false; bTransitioning=true; SetComponentTickEnabled(false);
    const FTraversalTarget FinishedTarget=CurrentTarget;
    if (UAnimInstance* Anim=Animation.Get())
    {
        Anim->OnMontageEnded.RemoveDynamic(this,&UTraversalComponent::HandleMontageEnded);
        Anim->SetRootMotionMode(static_cast<ERootMotionMode::Type>(SavedRootMotionMode));
        if (ActiveMontage && Anim->Montage_IsActive(ActiveMontage)) Anim->Montage_Stop(0.1f,ActiveMontage);
    }
    Animation.Reset(); ActiveMontage=nullptr;
    if (AController* Controller=LockedController.Get())
    {
        if (bLockedMove) Controller->SetIgnoreMoveInput(false);
        if (bLockedLook) Controller->SetIgnoreLookInput(false);
    }
    LockedController.Reset(); bLockedMove=false; bLockedLook=false;
    if (ACharacter* C=Character.Get())
    {
        if (UCharacterMovementComponent* Movement=C->GetCharacterMovement())
        {
            RemoveTickPrerequisiteComponent(Movement);
            // Never restore grapple's Flying. Let CharacterMovement find the actual floor.
            if (Movement->MovementMode==MOVE_Flying) { Movement->StopMovementImmediately(); Movement->ClearAccumulatedForces(); Movement->SetMovementMode(MOVE_Falling); }
        }
    }
    Character.Reset();
    bTransitioning=false;
    if (bNotify) OnTraversalFinished.Broadcast(Result,FinishedTarget);
}
