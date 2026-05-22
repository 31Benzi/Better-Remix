#include "pch.h"
#include "Shared.h"
#include "Looting.h"
#include <unordered_map>
#include <numeric>
#include <algorithm>
#include "Inventory.h"
#include <chrono>
INIT_MODULE(Looting);

template <typename T>
static T* PickWeighted(TArray<T*>& Map, float (*RandFunc)(float), bool bCheckZero = true)
{
    float TotalWeight = std::accumulate(Map.begin(), Map.end(), 0.0f,
        [&](float acc, T*& p)
        {
            return acc + p->Weight;
        });
    float RandomNumber = RandFunc(TotalWeight);

    for (auto& Element : Map)
    {
        float Weight = Element->Weight;
        if (bCheckZero && Weight == 0)
            continue;

        if (RandomNumber <= Weight)
            return Element;

        RandomNumber -= Weight;
    }

    return nullptr;
}

template <typename T>
static T* PickWeighted(std::vector<T*>& Map, float (*RandFunc)(float), bool bCheckZero = true)
{
    float TotalWeight = std::accumulate(Map.begin(), Map.end(), 0.0f,
        [&](float acc, T*& p)
        {
            return acc + p->Weight;
        });
    float RandomNumber = RandFunc(TotalWeight);

    for (auto& Element : Map)
    {
        float Weight = Element->Weight;
        if (bCheckZero && Weight == 0)
            continue;

        if (RandomNumber <= Weight)
            return Element;

        RandomNumber -= Weight;
    }

    return nullptr;
}

int InternalGetLevel(UFortItemDefinition* ItemLevel, FFortItemComponentData_LootLevelMapping* LevelMapping)
{
    auto GameMode = (AFortGameMode*)GetWorld()->AuthorityGameMode;
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;

    if (!LevelMapping->LootLevelCategoryHandle.DataTable)
        return 0;

    if (!LevelMapping->LootLevelCategoryHandle.ColumnName)
        return 0;

    if (!LevelMapping->LootLevelCategoryHandle.RowContents.DecryptIndex())
        return 0;

    int Level = 0;
    FFortLootLevelData* LootLevelData = nullptr;

    for (auto& LootLevelDataPair : (TMap<FName, FFortLootLevelData*>)GetRowMap(LevelMapping->LootLevelCategoryHandle.DataTable))
    {
        if (!LootLevelDataPair.Value())
            continue;

        if (LootLevelDataPair.Value()->category != LevelMapping->LootLevelCategoryHandle.RowContents || LootLevelDataPair.Value()->LootLevel > GameState->WorldLevel
            || LootLevelDataPair.Value()->LootLevel <= Level)
            continue;

        Level = LootLevelDataPair.Value()->LootLevel;
        LootLevelData = LootLevelDataPair.Value();
    }

    if (LootLevelData)
    {
        auto subbed = LootLevelData->MaxItemLevel - LootLevelData->MinItemLevel;

        if (subbed <= -1)
            subbed = 0;
        else
        {
            auto calc = (int)(((float)rand() / 32767) * (float)(subbed + 1));
            if (calc <= subbed)
                subbed = calc;
        }

        return subbed + LootLevelData->MinItemLevel;
    }

    return 0;
}

int GetLevel(UFortItemDefinition* ItemDefinition)
{
    auto GetLevelLimits = (FFortItemComponentData_LevelLimits * (*)(UFortItemDefinition*))(ImageBase + 0x3B44AE8);
    auto GetLevelMapping = (FFortItemComponentData_LootLevelMapping * (*)(UFortItemDefinition*))(ImageBase + 0x9CE5EEC);

    auto LevelLimits = GetLevelLimits(ItemDefinition);
    auto LevelMapping = GetLevelMapping(ItemDefinition);

    if (!LevelLimits || !LevelMapping)
        return 0;

    return std::clamp(InternalGetLevel(ItemDefinition, LevelMapping), LevelLimits->MinLevel, LevelLimits->MaxLevel);
}

