#include "pch.h"
#include "Shared.h"
INIT_MODULE(FortMissionLibrary);

void TeleportPlayerPawn(UObject* Context, FFrame& Stack, bool* Ret)
{
    UObject* WorldContextObject;
    AFortPlayerPawnAthena* PlayerPawn;
    FVector DestLocation;
    FRotator DestRotation;
    bool bIgnoreCollision;
    bool bIgnoreSupplementalKillVolumeSweep;

    Stack.StepCompiledIn(&WorldContextObject);
    Stack.StepCompiledIn(&PlayerPawn);
    Stack.StepCompiledIn(&DestLocation);
    Stack.StepCompiledIn(&DestRotation);
    Stack.StepCompiledIn(&bIgnoreCollision);
    Stack.StepCompiledIn(&bIgnoreSupplementalKillVolumeSweep);
    Stack.IncrementCode();

    PlayerPawn->K2_TeleportTo(DestLocation, DestRotation);
    *Ret = true;
}

void FortMissionLibrary__Init()
{
    ExecHook(UFortMissionLibrary::StaticClass()->FindFunction("TeleportPlayerPawn"), TeleportPlayerPawn);
}