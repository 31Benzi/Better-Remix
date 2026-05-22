#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
#include <algorithm>
#include <ShlObj.h>

UFortWorldItem* GiveItem(TScriptInterface<UFortInventoryInterface>& InventoryInterface, UFortItemDefinition* Def, int Count, int LoadedAmmo, int Level, bool updateInventory,
    int PhantomReserveAmmo, TArray<FFortItemEntryStateValue> StateValues)
{
    auto Inventory = (AFortInventory*)InventoryInterface.ObjectPointer;
    if (!InventoryInterface.ObjectPointer || !Def || !Count)
        return nullptr;

    if (!Inventory || !Def || !Count)
        return nullptr;

    UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, Level);

    if (!Item)
        return nullptr;

    if (Item->ItemEntry.ItemGuid.A == 0 && Item->ItemEntry.ItemGuid.B == 0 && Item->ItemEntry.ItemGuid.C == 0 && Item->ItemEntry.ItemGuid.D == 0)
    {
        //printf("[Runtime] Creating GUID for inventory item\n");
        CoCreateGuid((GUID*)&Item->ItemEntry.ItemGuid);

        if (Item->ItemEntry.TrackerGuid.A == 0 && Item->ItemEntry.TrackerGuid.B == 0 && Item->ItemEntry.TrackerGuid.C == 0
            && Item->ItemEntry.TrackerGuid.D == 0)
            CoCreateGuid((GUID*)&Item->ItemEntry.TrackerGuid);
    }

    Item->SetOwningControllerForTemporaryItem((AFortPlayerController*)Inventory->Owner);
    Item->ItemEntry.ParentInventory = Inventory;
    Item->ItemEntry.LoadedAmmo = LoadedAmmo;
    Item->ItemEntry.PhantomReserveAmmo = PhantomReserveAmmo;
    if (auto WeaponDef = Def->Cast<UFortWeaponItemDefinition>())
        Item->ItemEntry.WeaponModSlots = WeaponDef->GetWeaponModSlots();

    auto& repEntry = Inventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
    repEntry.bIsReplicatedCopy = true;
    //Inventory->Inventory.ItemInstances.Add(Item);

    /*if (Item->ItemEntry.ItemDefinition->bForceFocusWhenAdded)
    {
        ((AFortPlayerControllerAthena*)Owner)->ServerExecuteInventoryItem(Item->ItemEntry.ItemGuid);
        ((AFortPlayerControllerAthena*)Owner)->ClientEquipItem(Item->ItemEntry.ItemGuid, true);
    }*/

    Inventory->PendingInstances.Add(Item);

    if (updateInventory)
    {
        // Update(&Item->ItemEntry);

        Inventory->bRequiresLocalUpdate = true;
        Inventory->bRequiresSaving = true;

        Inventory->HandleInventoryLocalUpdate(); // calls UpdateItemInstances, the func we actually want

        repEntry.bIsDirty = false;
        Inventory->Inventory.MarkItemDirty(repEntry);
        Inventory->ForceNetUpdate();
        Item->ItemEntry.bIsDirty = true;
    }


    ((bool (*)(UFortWorldItem*, UInterface*))Item->VTable[0xB2])(Item, GetInterfaceAddress(Inventory->Owner, UFortInventoryOwnerInterface::StaticClass()));

    return Item;
}

UFortWorldItem* GiveItem(TScriptInterface<UFortInventoryInterface>& InventoryInterface, FFortItemEntry& entry, int Count, bool updateInventory)
{
    if (Count == -1)
        Count = entry.Count;

    return GiveItem(
        InventoryInterface, (UFortItemDefinition*)entry.ItemDefinition, Count, entry.LoadedAmmo, entry.Level, updateInventory, entry.PhantomReserveAmmo, entry.StateValues);
}

