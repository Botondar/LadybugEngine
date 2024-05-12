lbfn void BeginProfiler(profiler* Profiler)
{
    memset(Profiler, 0, sizeof(*Profiler));

    Profiler->BeginTSC = ReadTSC();
}

lbfn void EndProfiler(profiler* Profiler)
{
    Profiler->EndTSC = ReadTSC();
}

profile_block::profile_block(profiler* Profiler_, const char* Label_, u32 EntryIndex_)
{
    Profiler = Profiler_;
    Label = Label_;
    EntryIndex = EntryIndex_;

    profile_entry* Entry = Profiler->Entries[LB_TranslationUnit] + EntryIndex;
    OldInclusiveDeltaTSC = Entry->InclusiveDeltaTSC;

    LastEntryIndex = Profiler->CurrentEntryIndex;
    LastTranslationUnit = Profiler->CurrentTranslationUnit;
    Profiler->CurrentEntryIndex = EntryIndex;
    Profiler->CurrentTranslationUnit = LB_TranslationUnit;

    BeginTSC = ReadTSC();
}

profile_block::~profile_block()
{
    u64 DeltaTSC = ReadTSC() - BeginTSC;

    profile_entry* Entry = Profiler->Entries[LB_TranslationUnit] + EntryIndex;
    Entry->InclusiveDeltaTSC = OldInclusiveDeltaTSC + DeltaTSC;
    Entry->ExclusiveDeltaTSC += DeltaTSC;
    Entry->HitCount++;
    Entry->Label = Label;

    profile_entry* ParentEntry = Profiler->Entries[LastTranslationUnit] + LastEntryIndex;
    ParentEntry->ExclusiveDeltaTSC -= DeltaTSC;

    Profiler->CurrentEntryIndex = LastEntryIndex;
    Profiler->CurrentTranslationUnit = LastTranslationUnit;
}