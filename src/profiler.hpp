struct profile_entry
{
    u64 InclusiveDeltaTSC;
    u64 ExclusiveDeltaTSC;

    u64 HitCount;

    const char* Label;
};

struct profiler
{
    static constexpr u32 MaxThreadCount = 16;

    // TODO(boti): Cache-line pad these
    u32 CurrentTranslationUnit[MaxThreadCount];
    u32 CurrentEntryIndex[MaxThreadCount];

    static constexpr u32 MaxEntryCount = 4096;
    profile_entry Entries[MaxThreadCount][LB_TranslationUnitCount][MaxEntryCount];

    u64 BeginTSC;
    u64 EndTSC;
};

lbfn void BeginProfiler(profiler* Profiler);
lbfn void EndProfiler(profiler* Profiler);

struct profile_block
{
    profiler* Profiler;
    u64 BeginTSC;
    const char* Label;
    
    u64 OldInclusiveDeltaTSC;
    u32 EntryIndex;
    u32 ThreadIndex;
    
    u32 LastEntryIndex;
    u32 LastTranslationUnit;

    profile_block(profiler* Profiler, u32 ThreadIndex, const char* Label, u32 EntryIndex);
    ~profile_block();
};

#define TimedBlock(p, label) profile_block LB_Concat(ProfileBlock, __LINE__)(p, 0, label, __COUNTER__ + 1)
#define TimedFunction(p) TimedBlock(p, __FUNCTION__)

#define TimedBlockMT(p, t, label) profile_block LB_Concat(ProfileBlock, __LINE__)(p, t, label, __COUNTER__ + 1)
#define TimedFunctionMT(p, t) TimedBlockMT(p, t, __FUNCTION__)

#define ProfilerOverflowGuard static_assert(__COUNTER__ < profiler::MaxEntryCount, "Profiler overflow!")