#include "pch.h"
#include "Shared.h"
#include "HostAdmin.h"
#include "Inventory.h"
#include "Config.h"

static std::vector<FHostAdminPlayerEntry> GHostAdminPlayers;
static std::mutex GHostAdminMutex;

enum class EHostAdminPendingType : uint8
{
    GiveItem,
    KickPlayer
};

struct FHostAdminPendingAction
{
    EHostAdminPendingType Type = EHostAdminPendingType::GiveItem;
    AFortPlayerControllerAthena* Target = nullptr;
    UFortItemDefinition* ItemDef = nullptr;
    int32 Count = 1;
};

static std::vector<FHostAdminPendingAction> GHostAdminPendingActions;
static std::mutex GHostAdminPendingMutex;

static std::string SafePlayerDisplayName(AFortPlayerStateAthena* PlayerState)
{
    if (!PlayerState)
        return "Unknown";

    auto Id = UFortKismetLibrary::GetDebugStringForUniqueId(PlayerState->UniqueID);
    if (Id.Num() > 0)
        return std::string(Id.ToString().c_str());

    return "Player " + std::to_string(PlayerState->WorldPlayerId);
}

void RefreshHostAdminPlayerCache()
{
    std::vector<FHostAdminPlayerEntry> NewList;

    auto World = GetWorld();
    if (World && World->AuthorityGameMode)
    {
        static auto AthenaGameModeClass = FindObject<UClass>(L"/Script/FortniteGame.FortGameModeAthena");
        if (AthenaGameModeClass && World->AuthorityGameMode->IsA(AthenaGameModeClass))
        {
            auto GameMode = (AFortGameModeAthena*)World->AuthorityGameMode;

            for (int i = 0; i < GameMode->AlivePlayers.Num(); i++)
        {
            auto Player = GameMode->AlivePlayers[i];
            if (!Player || Player->bIsBeingKicked)
                continue;

            auto PlayerState = Player->PlayerState;
            if (!PlayerState || !PlayerState->IsA<AFortPlayerStateAthena>())
                continue;

            FHostAdminPlayerEntry Entry {};
            Entry.Controller = Player;
            Entry.DisplayName = SafePlayerDisplayName((AFortPlayerStateAthena*)PlayerState);
            NewList.push_back(Entry);
        }
        }
    }

    std::lock_guard<std::mutex> Lock(GHostAdminMutex);
    GHostAdminPlayers = std::move(NewList);
}

std::vector<FHostAdminPlayerEntry> GetHostAdminPlayerListSnapshot()
{
    std::lock_guard<std::mutex> Lock(GHostAdminMutex);
    return GHostAdminPlayers;
}

bool HostGiveItemToPlayer(AFortPlayerControllerAthena* Target, UFortItemDefinition* ItemDef, int32 Count, int32 LoadedAmmo)
{
    if (!Target || !ItemDef || Count <= 0 || !Target->WorldInventory.ObjectPointer)
        return false;

    int32 ClipSize = LoadedAmmo;
    if (ClipSize <= 0)
    {
        if (auto WeaponDef = ItemDef->Cast<UFortWeaponItemDefinition>())
        {
            if (auto Stats = GetStats(WeaponDef))
                ClipSize = Stats->ClipSize;
        }
    }

    GiveItem(Target->WorldInventory, ItemDef, Count, ClipSize);
    return true;
}

bool HostKickPlayer(AFortPlayerControllerAthena* Target)
{
    if (!Target || Target->bIsBeingKicked)
        return false;

    auto World = GetWorld();
    auto GameMode = World ? (AFortGameModeAthena*)World->AuthorityGameMode : nullptr;
    if (!GameMode)
        return false;

    Target->bIsBeingKicked = true;

    if (auto PlayerState = (AFortPlayerStateAthena*)Target->PlayerState)
        PlayerState->bIsDisconnected = true;

    FText Reason = UKismetTextLibrary::Conv_StringToText(FString(L"Removed by host."));
    Target->ClientWasKicked(Reason);

    if (Target->Pawn)
        Target->Pawn->K2_DestroyActor();

    GameMode->K2_OnLogout(Target);

    for (int i = 0; i < GameMode->AlivePlayers.Num(); i++)
    {
        if (GameMode->AlivePlayers[i] == Target)
        {
            GameMode->AlivePlayers.Remove(i);
            break;
        }
    }

    UGameplayStatics::RemovePlayer(Target, true);
    RefreshHostAdminPlayerCache();
    return true;
}

void QueueHostGiveItem(AFortPlayerControllerAthena* Target, UFortItemDefinition* ItemDef, int32 Count)
{
    if (!Target || !ItemDef || Count <= 0)
        return;

    std::lock_guard<std::mutex> Lock(GHostAdminPendingMutex);
    GHostAdminPendingActions.push_back({ EHostAdminPendingType::GiveItem, Target, ItemDef, Count });
}

void QueueHostKickPlayer(AFortPlayerControllerAthena* Target)
{
    if (!Target)
        return;

    std::lock_guard<std::mutex> Lock(GHostAdminPendingMutex);
    GHostAdminPendingActions.push_back({ EHostAdminPendingType::KickPlayer, Target, nullptr, 0 });
}

void HostAdmin__Tick()
{
    RefreshHostAdminPlayerCache();

    std::vector<FHostAdminPendingAction> Pending;
    {
        std::lock_guard<std::mutex> Lock(GHostAdminPendingMutex);
        Pending.swap(GHostAdminPendingActions);
    }

    for (auto& Action : Pending)
    {
        if (!Action.Target)
            continue;

        if (Action.Type == EHostAdminPendingType::GiveItem)
            HostGiveItemToPlayer(Action.Target, Action.ItemDef, Action.Count);
        else if (Action.Type == EHostAdminPendingType::KickPlayer)
            HostKickPlayer(Action.Target);
    }
}

void HostAdmin__Init()
{
}

INIT_MODULE(HostAdmin);
INIT_TICKER(HostAdmin);
