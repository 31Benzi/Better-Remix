#include "pch.h"
#include "Shared.h"
#include "Looting.h"
#include "Inventory.h"
#include <numeric>
#include <random>
#include "CurlHttp.h"
#include "Config.h"
#include "GUI.h"
#include "FloorLootLocations.h"

INIT_MODULE(GameMode);

void SpawnAI(const wchar_t* Path, AFortAthenaPatrolPath* PatrolPath)
{
    //printf("SpawnAI %ls\n", Path);
    auto Class = FindObject<UBlueprintGeneratedClass>(Path);
    auto SpawnerData = (UFortAthenaAIBotSpawnerData*)Class->DefaultObject;
    // auto SpawnerTag = SpawnerData->GetTyped()->DescriptorTag.GameplayTags[0].TagName;
    // auto SpawnerTagName = SpawnerTag.ToWString();
    // auto NPCName = SpawnerTagName.substr(SpawnerTagName.rfind(L'.') + 1);

    auto ComponentList = UFortAthenaAISpawnerData::CreateComponentListFromClass(Class, GetWorld());

    auto Transform = PatrolPath->PatrolPoints[0]->GetTransform();
    ((UAthenaAISystem*)GetWorld()->AISystem)->AISpawner->RequestSpawn(ComponentList, Transform, true);
}


bool ReadyToStartMatch(AFortGameModeAthena* _this)
{
    auto GameState = (AFortGameStateAthena*)_this->GameState;

    if (_this->WarmupRequiredPlayerCount != 1)
    {
        auto PlaylistObj = FindObject<UFortPlaylistAthena>(bEvent ? L"/QuailPlaylist/Playlist/Playlist_Quail.Playlist_Quail" : Playlist.c_str());

        if (!PlaylistObj)
        {
            printf("[Remix] Warning: Configured playlist could not be found! Falling back to /BRPlaylists/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo...\n");
            PlaylistObj = FindObject<UFortPlaylistAthena>(L"/BRPlaylists/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo");
        }

        if (!PlaylistObj)
        {
            printf("[Remix] Warning: Falling back to /Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo...\n");
            PlaylistObj = FindObject<UFortPlaylistAthena>(L"/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo");
        }

        if (!PlaylistObj)
        {
            printf("[Remix] Error: Critical - No valid playlist object could be loaded from memory! Match will not start.\n");
            return false;
        }

        auto Playlist = PlaylistObj;

        //static auto PlaylistObj = FindObject<UFortPlaylistAthena>(L"/BlastBerry/Playlists/Playlist_SunflowerSolo.Playlist_SunflowerSolo"); 
        //static auto PlaylistObj = FindObject<UFortPlaylistAthena>(L"/BlastBerry/Playlists/Playlist_PunchBerrySolo.Playlist_PunchBerrySolo"); 
        //static auto PlaylistObj = FindObject<UFortPlaylistAthena>(L"/Rumble/Playlists/Playlist_Respawn_24.Playlist_Respawn_24"); 
        auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(_this);

        _this->WarmupRequiredPlayerCount = 1;

        GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
        //GameState->CurrentPlaylistInfo.OverridePlaylist = PlaylistObj;
        GameState->CurrentPlaylistInfo.BasePlaylist->GarbageCollectionFrequency = 9999999999999999.f;

        GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
        if (GameState->CurrentPlaylistInfo.PlaylistReplicationKey == -1)
            GameState->CurrentPlaylistInfo.PlaylistReplicationKey = 0;
        //GameState->CurrentPlaylistInfo.MarkArrayDirty();

        GameState->CurrentPlaylistId = _this->CurrentPlaylistId = PlaylistObj->PlaylistId;
        _this->CurrentPlaylistName = Playlist->PlaylistName;
        GameState->OnRep_CurrentPlaylistInfo();
        GameState->OnRep_CurrentPlaylistId();

        _this->GameSession->MaxPlayers = Playlist->MaxPlayers;

        GamePhaseLogic->AirCraftBehavior = Playlist->AirCraftBehavior;
        GameState->WorldLevel = Playlist->LootLevel;

        GamePhaseLogic->bGameModeWillSkipAircraft = Playlist->bSkipAircraft;

        _this->AISettings = Playlist->AISettings.Get();

        if (_this->AISettings && !_this->AISettings->AIServices[1])
            _this->AISettings->AIServices[1] = UAthenaAIServicePlayerBots::StaticClass();

        ((UAthenaAISystem*)GetWorld()->AISystem)->AIPopulationTracker = (UAthenaAIPopulationTracker*)UGameplayStatics::SpawnObject(UAthenaAIPopulationTracker::StaticClass(), GetWorld()->AISystem);

        _this->DefaultPawnClass = FindObject<UClass>(L"/Game/Athena/PlayerPawn_Athena.PlayerPawn_Athena_C");

        auto MeshNetworkSubsystem = (UMeshNetworkSubsystem*)TUObjectArray::FindFirstObject("MeshNetworkSubsystem");

        if (MeshNetworkSubsystem)
            MeshNetworkSubsystem->NodeType = EMeshNetworkNodeType::Edge;

        int TickRate = 30;

        switch (Playlist->ServerMaxTickRate)
        {
        case EFortServerTickRate::Twenty:
            TickRate = 20;
            break;
        case EFortServerTickRate::Sixty:
            TickRate = 60;
            break;
        }
        GetWorld()->NetDriver->NetServerMaxTickRate = TickRate;

        
        if (_this->AISettings && _this->AISettings->bAllowAIDirector)
        {
            _this->AIDirector = SpawnActor<AAthenaAIDirector>({});
            _this->AIDirector->MaxActiveAlive = _this->MaxActiveAIActors;
            _this->AIDirector->Activate();
        }

        //FTransform Trans;
        //GameState->AffiliationManager = (UFortGameStateComponent_AffiliationManager*) GameState->AddComponentByClass(UFortGameStateComponent_AffiliationManager::StaticClass(), true, Trans, false);

        auto HoagieID = FindObject<UFortVehicleItemDefinition>(L"/Clyde/Valet/Hoagie/VID_HoagieVehicle_Clyde.VID_HoagieVehicle_Clyde");

        if (HoagieID)
        {
            HoagieID->VehicleMinSpawnPercent.Value = 100.f;
            HoagieID->VehicleMinSpawnPercent.Curve.CurveTable = nullptr;

            HoagieID->VehicleMaxSpawnPercent.Value = 100.f;
            HoagieID->VehicleMaxSpawnPercent.Curve.CurveTable = nullptr;
        }

        return false;
    }

    static auto WaitingToStart = FName(L"WaitingToStart");
    auto CountReadyPlayers = (int32_t (*)(AFortGameModeAthena*))(ImageBase + 0x917E138);
    auto AreAllAdditionalPlaylistLevelsVisible = (bool (*)(AFortGameStateAthena*))(ImageBase + 0x9A52B40);

    TArray<FPlaylistStreamedLevelData>& AdditonalPlaylistLevels = *(TArray<FPlaylistStreamedLevelData>*)(__int64(GameState) + 0x430);

    bool bAllLevelsFinishedStreaming = true;
    for (auto& Something : AdditonalPlaylistLevels)
    {
        if (!Something.bIsFinishedStreaming)
        {
            bAllLevelsFinishedStreaming = false;
            break;
        }
    }

    return _this->bWorldIsReady && GameState->bPlaylistDataIsLoaded && _this->MatchState == WaitingToStart && bAllLevelsFinishedStreaming && !GameState->bInSpawningStartup
        && AreAllAdditionalPlaylistLevelsVisible(GameState) && CountReadyPlayers(_this) >= _this->WarmupRequiredPlayerCount;
    //return _this->AlivePlayers.Num() > 0;
}

