#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
INIT_MODULE(FortAthenaSupplyDrop);

void SpawnPickupHook(UObject* Object, FFrame& Stack, AFortPickupAthena** Ret)
{
    UFortItemDefinition* ItemDefinition;
    int32 NumberToSpawn;
    AFortPlayerPawnAthena* TriggeringPawn;
    FVector Position;
    FVector Direction;
    Stack.StepCompiledIn(&ItemDefinition);
    Stack.StepCompiledIn(&NumberToSpawn);
    Stack.StepCompiledIn(&TriggeringPawn);
    Stack.StepCompiledIn(&Position);
    Stack.StepCompiledIn(&Direction);
    Stack.IncrementCode();

    *Ret = SpawnPickup(Position, ItemDefinition, NumberToSpawn,
        ItemDefinition->IsA<UFortWeaponItemDefinition>() ? GetStats((UFortWeaponItemDefinition*)ItemDefinition)->ClipSize : 0,
        EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::SupplyDrop);
}

void FortAthenaSupplyDrop__Init()
{
    ExecHook(AFortAthenaSupplyDrop::StaticClass()->FindFunction("SpawnPickup"), SpawnPickupHook);
}