#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
INIT_MODULE(BuildingSMActor);

const struct AttemptSpawnResources_Params
{
    TWeakObjectPtr<AFortPlayerPawn> InstigatorPawn;
    FGameplayTagContainer Tags;
    float ActualDamageDealt;
    bool bJustHitWeakspot;
};


// andew
void AttemptSpawnResources(ABuildingSMActor* _this, AttemptSpawnResources_Params Params)
{
    auto Pawn = Params.InstigatorPawn.Get();

    if (!Pawn)
        return;

    TScriptInterface<UFortInventoryInterface> InventoryInterface;

    if (auto PlayerController = Pawn->Controller->Cast<AFortPlayerController>())
        InventoryInterface = PlayerController->WorldInventory;
    else if (auto BotController = Pawn->Controller->Cast<AFortAthenaAIBotController>())
        InventoryInterface = BotController->Inventory;

    auto Inventory = (AFortInventory*)InventoryInterface.ObjectPointer;

    if (!Pawn->Controller || !Inventory)
        return;

    auto ClassData = GetSparseClassData<FBuildingSMActorClassData>(_this);
    FCurveTableRowHandle& BuildingResourceAmountOverride = ClassData->BuildingResourceAmountOverride;

    auto GameState = (AFortGameStateAthena*)GetWorld()->GameState;
    static auto Playlist = GameState->CurrentPlaylistInfo.BasePlaylist;
    static auto GameData = Playlist ? Playlist->ResourceRates.Get() : nullptr;
    if (!GameData)
        GameData = FindObject<UCurveTable>(L"/Game/Athena/Balance/DataTables/AthenaResourceRates.AthenaResourceRates");

    int ResCount = 0;

    if (BuildingResourceAmountOverride.RowName.DecryptIndex() > 0)
    {
        float Out = 0.f;
        UDataTableFunctionLibrary::EvaluateCurveTableRow(GameData, BuildingResourceAmountOverride.RowName, 0.f, nullptr, &Out, FString());

        float RC = Out / (_this->GetMaxHealth() / Params.ActualDamageDealt);

        ResCount = (int)round(RC);
    }

    if (!ResCount)
        return;

    auto Resource = UFortKismetLibrary::K2_GetResourceItemDefinition(_this->ResourceType);
    auto MaxMat = Resource->GetMaxStackSize(Pawn->AbilitySystemComponent);

    auto itemEntry = Inventory->Inventory.ReplicatedEntries.Search(
        [&](FFortItemEntry& entry)
        {
            return entry.ItemDefinition == Resource;
        });


    if (itemEntry)
    {
        auto Item = GetItem(InventoryInterface, itemEntry->ItemGuid);

        itemEntry->Count += ResCount;
        if (itemEntry->Count > MaxMat)
        {
            SpawnPickup(Pawn->K2_GetActorLocation(), (UFortItemDefinition*)Item->ItemEntry.ItemDefinition, itemEntry->Count - MaxMat, 0,
                EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset, Pawn);
            itemEntry->Count = MaxMat;
        }

        for (auto& StateValue : itemEntry->StateValues)
        {
            if (StateValue.StateType != EFortItemEntryState::ShouldShowItemToast)
                continue;

            StateValue.IntValue = 0;
            break;
        }

        Item->ItemEntry.Count = itemEntry->Count;
        SetToDirty(InventoryInterface, *itemEntry);
        Item->ItemEntry.bIsDirty = true;
    }
    else
    {
        if (ResCount > MaxMat)
        {
            SpawnPickup(Pawn->K2_GetActorLocation(), Resource, ResCount - MaxMat, 0, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset, Pawn);
            ResCount = MaxMat;
        }

        GiveItem(InventoryInterface, Resource, ResCount, 0, 0);
    }

    if (auto PlayerController = Pawn->Controller->Cast<AFortPlayerController>())
        PlayerController->ClientReportDamagedResourceBuilding(
            _this, _this->ResourceType, ResCount, _this->GetHealth() - Params.ActualDamageDealt <= 0, Params.bJustHitWeakspot);
}

void BuildingSMActor__Init()
{
    HookEvery<ABuildingSMActor>(0x1A5, AttemptSpawnResources);
}
