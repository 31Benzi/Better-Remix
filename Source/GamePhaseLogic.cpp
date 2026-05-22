#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
#include "Pickups.h"
#include "CurlHttp.h"
#include "Config.h"
#include "GUI.h"
#include <thread>
INIT_TICKER(GamePhaseLogic);

void SetGamePhaseStep(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic, EAthenaGamePhaseStep Step)
{
    GamePhaseLogic->GamePhaseStep = Step;
    GamePhaseLogic->HandleGamePhaseStepChanged(Step);
}

void StartAircraftPhase(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* GamePhaseLogic)
{
    GUI::gsStatus = StartedMatch;

    auto GameState = (AFortGameStateAthena*)GetWorld()->GameState;
    auto GameMode = (AFortGameModeAthena*)GetWorld()->AuthorityGameMode;
    auto Playlist = GameState->CurrentPlaylistInfo.BasePlaylist;
    auto Time = UGameplayStatics::GetTimeSeconds(GetWorld());

    if (bLateGame)
    {
        GameState->DefaultParachuteDeployTraceForGroundDistance = 2500.f;
    }

    if (Playlist->bSkipAircraft)
    {
        GamePhaseLogic->SetGamePhase(EAthenaGamePhase::SafeZones);
        SetGamePhaseStep(GamePhaseLogic, EAthenaGamePhaseStep::StormForming);

        return;
    }


    if (GameState->MapInfo->FlightInfos.Num() > 0)
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

    if (!GamePhaseLogic->SafeZoneIndicator)
    {
        AFortSafeZoneIndicator* SafeZoneIndicator = SpawnActor<AFortSafeZoneIndicator>(GamePhaseLogic->SafeZoneIndicatorClass, FVector {});

        if (SafeZoneIndicator)
        {
            auto GameState = (AFortGameStateAthena*)GetWorld()->GameState;
            FFortSafeZoneDefinition& SafeZoneDefinition = GameState->MapInfo->SafeZoneDefinition;
            float SafeZoneCount = EvaluateScalableFloat(SafeZoneDefinition.Count);

            SafeZoneIndicator->PlaylistMaxSafeZoneIndex = GameState->CurrentPlaylistInfo.BasePlaylist->LastSafeZoneIndex;
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

            for (float i = 0; i < SafeZoneCount; i++)
            {
                FFortSafeZonePhaseInfo PhaseInfo {};

                PhaseInfo.Radius = EvaluateScalableFloat(SafeZoneDefinition.Radius, i);
                PhaseInfo.WaitTime = EvaluateScalableFloat(SafeZoneDefinition.WaitTime, i);
                PhaseInfo.ShrinkTime = EvaluateScalableFloat(SafeZoneDefinition.ShrinkTime, i);
                PhaseInfo.PlayerCap = (int)EvaluateScalableFloat(SafeZoneDefinition.PlayerCapSolo, i);

                UDataTableFunctionLibrary::EvaluateCurveTableRow(
                    GameState->AthenaGameDataTable, FName(L"Default.SafeZone.Damage"), i, nullptr, &PhaseInfo.DamageInfo.Damage, FString());

                PhaseInfo.DamageInfo.bPercentageBasedDamage = true;
                PhaseInfo.TimeBetweenStormCapDamage = EvaluateScalableFloat(GamePhaseLogic->TimeBetweenStormCapDamage, i);
                PhaseInfo.StormCapDamagePerTick = EvaluateScalableFloat(GamePhaseLogic->StormCapDamagePerTick, i);
                PhaseInfo.StormCampingIncrementTimeAfterDelay = EvaluateScalableFloat(GamePhaseLogic->StormCampingIncrementTimeAfterDelay, i);
                PhaseInfo.StormCampingInitialDelayTime = EvaluateScalableFloat(GamePhaseLogic->StormCampingInitialDelayTime, i);
                PhaseInfo.MegaStormGridCellThickness = (int)EvaluateScalableFloat(SafeZoneDefinition.MegaStormGridCellThickness, i);
                PhaseInfo.UsePOIStormCenter = false;
                PhaseInfo.TravelSplineComponent = SafeZoneIndicator->CurrentTravelSplineComponent;

                PhaseInfo.Center = SafeZoneLocations[(int)i];

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

        auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(SafeZoneIndicator);
        SetGamePhaseStep(GamePhaseLogic, EAthenaGamePhaseStep::StormHolding);
    }
}

extern "C" void NtTerminateProcess(HANDLE hProcess, UINT uExitCode);
void GamePhaseLogic__Tick()
{
    auto GameState = (AFortGameStateAthena*)GetWorld()->GameState;
    auto GameMode = (AFortGameModeAthena*)GetWorld()->AuthorityGameMode;
    auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameState);
    auto PlaylistObj = GameState->CurrentPlaylistInfo.BasePlaylist;
    auto Time = UGameplayStatics::GetTimeSeconds(GetWorld());

    static bool finishedFlight = false;
    if (!PlaylistObj->bSkipAircraft)
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

                    if (bProd)
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
