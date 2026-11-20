#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../BlockoutGeometry.h"
#include "Algo/Reverse.h"
#include "../ArchitecturalBlockout.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlockoutPolygonTest,"ProjectMegaSample.Blockout.Polygons",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBlockoutPolygonTest::RunTest(const FString& Parameters)
{
    TArray<FVector2D> LShape={{0,0},{600,0},{600,200},{200,200},{200,600},{0,600}};
    TArray<int32> Triangles;
    for (int32 Pass=0; Pass<2; ++Pass)
    {
        TestTrue(TEXT("Concave room triangulates in either winding"),BlockoutGeometry::Triangulate(LShape,Triangles));
        TestEqual(TEXT("N-2 triangles"),Triangles.Num(),12);
        double Area=0;
        for (int32 I=0; I<Triangles.Num(); I+=3)
        {
            const FVector2D A=LShape[Triangles[I+1]]-LShape[Triangles[I]], B=LShape[Triangles[I+2]]-LShape[Triangles[I]];
            const double TriangleArea=(A.X*B.Y-A.Y*B.X)*0.5;
            TestTrue(TEXT("Floor triangles face upward"),TriangleArea>0); Area+=TriangleArea;
        }
        TestTrue(TEXT("Floor covers exactly the concave polygon"),FMath::IsNearlyEqual(Area,200000.0,0.01));
        Algo::Reverse(LShape);
    }
    TestFalse(TEXT("Crossed polygon rejected"),BlockoutGeometry::Triangulate({{0,0},{100,100},{0,100},{100,0}},Triangles));
    TestTrue(TEXT("Failed triangulation leaves no triangles"),Triangles.IsEmpty());
    TestFalse(TEXT("Collinear room rejected"),BlockoutGeometry::Triangulate({{0,0},{100,0},{200,0}},Triangles));
    TestFalse(TEXT("Duplicate points rejected"),BlockoutGeometry::IsSimple({{0,0},{0,0},{100,0}},false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlockoutWallTest,"ProjectMegaSample.Blockout.WallOffsets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBlockoutWallTest::RunTest(const FString& Parameters)
{
    TArray<FVector2D> Left,Right;
    TestTrue(TEXT("Open wall offsets"),BlockoutGeometry::OffsetWalls({{0,0},{500,0}},false,10,Left,Right));
    if (Left.Num()==2 && Right.Num()==2)
    {
        TestTrue(TEXT("20 cm thickness"),FMath::IsNearlyEqual(FVector2D::Distance(Left[0],Right[0]),20.0));
        TestTrue(TEXT("Correct left boundary"),Left[1].Equals(FVector2D(500,10),0.01));
    }
    TestTrue(TEXT("Right-angle corner"),BlockoutGeometry::OffsetWalls({{0,0},{500,0},{500,500}},false,10,Left,Right));
    if (Left.Num()==3) TestTrue(TEXT("Miter intersection"),Left[1].Equals(FVector2D(490,10),0.01));
    TestFalse(TEXT("Reversal rejected"),BlockoutGeometry::OffsetWalls({{0,0},{500,0},{10,0}},false,10,Left,Right));
    TestFalse(TEXT("Too thick for room"),BlockoutGeometry::OffsetWalls({{0,0},{100,0},{100,100},{0,100}},true,60,Left,Right));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlockoutActorTest,"ProjectMegaSample.Blockout.ActorAndCollision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBlockoutActorTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Settings=UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(true);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Settings);
    if (!TestNotNull(TEXT("Test world"),World)) return false;
    AArchitecturalBlockout* Actor=World->SpawnActor<AArchitecturalBlockout>();
    if (!TestNotNull(TEXT("Spawn native tool"),Actor)) { World->DestroyWorld(false); return false; }
    Actor->RebuildGeometry();
    TestTrue(TEXT("Default room ready"),Actor->BuildStatus.StartsWith(TEXT("Ready:")));
    TestEqual(TEXT("Default walls and solid floor"),Actor->GeneratedTriangleCount,44);
    FHitResult Hit;
    const FCollisionQueryParams Params(SCENE_QUERY_STAT(BlockoutTest),true);
    TestTrue(TEXT("Floor collision"),World->LineTraceSingleByChannel(Hit,FVector(300,250,100),FVector(300,250,-100),ECC_Visibility,Params));
    TestTrue(TEXT("Floor top at zero"),FMath::IsNearlyZero(Hit.ImpactPoint.Z,0.1));
    TestTrue(TEXT("Wall collision"),World->LineTraceSingleByChannel(Hit,FVector(300,250,150),FVector(300,-100,150),ECC_Visibility,Params));
    Actor->bGenerateCollision=false; Actor->RebuildGeometry();
    TestFalse(TEXT("Collision can be disabled"),World->LineTraceSingleByChannel(Hit,FVector(300,250,100),FVector(300,250,-100),ECC_Visibility,Params));

    Actor->bClosedRoom=false;
    Actor->Outline->ClearSplinePoints(false);
    Actor->Outline->AddSplinePoint(FVector(0,0,0),ESplineCoordinateSpace::Local,false);
    Actor->Outline->AddSplinePoint(FVector(500,300,0),ESplineCoordinateSpace::Local,false);
    Actor->Outline->SetSplinePointType(0,ESplinePointType::CurveCustomTangent,false);
    Actor->Outline->SetSplinePointType(1,ESplinePointType::CurveCustomTangent,false);
    Actor->Outline->SetTangentAtSplinePoint(0,FVector(600,0,0),ESplineCoordinateSpace::Local,false);
    Actor->Outline->SetTangentAtSplinePoint(1,FVector(0,600,0),ESplineCoordinateSpace::Local,true);
    Actor->RebuildGeometry();
    TestTrue(TEXT("Curved open wall ready"),Actor->BuildStatus.StartsWith(TEXT("Ready:")));
    TestTrue(TEXT("Curved wall is subdivided"),Actor->GeneratedTriangleCount>12);
    TestEqual(TEXT("No stale floor section on open wall"),Actor->GeneratedMesh->GetNumSections(),1);

    Actor->Outline->SetSplinePointType(0,ESplinePointType::Linear,true);
    Actor->RebuildGeometry();
    TestEqual(TEXT("Single straight capped wall"),Actor->GeneratedTriangleCount,12);
    Actor->Outline->SetLocationAtSplinePoint(1,FVector::ZeroVector,ESplineCoordinateSpace::Local,true);
    Actor->RebuildGeometry();
    TestEqual(TEXT("Invalid input clears old geometry"),Actor->GeneratedMesh->GetNumSections(),0);
    TestEqual(TEXT("Invalid input clears count"),Actor->GeneratedTriangleCount,0);
    World->DestroyWorld(false);
    return true;
}
#endif