void Remove(TScriptInterface<UFortInventoryInterface>& InventoryInterface, FGuid Guid)
{
    auto Inventory = (AFortInventory*)InventoryInterface.ObjectPointer;

    if (!Inventory)
        return;

    auto ItemEntryIdx = Inventory->Inventory.ReplicatedEntries.SearchIndex(
        [&](FFortItemEntry& entry)
        {
            return GuidEquals(entry.ItemGuid, Guid);
        });

    if (ItemEntryIdx == -1)
        return;

    auto& ItemEntry = Inventory->Inventory.ReplicatedEntries[ItemEntryIdx];
    auto EntryDef = ItemEntry.ItemDefinition;

    auto ItemInstanceIdx = Inventory->Inventory.ItemInstances.SearchIndex(
        [&](UFortWorldItem* entry)
        {
            return GuidEquals(entry->ItemEntry.ItemGuid, Guid);
        });
    auto ItemInstance = ItemInstanceIdx != -1 ? Inventory->Inventory.ItemInstances[ItemInstanceIdx] : nullptr;

    Inventory->Inventory.ReplicatedEntries.Remove(ItemEntryIdx);
    //if (ItemInstanceIdx != -1)
    //    Inventory->Inventory.ItemInstances.Remove(ItemInstanceIdx);

    Inventory->bRequiresLocalUpdate = true;
    Inventory->bRequiresSaving = true;

    Inventory->HandleInventoryLocalUpdate(); // calls UpdateItemInstances, the func we actually want

    Inventory->Inventory.MarkArrayDirty();
    Inventory->ForceNetUpdate();

    if (ItemInstance && ItemInstance->ItemEntry.ItemDefinition)
    {
        ((bool (*)(UFortWorldItem*, UInterface*, uint32_t))ItemInstance->VTable[0xB3])(
            ItemInstance, GetInterfaceAddress(Inventory->Owner, UFortInventoryOwnerInterface::StaticClass()), ItemInstance->ItemEntry.Count);
    }
}

void SetToDirty(TScriptInterface<UFortInventoryInterface>& InventoryInterface, FFortItemEntry& Entry)
{
    static auto SetToDirtyInternal = (void (*)(void*, FFortItemEntry&))(ImageBase + 0x9CA8768);

    return SetToDirtyInternal(InventoryInterface.InterfacePointer, Entry);
}

FFortRangedWeaponStats* GetStats(UFortWeaponItemDefinition* Def)
{
    if (!Def || !Def->WeaponStatHandle.DataTable)
        return nullptr;

    auto Val = GetRowMap(Def->WeaponStatHandle.DataTable).Search(
        [Def](FName& Key, uint8_t* Value)
        {
            return Def->WeaponStatHandle.RowName == Key && Value;
        });

    return Val ? *(FFortRangedWeaponStats**)Val : nullptr;
}

FFortItemEntry MakeItemEntry(UFortItemDefinition* ItemDefinition, int32 Count, int32 Level)
{
    FFortItemEntry ItemEntry {};

    ItemEntry.MostRecentArrayReplicationKey = -1;
    ItemEntry.ReplicationID = -1;
    ItemEntry.ReplicationKey = -1;
    ItemEntry.ItemDefinition = ItemDefinition;
    ItemEntry.Count = Count;
    ItemEntry.Durability = 1.f;
    ItemEntry.GameplayAbilitySpecHandle = FGameplayAbilitySpecHandle(-1);
    ItemEntry.ParentInventory.ObjectIndex = -1;
    ItemEntry.Level = Level;
    if (auto Weapon = ItemDefinition->IsA<UFortGadgetItemDefinition>() ? ((UFortGadgetItemDefinition*)ItemDefinition)->GetWeaponItemDefinition()
                                                                : ItemDefinition->Cast<UFortWeaponItemDefinition>())
    {
        auto Stats = GetStats(Weapon);
        if (Stats)
        {
            ItemEntry.LoadedAmmo = Stats->ClipSize;

            if (Weapon->bUsesPhantomReserveAmmo)
                ItemEntry.PhantomReserveAmmo = (Stats->InitialClips - 1) * Stats->ClipSize;
        }
    }
    if (auto WeaponDef = ItemDefinition->Cast<UFortWeaponItemDefinition>())
        ItemEntry.WeaponModSlots = WeaponDef->GetWeaponModSlots();
    ItemEntry.PickupVariantIndex = -1;
    ItemEntry.ItemVariantDataMappingIndex = -1;
    ItemEntry.OrderIndex = -1;

    return ItemEntry;
}

