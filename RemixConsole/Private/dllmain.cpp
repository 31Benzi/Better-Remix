#include "pch.h"
#include "memcury.h"
#include "../Public/Console.h"
#include <thread>

template <typename CVarT>
CVarT* FindCVar(const wchar_t* CVarStr)
{
    auto sRef = Memcury::Scanner::FindStringRef(CVarStr);

    if (!sRef.IsValid())
        return nullptr;

    uint64_t BeforeVars = 0;

    for (int i = 0; i < 2000; i++)
    {
        auto Ptr = (uint8_t*)(sRef.Get() - i);

        if (*Ptr == 0x48 && *(Ptr + 1) == 0x83 && *(Ptr + 2) == 0xEC)
        {
            BeforeVars = uint64_t(Ptr);
            break;
        }
        else if (*Ptr == 0x48 && *(Ptr + 1) == 0x81 && *(Ptr + 2) == 0xEC)
        {
            BeforeVars = uint64_t(Ptr);
            break;
        }
    }

    for (int i = 0; i < 2000; i++)
    {
        auto Ptr = (uint8_t*)(BeforeVars + i);

        if (*Ptr == 0x4C && *(Ptr + 1) == 0x8D && *(Ptr + 2) == 0x05)
            return Memcury::Scanner(Ptr).RelativeOffset(3).GetAs<CVarT*>();
    }

    return nullptr;
}

void ForceIris(uintptr_t IrisBool)
{
    const auto sizeOfImage = Memcury::PE::GetNTHeaders()->OptionalHeader.SizeOfImage;
    const auto scanBytes = reinterpret_cast<std::uint8_t*>(Memcury::PE::GetModuleBase());

    for (auto i = 0ul; i < sizeOfImage - 5; ++i)
    {
        if (scanBytes[i] == 0x83 || scanBytes[i] == 0x39)
        {
            if (Memcury::PE::Address(&scanBytes[i]).RelativeOffset(2, scanBytes[i] == 0x83).GetAs<void*>() == (void*)IrisBool)
            {
                DWORD og;
                VirtualProtect((LPVOID)(__int64(&scanBytes[i]) + 2), sizeof(uint32_t), PAGE_EXECUTE_READWRITE, &og);
                *(uint32_t*)(__int64(&scanBytes[i]) + 2) = 0x0;
                VirtualProtect((LPVOID)(__int64(&scanBytes[i]) + 2), sizeof(uint32_t), og, &og);
            }
        }
    }
}

void Main()
{
    while (true)
    {
        auto Engine = (UEngine*)TUObjectArray::FindFirstObject("FortEngine");
        if (Engine && Engine->GameViewport && Engine->GameViewport->World)
        {
            break;
        }
        Sleep(100);
    }

    auto Engine = (UEngine*)TUObjectArray::FindFirstObject("FortEngine");
    auto World = Engine->GameViewport->World;

    UKismetSystemLibrary::ExecuteConsoleCommand(World, FString(L"net.AllowEncryption 0"), nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(World, FString(L"log LogIris None"), nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(World, FString(L"log LogIrisRpc None"), nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(World, FString(L"log LogIrisBridge None"), nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(World, FString(L"log LogFortUIDirector None"), nullptr);

    UKismetSystemLibrary::ExecuteConsoleCommand(World, FString(L"Fort.MME.TacticalSprint 0"), nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(World, FString(L"Fort.MME.Hurdle 0"), nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(World, FString(L"Fort.MME.Sliding 0"), nullptr);

    auto IrisBool = FindCVar<uint32_t>(L"net.Iris.UseIrisReplication");
    if (IrisBool)
    {
        *IrisBool = 1;
        ForceIris((uintptr_t)IrisBool);
    }

    RemixConsole::Init();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        std::thread(Main).detach();
    }
    return TRUE;
}
