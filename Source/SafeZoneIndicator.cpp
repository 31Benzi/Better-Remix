#include "pch.h"
#include "Shared.h"
INIT_MODULE(SafeZoneIndicator);

void GetPhaseInfo(UObject* Context, FFrame& Stack, bool* Ret)
{
    auto& OutSafeZonePhase = Stack.StepCompiledInRef<FFortSafeZonePhaseInfo>();
    int32 InPhaseToGet;
    Stack.StepCompiledIn(&InPhaseToGet);
    Stack.IncrementCode();

    auto SafeZoneIndicator = (AFortSafeZoneIndicator*)Context;
    auto& Array = SafeZoneIndicator->SafeZonePhases;

    if (Array.IsValidIndex(InPhaseToGet))
    {
        OutSafeZonePhase = Array[InPhaseToGet];

        *Ret = true;
        return;
    }
    *Ret = false;
}

void SafeZoneIndicator__Init()
{
    ExecHook(AFortSafeZoneIndicator::StaticClass()->FindFunction("GetPhaseInfo"), GetPhaseInfo);
}