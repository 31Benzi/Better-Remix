// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <thread>
#include <string>
#include <algorithm>
#include "Shared.h"
#include "memcury.h"
#include "CurlHttp.h"
#include "Config.h"
#include "GUI.h"

static bool IsBlockedPackagePath(UObject* Object)
{
    if (!Object || !Object->Class)
        return false;

    // Walk the Outer chain to find the outermost package
    UObject* Outer = Object;
    while (Outer->Outer)
        Outer = Outer->Outer;

    auto PkgName = Outer->Name.ToString();

    // Block /DebugUI/ packages – their widget BPs reference missing super-structs
    // and cause CreateExport failures -> crash
    if (PkgName.find("DebugUI") != std::string::npos)
        return true;

    return false;
}

void ClientThread();

void CrashReporter__Init();

static void ActivateGameFeaturePlugin(const wchar_t* FeatureName)
{
    if (!FeatureName || !*FeatureName)
        return;

    auto CheatManagerCDO = UFortCheatManager::GetDefaultObj();
    if (!CheatManagerCDO)
    {
        printf("[Remix] ActivateGameFeaturePlugin(%ls): CheatManager CDO unavailable.\n", FeatureName);
        return;
    }

    printf("[Remix] Activating GameFeature plugin: %ls\n", FeatureName);
    CheatManagerCDO->LoadAndActivateGameFeaturePluginViaFeatureName(FString(FeatureName));
}

static UFortPlaylistAthena* WaitForPlaylistAsset(const std::wstring& PlaylistPath, int MaxAttempts = 20, int SleepMs = 250)
{
    for (int attempt = 0; attempt < MaxAttempts; ++attempt)
    {
        auto Obj = FindObject<UFortPlaylistAthena>(PlaylistPath.c_str());
        if (Obj)
        {
            if (attempt > 0)
                printf("[Remix] Playlist asset resolved after %d attempt(s): %ls\n", attempt + 1, PlaylistPath.c_str());
            return Obj;
        }
        Sleep(SleepMs);
    }
    printf("[Remix] Playlist asset not found after %d attempts: %ls\n", MaxAttempts, PlaylistPath.c_str());
    return nullptr;
}

static void LogPackagesMatching(const std::wstring& Needle)
{
    std::wstring lowerNeedle = Needle;
    std::transform(lowerNeedle.begin(), lowerNeedle.end(), lowerNeedle.begin(), ::towlower);

    int found = 0;
    for (uint32_t i = 0; i < TUObjectArray::Num(); ++i)
    {
        auto Obj = TUObjectArray::GetObjectByIndex(i);
        if (!Obj || !Obj->Class)
            continue;

        auto ClassName = Obj->Class->Name.ToString();
        if (ClassName != "Package")
            continue;

        auto Name = Obj->Name.ToString();
        std::wstring NameW(Name.begin(), Name.end());
        std::wstring lowerName = NameW;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
        if (lowerName.find(lowerNeedle) != std::wstring::npos)
        {
            printf("[Remix]   Package match: %ls\n", NameW.c_str());
            if (++found >= 20)
            {
                printf("[Remix]   (truncated after 20 matches)\n");
                break;
            }
        }
    }
    if (found == 0)
        printf("[Remix]   No mounted packages contain '%ls'.\n", Needle.c_str());
}

