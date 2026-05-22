#pragma once
#include "pch.h"
#include <unordered_map>

inline std::unordered_map<int32, TArray<FFortLootTierData*>> TierDataMap;
inline std::unordered_map<int32, TArray<FFortLootPackageData*>> LootPackageMap;
inline FName WarmupLootTierGroup;

void ChooseLootForContainer(TArray<FFortItemEntry>& LootDrops, FName TierGroup, int LootTier = -1, int WorldLevel = ((AFortGameStateAthena*)GetWorld()->GameState)->WorldLevel,
    ABuildingContainer* Container = nullptr);

void SpawnFloorLootForContainer(UClass* ContainerType);