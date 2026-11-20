#include "ArchitecturalBlockout.h"
#include "BlockoutGeometry.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"

namespace
{
    struct FBlockoutMesh
    {
        TArray<FVector> Vertices, Normals;
        TArray<int32> Triangles;
        TArray<FVector2D> UV;
        TArray<FProcMeshTangent> Tangents;
        void Quad(const FVector& A, const FVector& B, const FVector& C, const FVector& D, float Tile, double U=0)
        {
            const int32 Base=Vertices.Num();
            const FVector Normal=FVector::CrossProduct(B-A,C-A).GetSafeNormal();
            Vertices.Append({A,B,C,D});
            for (int32 I=0; I<4; ++I) { Normals.Add(Normal); Tangents.Add(FProcMeshTangent((B-A).GetSafeNormal(),false)); }
            const double Width=FVector::Distance(A,B)/Tile, Height=FVector::Distance(A,D)/Tile;
            UV.Append({FVector2D(U,0),FVector2D(U+Width,0),FVector2D(U+Width,Height),FVector2D(U,Height)});
            Triangles.Append({Base,Base+1,Base+2,Base,Base+2,Base+3});
        }
        void Surface(const TArray<FVector2D>& P, const TArray<int32>& Indices, double Z, bool bUp, float Tile)
        {
            const int32 Base=Vertices.Num();
            for (const FVector2D& V : P)
            {
                Vertices.Add(FVector(V.X,V.Y,Z)); Normals.Add(bUp ? FVector::UpVector : -FVector::UpVector);
                UV.Add(V/Tile); Tangents.Add(FProcMeshTangent(1,0,0));
            }
            for (int32 I=0; I<Indices.Num(); I+=3)
                Triangles.Append({Base+Indices[I],Base+Indices[I+(bUp?1:2)],Base+Indices[I+(bUp?2:1)]});
        }
        void Submit(UProceduralMeshComponent* Mesh, int32 Section, bool bCollision)
        {
            Mesh->CreateMeshSection_LinearColor(Section,Vertices,Triangles,Normals,UV,TArray<FLinearColor>(),Tangents,bCollision);
        }
    };
    FVector AtZ(const FVector2D& P, double Z) { return FVector(P.X,P.Y,Z); }
}

AArchitecturalBlockout::AArchitecturalBlockout()
{
    PrimaryActorTick.bCanEverTick=false;
    Outline=CreateDefaultSubobject<USplineComponent>(TEXT("Outline"));
    SetRootComponent(Outline);
    Outline->SetMobility(EComponentMobility::Static);
    Outline->ClearSplinePoints(false);
    for (const FVector P : {FVector(0,0,0), FVector(600,0,0), FVector(600,500,0), FVector(0,500,0)})
        Outline->AddSplinePoint(P,ESplineCoordinateSpace::Local,false);
    for (int32 I=0; I<4; ++I) Outline->SetSplinePointType(I,ESplinePointType::Linear,false);
    Outline->SetClosedLoop(true,true);
#if WITH_EDITORONLY_DATA
    Outline->bInputSplinePointsToConstructionScript=true;
#endif
    GeneratedMesh=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));
    GeneratedMesh->SetupAttachment(Outline);
    GeneratedMesh->SetMobility(EComponentMobility::Static);
    GeneratedMesh->SetCollisionProfileName(TEXT("BlockAll"));
    GeneratedMesh->bUseComplexAsSimpleCollision=true;
    GeneratedMesh->bUseAsyncCooking=false;
}

void AArchitecturalBlockout::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RebuildGeometry();
}
#if WITH_EDITOR
void AArchitecturalBlockout::PostEditUndo()
{
    Super::PostEditUndo();
    RebuildGeometry();
}
#endif

