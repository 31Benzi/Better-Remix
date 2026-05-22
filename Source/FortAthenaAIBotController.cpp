#include "pch.h"
#include "Shared.h"
#include "Pickups.h"
#include "Inventory.h"
#include "Looting.h"
INIT_MODULE(FortAthenaAIBotController);

void DropInventoryItems(AFortAthenaAIBotController* _this, AFortPawn* FortPawn)
{
    printf("[Runtime] DropInventoryItems\n");
    static auto ExternalShouldDropOrDestroyByItemType = (void (*)(AController*, UFortWorldItemDefinition*, bool*, bool*))(ImageBase + 0x9206ADC);
    static auto ExternalShouldAlwaysDropItemOnDeathOrLogout = (bool (*)(UFortWorldItem*, bool))(ImageBase + 0x9206ADC);
    auto Inventory = (AFortInventory*)_this->Inventory.ObjectPointer;

    TArray<UFortWorldItem*> ItemsToDrop, ItemsToDestroy;
    for (auto& Item : Inventory->Inventory.ItemInstances)
    {
        auto ItemDefinition = (UFortWorldItemDefinition*)Item->ItemEntry.ItemDefinition;

        bool bShouldDrop = ExternalShouldAlwaysDropItemOnDeathOrLogout(Item, false);
        bool bShouldDestroy;

        ExternalShouldDropOrDestroyByItemType(_this, ItemDefinition, &bShouldDrop, &bShouldDestroy);

        auto DropBehavior = GetPickupComponent(ItemDefinition)->DropBehavior;

        if (DropBehavior != EWorldItemDropBehavior::DropAsPickup && DropBehavior != EWorldItemDropBehavior::DropAsPickupEvenWhenEmpty
            && (DropBehavior != EWorldItemDropBehavior::DropAsPickupDestroyOnEmpty || Item->ItemEntry.Count <= 0))
            ItemsToDestroy.Add(Item);
        else
            ItemsToDrop.Add(Item);
    }

    for (auto& Item : ItemsToDrop)
    {
        SpawnPickup(FortPawn->K2_GetActorLocation(), Item->ItemEntry, EFortPickupSourceTypeFlag::AI, EFortPickupSpawnSource::BotElimination, (AFortPlayerPawn*)FortPawn);

        Remove(_this->Inventory, Item->ItemEntry.ItemGuid);
    }
    for (auto& Item : ItemsToDestroy)
        Remove(_this->Inventory, Item->ItemEntry.ItemGuid);

    ItemsToDrop.Free();
    ItemsToDestroy.Free();
}

void DropLootOnDeath(AFortAthenaAIBotController* _this, AFortPawn* FortPawn)
{
    auto InventoryParams = _this->CachedInventoryRuntimeParameters;

    if (InventoryParams->LootInfo.DataTable && InventoryParams->LootInfo.RowName.IsValid())
    {
        auto GetRow = (FFortAthenaAILootInfoDataTableRow * (*)(FDataTableRowHandle*, const wchar_t*))(ImageBase + 0xA76BC00);
        auto LootInfo = GetRow(&InventoryParams->LootInfo, L"AFortAthenaAIBotController::DropLootOnDeath");

        if (LootInfo)
        {
            FGameplayTagContainer TargetTags {};

            auto Interface = (UGameplayTagAssetInterface*)GetInterfaceAddress(FortPawn, UGameplayTagAssetInterface::StaticClass());
            if (Interface)
            {
                auto GetOwnedGameplayTags = (void (*)(UGameplayTagAssetInterface*, FGameplayTagContainer*))Interface->VTable[0x2];
                GetOwnedGameplayTags(Interface, &TargetTags);
                // Interface->GetOwnedGameplayTags(&TargetTags);
            }

            for (auto& Behavior : LootInfo->LootDroppingBehaviors)
            {
                if (Behavior.bShouldDropInventoryOnDeath)
                    DropInventoryItems(_this, FortPawn);

                if (Behavior.bShouldDropLootOnDeath)
                {
                    for (auto& TierGroup : Behavior.LootTiers)
                    {
                        TArray<FFortItemEntry> LootDrops {};

                        ChooseLootForContainer(LootDrops, TierGroup);

                        for (auto& ItemEntry : LootDrops)
                            SpawnPickup(FortPawn->K2_GetActorLocation(), ItemEntry, EFortPickupSourceTypeFlag::AI, EFortPickupSpawnSource::BotElimination,
                                (AFortPlayerPawn*)FortPawn);

                        LootDrops.Free();
                    }

                }
            }

            TargetTags.GameplayTags.Free();
            TargetTags.ParentTags.Free();
        }
    }
    else if (auto Pawn = FortPawn->Cast<AFortPlayerPawnAthena>(); Pawn && Pawn->bShouldDropItemsOnDeath)
        DropInventoryItems(_this, FortPawn);
}

void FortAthenaAIBotController__Init()
{
    Hook(ImageBase + 0xA778F28, DropLootOnDeath);
}
