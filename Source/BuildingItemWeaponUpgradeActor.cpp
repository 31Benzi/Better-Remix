#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
INIT_MODULE(BuildingItemWeaponUpgradeActor);

int GetUpgradeCost(UFortItemDefinition* UpgradedDef)
{
    if (!UpgradedDef)
        return 200;

    switch (UpgradedDef->Rarity)
    {
    case EFortRarity::Uncommon:
        return 50;
    case EFortRarity::Rare:
        return 100;
    case EFortRarity::Epic:
        return 150;
    case EFortRarity::Legendary:
        return 200;
    default:
        return 200;
    }
}

void GrantOutput(ABuildingItemWeaponUpgradeActor* _this, AFortPlayerController* LootingController, UFortWorldItemDefinition* InInputItem)
{
    if (!LootingController || !InInputItem)
        return;

    auto PlayerController = (AFortPlayerControllerAthena*)LootingController;
    auto Pawn = PlayerController->MyFortPawn;
    auto Inventory = (AFortInventory*)PlayerController->WorldInventory.ObjectPointer;
    if (!Inventory)
        return;

    int OldLoadedAmmo = 0;
    int OldPhantomReserve = 0;

    if (Pawn && Pawn->CurrentWeapon)
    {
        auto OldWeapon = Pawn->CurrentWeapon;

        auto OldEntry = GetReplicatedItemEntry(Inventory, OldWeapon->ItemEntryGuid);

        if (OldEntry)
        {
            OldLoadedAmmo = OldEntry->LoadedAmmo;
            OldPhantomReserve = OldEntry->PhantomReserveAmmo;
        }

        Remove(PlayerController->WorldInventory, OldWeapon->ItemEntryGuid);
    }

    int Cost = GetUpgradeCost((UFortItemDefinition*)InInputItem);

    auto DeductResource = [&](UFortResourceItemDefinition* ResourceDef, int Amount)
        {
            if (!ResourceDef || Amount <= 0)
                return;

            auto ResEntry = Inventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
                {
                    return entry.ItemDefinition == (UItemDefinitionBase*)ResourceDef;
                });

            if (!ResEntry)
                return;

            auto ResInstance = GetItem(PlayerController->WorldInventory, ResEntry->ItemGuid);

            ResEntry->Count -= Amount;

            if (ResEntry->Count <= 0)
            {
                Remove(PlayerController->WorldInventory, ResEntry->ItemGuid);
            }
            else
            {
                if (ResInstance)
                    ResInstance->ItemEntry.Count = ResEntry->Count;
                SetToDirty(PlayerController->WorldInventory, *ResEntry);
            }
        };

    DeductResource(_this->WoodItem, Cost);
    DeductResource(_this->MetalItem, Cost);
    DeductResource(_this->BrickItem, Cost);

    GiveItem(PlayerController->WorldInventory, (UFortItemDefinition*)InInputItem, 1, OldLoadedAmmo, 0, true, OldPhantomReserve);
}

void BuildingItemWeaponUpgradeActor__Init()
{
    Hook<ABuildingItemWeaponUpgradeActor>(uint32(0x194), GrantOutput);
}
