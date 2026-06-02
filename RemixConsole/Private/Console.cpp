#include "pch.h"
#include "memcury.h"
#include "MinHook.h"
#include "../Public/Console.h"

void ClientThread()
{
    while (true)
    {
        auto Engine = (UEngine*)TUObjectArray::FindFirstObject("FortEngine");
        if (Engine && Engine->GameViewport && Engine->GameViewport->World && Engine->GameViewport->World->OwningGameInstance)
        {
            auto& LocalPlayers = Engine->GameViewport->World->OwningGameInstance->LocalPlayers;
            if (LocalPlayers.Num() > 0 && LocalPlayers[0] && LocalPlayers[0]->PlayerController)
            {
                auto PlayerController = (APlayerController*)LocalPlayers[0]->PlayerController;
                if (PlayerController && !PlayerController->CheatManager && PlayerController->CheatClass)
                {
                    PlayerController->CheatManager = (UCheatManager*)UGameplayStatics::SpawnObject(PlayerController->CheatClass, PlayerController);
                    if (PlayerController->CheatManager)
                    {
                        PlayerController->CheatManager->ObjectFlags &= ~0x1000000;
                        auto Item = TUObjectArray::GetItemByIndex(PlayerController->CheatManager->GetIndex());
                        if (Item)
                        {
                            Item->Flags &= ~0x4000000;
                        }
                    }
                }
            }
        }
        Sleep(100);
    }
}

void RemixConsole::Init()
{
    // Spawns viewport console dynamically
    auto Engine = (UEngine*)TUObjectArray::FindFirstObject("FortEngine");
    if (Engine && Engine->GameViewport && Engine->ConsoleClass)
    {
        Engine->GameViewport->ViewportConsole = (UConsole*)UGameplayStatics::SpawnObject(Engine->ConsoleClass, Engine->GameViewport);
    }

    // Custom play list modifier for respawns if requested
    for (uint32_t i = 0; i < TUObjectArray::Num(); i++)
    {
        auto Object = TUObjectArray::GetObjectByIndex(i);
        if (Object && Object->IsA(UFortPlaylistAthena::StaticClass()))
        {
            auto Playlist = (UFortPlaylistAthena*)Object;

            if (Playlist->GameType == EFortGameType::BlastBerry)
                continue;

            Playlist->bRespawnInAir = true;

            Playlist->RespawnHeight.Curve.CurveTable = nullptr;
            Playlist->RespawnHeight.Curve.RowName = FName();
            Playlist->RespawnHeight.Value = 20000.f;

            Playlist->RespawnTime.Curve.CurveTable = nullptr;
            Playlist->RespawnTime.Curve.RowName = FName();
            Playlist->RespawnTime.Value = 3.f;

            Playlist->RespawnType = (enum EAthenaRespawnType)1; // InfiniteRespawnExceptStorm
            Playlist->bAllowJoinInProgress = true;
            Playlist->bForceCameraFadeOnRespawn = true;
        }
    }

    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)ClientThread, nullptr, 0, nullptr);
}
