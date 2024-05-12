struct profile_entry
{
    u64 InclusiveDeltaTSC;
    u64 ExclusiveDeltaTSC;

    u64 HitCount;

    const char* Label;
};

struct profiler
{
    u32 CurrentTranslationUnit;
    u32 CurrentEntryIndex;

    static constexpr u32 MaxEntryCount = 4096;
    profile_entry Entries[LB_TranslationUnitCount][MaxEntryCount];

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
    u32 LastEntryIndex;
    u32 LastTranslationUnit;

    profile_block(profiler* Profiler, const char* Label, u32 EntryIndex);
    ~profile_block();
};

#define TimedBlock(p, label) profile_block LB_Concat(ProfileBlock, __LINE__)(p, label, __COUNTER__ + 1)
#define TimedFunction(p) TimedBlock(p, __FUNCTION__)

#define ProfilerOverflowGuard static_assert(__COUNTER__ <= profiler::MaxEntryCount, "Profiler overflow!")