#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
#include "Pickups.h"
#include "CurlHttp.h"
#include "Config.h"
#include "GUI.h"
#include <thread>
INIT_TICKER(GamePhaseLogic);

static UFortPlaylistAthena* ResolveCurrentPlaylist(AFortGameStateAthena* GameState)
{
    if (!GameState)
        return nullptr;

    auto PlaylistObj = GameState->CurrentPlaylistInfo.BasePlaylist;
    if (IsValidUObject(PlaylistObj, UFortPlaylistAthena::StaticClass()))
        return PlaylistObj;

    if (PlaylistObj)
    {
        printf("[GamePhaseLogic] Invalid playlist pointer %p. Restoring configured playlist.\n", PlaylistObj);
        GameState->CurrentPlaylistInfo.BasePlaylist = nullptr;
    }

    PlaylistObj = FindObject<UFortPlaylistAthena>(bEvent ? L"/QuailPlaylist/Playlist/Playlist_Quail.Playlist_Quail" : Playlist.c_str());
    if (!PlaylistObj)
        PlaylistObj = FindObject<UFortPlaylistAthena>(L"/BRPlaylists/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo");
    if (!PlaylistObj)
        PlaylistObj = FindObject<UFortPlaylistAthena>(L"/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo");

    if (PlaylistObj)
    {
        GameState->CurrentPlaylistInfo.BasePlaylist = PlaylistObj;
        GameState->CurrentPlaylistId = PlaylistObj->PlaylistId;
    }

    return PlaylistObj;
}

void SetGamePhaseStep(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic, EAthenaGamePhaseStep Step)
{
    if (!GamePhaseLogic)
        return;

    GamePhaseLogic->GamePhaseStep = Step;
    GamePhaseLogic->HandleGamePhaseStepChanged(Step);
}