AFortPickupAthena* SpawnPickup(FVector Loc, FFortItemEntry& Entry, EFortPickupSourceTypeFlag SourceTypeFlag, EFortPickupSpawnSource SpawnSource, AFortPlayerPawn* Pawn,
    int OverrideCount,
    bool Toss, bool RandomRotation, bool bCombine, UClass* OverrideClass, FVector FinalLoc)
{
    if (!&Entry)
        return nullptr;
    AFortPickupAthena* NewPickup = SpawnActor<AFortPickupAthena>(OverrideClass ? OverrideClass : AFortPickupAthena::StaticClass(), Loc, {});
    if (!NewPickup)
        return nullptr;

    NewPickup->bRandomRotation = RandomRotation;
    if (Entry.Level != -1)
        NewPickup->PrimaryPickupItemEntry.Level = Entry.Level;
    NewPickup->PrimaryPickupItemEntry.ItemDefinition = Entry.ItemDefinition;
    NewPickup->PrimaryPickupItemEntry.LoadedAmmo = Entry.LoadedAmmo;
    NewPickup->PrimaryPickupItemEntry.Count = OverrideCount != -1 ? OverrideCount : Entry.Count;
    NewPickup->PrimaryPickupItemEntry.PhantomReserveAmmo = Entry.PhantomReserveAmmo;

    TArray<FFortItemEntry> a {};
    static auto SetPickupItems
        = (void (*)(AFortPickupAthena*, FFortItemEntry*, TArray<FFortItemEntry>*, EFortPickupSourceTypeFlag, bool, EFortPickupSpawnSource))(ImageBase + 0x9CDB314);

    SetPickupItems(NewPickup, &NewPickup->PrimaryPickupItemEntry, &a, SourceTypeFlag, false, SpawnSource);

    // NewPickup->OnRep_PrimaryPickupItemEntry();
    NewPickup->PawnWhoDroppedPickup = Pawn;

    auto FinalLocation = Loc;

    if (FinalLoc.X || FinalLoc.Y || FinalLoc.Z)
        FinalLocation = FinalLoc;
    NewPickup->TossPickup(FinalLocation, Pawn, -1, Toss, true, SourceTypeFlag, SpawnSource);

    NewPickup->bTossedFromContainer = SpawnSource == EFortPickupSpawnSource::Chest || SpawnSource == EFortPickupSpawnSource::AmmoBox;
    if (NewPickup->bTossedFromContainer)
        NewPickup->OnRep_TossedFromContainer();

    return NewPickup;
}

AFortPickupAthena* SpawnPickup(FVector Loc, UFortItemDefinition* ItemDefinition, int Count, int LoadedAmmo, EFortPickupSourceTypeFlag SourceTypeFlag, EFortPickupSpawnSource SpawnSource,
    AFortPlayerPawn* Pawn, bool Toss, bool bRandomRotation, UClass* OverrideClass)
{
    auto ItemEntry = MakeItemEntry(ItemDefinition, Count, -1);
    auto Pickup = SpawnPickup(Loc, ItemEntry, SourceTypeFlag, SpawnSource, Pawn, -1, Toss, true, bRandomRotation, OverrideClass, {});
    return Pickup;
}

