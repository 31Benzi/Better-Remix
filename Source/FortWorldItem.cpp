#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
INIT_MODULE(FortWorldItem);

void SetLoadedAmmo(UFortWorldItem* Item, int LoadedAmmo)
{
    auto Inventory = (AFortInventory*)Item->ItemEntry.ParentInventory.Get();

    TScriptInterface<UFortInventoryInterface> ScriptInterface;
    ScriptInterface.ObjectPointer = Inventory;
    ScriptInterface.InterfacePointer = (void*)(__int64(Inventory) + 0x2A8);

    Item->ItemEntry.LoadedAmmo = LoadedAmmo;

    SetToDirty(ScriptInterface, Item->ItemEntry);
}

void SetPhantomReserveAmmo(UFortWorldItem* Item, unsigned int PhantomReserveAmmo)
{
    auto Inventory = (AFortInventory*)Item->ItemEntry.ParentInventory.Get();

    TScriptInterface<UFortInventoryInterface> ScriptInterface;
    ScriptInterface.ObjectPointer = Inventory;
    ScriptInterface.InterfacePointer = (void*)(__int64(Inventory) + 0x2A8);

    Item->ItemEntry.PhantomReserveAmmo = PhantomReserveAmmo;

    SetToDirty(ScriptInterface, Item->ItemEntry);
}

void FortWorldItem__Init()
{
    Hook<UFortWorldItem>(uint32(0xC4), SetLoadedAmmo);
    Hook<UFortWorldItem>(uint32(0xC5), SetPhantomReserveAmmo);
}