#include "pch.h"
#include "Shared.h"
#include "GUI.h"
#include "Config.h"
#include "../ThirdParty/ImGui/imgui.h"
#include "../ThirdParty/ImGui/imgui_impl_dx11.h"
#include "../ThirdParty/ImGui/imgui_impl_win32.h"
#include <d3d11.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#pragma comment(lib, "d3d11.lib")

struct FVersionInfo
{
    double FortniteVersion = 32.11;
    double EngineVersion = 5.4;
} VersionInfo;

struct FConfigState
{
    std::filesystem::file_time_type LastJsonTime;
    std::filesystem::file_time_type LastIniTime;
    std::wstring CurrentPlaylist;
    int FrameCount = 59;
};

static FConfigState g_ConfigState;

static std::string GetGamemodeName(const std::wstring& playlist)
{
    if (playlist.find(L"Playlist_Quail") != std::wstring::npos)
        return "Remix: The Finale (Live Event)";
    if (playlist.find(L"Playlist_SunflowerSolo") != std::wstring::npos)
        return "Reload Solos (Venture Map)";
    if (playlist.find(L"Playlist_PunchBerrySolo") != std::wstring::npos)
        return "Reload Solos (Oasis Map)";
    if (playlist.find(L"Playlist_Respawn_24") != std::wstring::npos)
        return "Team Rumble";
    if (playlist.find(L"Playlist_DefaultSolo") != std::wstring::npos)
        return "Battle Royale Solos";
    return "Unknown Gamemode";
}

static std::string GetGamemodeName(const std::string& playlist)
{
    if (playlist.find("Playlist_Quail") != std::string::npos)
        return "Remix: The Finale (Live Event)";
    if (playlist.find("Playlist_SunflowerSolo") != std::string::npos)
        return "Reload Solos (Venture Map)";
    if (playlist.find("Playlist_PunchBerrySolo") != std::string::npos)
        return "Reload Solos (Oasis Map)";
    if (playlist.find("Playlist_Respawn_24") != std::string::npos)
        return "Team Rumble";
    if (playlist.find("Playlist_DefaultSolo") != std::string::npos)
        return "Battle Royale Solos";
    return "Unknown Gamemode";
}

static std::wstring ReadPlaylistFromConfig()
{
    std::wstring result = L"";
    if (std::filesystem::exists("config.json"))
    {
        std::ifstream file("config.json");
        if (file.is_open())
        {
            std::string line;
            while (std::getline(file, line))
            {
                size_t pos = line.find("\"playlist\"");
                if (pos == std::string::npos)
                    pos = line.find("\"Playlist\"");
                if (pos != std::string::npos)
                {
                    size_t colon = line.find(":", pos);
                    if (colon != std::string::npos)
                    {
                        size_t start = line.find("\"", colon);
                        if (start != std::string::npos)
                        {
                            size_t end = line.find("\"", start + 1);
                            if (end != std::string::npos)
                            {
                                std::string parsed = line.substr(start + 1, end - start - 1);
                                result = std::wstring(parsed.begin(), parsed.end());
                                break;
                            }
                        }
                    }
                }
            }
            file.close();
        }
    }
    if (result.empty() && std::filesystem::exists("config.ini"))
    {
        std::ifstream file("config.ini");
        if (file.is_open())
        {
            std::string line;
            while (std::getline(file, line))
            {
                size_t pos = line.find("playlist");
                if (pos == std::string::npos)
                    pos = line.find("Playlist");
                if (pos != std::string::npos)
                {
                    size_t eq = line.find("=", pos);
                    if (eq != std::string::npos)
                    {
                        std::string parsed = line.substr(eq + 1);
                        while (!parsed.empty() && (parsed.front() == ' ' || parsed.front() == '\t' || parsed.front() == '"'))
                            parsed.erase(parsed.begin());
                        while (!parsed.empty() && (parsed.back() == ' ' || parsed.back() == '\t' || parsed.back() == '\r' || parsed.back() == '\n' || parsed.back() == '"'))
                            parsed.pop_back();
                        result = std::wstring(parsed.begin(), parsed.end());
                        break;
                    }
                }
            }
            file.close();
        }
    }
    return result;
}

