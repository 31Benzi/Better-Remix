#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
#include "Pickups.h"
INIT_MODULE(InventoryManagementLibrary);

void RemoveItem(UObject* Object, FFrame& Stack, bool* Ret)
{
    TScriptInterface<class IFortInventoryOwnerInterface> InventoryOwner;
    FGuid ItemGuid;
    int32 Count;
    bool bForceRemoval;
    bool bForcePersistWhenEmpty;

    Stack.StepCompiledIn(&InventoryOwner);
    Stack.StepCompiledIn(&ItemGuid);
    Stack.StepCompiledIn(&Count);
    Stack.StepCompiledIn(&bForceRemoval);
    Stack.StepCompiledIn(&bForcePersistWhenEmpty);
    Stack.IncrementCode();

    auto PlayerController = (AFortPlayerControllerAthena*)InventoryOwner.ObjectPointer;
    auto Inventory = (AFortInventory*)PlayerController->WorldInventory.ObjectPointer;

    auto Item = GetItem(PlayerController->WorldInventory, ItemGuid);

    if (!Item)
        return;
    
    Item->ItemEntry.Count -= Count;
    if (!bForcePersistWhenEmpty && (Item->ItemEntry.Count <= 0 || Count < 0) || bForceRemoval)
        Remove(PlayerController->WorldInventory, Item->ItemEntry.ItemGuid);
    else
    {
        SetToDirty(PlayerController->WorldInventory, Item->ItemEntry);
    }

    *Ret = true;
}

void InventoryManagementLibrary__Init()
{
    ExecHook(UInventoryManagementLibrary::StaticClass()->FindFunction("RemoveItem"), RemoveItem);
}