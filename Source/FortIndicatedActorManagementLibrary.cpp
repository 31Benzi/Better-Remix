#include "pch.h"
#include "Shared.h"
INIT_MODULE(FortIndicatedActorManagementLibrary);

void AddActorsInRadiusToIndicatedList(UObject* Context, FFrame& Stack)
{
    AController* InstigatingController;
    TArray<FIndicatedActorDataWithFilter> IndicatedActorFilterDatas;
    bool bAddAsUnique;
    bool bReplaceExistingEntry;
    bool bRefreshExistingEntry;
    AActor* InstigatingActorOverride;

    Stack.StepCompiledIn(&InstigatingController);
    Stack.StepCompiledIn(&IndicatedActorFilterDatas);
    Stack.StepCompiledIn(&bAddAsUnique);
    Stack.StepCompiledIn(&bReplaceExistingEntry);
    Stack.StepCompiledIn(&bRefreshExistingEntry);
    Stack.StepCompiledIn(&InstigatingActorOverride);
    Stack.IncrementCode();

    auto Instigator = InstigatingActorOverride ? InstigatingActorOverride : InstigatingController->Pawn;

    if (!Instigator)
        return;

    auto InstigatorLocation = Instigator->K2_GetActorLocation();

    TArray<AActor*> IgnoreActors;

    for (auto& FilterData : IndicatedActorFilterDatas)
    {
        TArray<AActor*> Actors;

        UKismetSystemLibrary::SphereOverlapActors(
            GetWorld(), InstigatorLocation, FilterData.OverlapRadius, FilterData.ObjectTypes, FilterData.ActorClassFilter.Get(), IgnoreActors, &Actors);

        //printf("[" __FUNCTION__ "] SphereOverlapActors returned %d actors\n", Actors.Num());

        UFortIndicatedActorManagementLibrary::AddActorsToIndicatedList(
            InstigatingController, Actors, FilterData.IndicatedData, bAddAsUnique, true, bReplaceExistingEntry, bRefreshExistingEntry);
        Actors.Free();
    }
}

void AddActorsInRadiusToStenciledList(UObject* Context, FFrame& Stack)
{
    AController* InstigatingController;
    TArray<FIndicatedActorDataWithFilter> IndicatedActorFilterDatas;
    bool bAddAsUnique;
    bool bReplaceExistingEntry;
    bool bRefreshExistingEntry;
    AActor* InstigatingActorOverride;

    Stack.StepCompiledIn(&InstigatingController);
    Stack.StepCompiledIn(&IndicatedActorFilterDatas);
    Stack.StepCompiledIn(&bAddAsUnique);
    Stack.StepCompiledIn(&bReplaceExistingEntry);
    Stack.StepCompiledIn(&bRefreshExistingEntry);
    Stack.StepCompiledIn(&InstigatingActorOverride);
    Stack.IncrementCode();

    auto Instigator = InstigatingActorOverride ? InstigatingActorOverride : InstigatingController->Pawn;

    if (!Instigator)
        return;

    auto InstigatorLocation = Instigator->K2_GetActorLocation();

    TArray<AActor*> IgnoreActors;
    IgnoreActors.Add(Instigator);

    for (auto& FilterData : IndicatedActorFilterDatas)
    {
        TArray<AActor*> Actors;

        UKismetSystemLibrary::SphereOverlapActors(
            GetWorld(), InstigatorLocation, FilterData.OverlapRadius, FilterData.ObjectTypes, FilterData.ActorClassFilter.Get(), IgnoreActors, &Actors);

        //printf("[" __FUNCTION__ "] SphereOverlapActors returned %d actors\n", Actors.Num());

        UFortIndicatedActorManagementLibrary::AddActorsToStenciledList(
            InstigatingController, Actors, FilterData.StenciledData, bAddAsUnique, false, bRefreshExistingEntry);
        Actors.Free();
    }
}

void PE_AddActorsToIndicatedList(UFortControllerComponent_IndicatedActorManagement* _this, TArray<class AActor*> IndicatedActors, struct FIndicatedActorData Data,
    bool bAddAsUnique, bool bAllowOwningPlayer, bool bReplaceExistingEntry, bool bRefreshExistingEntry)
{
    static UFunction* FuncPtr = nullptr;

    if (!FuncPtr)
        FuncPtr = _this->Class->FindFunction("AddActorsToIndicatedList");

    struct __Parms
    {
        TArray<class AActor*> IndicatedActors; // 0x0 // Size: 0x10
        struct FIndicatedActorData Data; // 0x10 // Size: 0x100
        bool bAddAsUnique; // 0x110 // Size: 0x1
        bool bAllowOwningPlayer; // 0x111 // Size: 0x1
        bool bReplaceExistingEntry; // 0x112 // Size: 0x1
        bool bRefreshExistingEntry; // 0x113 // Size: 0x1
        uint8_t __Padding_0x114[0x4];
    } Parms;

    Parms.IndicatedActors = IndicatedActors;
    Parms.Data = Data;
    Parms.bAddAsUnique = bAddAsUnique;
    Parms.bAllowOwningPlayer = bAllowOwningPlayer;
    Parms.bReplaceExistingEntry = bReplaceExistingEntry;
    Parms.bRefreshExistingEntry = bRefreshExistingEntry;

    _this->ProcessEvent(FuncPtr, &Parms);
}