APawn* SpawnDefaultPawnFor(AFortGameModeAthena* _this, AFortPlayerControllerAthena* NewPlayer, AActor* StartSpot)
{
    static auto SpawnDefaultPawnFor_ = (APawn * (*)(AFortGameModeAthena*, AFortPlayerControllerAthena*, AActor*))(AFortGameMode::GetDefaultObj()->VTable[0xE8]);

    auto Pawn = SpawnDefaultPawnFor_(_this, NewPlayer, StartSpot);
    
    if (!Pawn)
    {
        Pawn = (AFortPlayerPawnAthena*)SpawnActor(
            _this->GetDefaultPawnClassForController(NewPlayer), StartSpot->GetTransform(), NewPlayer, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

        if (!Pawn)
        {
            auto PlayerStart = _this->ChoosePlayerStart(NewPlayer);
            if (PlayerStart)
                Pawn = (AFortPlayerPawnAthena*)SpawnActor(
                    _this->GetDefaultPawnClassForController(NewPlayer), PlayerStart->GetTransform(), NewPlayer, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
        }
    }

    return Pawn;
}

void (*FinishWorldInitializationOG)(AFortGameModeAthena* GameMode);
void FinishWorldInitialization(AFortGameModeAthena* _this)
{
    //_this->bWorldIsReady = true;
    FinishWorldInitializationOG(_this);

    GUI::gsStatus = Joinable;

    auto GameState = (AFortGameStateAthena*)_this->GameState;


    auto PlaylistObj = GameState->CurrentPlaylistInfo.BasePlaylist;

    auto AddToTierData = [&](UDataTable* Table, UEAllocatedVector<FFortLootTierData*>& TempArr)
    {
        printf("LootTierData: %s\n", Table->Name.ToString().c_str());

        if (auto CompositeTable = Table->Cast<UCompositeDataTable>())
            for (auto& ParentTable : CompositeTable->ParentTables)
                if (ParentTable)
                    for (auto& [Key, Val] : (TMap<FName, FFortLootTierData*>)GetRowMap(ParentTable))
                    {
                        TempArr.push_back(Val);
                    }

        for (auto& [Key, Val] : (TMap<FName, FFortLootTierData*>)GetRowMap(Table))
        {
            TempArr.push_back(Val);
        }
    };

    auto AddToPackages = [&](UDataTable* Table, UEAllocatedVector<FFortLootPackageData*>& TempArr)
    {
        printf("LootPackages: %s\n", Table->Name.ToString().c_str());

        if (auto CompositeTable = Table->Cast<UCompositeDataTable>())
            for (auto& ParentTable : CompositeTable->ParentTables)
                if (ParentTable)
                    for (auto& [Key, Val] : (TMap<FName, FFortLootPackageData*>)GetRowMap(ParentTable))
                    {
                        TempArr.push_back(Val);
                    }

        for (auto& [Key, Val] : (TMap<FName, FFortLootPackageData*>)GetRowMap(Table))
        {
            TempArr.push_back(Val);
        }
    };


    UEAllocatedVector<FFortLootTierData*> LootTierDataTempArr;
    auto LootTierData = PlaylistObj->LootTierData.Get();
    //if (!LootTierData)
    //    LootTierData = FindObject<UDataTable>(L"/Game/Items/Datatables/AthenaLootTierData_Client.AthenaLootTierData_Client");
    if (!LootTierData)
        for (auto& LootTierDataBR : GetGameData()->LootTierDataTablesBR)
            AddToTierData(LootTierDataBR.Get(), LootTierDataTempArr);
    else
        AddToTierData(LootTierData, LootTierDataTempArr);
    for (auto& Val : LootTierDataTempArr)
        TierDataMap[Val->TierGroup.EncryptedIndex].Add(Val);

    UEAllocatedVector<FFortLootPackageData*> LootPackageTempArr;
    auto LootPackages = PlaylistObj->LootPackages.Get();
    //if (!LootPackages)
    //    LootPackages = FindObject<UDataTable>(L"/Game/Items/Datatables/AthenaLootPackages_Client.AthenaLootPackages_Client");
    if (!LootPackages)
        for (auto& LootPackagesBR : GetGameData()->LootPackageDataTablesBR)
            AddToTierData(LootPackagesBR.Get(), LootTierDataTempArr);
    else
        AddToPackages(LootPackages, LootPackageTempArr);
    for (auto& Val : LootPackageTempArr)
        LootPackageMap[Val->LootPackageID.EncryptedIndex].Add(Val);

    for (uint32_t i = 0; i < TUObjectArray::Num(); i++)
    {
        auto Object = TUObjectArray::GetObjectByIndex(i);

        if (!Object || !Object->Class || Object->ObjectFlags & 0x10)
            continue;

        if (auto StateMachine = Object->Cast<UGameFeaturePluginStateMachine>())
        {
            auto GetFeatureDataIfActive = (UFortGameFeatureData * (*)(UGameFeaturePluginStateMachine*))(ImageBase + 0x1862848);

            auto GameFeatureData = GetFeatureDataIfActive(StateMachine);

            if (!GameFeatureData->Cast<UFortGameFeatureData>())
                continue;

            auto LootTableData = GameFeatureData->DefaultLootTableData;
            auto LTDFeatureData = LootTableData.LootTierData.Get();
            auto AbilitySet = GameFeatureData->PlayerAbilitySet.Get();
            auto LootPackageData = LootTableData.LootPackageData.Get();

            if (LTDFeatureData)
            {
                UEAllocatedVector<FFortLootTierData*> LTDTempData;

                AddToTierData(LTDFeatureData, LTDTempData);

                for (auto& Tag : PlaylistObj->GameplayTagContainer.GameplayTags)
                    for (auto& Override : GameFeatureData->PlaylistOverrideLootTableData)
                        if (Tag.TagName == Override.Second.TagName)
                            AddToTierData(Override.First.LootTierData.Get(), LTDTempData);

                for (auto& Val : LTDTempData)
                    TierDataMap[Val->TierGroup.EncryptedIndex].Add(Val);
            }
            if (LootPackageData)
            {
                UEAllocatedVector<FFortLootPackageData*> LPTempData;

                AddToPackages(LootPackageData, LPTempData);

                for (auto& Tag : PlaylistObj->GameplayTagContainer.GameplayTags)
                    for (auto& Override : GameFeatureData->PlaylistOverrideLootTableData)
                        if (Tag.TagName == Override.Second.TagName)
                            AddToPackages(Override.First.LootPackageData.Get(), LPTempData);

                for (auto& Val : LPTempData)
                    LootPackageMap[Val->LootPackageID.EncryptedIndex].Add(Val);
            }
        }
    }

    {
        auto WarmupClass = FindObject<UClass>(L"/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_Warmup.Tiered_Athena_FloorLoot_Warmup_C");
        TArray<AActor*> WarmupContainers;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), WarmupClass, &WarmupContainers);
        if (WarmupContainers.Num() > 0)
        {
            WarmupLootTierGroup = ((ABuildingContainer*)WarmupContainers[0])->SearchLootTierGroup;
        }

        WarmupContainers.Free();
        SpawnFloorLootForContainer(WarmupClass);
    }

    SpawnFloorLootForContainer(FindObject<UClass>(L"/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_01.Tiered_Athena_FloorLoot_01_C"));

    if (GameState->CurrentPlaylistInfo.BasePlaylist->GameType == EFortGameType::BlastBerry)
    {
        int Spawned = 0;
        for (auto& Loc : ReloadFloorLootPositions)
        {
            TArray<FFortItemEntry> LootDrops{};
            ChooseLootForContainer(LootDrops, WarmupLootTierGroup);

            for (auto& Drop : LootDrops)
                SpawnPickup(Loc, Drop);

            Spawned += LootDrops.Num();
            LootDrops.Free();
        }
    }

    GameState->DefaultParachuteDeployTraceForGroundDistance = 10000;

    if (GameState->MapInfo)
    {
        auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(_this);

        auto& SafeZoneLocations = *(TArray<FVector>*)(__int64(GamePhaseLogic) + 0x458);

        auto SafeZoneBlacklist = PlaylistObj->SafeZoneLocationBlacklist.Get();

        if (!SafeZoneBlacklist)
            SafeZoneBlacklist = FindObject<UCurveTable>(L"/Game/Athena/Balance/DataTables/AthenaSafeZoneBlacklist.AthenaSafeZoneBlacklist");

        auto& SZBCurve = *(TMap<FName, FRealCurve*>*)(__int64(SafeZoneBlacklist) + 0x30);

        TArray<FVector4> BlacklistLocations;

        for (auto& [Key, Curve] : SZBCurve)
        {
            FSimpleCurve* Row = (FSimpleCurve*)Curve;

            if (!Row)
                continue;

            FVector4 Loc {};

            for (auto& Key : Row->Keys)
            {
                if (Key.time == 0.f)
                    Loc.X = Key.Value;
                else if (Key.time == 1.f)
                    Loc.Y = Key.Value;
                else if (Key.time == 2.f)
                    Loc.Z = Key.Value;
                else if (Key.time == 3.f)
                    Loc.W = Key.Value;
            }

            if (Loc.X == 0 && Loc.Y == 0 && Loc.Z == 0 && Loc.W == 0)
                continue;

            BlacklistLocations.Add(Loc);
        }

        auto ZeroVector = FVector(0, 0, 0);
        /*auto SafeZoneCount = (float)PlaylistObj->LastSafeZoneIndex;
        if (SafeZoneCount == -1)
            SafeZoneCount = EvaluateScalableFloat(GameState->MapInfo->SafeZoneDefinition.Count);
        else
            SafeZoneCount++;*/
        auto SafeZoneCount = EvaluateScalableFloat(GameState->MapInfo->SafeZoneDefinition.Count);

        auto Center = GameState->MapInfo->GetMapCenter();
        // while (true)
        //     ;
        SafeZoneLocations.Clear();
        SafeZoneLocations.Reserve((int)SafeZoneCount);

        // printf("[GameModeModule] SafeZoneCount: %lf\n", SafeZoneCount);
        for (int i = (int)(SafeZoneCount - 1); i >= 0; i--)
        {
            auto Params = GameState->MapInfo->ConstructSafeZoneLocationParams(i, Center, i == SafeZoneCount - 1 ? ZeroVector : SafeZoneLocations[i + 1], i == SafeZoneCount - 1, 0);

            auto Location = GameState->MapInfo->PickSafeZoneLocation(Params, BlacklistLocations);

            SafeZoneLocations[i] = Location;
            // printf("Center %d %lf %lf %lf\n", (int)i, Location.X, Location.Y, Location.Z);
        }

        *(bool*)(__int64(GamePhaseLogic) + 0x468) = true;

        auto InitializeFlightPath
            = (void (*)(AFortAthenaMapInfo*, AFortGameStateAthena*, UFortGameStateComponent_BattleRoyaleGamePhaseLogic*, bool, double, float, float))(ImageBase + 0x9101F80);

        if (InitializeFlightPath)
            InitializeFlightPath(GameState->MapInfo, GameState, GamePhaseLogic, false, 0.f, 0.f, 360.f);

        std::unordered_map<int, float> WeightMap;
        float Sum = 0;
        float Weight;
        float TotalWeight;

        auto GameData = PlaylistObj ? PlaylistObj->GameData.Get() : nullptr;
        if (!GameData)
            GameData = FindObject<UCurveTable>(L"/Game/Athena/Balance/DataTables/AthenaGameData.AthenaGameData");

        for (int i = 0; i < 6; i++)
        {
            float Weight;
            UDataTableFunctionLibrary::EvaluateCurveTableRow(
                GameState->MapInfo->VendingMachineRarityCount.Curve.CurveTable, GameState->MapInfo->VendingMachineRarityCount.Curve.RowName, (float)i, nullptr, &Weight, FString());

            WeightMap[i] = Weight;
            Sum += Weight;
        }

        UDataTableFunctionLibrary::EvaluateCurveTableRow(
            GameState->MapInfo->VendingMachineRarityCount.Curve.CurveTable, GameState->MapInfo->VendingMachineRarityCount.Curve.RowName, 0.f, nullptr, &Weight, FString());

        TotalWeight = std::accumulate(WeightMap.begin(), WeightMap.end(), 0.0f,
            [&](float acc, const std::pair<int, float>& p)
            {
                return acc + p.second;
            });

        TArray<AActor*> Collectors {};

        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuildingItemCollectorActor::StaticClass(), &Collectors);

        for (auto& CollectorActor_ : Collectors)
        {
            auto CollectorActor = (ABuildingItemCollectorActor*)CollectorActor_;

            if (Sum > Weight)
            {
            PickNum:
                auto RandomNum = (float)rand() / (RAND_MAX / TotalWeight);

                int Rarity = 0;
                bool found = false;

                for (auto& Element : WeightMap)
                {
                    float Weight = Element.second;

                    if (Weight == 0)
                        continue;

                    if (RandomNum <= Weight)
                    {
                        Rarity = Element.first;

                        found = true;
                        break;
                    }

                    RandomNum -= Weight;
                }

                if (!found)
                    goto PickNum;

                if (Rarity == 0)
                {
                    CollectorActor->K2_DestroyActor();
                    continue;
                }

                int AttemptsToGetItem = 0;
                for (int i = 0; i < CollectorActor->ItemCollections.Num(); i++)
                {
                    if (AttemptsToGetItem > 10)
                    {
                        AttemptsToGetItem = 0;
                        goto PickNum;
                    }

                    auto& Collection = CollectorActor->ItemCollections[i];

                    if (Collection.bUseDefinedOutputItem)
                        continue;

                    TArray<FFortItemEntry> LootDrops {};

                    ChooseLootForContainer(LootDrops, CollectorActor->DefaultItemLootTierGroupName, Rarity);

                    if (Collection.OutputItemEntry.Num() > 0)
                    {
                        Collection.OutputItemEntry.ResetNum();
                        Collection.OutputItem = nullptr;
                    }

                    for (auto& LootDrop : LootDrops)
                    {
                        if (!Collection.OutputItem && IsPrimaryQuickbar((UFortItemDefinition*)LootDrop.ItemDefinition))
                        {
                            Collection.OutputItem = (UFortWorldItemDefinition*)LootDrop.ItemDefinition;
                        }

                        Collection.OutputItemEntry.Add(LootDrop);
                    }

                    LootDrops.Free();

                    if (!Collection.OutputItem)
                    {
                        i--;
                        AttemptsToGetItem++;

                        continue;
                    }

                    AttemptsToGetItem = 0;
                }

                CollectorActor->StartingGoalLevel = Rarity;
            }
            else
                CollectorActor->K2_DestroyActor();
        }
        Collectors.Free();

        FFortSafeZoneDefinition& SafeZoneDefinition = GameState->MapInfo->SafeZoneDefinition;

        auto LlamaClass = FindObject<UClass>(L"/Clyde/Assets/Clyde_AthenaSupplyDrop_Llama.Clyde_AthenaSupplyDrop_Llama_C");
        auto LlamaMin = EvaluateScalableFloat(GameState->MapInfo->LlamaQuantityMin);
        auto LlamaMax = EvaluateScalableFloat(GameState->MapInfo->LlamaQuantityMax);
        auto LlamaCount = UKismetMathLibrary::RandomIntegerInRange((int)LlamaMin, (int)LlamaMax);
        auto Radius = EvaluateScalableFloat(SafeZoneDefinition.Radius);

        if (Radius == 0)
            Radius = 120000;

        auto SpawnSupplyDropWithinCircle
            = (void (*)(AFortAthenaMapInfo*, FVector& CircleCenter, double CircleRadius, UClass* InSupplyDropClass, float TraceStartZ, float TraceEndZ))(ImageBase + 0x910A478);

        for (int i = 0; i < LlamaCount; i++)
        {
            SpawnSupplyDropWithinCircle(GameState->MapInfo, Center, Radius, LlamaClass, -1, -1);
            /*auto Loc = GameState->MapInfo->PickSupplyDropLocation(Center, Radius, false, -1, -1, true);

            if (Loc.X != 0 || Loc.Y != 0 || Loc.Z != 0)
            {
                FRotator Rot {};
                Rot.Yaw = (float)rand() * 0.010986663f;

                auto NewLlama = (AFortAthenaSupplyDrop*) SpawnActorUnfinished(LlamaClass, Loc, Rot);

                auto GroundLoc = NewLlama->FindGroundLocationAt(Loc);

                FinishSpawnActor(NewLlama, GroundLoc, Rot);

                NewLlama->InitializeKismetSpawnedBuildingActor(NewLlama, nullptr, false, nullptr, false);
            }*/
        }
    }

    auto AIServicePlayerBots = UAthenaAIBlueprintLibrary::GetAIServicePlayerBots(GameState);

    if (AIServicePlayerBots)
    {
        *(bool*)(__int64(AIServicePlayerBots) + 0x930) = true;
    }

    _this->bWorldIsReady = true;

    if (bProd)
    {
        if (bInit)
            gsSocket->send("{\"type\":\"ready\",\"payload\":{}}");
        else
            bReady = true;
    }
}

