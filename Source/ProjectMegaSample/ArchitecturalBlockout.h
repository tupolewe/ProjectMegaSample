#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArchitecturalBlockout.generated.h"

class USplineComponent;
class UProceduralMeshComponent;
class UMaterialInterface;

/** Editable planar wall outline. Distances are centimetres in actor-local space. */
UCLASS(BlueprintType, meta=(DisplayName="Architectural Blockout"))
class PROJECTMEGASAMPLE_API AArchitecturalBlockout : public AActor
{
    GENERATED_BODY()
public:
    AArchitecturalBlockout();
    virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
    virtual void PostEditUndo() override;
#endif

    /** Select this component, then edit its points in the viewport. Point Z is projected onto the actor's XY plane. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Blockout")
    TObjectPtr<USplineComponent> Outline;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Blockout")
    TObjectPtr<UProceduralMeshComponent> GeneratedMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blockout|Shape")
    bool bClosedRoom = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blockout|Shape", meta=(ClampMin="1", Units="cm"))
    float WallHeight = 300.f;

    /** Walls are centred on the outline. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blockout|Shape", meta=(ClampMin="1", Units="cm"))
    float WallThickness = 20.f;

    /** Maximum spacing of samples on curved segments. Straight segments need only their endpoints. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blockout|Shape", meta=(ClampMin="5", Units="cm"))
    float CurveSampleSpacing = 40.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blockout|Floor")
    bool bCreateFloor = true;

    /** Floor top is at local Z=0; thickness extends downwards. Closed rooms only. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blockout|Floor", meta=(ClampMin="1", Units="cm"))
    float FloorThickness = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blockout|Materials")
    TObjectPtr<UMaterialInterface> WallMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blockout|Materials")
    TObjectPtr<UMaterialInterface> FloorMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blockout|Materials", meta=(ClampMin="1", Units="cm"))
    float TextureTileSize = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Blockout|Collision")
    bool bGenerateCollision = true;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Blockout|Diagnostics")
    FString BuildStatus;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Blockout|Diagnostics")
    int32 GeneratedTriangleCount = 0;

    UFUNCTION(CallInEditor, BlueprintCallable, Category="Blockout")
    void RebuildGeometry();
};
