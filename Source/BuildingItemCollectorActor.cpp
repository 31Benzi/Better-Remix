#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
INIT_MODULE(BuildingItemCollectorActor);

void GrantOutput(ABuildingItemCollectorActor* _this, AFortPlayerController* LootingController, UFortWorldItemDefinition* InInputItem)
{
    auto GetDepositGoal = (int (*)(ABuildingItemCollectorActor*, AFortPlayerController*, UFortWorldItemDefinition*))(ImageBase + 0x34AA988);
    auto DepositGoal = GetDepositGoal(_this, LootingController, InInputItem);

    if (UFortKismetLibrary::K2_RemoveItemFromPlayer(LootingController, InInputItem, FGuid(), DepositGoal, false) <= 0)
        return;

    auto Collection = _this->ItemCollections.Search(
        [&](FCollectorUnitInfo& Coll)
        {
            return Coll.InputItem == InInputItem;
        });

    auto VMLoc = _this->K2_GetActorLocation();
    auto& SpawnLocation = _this->LootSpawnLocation;
    auto Loc = VecAdd(VecAdd(VecAdd(VMLoc, VecMul(_this->GetActorForwardVector(), SpawnLocation.X)), VecMul(_this->GetActorRightVector(), SpawnLocation.Y)),
        VecMul(_this->GetActorUpVector(), SpawnLocation.Z));

    for (auto& Item : Collection->OutputItemEntry)
    {
        SpawnPickup(Loc, Item);

        _this->PickupSpawned.Process();
    }
}

void BuildingItemCollectorActor__Init()
{
    Hook<ABuildingItemCollectorActor>(uint32(0x194), GrantOutput);

    Patch<uint32_t>(ImageBase + 0x950A242, 0x90909090);
    Patch<uint16_t>(ImageBase + 0x950A246, 0x9090);
}