void (*HandleMatchHasStartedOG)(AFortGameMode* GameMode);
void HandleMatchHasStarted(AFortGameMode* _this)
{
    HandleMatchHasStartedOG(_this);

    auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(_this);
    auto GameState = (AFortGameStateAthena*)_this->GameState;
    auto GameMode = (AFortGameModeAthena*)_this;

    auto Time = (float)UGameplayStatics::GetTimeSeconds(GetWorld());
    auto WarmupDuration = 60.f;

    GamePhaseLogic->WarmupCountdownStartTime = Time;
    GamePhaseLogic->WarmupCountdownEndTime = Time + GamePhaseLogic->WarmupCountdownDuration;
    //GamePhaseLogic->WarmupCountdownDuration = 10.f;
    //GamePhaseLogic->WarmupEarlyCountdownDuration = WarmupDuration - 10.f;

    GamePhaseLogic->SetGamePhase(EAthenaGamePhase::Warmup);
    GamePhaseLogic->GamePhaseStep = EAthenaGamePhaseStep::Warmup;
    GamePhaseLogic->HandleGamePhaseStepChanged(EAthenaGamePhaseStep::Warmup);

    for (auto& Player : GameMode->AlivePlayers)
        Player->bBuildFree = true;

    if (GamePhaseLogic->AirCraftBehavior != EAirCraftBehavior::NoAircraft)
    {
        int TeamCount = 1;

        if (GamePhaseLogic->AirCraftBehavior == EAirCraftBehavior::OpposingAirCraftForEachTeam)
            TeamCount = GameState->TeamCount;

        for (int i = 0; i < TeamCount; i++)
        {
            auto& FlightInfo = GameState->MapInfo->FlightInfos[i];

            if (bLateGame)
            {
                auto& SafeZoneLocations = *(TArray<FVector>*)(__int64(GamePhaseLogic) + 0x458);
                FVector Zone6Center = SafeZoneLocations.Num() > 5 ? SafeZoneLocations[5] : GameState->MapInfo->GetMapCenter();
                Zone6Center.Z = 17500.f;

                FlightInfo.FlightSpeed = 0.f;
                FlightInfo.FlightStartLocation.X = Zone6Center.X;
                FlightInfo.FlightStartLocation.Y = Zone6Center.Y;
                FlightInfo.FlightStartLocation.Z = Zone6Center.Z;
                FlightInfo.TimeTillFlightEnd = 7.f;
                FlightInfo.TimeTillDropEnd = 7.f;
                FlightInfo.TimeTillDropStart = 0.f;
            }

            if (!GameState->MapInfo->AircraftClass)
                return;

            auto Aircraft = AFortAthenaAircraft::SpawnAircraft(GetWorld(), GameState->MapInfo->AircraftClass, FlightInfo, _this);

            Aircraft->AircraftIndex = i;

            if (!Aircraft)
                return;

            GamePhaseLogic->Aircrafts_GameMode.Add(Aircraft);
            GamePhaseLogic->Aircrafts_GameState.Add(Aircraft);
        }
    }

    TArray<AActor*> PatrolPaths;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFortAthenaPatrolPath::StaticClass(), &PatrolPaths);

    auto SeasonalEventManager = ((UFortGameInstance*)GetEngine()->GameInstance)->SeasonalEventManager;
    bool bSeason3 = SeasonalEventManager ? SeasonalEventManager->IsEventFlagActive(L"ClydeSeason3Part1") : false;
    auto bSeason2Part2 = bSeason3 || (SeasonalEventManager ? SeasonalEventManager->IsEventFlagActive(L"ClydeSeason2Part2") : false);
    auto bSeason2 = bSeason2Part2 || (SeasonalEventManager ? SeasonalEventManager->IsEventFlagActive(L"ClydeSeason2Part1") : false);

    for (auto& PatrolPath__Uncasted : PatrolPaths)
    {
        auto PatrolPath = (AFortAthenaPatrolPath*)PatrolPath__Uncasted;

        if (PatrolPath->GameplayTags.GameplayTags.Num() == 0)
            continue;
        auto PathName = PatrolPath->GameplayTags.GameplayTags[0].TagName.ToString();

        if (PathName == "Clyde.AI.SpawnLocation.Catty.Henchman")
            SpawnAI(L"/Clyde/Athena/AI/Clyde/Henchman/BP_AIBotSpawnerData_Clyde_Henchman_KitBash.BP_AIBotSpawnerData_Clyde_Henchman_KitBash_C", PatrolPath);
        else if (PathName == "Clyde.AI.SpawnLocation.Catty.Boss1")
            SpawnAI(L"/Clyde/Athena/AI/Clyde/Bosses/CattyCorner/BP_AIBotSpawnerData_Clyde_CattyCornerBoss.BP_AIBotSpawnerData_Clyde_CattyCornerBoss_C", PatrolPath);

        else if (PathName == "Clyde.AI.SpawnLocation.Agency.Henchman.Nyx")
            SpawnAI(L"/Clyde/Athena/AI/Clyde/Henchman/BP_AIBotSpawnerData_Clyde_Henchman_Nyx.BP_AIBotSpawnerData_Clyde_Henchman_Nyx_C", PatrolPath);
        //else if (PathName == "Clyde.AI.SpawnLocation.Agency.Henchman.Ego")
        //    SpawnAI(L"/Clyde/Athena/AI/Clyde/Henchman/BP_AIBotSpawnerData_Clyde_Henchman_Ego.BP_AIBotSpawnerData_Clyde_Henchman_Ego_C", PatrolPath);
        else if (PathName == "Clyde.AI.SpawnLocation.Agency.Boss")
            SpawnAI(L"/Clyde/Athena/AI/Clyde/Bosses/LemonCart/BP_AIBotSpawnerData_Clyde_LemonCartBoss.BP_AIBotSpawnerData_Clyde_LemonCartBoss_C", PatrolPath);

        else if (PathName == "Clyde.AI.SpawnLocation.OilRig.Henchman")
            SpawnAI(L"/Clyde/Athena/AI/Clyde/Henchman/BP_AIBotSpawnerData_Clyde_Henchman_Alter.BP_AIBotSpawnerData_Clyde_Henchman_Alter_C", PatrolPath);
        else if (PathName == "Clyde.AI.SpawnLocation.OilRig.Boss")
            SpawnAI(L"/Clyde/Athena/AI/Clyde/Bosses/OilRig/BP_AIBotSpawnerData_Clyde_OilRigBoss.BP_AIBotSpawnerData_Clyde_OilRigBoss_C", PatrolPath);

        else if (PathName == "Clyde.AI.SpawnLocation.Yacht.Henchman")
            SpawnAI(L"/Clyde/Athena/AI/Clyde/Henchman/BP_AIBotSpawnerData_Clyde_Henchman_Alter.BP_AIBotSpawnerData_Clyde_Henchman_Alter_C", PatrolPath);
        else if (PathName == "Clyde.AI.SpawnLocation.Yacht.Boss")
            SpawnAI(L"/Clyde/Athena/AI/Clyde/Bosses/Yacht/BP_AIBotSpawnerData_Clyde_YachtBoss.BP_AIBotSpawnerData_Clyde_YachtBoss_C", PatrolPath);
        
        else if (bSeason2)
        {
            if (PathName == "Clyde.AI.SpawnLocation.Grotto.S2.Henchman")
                SpawnAI(L"/Clyde/Athena/AI/Clyde/Henchman/BP_AIBotSpawnerData_Clyde_Henchman_Ego.BP_AIBotSpawnerData_Clyde_Henchman_Ego_C", PatrolPath);
            else if (PathName == "Clyde.AI.SpawnLocation.Grotto.S2.Boss")
                SpawnAI(L"/Clyde/Athena/AI/Clyde/Bosses/Grotto/BP_AIBotSpawnerData_Clyde_ClayPlugBoss.BP_AIBotSpawnerData_Clyde_ClayPlugBoss_C", PatrolPath);
            else if (bSeason2Part2)
            {
                if (PathName == "Clyde.AI.SpawnLocation.Shark.S2.Henchman")
                    SpawnAI(L"/Clyde/Athena/AI/Clyde/Henchman/BP_AIBotSpawnerData_Clyde_Henchman_Ego.BP_AIBotSpawnerData_Clyde_Henchman_Ego_C", PatrolPath);
                else if (PathName == "Clyde.AI.SpawnLocation.Shark.S2.Boss")
                    SpawnAI(L"/Clyde/Athena/AI/Clyde/Bosses/BerryTart/BP_AIBotSpawnerData_Clyde_BerryTartBoss.BP_AIBotSpawnerData_Clyde_BerryTartBoss_C", PatrolPath);
            }
        }

        //printf("%s %s\n", PathName.c_str(), UKismetSystemLibrary::GetPathName(PatrolPath).ToString().c_str());
        // if (PathName.substr(PathName.rfind(L'.') + 1) == NPCName)
        //     PossibleSpawnPaths.Add(PatrolPath);
    }
}

