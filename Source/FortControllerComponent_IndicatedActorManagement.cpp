#include "pch.h"
#include "Shared.h"
INIT_MODULE(FortControllerComponent_IndicatedActorManagement);

void ControllerComp__AddActorsToIndicatedList(UObject* Context, FFrame& Stack)
{
    FIndicatedActorData Data;
    bool bAddAsUnique;
    bool bAllowOwningPlayer;
    bool bReplaceExistingEntry;
    bool bRefreshExistingEntry;

    auto IndicatedActors = Stack.StepCompiledInRef<TArray<AActor*>>();
    Stack.StepCompiledIn(&Data);
    Stack.StepCompiledIn(&bAddAsUnique);
    Stack.StepCompiledIn(&bAllowOwningPlayer);
    Stack.StepCompiledIn(&bReplaceExistingEntry);
    Stack.StepCompiledIn(&bRefreshExistingEntry);
    Stack.IncrementCode();
    auto Comp = (UFortControllerComponent_IndicatedActorManagement*)Context;

    //printf(__FUNCTION__ " with %d actors\n", IndicatedActors.Num());

    float TimeSeconds = UFortKismetLibrary::GetServerWorldTimeSeconds(GetWorld());

    for (auto& Actor : IndicatedActors)
    {
        //printf("Add to indicated %s for %f seconds\n", Actor->Name.ToString().c_str(), Data.duration);

        FIndicatedActorInfoEntry Entry { -1, -1, -1 };
        Entry.Actor.WeakPtr = Actor;
        Entry.Data = Data;
        CopyArray(Entry.Data.GroupIdentifier, Data.GroupIdentifier);
        CopyArray(Entry.Data.EventTags.GameplayTags, Data.EventTags.GameplayTags);
        CopyArray(Entry.Data.EventTags.ParentTags, Data.EventTags.ParentTags);
        Entry.EndTime = TimeSeconds + Data.duration;
        Entry.StartTime = TimeSeconds;
        Entry.bReplaceExistingWhenAdded = bReplaceExistingEntry;
        Entry.bRefreshExistingWhenAdded = bRefreshExistingEntry;

        FIndicatedActorInfoEntry* ContainedEntry = nullptr;

        for (auto& ExistingEntry : Comp->IndicatedActorList.Entries)
            if (ExistingEntry.Actor.WeakPtr == Entry.Actor.WeakPtr)
            {
                ContainedEntry = &ExistingEntry;
                break;
            }

        if (!ContainedEntry)
        {
            //printf("Add new\n");

            FIndicatedActorInfoEntry& Info = Comp->IndicatedActorList.Entries.Add(Entry);
            Comp->IndicatedActorList.MarkItemDirty(Info);
        }
        else
        {
            if (bReplaceExistingEntry)
                *ContainedEntry = Entry;
            else if (bRefreshExistingEntry)
                Comp->IndicatedActorList.MarkItemDirty(*ContainedEntry);
        }
    }
}

void ControllerComp__AddActorsToStenciledList(UObject* Context, FFrame& Stack)
{
    FStenciledActorData Data;
    bool bAddAsUnique;
    bool bReplaceExistingEntry;
    bool bRefreshExistingEntry;

    auto StenciledActors = Stack.StepCompiledInRef<TArray<AActor*>>();
    Stack.StepCompiledIn(&Data);
    Stack.StepCompiledIn(&bAddAsUnique);
    Stack.StepCompiledIn(&bReplaceExistingEntry);
    Stack.StepCompiledIn(&bRefreshExistingEntry);
    Stack.IncrementCode();
    auto Comp = (UFortControllerComponent_IndicatedActorManagement*)Context;

    //printf(__FUNCTION__ " with %d actors\n", StenciledActors.Num());

    float TimeSeconds = UFortKismetLibrary::GetServerWorldTimeSeconds(GetWorld());

    for (auto& Actor : StenciledActors)
    {
        //printf("Add to stenciled %s for %f seconds\n", Actor->Name.ToString().c_str(), Data.duration);

        FStenciledActorInfoEntry Entry { -1, -1, -1 };
        Entry.Actor.WeakPtr = Actor;
        Entry.Data = Data;
        CopyArray(Entry.Data.GroupIdentifier, Data.GroupIdentifier);
        Entry.EndTime = TimeSeconds + Data.duration;
        Entry.StartTime = TimeSeconds;
        Entry.bReplaceExistingWhenAdded = bReplaceExistingEntry;
        Entry.bRefreshExistingWhenAdded = bRefreshExistingEntry;

        FStenciledActorInfoEntry* ContainedEntry = nullptr;

        for (auto& ExistingEntry : Comp->StenciledActorList.Entries)
            if (ExistingEntry.Actor.WeakPtr == Entry.Actor.WeakPtr)
            {
                ContainedEntry = &ExistingEntry;
                break;
            }

        if (!ContainedEntry)
        {
            //printf("Add new\n");

            FStenciledActorInfoEntry& Info = Comp->StenciledActorList.Entries.Add(Entry);
            Comp->StenciledActorList.MarkItemDirty(Info);
        }
        else
        {
            if (bReplaceExistingEntry)
                *ContainedEntry = Entry;
            else if (bRefreshExistingEntry)
                Comp->StenciledActorList.MarkItemDirty(*ContainedEntry);
        }
    }
}

void FortControllerComponent_IndicatedActorManagement__Init()
{
    ExecHook(UFortControllerComponent_IndicatedActorManagement::StaticClass()->FindFunction("AddActorsToIndicatedList"), ControllerComp__AddActorsToIndicatedList);
    ExecHook(UFortControllerComponent_IndicatedActorManagement::StaticClass()->FindFunction("AddActorsToStenciledList"), ControllerComp__AddActorsToStenciledList);
}