static std::wstring PrepareAndResolveMapURL(const std::wstring& PlaylistPath, const std::wstring& FallbackShortName)
{
    auto Playlist = WaitForPlaylistAsset(PlaylistPath);
    if (!Playlist)
        return L"";

    printf("[Remix] Playlist '%ls' has %d required GameFeature plugin(s):\n",
        PlaylistPath.c_str(), Playlist->BuiltInGameFeaturePluginsToLoad.Num());
    for (auto& Name : Playlist->BuiltInGameFeaturePluginsToLoad)
    {
        auto NameStr = Name.ToWString();
        printf("[Remix]   - %ls\n", NameStr.c_str());
        ActivateGameFeaturePlugin(NameStr.c_str());
    }

    // Also activate via raw plugin URL list (some playlists use this instead).
    for (auto& Url : Playlist->GameFeaturePluginURLsToLoad)
    {
        auto UrlStr = Url.ToWString();
        if (UrlStr.empty())
            continue;
        printf("[Remix]   Activating plugin URL: %ls\n", UrlStr.c_str());
        UFortCheatManager::GetDefaultObj()->LoadAndActivateGameFeaturePlugin(FString(UrlStr.c_str()));
    }

    auto PackageName = Playlist->PreloadPersistentLevel.ObjectID.AssetPath.PackageName.ToString();
    std::wstring MapURL(PackageName.begin(), PackageName.end());

    if (MapURL.empty() || MapURL == L"None")
    {
        printf("[Remix] Playlist PreloadPersistentLevel is empty or None; will fall back to '%ls'.\n", FallbackShortName.c_str());
        LogPackagesMatching(FallbackShortName);
        return L"";
    }

    printf("[Remix] Playlist PreloadPersistentLevel: %ls\n", MapURL.c_str());
    return MapURL;
}