AFortPickupAthena* SpawnPickup(ABuildingContainer* Container, FFortItemEntry& Entry, AFortPlayerPawn* Pawn, int OverrideCount)
{
    if (!&Entry)
        return nullptr;

    auto ContainerLoc = Container->K2_GetActorLocation();
    auto SpawnLocation = Container->LootSpawnLocation_Athena;
    auto Loc = VecAdd(VecAdd(VecAdd(ContainerLoc, VecMul(Container->GetActorForwardVector(), SpawnLocation.X)), VecMul(Container->GetActorRightVector(), SpawnLocation.Y)),
        VecMul(Container->GetActorUpVector(), SpawnLocation.Z));
    AFortPickupAthena* NewPickup = SpawnActor<AFortPickupAthena>(Loc, {});

    if (!NewPickup)
        return nullptr;

    NewPickup->bRandomRotation = true;
    NewPickup->PrimaryPickupItemEntry.Level = Entry.Level;
    NewPickup->PrimaryPickupItemEntry.ItemDefinition = Entry.ItemDefinition;
    NewPickup->PrimaryPickupItemEntry.LoadedAmmo = Entry.LoadedAmmo;
    NewPickup->PrimaryPickupItemEntry.Count = OverrideCount != -1 ? OverrideCount : Entry.Count;
    NewPickup->PrimaryPickupItemEntry.PhantomReserveAmmo = Entry.PhantomReserveAmmo;

    TArray<FFortItemEntry> a {};

    static auto SetPickupItems
        = (void (*)(AFortPickupAthena*, FFortItemEntry*, TArray<FFortItemEntry>*, EFortPickupSourceTypeFlag, bool, EFortPickupSpawnSource))(ImageBase + 0x9CDB314);

    SetPickupItems(NewPickup, &NewPickup->PrimaryPickupItemEntry, &a, EFortPickupSourceTypeFlag::Container, false, EFortPickupSpawnSource::Chest);

    NewPickup->PawnWhoDroppedPickup = Pawn;
    
    FRotator DirectionD;
    DirectionD.pitch = Container->LootTossDirection_Athena.pitch;
    DirectionD.Roll = Container->LootTossDirection_Athena.Roll;
    DirectionD.Yaw = Container->LootTossDirection_Athena.Yaw;

    UFortKismetLibrary::TossPickupFromContainer(GetWorld(), Container, NewPickup, 10, (int32)std::clamp((float)rand() * 0.0003357036f, 0.f, 10.f),
        Container->LootTossConeHalfAngle_Athena, DirectionD, Container->LootTossSpeed_Athena, Container->bForceHidePickupMinimapIndicator);

    NewPickup->bTossedFromContainer = true;
    NewPickup->OnRep_TossedFromContainer();

    return NewPickup;
}

bool IsPrimaryQuickbar(UFortItemDefinition* ItemDefinition)
{
    if (!ItemDefinition)
        return false;

    return ItemDefinition->ItemType == EFortItemType::WeaponHarvest || ItemDefinition->ItemType == EFortItemType::WorldResource || ItemDefinition->ItemType == EFortItemType::Ammo
            || ItemDefinition->ItemType == EFortItemType::Trap || ItemDefinition->ItemType == EFortItemType::BuildingPiece || ItemDefinition->ItemType == EFortItemType::EditTool
            || ItemDefinition->ItemType == EFortItemType::Ingredient
        ? false
        : true;
}


UFortWorldItem* GetItem(TScriptInterface<UFortInventoryInterface>& InventoryInterface, FGuid& ItemGuid)
{
    if (!InventoryInterface.InterfacePointer || !InventoryInterface.ObjectPointer)
        return nullptr;

    /*auto Inventory = (AFortInventory*)InventoryInterface.ObjectPointer;

    for (auto& Item : Inventory->Inventory.ItemInstances)
        if (GuidEquals(Item->ItemEntry.ItemGuid, ItemGuid))
            return Item;

    return nullptr;*/
    static auto GetItemInternal = (UFortWorldItem * (*)(void*, FGuid&))(ImageBase + 0x2B4A6E4);

    return GetItemInternal(InventoryInterface.InterfacePointer, ItemGuid);
}

FFortItemEntry* GetReplicatedItemEntry(AFortInventory* Inventory, FGuid& ItemGuid)
{
    if (!Inventory)
        return nullptr;

    /*for (auto& ItemEntry : Inventory->Inventory.ReplicatedEntries)
        if (GuidEquals(ItemEntry.ItemGuid, ItemGuid))
            return &ItemEntry;

    return nullptr;*/
    static auto GetReplicatedItemEntryInternal = (FFortItemEntry*(*)(AFortInventory*, FGuid&))(ImageBase + 0x2B4A76C);

    return GetReplicatedItemEntryInternal(Inventory, ItemGuid);
}