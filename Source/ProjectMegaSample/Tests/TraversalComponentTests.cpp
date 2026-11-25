#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../TraversalComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/BoxComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTraversalLifecycleTest,"ProjectMegaSample.Traversal.DetectionAndLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTraversalLifecycleTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Settings=UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(true);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Settings);
    if (!TestNotNull(TEXT("World"),World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto Box=[&](FVector Position,FVector Extent)
    {
        AActor* A=World->SpawnActor<AActor>();
        UBoxComponent* B=NewObject<UBoxComponent>(A); A->SetRootComponent(B);
        B->SetBoxExtent(Extent); B->SetCollisionProfileName(TEXT("BlockAll")); B->RegisterComponent();
        A->SetActorLocation(Position); return A;
    };
    Box(FVector(0,0,-20),FVector(2000,2000,20));
    AActor* Obstacle=Box(FVector(150,0,40),FVector(50,200,40));
    FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ACharacter* C=World->SpawnActor<ACharacter>(FVector(0,0,100),FRotator::ZeroRotator,Spawn);
    APlayerController* Controller=World->SpawnActor<APlayerController>(); Controller->Possess(C);
    C->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    UTraversalComponent* Tool=NewObject<UTraversalComponent>(C); Tool->RegisterComponent();
    FTraversalTarget Target; FString Reason;
    TestTrue(TEXT("Detect low obstacle"),Tool->FindTraversalTarget(Target,Reason));
    TestTrue(TEXT("Low narrow obstacle selects vault"),Target.Action==ETraversalAction::Vault);
    Controller->SetIgnoreLookInput(true);
    TestTrue(TEXT("Start"),Tool->TryTraverse());
    TestFalse(TEXT("Reject overlapping traversal"),Tool->TryTraverse());
    TestTrue(TEXT("Input locked during traversal"),Controller->IsMoveInputIgnored());
    Tool->TickComponent(1.f,LEVELTICK_All,nullptr);
    TestFalse(TEXT("Action completed even across multiple corners in one tick"),Tool->IsTraversing());
    TestTrue(TEXT("Reached landing"),C->GetActorLocation().Equals(Target.Landing,0.1));
    TestFalse(TEXT("Own movement lock released"),Controller->IsMoveInputIgnored());
    TestTrue(TEXT("Preexisting look lock preserved"),Controller->IsLookInputIgnored());
    Controller->SetIgnoreLookInput(false);
    TestFalse(TEXT("No leaked look lock"),Controller->IsLookInputIgnored());
    TestTrue(TEXT("Flying is released"),C->GetCharacterMovement()->IsFalling());

    C->SetActorLocation(FVector(0,0,100));
    TestTrue(TEXT("Can restart"),Tool->TryTraverse()); Tool->CancelTraversal(); Tool->CancelTraversal();
    TestFalse(TEXT("Cancel is idempotent"),Tool->IsTraversing());
    TestFalse(TEXT("Cancel releases movement lock"),Controller->IsMoveInputIgnored());

    TestTrue(TEXT("Restart for dynamic obstruction"),Tool->TryTraverse());
    AActor* Ceiling=Box(FVector(0,0,210),FVector(80,80,10));
    Tool->TickComponent(1.f,LEVELTICK_All,nullptr);
    TestFalse(TEXT("New obstacle aborts movement"),Tool->IsTraversing());
    TestFalse(TEXT("Blocked move does not reach destination"),C->GetActorLocation().Equals(Target.Landing,0.1));
    Ceiling->Destroy();

    C->SetActorLocation(FVector(0,0,100));
    Obstacle->SetActorLocation(FVector(150,0,90));
    Cast<UBoxComponent>(Obstacle->GetRootComponent())->SetBoxExtent(FVector(50,200,90));
    TestTrue(TEXT("Detect tall platform"),Tool->FindTraversalTarget(Target,Reason));
    TestTrue(TEXT("Tall platform selects mantle"),Target.Action==ETraversalAction::Mantle);
    TestTrue(TEXT("Mantle can start"),Tool->TryTraverse()); Tool->CancelTraversal();
    Obstacle->Destroy();
    TestFalse(TEXT("Empty space fails cleanly"),Tool->TryTraverse());
    TestFalse(TEXT("Failed attempt acquires no locks"),Controller->IsMoveInputIgnored());
    TestTrue(TEXT("Failure provides explanation"),!Tool->LastFailureReason.IsEmpty());
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
