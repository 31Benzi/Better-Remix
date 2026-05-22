#pragma once
#include "pch.h"

UFortWorldItem* GiveItem(TScriptInterface<UFortInventoryInterface>& InventoryInterface, UFortItemDefinition* Def, int Count = 1, int LoadedAmmo = 0, int Level = 0,
    bool updateInventory = true,
    int PhantomReserveAmmo = 0, TArray<FFortItemEntryStateValue> StateValues = {});
UFortWorldItem* GiveItem(TScriptInterface<UFortInventoryInterface>& InventoryInterface, FFortItemEntry& entry, int Count = -1, bool updateInventory = true);

void Remove(TScriptInterface<UFortInventoryInterface>&, FGuid Guid);

FFortItemEntry MakeItemEntry(UFortItemDefinition* ItemDefinition, int32 Count, int32 Level);

void SetToDirty(TScriptInterface<UFortInventoryInterface>&, FFortItemEntry& Entry);

AFortPickupAthena* SpawnPickup(FVector Loc, FFortItemEntry& ItemEntry, EFortPickupSourceTypeFlag SourceTypeFlag = EFortPickupSourceTypeFlag::Other,
    EFortPickupSpawnSource = EFortPickupSpawnSource::Unset, AFortPlayerPawn* Pawn = nullptr, int OverrideCount = -1, bool Toss = true, bool RandomRotation = true,
    bool bCombine = true, UClass* OverrideClass = nullptr, FVector FinalLoc = FVector());
AFortPickupAthena* SpawnPickup(FVector Loc, UFortItemDefinition* ItemDefinition, int Count, int LoadedAmmo,
    EFortPickupSourceTypeFlag SourceTypeFlag = EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource = EFortPickupSpawnSource::Unset, AFortPlayerPawn* Pawn = nullptr,
    bool Toss = true, bool bRandomRotation = true, UClass* OverrideClass = nullptr);
AFortPickupAthena* SpawnPickup(ABuildingContainer* Container, FFortItemEntry& Entry, AFortPlayerPawn* Pawn = nullptr, int OverrideCount = -1);

bool IsPrimaryQuickbar(UFortItemDefinition* ItemDefinition);

FFortRangedWeaponStats* GetStats(UFortWeaponItemDefinition* Def);

UFortWorldItem* GetItem(TScriptInterface<UFortInventoryInterface>& InventoryInterface, FGuid& ItemGuid);
FFortItemEntry* GetReplicatedItemEntry(AFortInventory* Inventory, FGuid& ItemGuid);