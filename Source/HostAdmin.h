#pragma once
#include "pch.h"
#include <mutex>
#include <vector>

struct FHostAdminPlayerEntry
{
    AFortPlayerControllerAthena* Controller = nullptr;
    std::string DisplayName;
};

// Refreshed on the game thread each tick (safe for GUI to read the snapshot).
void RefreshHostAdminPlayerCache();
std::vector<FHostAdminPlayerEntry> GetHostAdminPlayerListSnapshot();

// Gives an item directly to a player's world inventory and replicates the change.
bool HostGiveItemToPlayer(AFortPlayerControllerAthena* Target, UFortItemDefinition* ItemDef, int32 Count, int32 LoadedAmmo = 0);

// Disconnects a player without crashing the gameserver (game thread only).
bool HostKickPlayer(AFortPlayerControllerAthena* Target);

// Queue actions from the GUI thread; processed on the next game tick.
void QueueHostGiveItem(AFortPlayerControllerAthena* Target, UFortItemDefinition* ItemDef, int32 Count);
void QueueHostKickPlayer(AFortPlayerControllerAthena* Target);