void Main(HMODULE hModule)
{
    CrashReporter__Init();

    if (bEnableConsole)
    {
        AllocConsole();
        FILE* pFile;
        freopen_s(&pFile, "CONOUT$", "w", stdout);
        freopen_s(&pFile, "CONOUT$", "w", stderr);
        freopen_s(&pFile, "CONIN$", "r", stdin);
    }

    if (bGUI)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)GUI::Init, 0, 0, 0);
    }


    //UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), L"Fort.MME.TacticalSprint 0", nullptr);
    //UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), L"Fort.MME.Clambering 0", nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), L"net.AllowEncryption 0", nullptr);

    Patch<uint8_t>(ImageBase + 0x537F4A0, 0xC3); // UnsafeEnvironment
    Patch<uint8_t>(ImageBase + 0x4336BAC, 0xC3); // RequestExit
    Patch<uint8_t>(ImageBase + 0x27B3958, 0xC3); // ChangeGameSessionID crash 1
    Patch<uint8_t>(ImageBase + 0x27B3598, 0xC3); // ChangeGameSessionID crash 2
    //Patch<uint8_t>(ImageBase + 0x29EB6C0, 0xC3); // phys crash
    Patch<uint8_t>(ImageBase + 0x1D91EEC, 0xC3); // GFX crash
    Patch<uint8_t>(ImageBase + 0x2C7D6F0, 0xC3); // Pedestal BeginPlay
    //Patch<uint8_t>(ImageBase + 0x93D72F8, 0xC3); // some mutator crash
    Patch<uint8_t>(ImageBase + 0x1BED9A8, 0xC3); // KickPlayer
    Patch<uint8_t>(ImageBase + 0x338C8F8, 0x01); // SpawnServerActor patch
    Patch<uint8_t>(ImageBase + 0x721A50C, 0xEB);
    Patch<uint8_t>(ImageBase + 0x2C7736C, 0xC3); // controller disconnected
    Patch<uint8_t>(ImageBase + 0x3E2C464, 0xC3); // upd required
    Patch<uint8_t>(ImageBase + 0x2C78CDC, 0xC3); // nother controller
    Patch<uint8_t>(ImageBase + 0x3E6E09D, 0xEB); // goofy crash
    Patch<uint8_t>(ImageBase + 0x27306AC, 0xC3); // more crash
    Patch<uint8_t>(ImageBase + 0xAF29848, 0xC3); // pawn crash
    Patch<uint8_t>(ImageBase + 0x19D7D70, 0xC3); // wep crash
    Patch<uint8_t>(ImageBase + 0x2CAF22C, 0xC3); // widget crash
    Patch<uint8_t>(ImageBase + 0x2CB06C8, 0xC3); // widget crash
    Patch<uint8_t>(ImageBase + 0x1E349CB, 0x85); // gamephase step
    Patch<uint16_t>(ImageBase + 0xA6E634D, 0xE990); // respawn kick
    Patch<uint32_t>(ImageBase + 0x20AEF8C, 0xC0FFC031); // localplayer spawnplayactor
    Patch<uint8_t>(ImageBase + 0x20AEF90, 0xC3);
    Patch<uint8_t>(ImageBase + 0x9B2A080, 0xC3); // entitlement crash
    //Patch<uint32_t>(ImageBase + 0x9B43D81, 0x90909090); // clanker log
    //Patch<uint8_t>(ImageBase + 0x9B43D85, 0x90);
    Patch<uint32_t>(ImageBase + 0x7C86D88, 0xC0FFC031); // canactivateability
    Patch<uint8_t>(ImageBase + 0x7C86D8C, 0xC3);
    Patch<uint8_t>(ImageBase + 0x6D3E210, 0xC3); // some weird cosmetic crash
    Patch<uint8_t>(ImageBase + 0xC6A3B28, 0xC3); // fire spread fix

    /*auto req = CreateRequest();
    req->SetURL(L"http://127.0.0.1:3551/fortnitediddy");
    req->SetVerb(L"POST");
    req->OnRequestComplete(nullptr, Diddy);
    req->ProcessRequest();*/

    //Patch<uint8_t>(ImageBase + 0x27346A8, 0xC3);

    MH_Initialize();

    for (auto& Initter : Initters)
        Initter();

    MH_EnableHook(MH_ALL_HOOKS);

    *(bool*)(ImageBase + 0x12D4E1CA) = false;
    *(bool*)(ImageBase + 0x12D4E166) = true;

    auto ReplicationBridgeConfig = UObjectReplicationBridgeConfig::GetDefaultObj();

    auto FortInventoryName = FName(L"/Script/FortniteGame.FortInventory");
    for (auto& FilterConfig : ReplicationBridgeConfig->FilterConfigs)
    {
        if (FilterConfig.ClassName == FortInventoryName)
        {
            FilterConfig.DynamicFilterName = FName(0);
            break;
        }
    }

    for (uint32 i = 0; i < TUObjectArray::Num(); ++i)
    {
        UObject* Object = TUObjectArray::GetObjectByIndex(i);

        if (Object == NULL || !Object->IsDefaultObject())
            continue;

        if (IsBlockedPackagePath(Object))
        {
            VirtualSwap(Object->VTable, 0x36, ReturnFalse); // NeedsLoadForClient
            if (Object->IsA<AActor>())
            {
                auto Actor = (AActor*)Object;
                Actor->bReplicates = false;
                Actor->bAlwaysRelevant = false;
            }
            continue;
        }

        VirtualSwap(Object->VTable, 0x36, ReturnTrue); // NeedsLoadForClient
    }

    UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), L"log LogFortUIDirector None", nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), L"log LogFortUIManager None", nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), L"log LogStreaming Error", nullptr);

    ////GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);
    ////
    auto cmdAllocated = UKismetSystemLibrary::GetCommandLine().ToString();
    std::string cmd(cmdAllocated.c_str());
    printf("[Remix] Server CommandLine: %s\n", cmd.c_str());

    size_t pos = cmd.find("playlist=");
    if (pos == std::string::npos)
        pos = cmd.find("Playlist=");

    if (pos != std::string::npos)
    {
        size_t start = pos + 9;
        size_t end = cmd.find_first_of(" \t?&-\"", start);
        std::string parsed = cmd.substr(start, end - start);

        if (!parsed.empty())
        {
            std::wstring wParsed(parsed.begin(), parsed.end());

            if (wParsed.find(L"/") == std::wstring::npos)
            {
                if (wParsed == L"Playlist_Quail" || wParsed == L"playlist_quail")
                    Playlist = L"/QuailPlaylist/Playlist/Playlist_Quail.Playlist_Quail";
                else if (wParsed == L"Playlist_SunflowerSolo" || wParsed == L"playlist_sunflowersolo")
                    Playlist = L"/BlastBerry/Playlists/Playlist_SunflowerSolo.Playlist_SunflowerSolo";
                else if (wParsed == L"Playlist_PunchBerrySolo" || wParsed == L"playlist_punchberrysolo")
                    Playlist = L"/BlastBerry/Playlists/Playlist_PunchBerrySolo.Playlist_PunchBerrySolo";
                else if (wParsed == L"Playlist_Respawn_24" || wParsed == L"playlist_respawn_24")
                    Playlist = L"/Rumble/Playlists/Playlist_Respawn_24.Playlist_Respawn_24";
                else if (wParsed == L"Playlist_DefaultSolo" || wParsed == L"playlist_defaultsolo")
                    Playlist = L"/BRPlaylists/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo";
                else if (wParsed == L"Playlist_TestBuild_Solo" || wParsed == L"playlist_testbuild_solo")
                    Playlist = L"/BRPlaylists/Athena/Playlists/TestBuild/Playlist_TestBuild_Solo.Playlist_TestBuild_Solo";
                else if (wParsed == L"Playlist_Creative_Play_V2" || wParsed == L"playlist_creative_play_v2")
                    Playlist = L"/CreativeCore/Playlists/Creative_PlayOnlyPlaylists/Playlist_Creative_Play_V2.Playlist_Creative_Play_V2";
                else
                {
                    Playlist = L"/BRPlaylists/Athena/Playlists/" + wParsed + L"." + wParsed;
                }
            }
            else
            {
                Playlist = wParsed;
            }

            printf("[Remix] Parsed playlist dynamically from command line: %ls\n", Playlist.c_str());
        }
    }

    std::wstring fallbackMap = L"BlastBerry_Terrain";
    if (Playlist == L"/QuailPlaylist/Playlist/Playlist_Quail.Playlist_Quail")
        fallbackMap = L"Apollo_Terrain_Retro";
    else if (Playlist == L"/BlastBerry/Playlists/Playlist_SunflowerSolo.Playlist_SunflowerSolo")
        fallbackMap = L"BlastBerry_Terrain";
    else if (Playlist == L"/BlastBerry/Playlists/Playlist_PunchBerrySolo.Playlist_PunchBerrySolo")
        fallbackMap = L"PunchBerry_Terrain";
    else if (Playlist == L"/Rumble/Playlists/Playlist_Respawn_24.Playlist_Respawn_24")
        fallbackMap = L"Apollo_Terrain_Retro";
    else if (Playlist == L"/BRPlaylists/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo")
        fallbackMap = L"Apollo_Terrain_Retro";
    else if (Playlist == L"/BRPlaylists/Athena/Playlists/TestBuild/Playlist_TestBuild_Solo.Playlist_TestBuild_Solo")
        fallbackMap = L"Apollo_Terrain_Retro";
    else if (Playlist == L"/CreativeCore/Playlists/Creative_PlayOnlyPlaylists/Playlist_Creative_Play_V2.Playlist_Creative_Play_V2")
        fallbackMap = L"Creative_NoApollo_Terrain";

    auto effectivePlaylist = bEvent ? std::wstring(L"/QuailPlaylist/Playlist/Playlist_Quail.Playlist_Quail") : Playlist;
    std::wstring startupMap = PrepareAndResolveMapURL(effectivePlaylist, fallbackMap);
    if (startupMap.empty())
        startupMap = fallbackMap;

    std::wstring openCmd = L"open " + startupMap;
    printf("[Remix] Executing console command: %ls\n", openCmd.c_str());
    UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), openCmd.c_str(), nullptr);

    //APlayerController* LocalPlayer = GetWorld()->OwningGameInstance->LocalPlayers[0]->PlayerController;

    //LocalPlayer->SwitchLevel(L"/Game/Jett/TiltedZW/TiltedZW_Jett");
    //UGameplayStatics::RemovePlayer(LocalPlayer, true);
}

void ClientThread()
{
    while (true)
    {
        if (GetWorld() && GetWorld()->OwningGameInstance)
        {
            auto& LocalPlayers = GetWorld()->OwningGameInstance->LocalPlayers;

            if (LocalPlayers.Num() > 0)
            {
                auto PlayerController = (AFortPlayerControllerAthena*)LocalPlayers[0]->PlayerController;

                if (PlayerController && !PlayerController->CheatManager)
                {
                    PlayerController->CheatManager = (UFortCheatManager*)UGameplayStatics::SpawnObject(PlayerController->CheatClass, PlayerController);
                    PlayerController->CheatManager->ObjectFlags &= ~0x1000000;
                    TUObjectArray::GetItemByIndex(PlayerController->CheatManager->GetIndex())->Flags &= ~0x4000000;
                }
            }
        }
        
        Sleep(33);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        std::thread(Main, hModule).detach();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