bool RetTrue()
{
    return true;
}

void (*HandleStartingNewPlayerOG)(AFortGameMode* _this, AFortPlayerControllerAthena* NewPlayer);
void HandleStartingNewPlayer(AFortGameMode* _this, AFortPlayerControllerAthena* NewPlayer)
{
    auto GameState = (AFortGameStateAthena*)_this->GameState;
    AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)NewPlayer->PlayerState;

    PlayerState->SquadID = PlayerState->TeamIndex - 3;
    PlayerState->OnRep_SquadId();

    FGameMemberInfo Member {};

    Member.MostRecentArrayReplicationKey = -1;
    Member.ReplicationID = -1;
    Member.ReplicationKey = -1;
    Member.TeamIndex = PlayerState->TeamIndex;
    Member.SquadID = PlayerState->SquadID;
    Member.MemberUniqueId = PlayerState->UniqueID;

    auto& NewMember = GameState->GameMemberInfoArray.Members.Add(Member);
    GameState->GameMemberInfoArray.MarkItemDirty(NewMember);

    static auto NotifyGameMemberAdded = (void (*)(AFortGameStateAthena*, uint8_t, uint8_t, FUniqueNetIdRepl*))(ImageBase + 0x27E12C0);
    if (NotifyGameMemberAdded)
        NotifyGameMemberAdded(GameState, Member.SquadID, Member.TeamIndex, &Member.MemberUniqueId);

    PlayerState->WorldPlayerId = PlayerState->PlayerId;

    if (!NewPlayer->WorldInventory)
    {
        auto Inventory = SpawnActor<AFortInventory>(NewPlayer->WorldInventoryClass, FVector {}, FRotator {}, NewPlayer);
        NewPlayer->WorldInventory.ObjectPointer = Inventory;
        NewPlayer->WorldInventory.InterfacePointer = GetInterfaceAddress(Inventory, UFortInventoryInterface::StaticClass());
        Inventory->InventoryType = EFortInventoryType::World;
    }

    auto GamePhaseLogic2 = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(_this);
    if (GamePhaseLogic2 && GamePhaseLogic2->GamePhase == EAthenaGamePhase::Warmup)
        NewPlayer->bBuildFree = true;

    return HandleStartingNewPlayerOG(_this, NewPlayer);
}

