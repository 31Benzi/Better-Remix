#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
#include "Pickups.h"
#include <ShlObj.h>
#include "Config.h"
INIT_MODULE(FortInventoryOwnerInterface);

bool PersistInInventoryWhenFinalStackEmpty(UFortItemDefinition* ItemDefinition)
{
    return HasTrait(ItemDefinition, L"Item.Trait.AllowEmptyFinalStack");
}

bool RemoveInventoryItem(UInterface* Interface, FGuid& ItemGuid, int Count, bool bForceRemoval)
{
    if (bInfiniteAmmo)
        return true;
    auto Interface__GetOwner = (UObject * (*)(UInterface*)) Interface->VTable[0x1];
    auto InterfaceOwner = (AController*)Interface__GetOwner(Interface);

    TScriptInterface<UFortInventoryInterface> InventoryInterface;

    if (auto PlayerController = InterfaceOwner->Cast<AFortPlayerController>())
        InventoryInterface = PlayerController->WorldInventory;
    else if (auto BotController = InterfaceOwner->Cast<AFortAthenaAIBotController>())
        InventoryInterface = BotController->Inventory;

    auto Inventory = (AFortInventory*)InventoryInterface.ObjectPointer;

    auto Item = GetItem(InventoryInterface, ItemGuid);

    if (Item)
    {
        /*for (int i = 0; i < itemEntry->StateValues.Num(); i++)
        {
            auto& StateValue = itemEntry->StateValues.Get(i, FFortItemEntryStateValue::Size());

            if (StateValue.StateType != 2)
                continue;

            StateValue.IntValue = 0;
        }*/

        Item->ItemEntry.Count -= max(Count, 0);
        if (Count < 0 || Item->ItemEntry.Count <= 0 || bForceRemoval)
        {
            if (PersistInInventoryWhenFinalStackEmpty((UFortItemDefinition*)Item->ItemEntry.ItemDefinition) && Count > 0)
            {
                auto OtherStack = Inventory->Inventory.ReplicatedEntries.Search(
                    [&](FFortItemEntry& item)
                    {
                        return item.ItemDefinition == Item->ItemEntry.ItemDefinition && !GuidEquals(item.ItemGuid, ItemGuid);
                    });

                if (!OtherStack)
                {
                    /*for (int i = 0; i < itemEntry->StateValues.Num(); i++)
                    {
                        auto& StateValue = itemEntry->StateValues.Get(i, FFortItemEntryStateValue::Size());

                        if (StateValue.StateType != 2)
                            continue;

                        StateValue.IntValue = 0;
                        break;
                    }*/

                    SetToDirty(InventoryInterface, Item->ItemEntry);
                }
                else
                    Remove(InventoryInterface, ItemGuid);
            }
            else
                Remove(InventoryInterface, ItemGuid);
        }
        else
        {
            /*for (int i = 0; i < itemEntry->StateValues.Num(); i++)
            {
                auto& StateValue = itemEntry->StateValues.Get(i, FFortItemEntryStateValue::Size());

                if (StateValue.StateType != 2)
                    continue;

                StateValue.IntValue = 0;
                break;
            }*/

            SetToDirty(InventoryInterface, Item->ItemEntry);
        }

        return true;
    }

    return false;
}

bool CreateInventory(UFortInventoryOwnerInterface* Interface)
{
    auto GetUObject = (UObject* (*)(UFortInventoryOwnerInterface*))Interface->VTable[0x1];
    auto InventoryOwner = GetUObject(Interface);

    if (auto BotController = InventoryOwner->Cast<AFortAthenaAIBotController>())
    {
        auto Inventory = SpawnActor<AFortInventory>(FVector {}, FRotator {}, BotController);
        BotController->Inventory.ObjectPointer = Inventory;
        BotController->Inventory.InterfacePointer = GetInterfaceAddress(Inventory, UFortInventoryInterface::StaticClass());
        Inventory->InventoryType = EFortInventoryType::World;

        return true;
    }

    return true;
}

