#include "BlockoutGeometry.h"

namespace BlockoutGeometry
{
    static double Cross(const FVector2D& A, const FVector2D& B) { return A.X * B.Y - A.Y * B.X; }
    static bool OnSegment(const FVector2D& A, const FVector2D& B, const FVector2D& P)
    {
        return FMath::Abs(Cross(B-A, P-A)) < 0.001 && FVector2D::DotProduct(P-A, P-B) <= 0.001;
    }
    static bool Intersects(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D)
    {
        const double X = Cross(B-A, C-A), Y = Cross(B-A, D-A);
        const double Z = Cross(D-C, A-C), W = Cross(D-C, B-C);
        return (X*Y < 0 && Z*W < 0) || OnSegment(A,B,C) || OnSegment(A,B,D) || OnSegment(C,D,A) || OnSegment(C,D,B);
    }
    bool IsSimple(const TArray<FVector2D>& P, bool bClosed)
    {
        const int32 N = P.Num(), Edges = bClosed ? N : N-1;
        if (N < (bClosed ? 3 : 2)) return false;
        for (int32 I=0; I<Edges; ++I)
        {
            if (FVector2D::Distance(P[I], P[(I+1)%N]) < 0.1) return false;
            for (int32 J=I+1; J<Edges; ++J)
            {
                if (J==I+1 || (bClosed && I==0 && J==Edges-1)) continue;
                if (Intersects(P[I], P[(I+1)%N], P[J], P[(J+1)%N])) return false;
            }
        }
        return true;
    }
    bool Triangulate(const TArray<FVector2D>& P, TArray<int32>& Out)
    {
        Out.Reset();
        if (!IsSimple(P,true)) return false;
        double Area=0;
        for (int32 I=0; I<P.Num(); ++I) Area += Cross(P[I],P[(I+1)%P.Num()]);
        if (FMath::Abs(Area)<0.01) return false;
        TArray<int32> Remaining;
        for (int32 I=0; I<P.Num(); ++I) Remaining.Add(Area>0 ? I : P.Num()-1-I);
        while (Remaining.Num()>3)
        {
            bool bFound=false;
            for (int32 I=0; I<Remaining.Num(); ++I)
            {
                const int32 A=Remaining[(I+Remaining.Num()-1)%Remaining.Num()], B=Remaining[I], C=Remaining[(I+1)%Remaining.Num()];
                if (Cross(P[B]-P[A],P[C]-P[B]) <= 0.0001) continue;
                bool bContains=false;
                for (int32 V : Remaining)
                {
                    if (V==A || V==B || V==C) continue;
                    if (Cross(P[B]-P[A],P[V]-P[A]) >= -0.0001 && Cross(P[C]-P[B],P[V]-P[B]) >= -0.0001 && Cross(P[A]-P[C],P[V]-P[C]) >= -0.0001)
                    { bContains=true; break; }
                }
                if (bContains) continue;
                Out.Append({A,B,C}); Remaining.RemoveAt(I); bFound=true; break;
            }
            if (!bFound) { Out.Reset(); return false; }
        }
        if (Cross(P[Remaining[1]]-P[Remaining[0]],P[Remaining[2]]-P[Remaining[0]]) <= 0.0001) { Out.Reset(); return false; }
        Out.Append(Remaining);
        return true;
    }
    bool OffsetWalls(const TArray<FVector2D>& P, bool bClosed, double HalfWidth, TArray<FVector2D>& L, TArray<FVector2D>& R)
    {
        L.Reset(); R.Reset();
        if (!IsSimple(P,bClosed) || HalfWidth<=0) return false;
        for (int32 I=0; I<P.Num(); ++I)
        {
            const FVector2D Prev = (!bClosed && I==0) ? (P[1]-P[0]).GetSafeNormal() : (P[I]-P[(I+P.Num()-1)%P.Num()]).GetSafeNormal();
            const FVector2D Next = (!bClosed && I==P.Num()-1) ? Prev : (P[(I+1)%P.Num()]-P[I]).GetSafeNormal();
            const FVector2D N1(-Prev.Y,Prev.X), N2(-Next.Y,Next.X);
            const FVector2D M=(N1+N2).GetSafeNormal();
            const double Denom=FVector2D::DotProduct(M,N2);
            // Reject near-reversals instead of creating very long corner spikes.
            if (Denom<0.25) return false;
            const FVector2D Offset=M*(HalfWidth/Denom);
            L.Add(P[I]+Offset); R.Add(P[I]-Offset);
        }
        // Reject thicknesses that fold a wall boundary back over itself.
        if (!IsSimple(L,bClosed) || !IsSimple(R,bClosed)) return false;
        const int32 Edges=bClosed ? P.Num() : P.Num()-1;
        for (int32 I=0; I<Edges; ++I)
        {
            const int32 J=(I+1)%P.Num();
            if (FVector2D::DotProduct(L[J]-L[I],P[J]-P[I])<=0 || FVector2D::DotProduct(R[J]-R[I],P[J]-P[I])<=0) return false;
            for (int32 K=0; K<Edges; ++K)
                if (Intersects(L[I],L[J],R[K],R[(K+1)%P.Num()])) return false;
        }
        return true;
    }
}