void (*HandleMatchHasEndedOG)(AFortGameModeAthena*);
void HandleMatchHasEnded(AFortGameModeAthena* _this)
{
    auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(_this);

    GamePhaseLogic->SetGamePhase(EAthenaGamePhase::EndGame);
    GamePhaseLogic->GamePhaseStep = EAthenaGamePhaseStep::EndGame;
    GamePhaseLogic->HandleGamePhaseStepChanged(EAthenaGamePhaseStep::EndGame);

    return HandleMatchHasEndedOG(_this);
}

UClass** GetGameSessionClass(AFortGameModeBR* _this, UClass** OutClass)
{
    *OutClass = AFortGameSessionDedicatedAthena::StaticClass();
    return OutClass;
}

uint8_t CurrentTeam = 3;
uint8_t PlayersOnCurTeam = 0;
uint8_t PickTeam(AFortGameMode* GameMode, uint8_t PreferredTeam, AFortPlayerControllerAthena* Controller)
{
    uint8_t ret = CurrentTeam;
    auto Playlist = ((AFortGameStateAthena*)GameMode->GameState)->CurrentPlaylistInfo.BasePlaylist;

    if (Playlist->bIsLargeTeamGame)
    {
        if (CurrentTeam == 4)
            CurrentTeam = 3;
        else
            CurrentTeam = 4;
    }
    else
    {
        if (++PlayersOnCurTeam >= (Playlist ? Playlist->MaxSquadSize : 1))
        {
            CurrentTeam++;
            PlayersOnCurTeam = 0;
        }
    }

    return ret;
}