void SetupLDSForPackage(TArray<FFortItemEntry>& LootDrops, FName Package, int i, FName TierGroup, int WorldLevel, ABuildingContainer* Container)
{
    TArray<FFortLootPackageData*> LPGroups {};

    for (auto const& Val : LootPackageMap[Package.EncryptedIndex])
    {
        if (!Val)
            continue;

        if (i != -1 && Val->LootPackageCategory != i)
            continue;

        if (WorldLevel >= 0)
        {
            if (Val->MaxWorldLevel >= 0 && WorldLevel > Val->MaxWorldLevel)
                continue;

            if (Val->MinWorldLevel >= 0 && WorldLevel < Val->MinWorldLevel)
                continue;
        }

        LPGroups.Add(Val);
    }

    if (LPGroups.Num() == 0)
        return;

    auto LootPackage = PickWeighted(LPGroups,
        [](float Total)
        {
            return ((float)rand() / 32767.f) * Total;
        });
    if (!LootPackage)
        return;

    if (LootPackage->LootPackageCall.Num() > 1)
    {
        for (int i = 0; i < LootPackage->Count; i++)
            SetupLDSForPackage(LootDrops, UKismetStringLibrary::Conv_StringToName(LootPackage->LootPackageCall), 0, TierGroup, WorldLevel, Container);

        return;
    }

    auto ItemDefinition = LootPackage->ItemDefinition.Get();

    if (!ItemDefinition)
        return;

    FFortItemEntry AmmoEntry {};

    //GetLevel(ItemDefinition);
    auto t2 = std::chrono::high_resolution_clock::now();

    if (auto WorldItemDefinition = ItemDefinition->Cast<UFortWorldItemDefinition>())
    {
        auto AmmoDef = WorldItemDefinition->GetAmmoWorldItemDefinition_BP();

        if (auto AmmoDefinition = AmmoDef->Cast<UFortAmmoItemDefinition>())
        {
            auto SpawnAmmoData = FindObject<UFortWeaponPickupSpawnAmmoData>(L"/Game/Athena/Balance/Pickups/FortWeaponPickupSpawnAmmoData.FortWeaponPickupSpawnAmmoData");

            FGameplayTagContainer AmmoTags {};

            auto Interface = (UGameplayTagAssetInterface*)GetInterfaceAddress(AmmoDefinition, UGameplayTagAssetInterface::StaticClass());
            if (Interface)
            {
                auto GetOwnedGameplayTags = (void (*)(UGameplayTagAssetInterface*, FGameplayTagContainer*))Interface->VTable[0x2];
                GetOwnedGameplayTags(Interface, &AmmoTags);
                // Interface->GetOwnedGameplayTags(&TargetTags);
            }

            float AmmoCount = 0.f;
            for (auto& AmmoCountData : SpawnAmmoData->WeaponPickupAmmoCountArray)
            {
                if (HasTag(AmmoTags, AmmoCountData.AmmoItemDefinitionTag))
                {
                    AmmoCount = EvaluateScalableFloat(AmmoCountData.SpawnCount, (float)WorldLevel);
                    break;
                }
            }

            auto Multiplier = EvaluateScalableFloat(SpawnAmmoData->DefaultWeaponAmmoMultiplier, (float)WorldLevel);

            FGameplayTagContainer WeaponTags {};

            auto Interface2 = (UGameplayTagAssetInterface*)GetInterfaceAddress(ItemDefinition, UGameplayTagAssetInterface::StaticClass());
            if (Interface2)
            {
                auto GetOwnedGameplayTags = (void (*)(UGameplayTagAssetInterface*, FGameplayTagContainer*))Interface2->VTable[0x2];
                GetOwnedGameplayTags(Interface2, &WeaponTags);
                // Interface->GetOwnedGameplayTags(&TargetTags);
            }

            for (auto& MultiplierData : SpawnAmmoData->WeaponPickupAmmoMultiplierOverrideArray)
            {
                if (HasTag(WeaponTags, MultiplierData.Tag))
                {
                    Multiplier = EvaluateScalableFloat(MultiplierData.CountMultiplier, (float)WorldLevel);
                    break;
                }
            }

            float SourceMultiplier = 1.f;

            if (Container)
            {
                FGameplayTagContainer SourceTags {};

                auto Interface3 = (UGameplayTagAssetInterface*)GetInterfaceAddress(Container, UGameplayTagAssetInterface::StaticClass());
                if (Interface3)
                {
                    auto GetOwnedGameplayTags = (void (*)(UGameplayTagAssetInterface*, FGameplayTagContainer*))Interface3->VTable[0x2];
                    GetOwnedGameplayTags(Interface3, &WeaponTags);
                    // Interface->GetOwnedGameplayTags(&TargetTags);
                }

                for (auto& MultiplierData : SpawnAmmoData->SourceToAmmoMultiplierOverrideArray)
                {
                    if (HasTag(SourceTags, MultiplierData.Tag))
                    {
                        SourceMultiplier = EvaluateScalableFloat(MultiplierData.CountMultiplier, (float)WorldLevel);
                        break;
                    }
                }
                SourceTags.GameplayTags.Free();
                SourceTags.ParentTags.Free();
            }

            auto FinalCount = AmmoCount * Multiplier * SourceMultiplier;

            if (FinalCount > 0.f)
                AmmoEntry = MakeItemEntry(AmmoDefinition, (int)FinalCount, GetLevel(AmmoDefinition));

            AmmoTags.GameplayTags.Free();
            AmmoTags.ParentTags.Free();
            WeaponTags.GameplayTags.Free();
            WeaponTags.ParentTags.Free();
        }
    }

    auto MaxStack = ItemDefinition->GetMaxStackSize(nullptr);
    bool found = false;
    bool foundAmmo = false;
    for (auto& LootDrop : LootDrops)
    {
        if (/*(!AmmoDef || AmmoDef->DropCount) && */ LootDrop.ItemDefinition == ItemDefinition && LootDrop.Count < MaxStack)
        {
            LootDrop.Count += LootPackage->Count;

            if (LootDrop.Count > MaxStack)
            {
                auto OGCount = LootDrop.Count;
                LootDrop.Count = MaxStack;

                // if (Inventory::GetQuickbar(LootDrop.ItemDefinition) == EFortQuickBars::Secondary)
                LootDrops.Add(MakeItemEntry(ItemDefinition, OGCount - (int32)MaxStack, GetLevel(ItemDefinition)));
            }

            // if (Inventory::GetQuickbar(LootDrop.ItemDefinition) == EFortQuickBars::Secondary)
            found = true;
            break;
        }

        if (AmmoEntry.ItemDefinition && LootDrop.ItemDefinition == AmmoEntry.ItemDefinition)
        {
            auto AmmoMaxStack = ((UFortItemDefinition*)AmmoEntry.ItemDefinition)->GetMaxStackSize(nullptr);
            LootDrop.Count += AmmoEntry.Count;

            if (LootDrop.Count > AmmoMaxStack)
            {
                auto OGCount = LootDrop.Count;
                LootDrop.Count = AmmoMaxStack;

                // if (!AFortInventory::IsPrimaryQuickbar(LootDrop->ItemDefinition))
                LootDrops.Add(MakeItemEntry((UFortItemDefinition*)AmmoEntry.ItemDefinition, OGCount - AmmoMaxStack, GetLevel((UFortItemDefinition*)AmmoEntry.ItemDefinition))); 
            }

            foundAmmo = true;
        }
    }

    if (!found && LootPackage->Count > 0)
        LootDrops.Add(MakeItemEntry(ItemDefinition, LootPackage->Count, GetLevel(ItemDefinition)));

    if (!foundAmmo && AmmoEntry.ItemDefinition && LootPackage->Count > 0)
        LootDrops.Add(AmmoEntry);
}