void AArchitecturalBlockout::RebuildGeometry()
{
    if (!Outline || !GeneratedMesh) return;
    GeneratedMesh->ClearAllMeshSections();
    GeneratedTriangleCount=0;
    auto Fail=[this](const TCHAR* Message) { BuildStatus=Message; };
    if (!FMath::IsFinite(WallHeight) || !FMath::IsFinite(WallThickness) || !FMath::IsFinite(CurveSampleSpacing)
        || !FMath::IsFinite(FloorThickness) || !FMath::IsFinite(TextureTileSize)
        || WallHeight<1 || WallThickness<1 || CurveSampleSpacing<5 || FloorThickness<1 || TextureTileSize<1)
    { Fail(TEXT("Invalid dimensions: use positive values (curve spacing at least 5 cm).")); return; }
    Outline->SetClosedLoop(bClosedRoom,true);
    const int32 PointCount=Outline->GetNumberOfSplinePoints();
    if (PointCount<(bClosedRoom?3:2)) { Fail(TEXT("Add more outline points: room needs 3, open wall needs 2.")); return; }
    const int32 Segments=bClosedRoom?PointCount:PointCount-1;
    TArray<FVector2D> P;
    bool bProjected=false;
    for (int32 Segment=0; Segment<Segments; ++Segment)
    {
        const auto Type=Outline->GetSplinePointType(Segment);
        if (Type==ESplinePointType::Constant) { Fail(TEXT("Constant spline segments are unsupported. Choose Linear or Curve.")); return; }
        const double Start=Outline->GetDistanceAlongSplineAtSplinePoint(Segment);
        const double End=Segment+1<PointCount ? Outline->GetDistanceAlongSplineAtSplinePoint(Segment+1) : Outline->GetSplineLength();
        if (!FMath::IsFinite(Start) || !FMath::IsFinite(End) || End-Start<0.1) { Fail(TEXT("Outline has coincident or invalid points.")); return; }
        const double DesiredSteps=Type==ESplinePointType::Linear ? 1 : FMath::CeilToDouble((End-Start)/CurveSampleSpacing);
        if (DesiredSteps>1024 || P.Num()+DesiredSteps>1024) { Fail(TEXT("Outline exceeds 1024 samples. Increase curve sample spacing.")); return; }
        const int32 Steps=FMath::Max(1,static_cast<int32>(DesiredSteps));
        for (int32 S=0; S<Steps; ++S)
        {
            const FVector V=Outline->GetLocationAtDistanceAlongSpline(FMath::Lerp(Start,End,double(S)/Steps),ESplineCoordinateSpace::Local);
            if (V.ContainsNaN()) { Fail(TEXT("Outline contains invalid coordinates.")); return; }
            bProjected |= FMath::Abs(V.Z)>0.1;
            P.Add(FVector2D(V.X,V.Y));
        }
    }
    if (!bClosedRoom)
    {
        const FVector V=Outline->GetLocationAtSplinePoint(PointCount-1,ESplineCoordinateSpace::Local);
        if (V.ContainsNaN()) { Fail(TEXT("Outline contains invalid coordinates.")); return; }
        bProjected |= FMath::Abs(V.Z)>0.1;
        P.Add(FVector2D(V.X,V.Y));
    }
    if (!BlockoutGeometry::IsSimple(P,bClosedRoom)) { Fail(TEXT("Outline crosses or touches itself, or contains duplicate points.")); return; }
    TArray<FVector2D> L,R;
    if (!BlockoutGeometry::OffsetWalls(P,bClosedRoom,WallThickness*0.5,L,R))
    { Fail(TEXT("Wall thickness does not fit this outline. Reduce thickness or soften sharp corners.")); return; }
    TArray<int32> FloorTriangles;
    if (bClosedRoom && !BlockoutGeometry::Triangulate(P,FloorTriangles))
    { Fail(TEXT("Room outline has zero area or cannot be triangulated.")); return; }

    FBlockoutMesh Walls, Floor;
    double Distance=0;
    const int32 EdgeCount=bClosedRoom?P.Num():P.Num()-1;
    for (int32 I=0; I<EdgeCount; ++I)
    {
        const int32 J=(I+1)%P.Num();
        const FVector A=AtZ(L[I],0), B=AtZ(L[J],0), C=AtZ(R[I],0), D=AtZ(R[J],0), H(0,0,WallHeight);
        Walls.Quad(B,A,A+H,B+H,TextureTileSize,Distance/TextureTileSize);
        Walls.Quad(C,D,D+H,C+H,TextureTileSize,Distance/TextureTileSize);
        Walls.Quad(A+H,C+H,D+H,B+H,TextureTileSize);
        Walls.Quad(C,A,B,D,TextureTileSize);
        Distance+=FVector2D::Distance(P[I],P[J]);
    }
    if (!bClosedRoom)
    {
        const FVector H(0,0,WallHeight);
        Walls.Quad(AtZ(L[0],0),AtZ(R[0],0),AtZ(R[0],0)+H,AtZ(L[0],0)+H,TextureTileSize);
        Walls.Quad(AtZ(R.Last(),0),AtZ(L.Last(),0),AtZ(L.Last(),0)+H,AtZ(R.Last(),0)+H,TextureTileSize);
    }
    if (bClosedRoom && bCreateFloor)
    {
        Floor.Surface(P,FloorTriangles,0,true,TextureTileSize);
        Floor.Surface(P,FloorTriangles,-FloorThickness,false,TextureTileSize);
        double Area=0;
        for (int32 I=0; I<P.Num(); ++I) { const auto& A=P[I]; const auto& B=P[(I+1)%P.Num()]; Area+=A.X*B.Y-A.Y*B.X; }
        for (int32 I=0; I<P.Num(); ++I)
        {
            const int32 J=(I+1)%P.Num();
            const FVector2D A=Area>0?P[I]:P[J], B=Area>0?P[J]:P[I];
            Floor.Quad(AtZ(A,-FloorThickness),AtZ(B,-FloorThickness),AtZ(B,0),AtZ(A,0),TextureTileSize);
        }
    }
    GeneratedMesh->SetCollisionEnabled(bGenerateCollision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
    Walls.Submit(GeneratedMesh,0,bGenerateCollision);
    if (!Floor.Vertices.IsEmpty()) Floor.Submit(GeneratedMesh,1,bGenerateCollision);
    GeneratedMesh->SetMaterial(0,WallMaterial);
    GeneratedMesh->SetMaterial(1,FloorMaterial);
    GeneratedTriangleCount=(Walls.Triangles.Num()+Floor.Triangles.Num())/3;
    BuildStatus=FString::Printf(TEXT("Ready: %d outline samples, %d triangles.%s%s"),P.Num(),GeneratedTriangleCount,
        bProjected?TEXT(" Point Z projected onto local XY plane."):TEXT(""),
        !bClosedRoom && bCreateFloor?TEXT(" Floor skipped for open wall."):TEXT(""));
}