void AddActorsToIndicatedList(UObject* Context, FFrame& Stack)
{
    AController* InstigatingController;
    TArray<AActor*> IndicatedActors;
    FIndicatedActorData IndicatedActorData;
    bool bAddAsUnique;
    bool bAllowOwningPlayer;
    bool bReplaceExistingEntry;
    bool bRefreshExistingEntry;

    Stack.StepCompiledIn(&InstigatingController);
    Stack.StepCompiledIn(&IndicatedActors);
    Stack.StepCompiledIn(&IndicatedActorData);
    Stack.StepCompiledIn(&bAddAsUnique);
    Stack.StepCompiledIn(&bAllowOwningPlayer);
    Stack.StepCompiledIn(&bReplaceExistingEntry);
    Stack.StepCompiledIn(&bRefreshExistingEntry);
    Stack.IncrementCode();

    auto IndicatedActorComp
        = (UFortControllerComponent_IndicatedActorManagement*)InstigatingController->GetComponentByClass(UFortControllerComponent_IndicatedActorManagement::StaticClass());

    if (!IndicatedActorComp)
        return;

    //printf("AddActorsToIndicatedList with %d actors\n", IndicatedActors.Num());

    PE_AddActorsToIndicatedList(IndicatedActorComp, IndicatedActors, IndicatedActorData, bAddAsUnique, bAllowOwningPlayer, bReplaceExistingEntry, bRefreshExistingEntry);
}

void PE_AddActorsToStenciledList(UFortControllerComponent_IndicatedActorManagement* _this, TArray<class AActor*> StenciledActors, struct FStenciledActorData Data,
    bool bAddAsUnique, bool bReplaceExistingEntry, bool bRefreshExistingEntry)
{
    static UFunction* FuncPtr = nullptr;

    if (!FuncPtr)
        FuncPtr = _this->Class->FindFunction("AddActorsToStenciledList");

    struct __Parms
    {
        TArray<class AActor*> StenciledActors; // 0x0 // Size: 0x10
        struct FStenciledActorData Data; // 0x10 // Size: 0x80
        bool bAddAsUnique; // 0x90 // Size: 0x1
        bool bReplaceExistingEntry; // 0x91 // Size: 0x1
        bool bRefreshExistingEntry; // 0x92 // Size: 0x1
        uint8_t __Padding_0x93[0x5];
    } Parms;

    Parms.StenciledActors = StenciledActors;
    Parms.Data = Data;
    Parms.bAddAsUnique = bAddAsUnique;
    Parms.bReplaceExistingEntry = bReplaceExistingEntry;
    Parms.bRefreshExistingEntry = bRefreshExistingEntry;

    _this->ProcessEvent(FuncPtr, &Parms);
}

void AddActorsToStenciledList(UObject* Context, FFrame& Stack)
{
    AController* InstigatingController;
    TArray<AActor*> StenciledActors;
    FStenciledActorData StenciledActorData;
    bool bAddAsUnique;
    bool bAllowOwningPlayer;
    bool bReplaceExistingEntry;
    bool bRefreshExistingEntry;
    bool bPerformRadiusCheck;

    Stack.StepCompiledIn(&InstigatingController);
    Stack.StepCompiledIn(&StenciledActors);
    Stack.StepCompiledIn(&StenciledActorData);
    Stack.StepCompiledIn(&bAddAsUnique);
    Stack.StepCompiledIn(&bAllowOwningPlayer);
    Stack.StepCompiledIn(&bReplaceExistingEntry);
    Stack.StepCompiledIn(&bRefreshExistingEntry);
    Stack.StepCompiledIn(&bPerformRadiusCheck);
    Stack.IncrementCode();

    auto IndicatedActorComp
        = (UFortControllerComponent_IndicatedActorManagement*)InstigatingController->GetComponentByClass(UFortControllerComponent_IndicatedActorManagement::StaticClass());

    if (!IndicatedActorComp)
        return;

    //printf("AddActorsToStenciledList with %d actors\n", StenciledActors.Num());

    PE_AddActorsToStenciledList(IndicatedActorComp, StenciledActors, StenciledActorData, bAddAsUnique, bReplaceExistingEntry, bRefreshExistingEntry);
}

void FortIndicatedActorManagementLibrary__Init()
{
    ExecHook(UFortIndicatedActorManagementLibrary::StaticClass()->FindFunction("AddActorsInRadiusToIndicatedList"), AddActorsInRadiusToIndicatedList);
    ExecHook(UFortIndicatedActorManagementLibrary::StaticClass()->FindFunction("AddActorsInRadiusToStenciledList"), AddActorsInRadiusToStenciledList);
    ExecHook(UFortIndicatedActorManagementLibrary::StaticClass()->FindFunction("AddActorsToIndicatedList"), AddActorsToIndicatedList);
    ExecHook(UFortIndicatedActorManagementLibrary::StaticClass()->FindFunction("AddActorsToStenciledList"), AddActorsToStenciledList);
}