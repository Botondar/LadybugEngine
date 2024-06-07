//
// Entity
//
typedef flags32 entity_flags;
enum entity_flag_bits : entity_flags
{
    EntityFlag_None         = 0,

    EntityFlag_Mesh         = (1u << 0),
    EntityFlag_Skin         = (1u << 1),
    EntityFlag_LightSource  = (1u << 2),
    EntityFlag_Terrain      = (1u << 3),
};

struct entity_piece
{
    u32 MeshID;
    v3 OffsetP;
};

struct entity
{
    entity_flags Flags;
    m4 Transform;

    // EntityFlag_Mesh
    static constexpr u32 MaxPieceCount = 256;
    u32 PieceCount;
    entity_piece Pieces[MaxPieceCount];

    // EntityFlag_Skin
    u32 SkinID;
    u32 CurrentAnimationID;
    b32 DoAnimation;
    f32 AnimationCounter;

    // EntityFlag_LightSource
    v3 LightEmission;
};

struct entity_id
{
    u32 Value;
};
inline b32 IsValid(entity_id ID) { return (ID.Value != 0); }

//
// Particle
//
struct particle
{
    v3 P;
    v3 dP;
    v3 ddP;
    v3 Color;
    v3 dColor;
    u32 TextureIndex;
};

enum particle_system_type
{
    ParticleSystem_Undefined = 0,
    ParticleSystem_Magic,
    ParticleSystem_Fire,

    ParticleSystem_COUNT,
};

struct particle_system
{
    entity_id ParentID;
    particle_system_type Type;

    v3 EmitterOffset;
    billboard_mode Mode;
    v2 ParticleHalfExtent;

    b32 CullOutOfBoundsParticles;
    f32 Counter;
    f32 EmissionRate;

    mmbox Bounds;
    u32 NextParticle;

    static constexpr u32 MaxParticleCount = 512;
    u32 ParticleCount;
    particle Particles[MaxParticleCount];
};

//
// Camera
//
struct camera
{
    v3 P;
    v3 dP;
    v3 dPTarget;
    f32 Yaw;
    f32 Pitch;
    f32 NearZ;
    f32 FarZ;
    f32 FieldOfView;
};

lbfn m4 GetLocalTransform(const camera* Camera);
lbfn m4 GetTransform(const camera* Camera);

//
// Terrain
//
struct noise2
{
    u32 Seed;
};

lbfn f32 SampleNoise(noise2* Noise, v2 P);

struct height_field
{
    u32 TexelCountX;
    u32 TexelCountY;
    f32 TexelsPerMeter;
    f32* HeightData;
};

inline f32 SampleHeight(height_field* Field, v2 Height);

//
// World
//
struct game_world
{
    memory_arena* Arena;
    b32 IsLoaded;
    f32 LightProxyScale; // NOTE(boti): Sphere scale for editor selection and rendering

    camera Camera;
    v3 SunL;
    v3 SunV;

    entropy32 EffectEntropy; // NOTE(boti): for visual effects only

    entropy32 GeneratorEntropy;
    noise2 TerrainNoise;
    u32 TerrainMaterialID;
    height_field HeightField;
    v2 TerrainP;

    entity_id IKControlID; // NOTE(boti): Dummy entity for IK testing

    static constexpr u32 MaxEntityCount = (1u << 18);
    entity_id NextEntityID;
    entity Entities[MaxEntityCount];

    static constexpr u32 MaxParticleSystemCount = 512u;
    u32 ParticleSystemCount;
    particle_system ParticleSystems[MaxParticleSystemCount];

    // NOTE(boti): Ad-hoc lights are just for testing the light binning
    f32 AdHocLightUpdateRate;
    f32 AdHocLightCounter;
    mmbox AdHocLightBounds;
    static constexpr u32 MaxAdHocLightCount = 64u;
    u32 AdHocLightCount;
    v3 AdHocLightdPs[MaxAdHocLightCount];
    light AdHocLights[MaxAdHocLightCount];
};