static std::string GetActiveGamemodeName()
{
    auto GameState = GetWorld() ? (AFortGameStateAthena*)GetWorld()->GameState : nullptr;
    auto ActivePlaylist = GameState ? GameState->CurrentPlaylistInfo.BasePlaylist : nullptr;
    if (ActivePlaylist)
    {
        std::string pathName = UKismetSystemLibrary::GetPathName(ActivePlaylist).ToString().c_str();
        return GetGamemodeName(pathName);
    }
    g_ConfigState.FrameCount++;
    if (g_ConfigState.FrameCount >= 60)
    {
        g_ConfigState.FrameCount = 0;
        bool bNeedsReload = false;
        std::error_code ec;
        if (std::filesystem::exists("config.json"))
        {
            auto jsonTime = std::filesystem::last_write_time("config.json", ec);
            if (!ec && jsonTime != g_ConfigState.LastJsonTime)
            {
                g_ConfigState.LastJsonTime = jsonTime;
                bNeedsReload = true;
            }
        }
        if (std::filesystem::exists("config.ini"))
        {
            auto iniTime = std::filesystem::last_write_time("config.ini", ec);
            if (!ec && iniTime != g_ConfigState.LastIniTime)
            {
                g_ConfigState.LastIniTime = iniTime;
                bNeedsReload = true;
            }
        }
        if (bNeedsReload || g_ConfigState.CurrentPlaylist.empty())
        {
            auto parsed = ReadPlaylistFromConfig();
            if (!parsed.empty())
                g_ConfigState.CurrentPlaylist = parsed;
        }
    }
    if (!g_ConfigState.CurrentPlaylist.empty())
        return GetGamemodeName(g_ConfigState.CurrentPlaylist);
    return GetGamemodeName(Playlist);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

UINT g_ResizeWidth = 0, g_ResizeHeight = 0;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

auto WindowWidth = 533;
auto WindowHeight = 400;

void GUI::Init()
{
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT { 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASS wc {};
    wc.lpszClassName = L"RemixWC";
    wc.lpfnWndProc = WndProc;
    RegisterClass(&wc);

    RECT rect = { 0, 0, (LONG)(WindowWidth * main_scale), (LONG)(WindowHeight * main_scale) };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME, FALSE);

    auto hWnd = CreateWindow(wc.lpszClassName, L"Better-Remix", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME, 100, 100, rect.right - rect.left,
        rect.bottom - rect.top, nullptr, nullptr, nullptr, nullptr);

    IDXGISwapChain* g_pSwapChain = nullptr;
    ID3D11Device* g_pd3dDevice = nullptr;
    ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice,
            &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return;

    ID3D11RenderTargetView* g_mainRenderTargetView;

    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();

    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);
    DWORD dwMyID = ::GetCurrentThreadId();
    DWORD dwCurID = ::GetWindowThreadProcessId(hWnd, NULL);
    AttachThreadInput(dwCurID, dwMyID, TRUE);
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);
    SetActiveWindow(hWnd);
    AttachThreadInput(dwCurID, dwMyID, FALSE);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.IniFilename = NULL;
    // io.DisplaySize = ImGui::GetMainViewport()->Size;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImFontConfig FontConfig;
    FontConfig.FontDataOwnedByAtlas = false;
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)font, sizeof(font), 17.f, &FontConfig);

    auto& mStyle = ImGui::GetStyle();
    mStyle.WindowRounding = 12.f;
    mStyle.FrameRounding = 6.f;
    mStyle.ChildRounding = 8.f;
    mStyle.GrabRounding = 12.f;
    mStyle.ScrollbarRounding = 12.f;
    mStyle.WindowBorderSize = 0.f;
    mStyle.ChildBorderSize = 1.f;
    mStyle.FrameBorderSize = 0.f;
    mStyle.PopupRounding = 8.f;
    mStyle.ItemSpacing = ImVec2(10, 8);
    mStyle.ItemInnerSpacing = ImVec2(8, 6);
    mStyle.WindowPadding = ImVec2(10, 10);

    ImGuiStyle& style = mStyle;
    style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.95f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.50f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.15f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.60f);

    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImVec4 clear_color = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    bool done = false;
    bool g_SwapChainOccluded = false;

    while (!done)
    {
        MSG msg;

        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }

        if (done)
            break;

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            Sleep(10);
            continue;
        }

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            g_mainRenderTargetView->Release();

            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;

            ID3D11Texture2D* pBackBuffer;
            g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
            pBackBuffer->Release();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT { 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(WindowWidth * main_scale, WindowHeight * main_scale), ImGuiCond_Always);

        ImGui::Begin("Remix", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

        static int SelectedUI = 0;
        static int hasEvent = 0;

        if (hasEvent == 0)
        {
            hasEvent = 1;
            TArray<AActor*> EventScripts;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpecialEventScript::StaticClass(), &EventScripts);
            if (EventScripts.Num() > 0)
            {
                hasEvent = 2;
            }
            EventScripts.Free();
        }

        ImGui::BeginChild("Sidebar", ImVec2(130 * main_scale, 0), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
        ImGui::Text(" BETTER");
        ImGui::Text(" REMIX");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        auto renderNavButton = [](const char* label, int index, int& selectedVar) {
            bool isActive = (selectedVar == index);
            if (isActive)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.35f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.45f, 0.45f, 1.00f));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.15f, 0.15f, 0.50f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.20f, 0.20f, 0.80f));
            }

            if (ImGui::Button(label, ImVec2(-1, 32)))
            {
                selectedVar = index;
            }

            ImGui::PopStyleColor(3);
        };

        renderNavButton("Main", 0, SelectedUI);
        ImGui::Spacing();

        if (gsStatus == StartedMatch)
        {
            renderNavButton("Zones", 1, SelectedUI);
            ImGui::Spacing();

            if (hasEvent == 2)
            {
                renderNavButton("Events", 2, SelectedUI);
                ImGui::Spacing();
            }
        }

        renderNavButton("Misc", 3, SelectedUI);

        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 35 * main_scale);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.00f));
        ImGui::Text("  by Benzi");
        ImGui::PopStyleColor();

        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("Content", ImVec2(0, 0), false);

        static char commandBuffer[1024] = { 0 };
        auto GameMode = GetWorld() ? (AFortGameModeAthena*)GetWorld()->AuthorityGameMode : nullptr;
        auto GameState = GetWorld() ? (AFortGameStateAthena*)GetWorld()->GameState : nullptr;
        switch (SelectedUI)
        {
        case 0:
            if (gsStatus >= Joinable)
            {
            ImGui::BeginChild("ServerInfo", ImVec2(0, 200 * main_scale), true, ImGuiWindowFlags_NoScrollbar);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
                ImGui::Text("SERVER STATS");
                ImGui::PopStyleColor();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Text((std::string("Status: ") + (gsStatus == NotReady ? "Setting up..." : (gsStatus == Joinable ? "Joinable!" : "Match Started"))).c_str());
                if (GameMode)
                {
                    ImGui::Text((std::string("Players: ") + std::to_string(GameMode->AlivePlayers.Num()) + "  |  Port: " + std::to_string(Port)).c_str());
                    std::string GamemodeName = GetActiveGamemodeName();
                    ImGui::Text((std::string("Mode: ") + GamemodeName).c_str());
                    ImGui::Text((std::string("Time: ") + std::to_string((int)floor(UGameplayStatics::GetTimeSeconds(GameMode))) + "s").c_str());
                }
                ImGui::EndChild();
            }
            else
            {
                ImGui::BeginChild("ServerInfo", ImVec2(0, 60 * main_scale), true);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
                ImGui::Text("SERVER STATS");
                ImGui::PopStyleColor();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Text("Status: Setting up the server...");
                ImGui::EndChild();
            }
            ImGui::Spacing();

            if (gsStatus <= Joinable)
            {
                ImGui::Checkbox("Lategame", &bLateGame);
                ImGui::Spacing();
            }

            if (gsStatus == Joinable)
            {
                if (ImGui::Button("Start Bus Early", ImVec2(-1, 30)))
                {
                    bStartBusEarlyRequest = true;
                }
                ImGui::Spacing();
            }

            ImGui::BeginChild("ConsoleArea", ImVec2(0, 200 * main_scale), true, ImGuiWindowFlags_NoScrollbar);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
            ImGui::Text("CONSOLE");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::PushItemWidth(-1);
            ImGui::InputTextWithHint("##ConsoleCmd", "Enter console command...", commandBuffer, 1024);
            ImGui::PopItemWidth();
            ImGui::Spacing();

            if (ImGui::Button("Execute Command", ImVec2(-1, 30)))
            {
                std::string str = commandBuffer;
                auto wstr = std::wstring(str.begin(), str.end());
                UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), FString(wstr.c_str()), nullptr);
            }
            ImGui::EndChild();
            break;
        case 1:
            ImGui::BeginChild("ZoneControls", ImVec2(0, 0), true);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
            ImGui::Text("SAFE ZONE MANAGEMENT");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Pause Safe Zone", ImVec2(-1, 35)))
            {
                UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), FString(L"pausesafezone"), nullptr);
            }
            ImGui::Spacing();

            if (ImGui::Button("Resume Safe Zone", ImVec2(-1, 35)))
            {
                UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), FString(L"startsafezone"), nullptr);
            }
            ImGui::Spacing();

            if (ImGui::Button("Skip Safe Zone", ImVec2(-1, 35)))
            {
                auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GetWorld());
                if (GamePhaseLogic && GamePhaseLogic->SafeZoneIndicator)
                {
                    GamePhaseLogic->SafeZoneIndicator->SafeZoneStartShrinkTime = (float)UGameplayStatics::GetTimeSeconds(GetWorld());
                    GamePhaseLogic->SafeZoneIndicator->SafeZoneFinishShrinkTime = GamePhaseLogic->SafeZoneIndicator->SafeZoneStartShrinkTime + 0.05f;
                }
            }
            ImGui::Spacing();

            if (ImGui::Button("Start Shrinking Safe Zone", ImVec2(-1, 35)))
            {
                auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GetWorld());
                if (GamePhaseLogic && GamePhaseLogic->SafeZoneIndicator)
                {
                    GamePhaseLogic->SafeZoneIndicator->SafeZoneStartShrinkTime = (float)UGameplayStatics::GetTimeSeconds(GetWorld());
                }
            }
            ImGui::EndChild();
            break;
        case 2:
            ImGui::BeginChild("EventControls", ImVec2(0, 0), true);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
            ImGui::Text("SPECIAL EVENTS");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextWrapped("Triggering the event will initialize all required mesh nodes and start the server-side cinematic event sequence.");
            ImGui::Spacing();
            ImGui::Spacing();

            if (ImGui::Button("Start Event", ImVec2(-1, 40)))
            {
                TArray<AActor*> MeshActors;
                UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpecialEventScriptMeshActor::StaticClass(), &MeshActors);

                TArray<AActor*> EventScripts;
                UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpecialEventScript::StaticClass(), &EventScripts);

                if (MeshActors.Num() > 0 && EventScripts.Num() > 0)
                {
                    auto MeshActor = (ASpecialEventScriptMeshActor*)MeshActors[0];
                    auto Scr = (ASpecialEventScript*)EventScripts[0];

                    Scr->DelayAfterConentLoad.Curve.CurveTable = nullptr;
                    Scr->DelayAfterConentLoad.Value = 0.6f;

                    auto MeshNetworkSubsystem = (UMeshNetworkSubsystem*)TUObjectArray::FindFirstObject("MeshNetworkSubsystem");

                    if (MeshNetworkSubsystem)
                        MeshNetworkSubsystem->NodeType = EMeshNetworkNodeType::Root;

                    MeshActor->MeshRootStartEvent();

                    if (MeshNetworkSubsystem)
                        MeshNetworkSubsystem->NodeType = EMeshNetworkNodeType::Edge;
                    MeshActor->OnRep_RootStartTime(FDateTime());
                }
                MeshActors.Free();
                EventScripts.Free();
            }
            ImGui::EndChild();
            break;
        case 3:
            ImGui::BeginChild("GameplayModifiers", ImVec2(0, 195 * main_scale), true);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
            ImGui::Text("GAMEPLAY MODIFIERS");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Columns(2, nullptr, false);
            ImGui::Checkbox("Infinite Materials", &bInfiniteMats);
            ImGui::Checkbox("Infinite Ammo", &bInfiniteAmmo);
            ImGui::NextColumn();
            ImGui::Checkbox("Keep Inventory", &bKeepInventory);
            ImGui::Checkbox("No Fall Damage", &bNoFallDamage);
            ImGui::Columns(1);

            ImGui::Spacing();
            ImGui::SliderInt("Siphon Amount", &SiphonAmount, 0, 200);
            ImGui::Spacing();
            if (ImGui::SliderInt("Tick Rate", &MaxTickRate, 30, 120))
            {
                if (GetWorld() && GetWorld()->NetDriver)
                {
                    GetWorld()->NetDriver->NetServerMaxTickRate = MaxTickRate;
                }
            }
            ImGui::EndChild();
            ImGui::Spacing();

            ImGui::BeginChild("DangerZone", ImVec2(0, 95 * main_scale), true, ImGuiWindowFlags_NoScrollbar);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.35f, 0.35f, 1.00f));
            ImGui::Text("DANGER ZONE");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.15f, 0.15f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.18f, 0.18f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.20f, 0.20f, 1.00f));
            if (ImGui::Button("Reset Builds", ImVec2(-1, 35)))
            {
                TArray<AActor*> Actors;
                UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuildingSMActor::StaticClass(), &Actors);

                for (auto& Actor : Actors)
                {
                    auto Build = (ABuildingSMActor*)Actor;
                    if (Build && Build->bPlayerPlaced)
                        Build->K2_DestroyActor();
                }

                Actors.Free();
            }
            ImGui::PopStyleColor(3);
            ImGui::EndChild();
            break;
        }
        ImGui::EndChild();

        ImGui::End();

        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    g_pSwapChain->Release();
    g_pd3dDeviceContext->Release();
    g_pd3dDevice->Release();
    DestroyWindow(hWnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    TerminateProcess(GetCurrentProcess(), 0);
}