void ChooseLootForContainer(TArray<FFortItemEntry>& LootDrops, FName TierGroup, int LootTier, int WorldLevel, ABuildingContainer* Container)
{
    TArray<FFortLootTierData*> TierDataGroups {};

    for (auto const& Val : TierDataMap[TierGroup.EncryptedIndex])
        if (LootTier == -1 ? true : LootTier == Val->LootTier)
            TierDataGroups.Add(Val);

    auto LootTierData = PickWeighted(TierDataGroups,
        [](float Total)
        {
            return ((float)rand() / 32767.f) * Total;
        });

    if (!LootTierData)
        return;

    if (LootTierData->NumLootPackageDrops <= 0)
        return;

    int DropCount;
    if (LootTierData->NumLootPackageDrops < 1.f)
        DropCount = 1;
    else
    {
        DropCount = (int)((LootTierData->NumLootPackageDrops * 2.f) - .5f) >> 1;

        float RemainderSomething = LootTierData->NumLootPackageDrops - (float)DropCount;

        if (RemainderSomething > 0.0000099999997f)
            DropCount += RemainderSomething >= ((float)rand() / 32767.f);
    }

    float AmountOfLootDrops = 0;
    float MinLootDrops = 0;

    std::unordered_map<int, int> NumMap;

    for (int i = 0; i < LootTierData->LootPackageCategoryMinArray.Num(); i++)
    {
        NumMap[i] = LootTierData->LootPackageCategoryMinArray[i];
        AmountOfLootDrops += LootTierData->LootPackageCategoryMinArray[i];
    }

    int SumWeights = 0;
    std::unordered_map<int, int> WeightMap;

    for (int i = 0; i < LootTierData->LootPackageCategoryWeightArray.Num(); i++)
    {
        if (LootTierData->LootPackageCategoryWeightArray[i] > 0 && (LootTierData->LootPackageCategoryMaxArray[i] < 0 || NumMap[i] < LootTierData->LootPackageCategoryMaxArray[i]))
        {
            WeightMap[i] = LootTierData->LootPackageCategoryWeightArray[i];
            SumWeights += LootTierData->LootPackageCategoryWeightArray[i];
        }
    }

    if (AmountOfLootDrops < DropCount)
        while (SumWeights > 0)
        {
            auto RandomValue = (float)rand() / 32767.f;
            auto RandomWeight = (int)std::floor(RandomValue * SumWeights);

            int Category = -1;
            for (auto& [DropCategory, Weight] : WeightMap)
            {
                if (RandomWeight <= Weight && RandomWeight <= LootTierData->NumLootPackageDrops)
                {
                    Category = DropCategory;
                    break;
                }

                RandomWeight -= Weight;
            }

            if (Category != -1)
            {
                AmountOfLootDrops++;
                NumMap[Category]++;

                if (LootTierData->LootPackageCategoryMaxArray[Category] >= 0 && NumMap[Category] >= LootTierData->LootPackageCategoryMaxArray[Category])
                {
                    SumWeights -= LootTierData->LootPackageCategoryWeightArray[Category];
                    WeightMap.erase(Category);
                }

                if (AmountOfLootDrops >= DropCount)
                    break;
            }
        }

    /*if (AmountOfLootDrops < DropCount)
            while (SumWeights > 0)
            {
                    AmountOfLootDrops++;

                    if (AmountOfLootDrops >= DropCount)
                    {
                            //AmountOfLootDrops = AmountOfLootDrops;
                            break;
                    }

                    SumWeights--;
            }

    if (!AmountOfLootDrops)
            return {};*/

    LootDrops.Reserve((int)DropCount);

    int SpawnedItems = 0;
    int CurrentCategory = 0;
    while (SpawnedItems < DropCount && CurrentCategory < NumMap.size())
    {
        for (int j = 0; j < NumMap[CurrentCategory]; j++)
            SetupLDSForPackage(LootDrops, LootTierData->LootPackage, CurrentCategory, TierGroup, WorldLevel, Container);

        SpawnedItems += NumMap[CurrentCategory];
        CurrentCategory++;
    }
    TierDataGroups.Free();
}