void OnVerifyTravel(void* _this, FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSucceeded)
{
    auto Connection = (UNetConnection*)_this;

    if (!Response.Get())
        return;

    auto Status = Response->GetResponseCode();

    if (Status != 200)
    {
        printf("[Arc] Cleaning up invalid session\n");
        auto Cleanup = (void (*)(UNetConnection*))Connection->VTable[0x6B];

        Cleanup(Connection);
    }
}

AFortPlayerController* (*LoginOG)(
    AFortGameModeAthena* _this, UNetConnection* NewPlayer, ENetRole InRemoteRole, const FString& Portal, FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage);
AFortPlayerController* Login(
    AFortGameModeAthena* _this, UNetConnection* NewPlayer, ENetRole InRemoteRole, const FString& Portal, FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    auto StdOptions = Options.ToWString();
    auto Platform = UGameplayStatics::ParseOption(Options, L"Platform");

    if (Platform.Num() > 0 && Platform == L"WIN")
    {
        auto ArcToken = UGameplayStatics::ParseOption(Options, L"ArcToken");

        if (ArcToken.Num() == 0)
        {
            FString KickMsg = L"Failed to verify session.";
            CopyArray(ErrorMessage, KickMsg);
            return nullptr;
        }

        printf("[GameMode] ArcToken: %ls\n", ArcToken.CStr());

        auto RequestBody = std::wstring(L"{\"ip\":\"") + GameserverIP + L"\",\"port\":" + std::to_wstring(Port) + L"}";

        auto req = CreateRequest();
        req->SetURL(L"https://dev-anticheat-v1.arc-services.dev/router/v1/anticheat/public/verifyTravel");
        req->SetHeader(L"X-Arc-Client", L"zszxfhpnjgzxzvpiiopxqrfpivjqhxbh");
        req->SetHeader(L"X-Arc-Auth", std::move(ArcToken));
        req->SetHeader(L"Content-Type", L"application/json");
        req->SetContentAsString(RequestBody.c_str());
        req->OnRequestComplete(NewPlayer, OnVerifyTravel);
        req->SetVerb(L"POST");
        req->ProcessRequest();
    }

    return LoginOG(_this, NewPlayer, InRemoteRole, Portal, Options, UniqueId, ErrorMessage);
}

void GameMode__Init()
{
    Hook(ImageBase + 0x918B180, ReadyToStartMatch);
    Hook(ImageBase + 0x918E5B8, SpawnDefaultPawnFor);
    Hook(ImageBase + 0x918177C, FinishWorldInitialization, FinishWorldInitializationOG);
    Hook(ImageBase + 0x9184E1C, HandleMatchHasStarted, HandleMatchHasStartedOG);
    //Hook<AFortGameModeBR>(uint32(0xE9), RetTrue);
    Hook<AFortGameModeBR>(uint32(0xEE), HandleStartingNewPlayer, HandleStartingNewPlayerOG);
    Hook(ImageBase + 0x9184AC0, HandleMatchHasEnded, HandleMatchHasEndedOG);
    Hook(ImageBase + 0x3F46314, GetGameSessionClass);
    Hook(ImageBase + 0x9189894, PickTeam);

    if (bProd)
        Hook(ImageBase + 0x9186E98, Login, LoginOG);
}