void StartAircraftPhase(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic)
{
    GUI::gsStatus = StartedMatch;

    auto World = GetWorld();
    if (!World || !World->GameState || !World->AuthorityGameMode || !GamePhaseLogic)
        return;

    auto GameState = (AFortGameStateAthena*)World->GameState;
    auto GameMode = (AFortGameModeAthena*)World->AuthorityGameMode;
    auto PlaylistObj = ResolveCurrentPlaylist(GameState);
    auto Time = UGameplayStatics::GetTimeSeconds(World);

    if (bLateGame)
    {
        GameState->DefaultParachuteDeployTraceForGroundDistance = 2500.f;
    }

    if (PlaylistObj && PlaylistObj->bSkipAircraft)
    {
        GamePhaseLogic->SetGamePhase(EAthenaGamePhase::SafeZones);
        SetGamePhaseStep(GamePhaseLogic, EAthenaGamePhaseStep::StormForming);

        return;
    }


    if (GameState->MapInfo && GameState->MapInfo->FlightInfos.Num() > 0)
    {
        // TArray<TWeakObjectPtr<AFortAthenaAircraft>> Aircrafts;

        for (auto& Aircraft : GamePhaseLogic->Aircrafts_GameState)
        {
            Aircraft->FlightElapsedTime = 0;
            Aircraft->DropStartTime = (float)Time + Aircraft->FlightInfo.TimeTillDropStart;
            Aircraft->DropEndTime = (float)Time + Aircraft->FlightInfo.TimeTillDropEnd;
            Aircraft->FlightStartTime = (float)Time;
            Aircraft->FlightEndTime = (float)Time + Aircraft->FlightInfo.TimeTillFlightEnd;
            Aircraft->ReplicatedFlightTimestamp = (float)Time;
        }
        GamePhaseLogic->bAircraftIsLocked = true;

        for (auto& Player : GameMode->AlivePlayers)
        {
            auto Pawn = (AFortPlayerPawnAthena*)Player->Pawn;

            Player->LastDamager = nullptr;
            Player->LastFallInstigator = nullptr;
            if (Pawn)
            {
                auto EquipWeapon = (void (*)(AFortPawn*, AFortWeapon*))(ImageBase + 0xA145620);
                EquipWeapon(Pawn, nullptr); // UnequipCurrentWeapon

                if (Pawn->Role == ENetRole::ROLE_Authority)
                {
                    if (Pawn->bIsInAnyStorm)
                    {
                        Pawn->bIsInAnyStorm = false;
                        Pawn->OnRep_IsInAnyStorm();
                    }
                }
                Pawn->bIsInsideSafeZone = true;
                Pawn->OnRep_IsInsideSafeZone();
                Pawn->OnEnteredAircraft.Process();
            }

            Player->ClientActivateSlot(EFortQuickBars::Primary, 0, 0.f, true, false);
            if (Pawn)
                Pawn->K2_DestroyActor();
            
            UEAllocatedVector<FGuid> GuidsToRemove;
            for (auto& Entry : ((AFortInventory*)Player->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries)
            {
                auto PickupComponent = GetPickupComponent((UFortItemDefinition*)Entry.ItemDefinition);
                if (PickupComponent && PickupComponent->bCanBeDroppedFromInventory)
                {
                    GuidsToRemove.push_back(Entry.ItemGuid);
                }
            }

            for (auto& Guid : GuidsToRemove)
                Remove(Player->WorldInventory, Guid);

            auto Reset = (void (*)(AFortPlayerControllerAthena*))(ImageBase + 0x7113FD8);
            Reset(Player);

            Player->ClientGotoState(FName(L"Spectating"));
        }
        // SetAircrafts(Aircrafts);
        // OnRep_Aircrafts();
    }

    for (auto& Player : GameMode->AlivePlayers)
        Player->bBuildFree = false;

    GamePhaseLogic->SetGamePhase(EAthenaGamePhase::Aircraft);
    SetGamePhaseStep(GamePhaseLogic, EAthenaGamePhaseStep::BusLocked);
}


AFortSafeZoneIndicator* SetupSafeZoneIndicator(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic)
{
    // thanks heliato

    if (!GamePhaseLogic)
        return nullptr;

    if (!GamePhaseLogic->SafeZoneIndicator)
    {
        AFortSafeZoneIndicator* SafeZoneIndicator = SpawnActor<AFortSafeZoneIndicator>(GamePhaseLogic->SafeZoneIndicatorClass, FVector {});

        if (SafeZoneIndicator)
        {
            auto World = GetWorld();
            auto GameState = World ? (AFortGameStateAthena*)World->GameState : nullptr;
            if (!GameState || !GameState->MapInfo)
                return GamePhaseLogic->SafeZoneIndicator;

            auto PlaylistObj = ResolveCurrentPlaylist(GameState);
            FFortSafeZoneDefinition& SafeZoneDefinition = GameState->MapInfo->SafeZoneDefinition;
            int SafeZoneCount = (int)EvaluateScalableFloat(SafeZoneDefinition.Count);

            SafeZoneIndicator->PlaylistMaxSafeZoneIndex = PlaylistObj ? PlaylistObj->LastSafeZoneIndex : -1;
            /*auto SafeZoneCount = (float)GameState->CurrentPlaylistInfo.BasePlaylist->LastSafeZoneIndex;
            if (SafeZoneCount == -1)
                SafeZoneCount = EvaluateScalableFloat(GameState->MapInfo->SafeZoneDefinition.Count);
            else
                SafeZoneCount++;*/

            auto& Array = SafeZoneIndicator->SafeZonePhases;

            if (Array.IsValid())
                Array.Free();

            const float Time = (float)UGameplayStatics::GetTimeSeconds(GameState);
            auto& SafeZoneLocations = *(TArray<FVector>*)(__int64(GamePhaseLogic) + 0x458);

            if (SafeZoneCount > SafeZoneLocations.Num())
                SafeZoneCount = SafeZoneLocations.Num();

            for (int i = 0; i < SafeZoneCount; i++)
            {
                FFortSafeZonePhaseInfo PhaseInfo {};

                PhaseInfo.Radius = EvaluateScalableFloat(SafeZoneDefinition.Radius, (float)i);
                PhaseInfo.WaitTime = EvaluateScalableFloat(SafeZoneDefinition.WaitTime, (float)i);
                PhaseInfo.ShrinkTime = EvaluateScalableFloat(SafeZoneDefinition.ShrinkTime, (float)i);
                PhaseInfo.PlayerCap = (int)EvaluateScalableFloat(SafeZoneDefinition.PlayerCapSolo, (float)i);

                UDataTableFunctionLibrary::EvaluateCurveTableRow(
                    GameState->AthenaGameDataTable, FName(L"Default.SafeZone.Damage"), (float)i, nullptr, &PhaseInfo.DamageInfo.Damage, FString());

                PhaseInfo.DamageInfo.bPercentageBasedDamage = true;
                PhaseInfo.TimeBetweenStormCapDamage = EvaluateScalableFloat(GamePhaseLogic->TimeBetweenStormCapDamage, (float)i);
                PhaseInfo.StormCapDamagePerTick = EvaluateScalableFloat(GamePhaseLogic->StormCapDamagePerTick, (float)i);
                PhaseInfo.StormCampingIncrementTimeAfterDelay = EvaluateScalableFloat(GamePhaseLogic->StormCampingIncrementTimeAfterDelay, (float)i);
                PhaseInfo.StormCampingInitialDelayTime = EvaluateScalableFloat(GamePhaseLogic->StormCampingInitialDelayTime, (float)i);
                PhaseInfo.MegaStormGridCellThickness = (int)EvaluateScalableFloat(SafeZoneDefinition.MegaStormGridCellThickness, (float)i);
                PhaseInfo.UsePOIStormCenter = false;
                PhaseInfo.TravelSplineComponent = SafeZoneIndicator->CurrentTravelSplineComponent;

                PhaseInfo.Center = SafeZoneLocations[i];

                Array.Add(PhaseInfo);

                SafeZoneIndicator->PhaseCount++;
            }

            SafeZoneIndicator->OnRep_PhaseCount();

            /*SafeZoneIndicator->SafeZoneStartShrinkTime = Time + Array[0].WaitTime;
            SafeZoneIndicator->SafeZoneFinishShrinkTime = SafeZoneIndicator->SafeZoneStartShrinkTime + Array[0].ShrinkTime;

            SafeZoneIndicator->CurrentPhase = 0;
            SafeZoneIndicator->OnRep_CurrentPhase();*/
        }

        GamePhaseLogic->SafeZoneIndicator = SafeZoneIndicator;
        GamePhaseLogic->OnRep_SafeZoneIndicator();
    }

    return GamePhaseLogic->SafeZoneIndicator;
}

void StartNewSafeZonePhase(AFortSafeZoneIndicator* SafeZoneIndicator, int NewSafeZonePhase)
{
    if (!SafeZoneIndicator)
        return;

    float TimeSeconds = (float)UGameplayStatics::GetTimeSeconds(GetWorld());
    auto& Array = SafeZoneIndicator->SafeZonePhases;

    if (Array.IsValidIndex(NewSafeZonePhase))
    {
        if (Array.IsValidIndex(NewSafeZonePhase - 1))
        {
            auto& PreviousPhaseInfo = Array[NewSafeZonePhase - 1];

            SafeZoneIndicator->PreviousCenter = (FVector_NetQuantize100)PreviousPhaseInfo.Center;
            SafeZoneIndicator->PreviousRadius = PreviousPhaseInfo.Radius;
        }

        auto& PhaseInfo = Array[NewSafeZonePhase];

        SafeZoneIndicator->NextCenter = (FVector_NetQuantize100)PhaseInfo.Center;
        SafeZoneIndicator->NextRadius = PhaseInfo.Radius;
        SafeZoneIndicator->NextMegaStormGridCellThickness = PhaseInfo.MegaStormGridCellThickness;

        if (Array.IsValidIndex(NewSafeZonePhase + 1))
        {
            auto& NextPhaseInfo = Array[NewSafeZonePhase + 1];

            if (SafeZoneIndicator->FutureReplicator)
            {
                SafeZoneIndicator->FutureReplicator->NextNextCenter = (FVector_NetQuantize100)NextPhaseInfo.Center;
                SafeZoneIndicator->FutureReplicator->NextNextRadius = NextPhaseInfo.Radius;
            }

            SafeZoneIndicator->NextNextCenter = (FVector_NetQuantize100)NextPhaseInfo.Center;
            SafeZoneIndicator->NextNextRadius = NextPhaseInfo.Radius;
            SafeZoneIndicator->NextNextMegaStormGridCellThickness = NextPhaseInfo.MegaStormGridCellThickness;
        }

        SafeZoneIndicator->SafeZoneStartShrinkTime = TimeSeconds + PhaseInfo.WaitTime;
        SafeZoneIndicator->SafeZoneFinishShrinkTime = SafeZoneIndicator->SafeZoneStartShrinkTime + PhaseInfo.ShrinkTime;

        SafeZoneIndicator->CurrentDamageInfo = PhaseInfo.DamageInfo;
        SafeZoneIndicator->OnRep_CurrentDamageInfo();

        auto OldPhase = SafeZoneIndicator->CurrentPhase;
        SafeZoneIndicator->CurrentPhase = NewSafeZonePhase;
        SafeZoneIndicator->OnRep_CurrentPhase();

        SafeZoneIndicator->OnSafeZonePhaseChanged.Process();

        auto& SafeZoneState = *(uint8_t*)(__int64(&SafeZoneIndicator->FutureReplicator) - 0x4);
        SafeZoneState = 2;
        bool bInitial = OldPhase <= 0;

        SafeZoneIndicator->OnSafeZoneStateChange(EFortSafeZoneState::Holding, bInitial);
        struct Parms
        {
            AFortSafeZoneIndicator* SafeZoneIndicator;
            EFortSafeZoneState State;
        } P;

        P.SafeZoneIndicator = SafeZoneIndicator;
        P.State = EFortSafeZoneState::Holding;
        SafeZoneIndicator->SafezoneStateChangedDelegate.Process(&P);

        if (auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(SafeZoneIndicator))
            SetGamePhaseStep(GamePhaseLogic, EAthenaGamePhaseStep::StormHolding);
    }
}

extern "C" void NtTerminateProcess(HANDLE hProcess, UINT uExitCode);
void GamePhaseLogic__Tick()
{
    auto World = GetWorld();
    if (!World || !World->GameState || !World->AuthorityGameMode)
        return;

    auto GameState = (AFortGameStateAthena*)World->GameState;
    auto GameMode = (AFortGameModeAthena*)World->AuthorityGameMode;
    auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameState);
    if (!GamePhaseLogic)
        return;

    auto PlaylistObj = ResolveCurrentPlaylist(GameState);
    auto Time = UGameplayStatics::GetTimeSeconds(World);

    static bool finishedFlight = false;
    if (!PlaylistObj || !PlaylistObj->bSkipAircraft)
    {
        if (GamePhaseLogic->GamePhase == EAthenaGamePhase::Warmup)
        {
            if (bStartBusEarlyRequest)
            {
                bStartBusEarlyRequest = false;
                if (bProd && gsSocket)
                {
                    gsSocket->close();
                    gsSocket = nullptr;
                }
                SetGamePhaseStep(GamePhaseLogic, EAthenaGamePhaseStep::GetReady);
                StartAircraftPhase(GamePhaseLogic);
                return;
            }

            static bool gettingReady = false;
            if (!gettingReady)
            {
                if (GameMode->AlivePlayers.Num() > 0 && GamePhaseLogic->WarmupCountdownStartTime != -1
                    && GamePhaseLogic->WarmupCountdownEndTime - 10.f <= Time)
                {
                    gettingReady = true;

                    if (bProd && gsSocket)
                    {
                        gsSocket->close();
                        gsSocket = nullptr;
                    }
                    SetGamePhaseStep(GamePhaseLogic, EAthenaGamePhaseStep::GetReady);
                    return;
                }
            }

            if (gettingReady)
            {
                if (GameMode->AlivePlayers.Num() > 0 && GamePhaseLogic->WarmupCountdownEndTime != -1 && GamePhaseLogic->WarmupCountdownEndTime <= Time)
                {
                    StartAircraftPhase(GamePhaseLogic);

                    return;
                }
            }
        }

        if (GamePhaseLogic->GamePhase == EAthenaGamePhase::Aircraft)
        {
            static bool busUnlocked = false;
            if (!busUnlocked)
            {
                if (GameMode->AlivePlayers.Num() > 0 && GamePhaseLogic->Aircrafts_GameState.Num() > 0 && GamePhaseLogic->Aircrafts_GameState[0].Get() && GamePhaseLogic->Aircrafts_GameState[0]->DropStartTime <= Time)
                {
                    busUnlocked = true;

                    GamePhaseLogic->bAircraftIsLocked = false;
                    SetGamePhaseStep(GamePhaseLogic, EAthenaGamePhaseStep::BusFlying);
                    return;
                }
            }

            static bool startedForming = false;
            if (!startedForming)
            {
                if (GameMode->AlivePlayers.Num() > 0 && GamePhaseLogic->Aircrafts_GameState.Num() > 0 && GamePhaseLogic->Aircrafts_GameState[0].Get() && GamePhaseLogic->Aircrafts_GameState[0]->DropEndTime != -1
                    && GamePhaseLogic->Aircrafts_GameState[0]->DropEndTime <= Time)
                {
                    startedForming = true;

                    for (auto& Player : GameMode->AlivePlayers)
                    {
                        if (!Player || !Player->PlayerState)
                            continue;
                        if (!Player->PlayerState->bIsABot && Player->IsInAircraft())
                        {
                            auto AircraftComp = Player->GetAircraftComponent();
                            if (AircraftComp)
                                AircraftComp->ServerAttemptAircraftJump(FRotator {});
                        }
                    }

                    /*if (bLateGame)
                    {
                            GameState->GamePhase = EAthenaGamePhase::SafeZones;
                            GameState->GamePhaseStep = EAthenaGamePhaseStep::StormHolding;
                            GameState->OnRep_GamePhase(EAthenaGamePhase::Aircraft);
                    }*/
                    GamePhaseLogic->SafeZonesStartTime = bLateGame ? (float)Time : ((float)Time + EvaluateScalableFloat(GameState->MapInfo->SafeZoneStartDelay));

                    GamePhaseLogic->SetGamePhase(EAthenaGamePhase::SafeZones);
                    SetGamePhaseStep(GamePhaseLogic, EAthenaGamePhaseStep::StormForming);
                }
            }
        }
    }
    else
    {
        if (!finishedFlight)
        {
            finishedFlight = true;

            GamePhaseLogic->SetGamePhase(EAthenaGamePhase::SafeZones);
            SetGamePhaseStep(GamePhaseLogic, EAthenaGamePhaseStep::StormForming);
        }
    }

    if (bEnableZones && GamePhaseLogic->GamePhase == EAthenaGamePhase::SafeZones)
    {
        if (!finishedFlight)
        {
            if (GameMode->AlivePlayers.Num() > 0 && GamePhaseLogic->Aircrafts_GameState.Num() > 0 && GamePhaseLogic->Aircrafts_GameState[0].Get() && GamePhaseLogic->Aircrafts_GameState[0]->FlightEndTime != -1
                && GamePhaseLogic->Aircrafts_GameState[0]->FlightEndTime <= Time)
            {
                finishedFlight = true;

                for (auto& Aircraft : GamePhaseLogic->Aircrafts_GameState)
                    Aircraft->K2_DestroyActor();

                GamePhaseLogic->Aircrafts_GameState.Clear();
                GamePhaseLogic->Aircrafts_GameMode.Clear();
            }
        }

        static bool formedZone = false;
        if (!formedZone)
        {
            if (GameMode->AlivePlayers.Num() > 0 && GamePhaseLogic->SafeZonesStartTime != -1 && GamePhaseLogic->SafeZonesStartTime <= Time)
            {
                formedZone = true;
                auto SafeZoneIndicator = SetupSafeZoneIndicator(GamePhaseLogic);
                StartNewSafeZonePhase(GamePhaseLogic->SafeZoneIndicator, bLateGame ? 6 : 1);
                return;
            }
        }

        static bool bUpdatedPhase = false;
        if (formedZone && GamePhaseLogic->SafeZoneIndicator)
        {
            if (GamePhaseLogic->SafeZoneIndicator->SafeZonePhases.IsValidIndex(GamePhaseLogic->SafeZoneIndicator->CurrentPhase))
            {
                bool bStartedNewPhase = false;
                if (!bUpdatedPhase && GamePhaseLogic->SafeZoneIndicator->SafeZoneStartShrinkTime <= Time)
                {
                    bUpdatedPhase = true;

                    auto& SafeZoneState = *(uint8_t*)(__int64(&GamePhaseLogic->SafeZoneIndicator->FutureReplicator) - 0x4);
                    SafeZoneState = 3;

                    GamePhaseLogic->SafeZoneIndicator->OnSafeZoneStateChange(EFortSafeZoneState::Shrinking, false);
                    struct Parms
                    {
                        AFortSafeZoneIndicator* SafeZoneIndicator;
                        EFortSafeZoneState State;
                    } P;

                    P.SafeZoneIndicator = GamePhaseLogic->SafeZoneIndicator;
                    P.State = EFortSafeZoneState::Shrinking;
                    GamePhaseLogic->SafeZoneIndicator->SafezoneStateChangedDelegate.Process(&P);

                    SetGamePhaseStep(GamePhaseLogic, EAthenaGamePhaseStep::StormShrinking);
                }
                else if (GamePhaseLogic->SafeZoneIndicator->SafeZoneFinishShrinkTime <= Time)
                {
                    bStartedNewPhase = true;

                    if (GamePhaseLogic->SafeZoneIndicator->SafeZonePhases.IsValidIndex(GamePhaseLogic->SafeZoneIndicator->CurrentPhase + 1))
                    {
                        StartNewSafeZonePhase(GamePhaseLogic->SafeZoneIndicator, GamePhaseLogic->SafeZoneIndicator->CurrentPhase + 1);
                        bUpdatedPhase = false;
                    }
                }
            }

            static auto ZoneEffect = FindObject<UClass>(L"/Game/Athena/SafeZone/GE_OutsideSafeZoneDamage.GE_OutsideSafeZoneDamage_C");

            for (auto& Player : GameMode->AlivePlayers)
            {
                if (!Player)
                    continue;
                if (auto Pawn = Player->MyFortPawn)
                {
                    auto Loc = Pawn->K2_GetActorLocation();
                    bool bInZone = GamePhaseLogic->IsInCurrentSafeZone(Loc, false);

                    if (Pawn->bIsInsideSafeZone != bInZone || Pawn->bIsInAnyStorm != !bInZone)
                    {
                        Pawn->bIsInAnyStorm = !bInZone;
                        Pawn->OnRep_IsInAnyStorm();
                        Pawn->bIsInsideSafeZone = bInZone;
                        Pawn->OnRep_IsInsideSafeZone();
                    }
                }
            }
        }
    }

    if (GamePhaseLogic->GamePhase == EAthenaGamePhase::EndGame && bProd)
    {
        if (GetWorld()->NetDriver->ClientConnections.Num() <= 0)
        {
            printf("Closing...");
            //GameMode->RestartGame();

            NtTerminateProcess(GetCurrentProcess(), 0);
        }
    }
}