UFortWorldItem* GiveItem(AFortAthenaAIBotController* Controller, FItemAndCount ItemAndCount)
{
    UFortWorldItem* Item = (UFortWorldItem*)((UFortItemDefinition*)ItemAndCount.Item)->CreateTemporaryItemInstanceBP(ItemAndCount.Count, -1);

    if (!Item)
        return nullptr;

    if (Item->ItemEntry.ItemGuid.A == 0 && Item->ItemEntry.ItemGuid.B == 0 && Item->ItemEntry.ItemGuid.C == 0 && Item->ItemEntry.ItemGuid.D == 0)
    {
        // printf("[Runtime] Creating GUID for inventory item\n");
        CoCreateGuid((GUID*)&Item->ItemEntry.ItemGuid);

        if (Item->ItemEntry.TrackerGuid.A == 0 && Item->ItemEntry.TrackerGuid.B == 0 && Item->ItemEntry.TrackerGuid.C == 0 && Item->ItemEntry.TrackerGuid.D == 0)
            CoCreateGuid((GUID*)&Item->ItemEntry.TrackerGuid);
    }

    //printf("AI GiveItem %s\n", ItemAndCount.Item->Name.ToString().c_str());

    if (auto Weapon = ItemAndCount.Item->Cast<UFortWeaponItemDefinition>())
        Item->ItemEntry.LoadedAmmo = GetStats(Weapon)->ClipSize;

    Item->ItemEntry.ItemVariantGuid = ItemAndCount.ItemVariantGuid;
    Item->ItemEntry.WeaponModSlots = ItemAndCount.WeaponModSlots;
    Item->ItemEntry.ParentInventory = Controller->Inventory.ObjectPointer;

    auto Inventory = (AFortInventory*)Controller->Inventory.ObjectPointer;
    auto& ItemEntry = Inventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
    //auto ItemInstance = Inventory->Inventory.ItemInstances.Add(Item);

    Inventory->PendingInstances.Add(Item);

    Inventory->bRequiresLocalUpdate = true;
    Inventory->bRequiresSaving = true;

    Inventory->HandleInventoryLocalUpdate(); // calls UpdateItemInstances, the func we actually want

    ItemEntry.bIsDirty = false;
    Inventory->Inventory.MarkItemDirty(ItemEntry);
    Inventory->ForceNetUpdate();
    Item->ItemEntry.bIsDirty = true;

    ((bool (*)(UFortWorldItem*, UInterface*))Item->VTable[0xB2])(Item, GetInterfaceAddress(Controller, UFortInventoryOwnerInterface::StaticClass()));

    /*if (auto WeaponDef = ItemAndCount.Item->Cast<UFortWeaponRangedItemDefinition>())
    {
        if (WeaponDef->ItemType == EFortItemType::WeaponRanged)
        {
            Controller->EquipWeapon(Item, true);
            // Controller->EquipWeapon(Item);
            //Controller->PendingEquipWeapon = Item;
            //Controller->PlayerBotPawn->EquipWeaponDefinition(WeaponDef, ItemEntry.ItemGuid, ItemEntry.TrackerGuid, false);
        }
    }*/

    static auto WouldItemFitInBotInventory = (bool (*)(AFortAthenaAIBotController*, UFortWorldItem*, int*, bool*, bool*))(ImageBase + 0xA7A1DE8);
    static auto FindWeaponTypeForStats = (EFortWeaponType (*)(UFortItemDefinition*))(ImageBase + 0x356C5A4);

    int InventorySlot = -1;
    bool bIsItemSupported = false;
    bool param5ForSomeReason = false;
    bool bFits = WouldItemFitInBotInventory(Controller, Item, &InventorySlot, &bIsItemSupported, &param5ForSomeReason);

    if (bFits && InventorySlot != -1)
    {
        auto Def = (UFortItemDefinition*)ItemAndCount.Item;
        auto SetItem = (void (*)(FFortBotInventoryInfo*, UFortWorldItem*))(ImageBase + 0xA7980C0);

        SetItem(&Controller->SlotItems[InventorySlot], Item);

        /*BotController->SlotItems[InventorySlot].BotController = BotController;
        BotController->SlotItems[InventorySlot].FortItem = It;
        BotController->SlotItems[InventorySlot].ItemDefinition = Def;
        *(EFortItemType*)(__int64(&BotController->SlotItems[InventorySlot]) + 0x10) = Def->ItemType;
        if (auto Weapon = Def->Cast<UFortWeaponItemDefinition>())
        {
            BotController->SlotItems[InventorySlot].WeaponDefinitionCache = Weapon;
            *(EFortWeaponType*)(__int64(&BotController->SlotItems[InventorySlot]) + 0x11) = FindWeaponTypeForStats(Def);
        }*/
    }

    return Item;
}

void InitializeInventory(UFortInventoryOwnerInterface* Interface, TArray<FItemAndCount> Items)
{
    auto GetUObject = (UObject * (*)(UFortInventoryOwnerInterface*)) Interface->VTable[0x1];
    auto InventoryOwner = GetUObject(Interface);

    if (auto BotController = InventoryOwner->Cast<AFortAthenaAIBotController>())
    {
        for (auto& Item : Items)
        {
            auto It = GiveItem(BotController, Item);
            // i++;
        }

        BotController->EquipBestWeapon(true);
    }
}

void FortInventoryOwnerInterface__Init()
{

    auto IFaceForBC = (void**)(ImageBase + 0xFB3F4B8);
    auto IFaceForPC = (void**)(ImageBase + 0xF85B548);

    auto HookVFTForIFace = [&](uint32_t Index, void* Detour, void** Original)
    {
        VirtualSwap(IFaceForBC, Index, Detour, Original);
        VirtualSwap(IFaceForPC, Index, Detour, Original);
    };

    HookVFTForIFace(0x3D, RemoveInventoryItem, nullptr);

    HookVFTForIFace(0x3, CreateInventory, nullptr);
    HookVFTForIFace(0x4, InitializeInventory, nullptr);
}