bool SpawnLoot(ABuildingContainer* _this)
{
    if (_this->bAlreadySearched)
        return false;

    auto GameMode = (AFortGameModeAthena*)GetWorld()->AuthorityGameMode;
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;
    auto RealTierGroup = _this->SearchLootTierGroup;

    for (const auto& [OldTierGroup, RedirectedTierGroup] : GameMode->RedirectAthenaLootTierGroups)
    {
        if (OldTierGroup == _this->SearchLootTierGroup)
        {
            RealTierGroup.EncryptedIndex = RedirectedTierGroup.EncryptedIndex;
            break;
        }
    }

    TArray<FFortItemEntry> LootDrops {};

    ChooseLootForContainer(LootDrops, RealTierGroup, _this->ReplicatedLootTier, GameState->WorldLevel, _this);

    for (auto& LootDrop : LootDrops)
        SpawnPickup(_this, LootDrop);

    LootDrops.Free();

    _this->bAlreadySearched = true;
    _this->OnRep_bAlreadySearched();
    _this->SearchBounceData.SearchAnimationCount++;
    _this->BounceContainer();
    // if (Container->bDestroyContainerOnSearch)
    //	Container->K2_DestroyActor();

    return true;
}

void SpawnFloorLootForContainer(UClass* ContainerType)
{
    TArray<AActor*> Containers;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ContainerType, &Containers);
    //Utils::GetAll<ABuildingContainer>(ContainerType, Containers);

    for (auto& BuildingContainer : Containers)
        BuildingContainer->K2_DestroyActor();

    Containers.Free();
}


void (*OnAuthorityRandomUpgradeAppliedOG)(ABuildingContainer*, FName&);
void OnAuthorityRandomUpgradeApplied(ABuildingContainer* Container, FName& UpgradeTierGroup)
{

    auto ChosenRandomUpgrade = Container->ChosenRandomUpgrade;

    auto ClassData = GetSparseClassData<FBuildingSMActorClassData>(Container);
    if (ChosenRandomUpgrade < 0 || ChosenRandomUpgrade >= ClassData->AlternateMeshes.Num())
        return OnAuthorityRandomUpgradeAppliedOG(Container, UpgradeTierGroup);

    auto& AlternateMeshSet = ClassData->AlternateMeshes[ChosenRandomUpgrade];

    Container->ReplicatedLootTier = AlternateMeshSet.Tier;
    Container->OnRep_LootTier();

    return OnAuthorityRandomUpgradeAppliedOG(Container, UpgradeTierGroup);
}

void Looting__Init()
{
    Hook(ImageBase + 0x94C7DCC, OnAuthorityRandomUpgradeApplied, OnAuthorityRandomUpgradeAppliedOG);
    Hook(ImageBase + 0x94CCD48, SpawnLoot);
}