inline entity* MakeEntity(game_world* World, entity_id* ID)
{
    u32 IDValue = 0;
    entity* Result = nullptr;
    if (World->NextEntityID.Value < World->MaxEntityCount)
    {
        IDValue = World->NextEntityID.Value++;
        Result = World->Entities + IDValue;
        memset(Result, 0, sizeof(*Result));
    }

    if (ID)
    {
        ID->Value = IDValue;
    }

    return(Result);
}

inline entity* GetEntity(game_world* World, entity_id ID)
{
    Assert(ID.Value < World->NextEntityID.Value);
    entity* Result = World->Entities + ID.Value;
    return(Result);
}

struct entity_iterator
{
    entity_id ID;
    entity* Entity;

    entity_id OnePastLastID;
};

inline b32 IsValid(entity_iterator It) { return It.ID.Value < It.OnePastLastID.Value; }
inline entity_iterator Next(entity_iterator It) 
{
    ++It.ID.Value;
    ++It.Entity;
    return(It);
}

inline entity_iterator MakeEntityIterator(game_world* World)
{
    entity_iterator Result =
    {
        .ID = { 1 },
        .Entity = World->Entities + 1,
        .OnePastLastID = World->NextEntityID,
    };
    return(Result);
}

lbfn f32 SampleTerrainHeight(game_world* World, v2 P);
// NOTE(boti): Returns tangent plane
lbfn v4 SampleTerrain(game_world* World, v2 P);

lbfn u32 
MakeParticleSystem(game_world* World, particle_system_type Type, entity_id ParentID, 
                   v3 EmitterOffset, mmbox Bounds);

lbfn void UpdateAndRenderWorld(
    game_world* World, 
    struct assets* Assets, 
    render_frame* Frame, 
    game_io* IO, 
    memory_arena* Scratch, 
    debug_flags DebugFlags);

internal mesh_data 
GenerateTerrainChunk(height_field* Field, memory_arena* Arena);

enum debug_scene_type
{
    DebugScene_None = 0,
    DebugScene_Sponza,
    DebugScene_Terrain,
    DebugScene_TransmissionTest,
};

typedef flags32 debug_scene_flags;
enum debug_scene_flag_bits : debug_scene_flags
{
    DebugSceneFlag_None                 = 0,
    DebugSceneFlag_AnimatedFox          = (1u << 0),
    DebugSceneFlag_TransparentDragon    = (1u << 1),
    DebugSceneFlag_SponzaParticles      = (1u << 2),
    DebugSceneFlag_SponzaAdHocLights    = (1u << 3),
};

internal void 
DEBUGInitializeWorld(
    game_world* World, 
    assets* Assets, 
    render_frame* Frame, 
    memory_arena* Scratch,
    debug_scene_type Type,
    debug_scene_flags Flags);

//
// Implementation
//
inline f32 SampleHeight(height_field* Field, v2 UV)
{
    v2u Coord = 
    {
        (u32)Floor(Field->TexelCountX * UV.X),
        (u32)Floor(Field->TexelCountY * UV.Y),
    };

    f32 Result = Field->HeightData[Coord.X + Coord.Y * Field->TexelCountX];
    return(Result);
}

lbfn v4 SampleTerrain(game_world* World, v2 P)
{
    v4 Result = {};
    #if 0
    v2 BaseP = P + World->TerrainP;

    v2 UV = P; // TODO
    f32 Height = SampleHeight(&World->HeightField, UV);
    v3 PX0 = SampleHeight(&World->HeightField, { P.X - 1.0f, P.Y });
    v3 PX1 = SampleHeight(&World->HeightField, { P.X + 1.0f, P.Y });
    v3 PY0 = SampleHeight(&World->HeightField, { P.X, P.Y - 1.0f });
    v3 PY1 = SampleHeight(&World->HeightField, { P.X, P.Y + 1.0f });

    v3 T = NOZ(PX1 - PX0);
    v3 B = NOZ(PY1 - PY0);
    v3 N = NOZ(Cross(T, B));
    v4 Result = { N.X, N.Y, N.Z, -Dot(N, v3{ P.X, P.Y, Height }) };
    #endif

    return(Result);
}