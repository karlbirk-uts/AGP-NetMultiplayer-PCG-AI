#include "PCGTerrainLandscape.h"
#include "KismetProceduralMeshLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

APCGTerrainLandscape::APCGTerrainLandscape()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
    SetRootComponent(ProceduralMesh);
    
    bReplicates = true;
}

void APCGTerrainLandscape::BeginPlay()
{
    Super::BeginPlay();
    
}

void APCGTerrainLandscape::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bShouldRegenerate)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Regeneration requested. Clearing and rebuilding landscape."));

        ClearLandscape();
        GenerateLandscape();

        bShouldRegenerate = false;

        
    }
}

void APCGTerrainLandscape::ClearLandscape()
{
    TerrainHeights.Empty();   // clear stored heights

    if (ProceduralMesh)
    {
        ProceduralMesh->ClearAllMeshSections();
    }

    UKismetSystemLibrary::FlushPersistentDebugLines(GetWorld());
}

/*
bool APCGTerrainLandscape::ShouldTickIfViewportsOnly() const
{
    return false;
}
*/

void APCGTerrainLandscape::GenerateLandscape()
{
    float PerlinA;
    float PerlinB;

    // LEFT TERRAIN: remove RIGHT fence (maze on its right)
    GenerateTerrain(-4500.0f, 0, PerlinA);
    GenerateFence(-4500.0f, 1, PerlinA, false, true);

    // RIGHT TERRAIN: remove LEFT fence (maze on its left)
    GenerateTerrain(4500.0f, 2, PerlinB);
    GenerateFence(4500.0f, 3, PerlinB, true, false);
}


// ------------------------- TERRAIN GENERATION -------------------------

void APCGTerrainLandscape::GenerateTerrain(float XOffset, int32 SectionIndex, float& OutPerlinOffset)
{
    
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector2D> UVs;

    TerrainHeights.Empty();   // reset height storage
    TerrainHeights.Reserve(Width * Height);

    OutPerlinOffset = RandomStream.FRandRange(-1000000.0f, 1000000.0f);
    //check, del later
    UE_LOG(
    LogTemp,
    Warning,
    TEXT("Perlin Offset = %f"),
    OutPerlinOffset);

    for (int32 Y = 0; Y < Height; Y++)
    {
        for (int32 X = 0; X < Width; X++)
        {
            float TerrainZ;

            bool bFlattenLeftEdge  = (SectionIndex == 2); // right terrain
            bool bFlattenRightEdge = (SectionIndex == 0); // left terrain

            if (bFlattenLeftEdge && X == 0)
            {
                TerrainZ = 0.0f;
            }
            else if (bFlattenRightEdge && X == Width - 1)
            {
                TerrainZ = 0.0f;
            }
            else
            {
                TerrainZ = FMath::PerlinNoise2D(
                    FVector2D(
                        (X * VertexSpacing * PerlinRoughness) + OutPerlinOffset,
                        (Y * VertexSpacing * PerlinRoughness) + OutPerlinOffset
                    )
                ) * PerlinScale;
            }

            TerrainHeights.Add(TerrainZ);   // store height

            FVector Pos(XOffset + X * VertexSpacing, Y * VertexSpacing, TerrainZ);
            Vertices.Add(Pos);

            UVs.Add(FVector2D(
                static_cast<float>(X) / (Width - 1),
                static_cast<float>(Y) / (Height - 1)
            ));
        }
    }

    for (int32 Y = 0; Y < Height - 1; Y++)
    {
        for (int32 X = 0; X < Width - 1; X++)
        {
            int32 BL = Y * Width + X;
            int32 BR = BL + 1;
            int32 TL = (Y + 1) * Width + X;
            int32 TR = TL + 1;

            Triangles.Add(BL); Triangles.Add(TL); Triangles.Add(BR);
            Triangles.Add(BR); Triangles.Add(TL); Triangles.Add(TR);
        }
    }

    TArray<FVector> Normals;
    TArray<FProcMeshTangent> Tangents;

    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
        Vertices, Triangles, UVs, Normals, Tangents);

    ProceduralMesh->CreateMeshSection(
        SectionIndex,
        Vertices,
        Triangles,
        Normals,
        UVs,
        TArray<FColor>(),
        Tangents,
        true
    );
    
    if (TerrainMaterial)
    {
        UE_LOG(LogTemp, Warning,
        TEXT("Applying terrain material: %s"),
        *TerrainMaterial->GetName());
        ProceduralMesh->SetMaterial(SectionIndex, TerrainMaterial);
    } else
    {
        UE_LOG(LogTemp, Warning, TEXT("terrain material is null"))
    }
    
}


// ------------------------- FENCE GENERATION -------------------------

