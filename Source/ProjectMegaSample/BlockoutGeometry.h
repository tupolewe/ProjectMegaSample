#pragma once

#include "CoreMinimal.h"

// Pure geometry helpers, shared with the automation tests.
namespace BlockoutGeometry
{
    bool IsSimple(const TArray<FVector2D>& Points, bool bClosed);
    bool Triangulate(const TArray<FVector2D>& Points, TArray<int32>& Triangles);
    bool OffsetWalls(const TArray<FVector2D>& Points, bool bClosed, double HalfWidth,
        TArray<FVector2D>& Left, TArray<FVector2D>& Right);
}