void APCGTerrainLandscape::GenerateFence(float XOffset, int32 SectionIndex, float PerlinOffset, bool bRemoveLeft, bool bRemoveRight)
{

    // build full perimeter (each side gets added)
    TArray<int32> Perimeter;

    for (int32 X = 0; X < Width; X++)
        Perimeter.Add(X);

    for (int32 Y = 1; Y < Height - 1; Y++)
        Perimeter.Add(Y * Width + (Width - 1));

    for (int32 X = Width - 1; X >= 0; X--)
        Perimeter.Add((Height - 1) * Width + X);

    for (int32 Y = Height - 2; Y > 0; Y--)
        Perimeter.Add(Y * Width);

    // build vertices (bottom row)
    TArray<FVector> FenceVerts;
    TArray<FVector2D> FenceUVs;

    for (int32 i = 0; i < Perimeter.Num(); i++)
    {
        int32 Index = Perimeter[i];
        int32 GridY = Index / Width;
        int32 GridX = Index % Width;

        float TerrainZ = TerrainHeights[Index];   // use terrain height

        FenceVerts.Add(FVector(
            XOffset + GridX * VertexSpacing,
            GridY * VertexSpacing,
            TerrainZ
        ));
    }

    int32 Count = FenceVerts.Num();
    if (Count < 2)
        return;

    // tiled UVs for bottom row
    for (int32 i = 0; i < Count; i++)
    {
        float U = (i % 2 == 0) ? 0.0f : 1.0f;
        FenceUVs.Add(FVector2D(U, 1.0f));
    }

    // add raised vertices (top row)
    int32 TopStart = FenceVerts.Num();

    for (int32 i = 0; i < Count; i++)
    {
        FVector Raised = FenceVerts[i];
        Raised.Z += 250.0f;

        FenceVerts.Add(Raised);

        float U = (i % 2 == 0) ? 0.0f : 1.0f;
        FenceUVs.Add(FVector2D(U, 0.0f));
    }

    // helper to detect removed-side vertices
    auto IsRemoved = [&](int32 VertexIndex)
    {
        int32 BaseIndex = (VertexIndex >= TopStart)
            ? VertexIndex - TopStart
            : VertexIndex;

        int32 PerimIndex = Perimeter[BaseIndex];
        int32 GridY = PerimIndex / Width;
        int32 GridX = PerimIndex % Width;

        bool bIsCorner =
            (GridX == 0 || GridX == Width - 1) &&
            (GridY == 0 || GridY == Height - 1);

        if (bIsCorner)
            return false;

        bool bIsLeftEdge  = (GridX == 0 && GridY > 0 && GridY < Height - 1);
        bool bIsRightEdge = (GridX == Width - 1 && GridY > 0 && GridY < Height - 1);

        if (bRemoveLeft && bIsLeftEdge)  return true;
        if (bRemoveRight && bIsRightEdge) return true;

        return false;
    };

    // build triangles (with skipping)
    TArray<int32> FenceTris;

    for (int32 i = 0; i < Count - 1; i++)
    {
        int32 BL = i;
        int32 BR = i + 1;
        int32 TL = TopStart + i;
        int32 TR = TopStart + i + 1;

        if (IsRemoved(BL) || IsRemoved(BR) || IsRemoved(TL) || IsRemoved(TR))
            continue;

        FenceTris.Add(BL); FenceTris.Add(TL); FenceTris.Add(BR);
        FenceTris.Add(BR); FenceTris.Add(TL); FenceTris.Add(TR);
    }

    // wrap-around quad
    {
        int32 BL = Count - 1;
        int32 BR = 0;
        int32 TL = TopStart + (Count - 1);
        int32 TR = TopStart + 0;

        if (!IsRemoved(BL) && !IsRemoved(BR) && !IsRemoved(TL) && !IsRemoved(TR))
        {
            FenceTris.Add(BL); FenceTris.Add(TL); FenceTris.Add(BR);
            FenceTris.Add(BR); FenceTris.Add(TL); FenceTris.Add(TR);
        }
    }

    // build mesh
    TArray<FVector> Normals;
    TArray<FProcMeshTangent> Tangents;

    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
        FenceVerts, FenceTris, FenceUVs, Normals, Tangents);
    
        
    ProceduralMesh->CreateMeshSection(
        SectionIndex,
        FenceVerts,
        FenceTris,
        Normals,
        FenceUVs,
        TArray<FColor>(),
        Tangents,
        true
    );

    if (FenceMaterial)
        ProceduralMesh->SetMaterial(SectionIndex, FenceMaterial);
}

void APCGTerrainLandscape::MulticastRegenerateTerrain_Implementation(
    int32 Seed,
    float NewPerlinScale,
    float NewPerlinRoughness)
{
    PerlinScale = NewPerlinScale;
    PerlinRoughness = NewPerlinRoughness;

    GenerateTerrainFromSeed(Seed);
}

void APCGTerrainLandscape::GenerateTerrainFromSeed(
    int32 Seed)
{
    UE_LOG(LogTemp, Warning,
        TEXT("Generating Terrain From Seed: %d"),
        Seed);

    CurrentSeed = Seed;

    RandomStream.Initialize(Seed);

    ClearLandscape();
    GenerateLandscape();
}