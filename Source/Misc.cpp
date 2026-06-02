#include "pch.h"
#include "Shared.h"
#include "memcury.h"
#include <algorithm>
#include <unordered_set>
#include "Config.h"
#include "CurlHttp.h"
#include <random>
#include <thread>
#include <chrono>
INIT_MODULE(Misc);

void PatchAllNetModes(uintptr_t AttemptDeriveFromURL)
{
    Memcury::PE::Address add{ nullptr };

    const auto sizeOfImage = Memcury::PE::GetNTHeaders()->OptionalHeader.SizeOfImage;
    const auto scanBytes = reinterpret_cast<std::uint8_t*>(Memcury::PE::GetModuleBase());

    for (auto i = 0ul; i < sizeOfImage - 5; ++i)
    {
        if (scanBytes[i] == 0xE8 || scanBytes[i] == 0xE9)
        {
            if (Memcury::PE::Address(&scanBytes[i]).RelativeOffset(1).GetAs<void*>() == (void*)AttemptDeriveFromURL)
            {
                add = Memcury::PE::Address(&scanBytes[i]);

                // scan for the read of World->NetDriver

                for (auto j = 0; j > -0x100000; j--) // so we find everything. no func is actually 1mb
                {
                    if ((scanBytes[i + j] & 0xF8) == 0x48 && ((scanBytes[i + j + 1] & 0xFC) == 0x80 || (scanBytes[i + j + 1] & 0xF8) == 0x38)
                        && (scanBytes[i + j + 2] & 0xF0) < 0xC0 && scanBytes[i + j + 2] != 0x65 && scanBytes[i + j + 2] != 0xBB && scanBytes[i + j + 3] == 0x48
                        && ((scanBytes[i + j + 1] & 0xFC) != 0x80 || scanBytes[i + j + 4] == 0x0))
                    {
                        // now, scan for if (NetDriver) return NM_Client;

                        bool found = false;
                        for (auto k = 4; k < 0x104; k++)
                        {
                            if (scanBytes[i + j + k] == 0x75)
                            {
                                auto Scuffness = __int64(&scanBytes[i + j + k + 5]);

                                if (*(uint32_t*)Scuffness != 0x108 && (scanBytes[i + j + k + 4] != 0xC || scanBytes[i + j + k + 5] != 0xB) && scanBytes[i + j + k + 4] != 0x09)
                                    continue;

                                Patch<uint16_t>(__int64(&scanBytes[i + j + k]), 0x9090);
                                if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
                                    Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
                                else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
                                {
                                    DWORD og;
                                    VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
                                    *(uint32*)(&scanBytes[i + j]) = 0x90909090;
                                    *(uint8*)(&scanBytes[i + j + 4]) = 0x90;
                                    VirtualProtect(&scanBytes[i + j], 5, og, &og);
                                }
                                FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
                                FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 2);
                                found = true;
                                break;
                            }
                            else if (scanBytes[i + j + k] == 0x74)
                            {
                                auto Scuffness = __int64(&scanBytes[i + j + k]);
                                Scuffness = (Scuffness + 2) + *(int8_t*)(Scuffness + 1);

                                if (*(uint32_t*)(Scuffness + 3) != 0x108 && (*(uint8_t*)(Scuffness + 2) != 0xC || *(uint8_t*)(Scuffness + 3) != 0xB)
                                    && *(uint8_t*)(Scuffness + 2) != 0x09)
                                    continue;

                                Patch<uint8_t>(__int64(&scanBytes[i + j + k]), 0xeb);
                                if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
                                    Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
                                else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
                                {
                                    DWORD og;
                                    VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
                                    *(uint32*)(&scanBytes[i + j]) = 0x90909090;
                                    *(uint8*)(&scanBytes[i + j + 4]) = 0x90;
                                    VirtualProtect(&scanBytes[i + j], 5, og, &og);
                                }
                                FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
                                FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 1);
                                found = true;
                                break;
                            }
                            else if (scanBytes[i + j + k] == 0x0F && scanBytes[i + j + k + 1] == 0x85)
                            {
                                auto Scuffness = __int64(&scanBytes[i + j + k + 9]);

                                if (*(uint32_t*)Scuffness != 0x108 && (scanBytes[i + j + k + 8] != 0xC || scanBytes[i + j + k + 9] != 0xB) && scanBytes[i + j + k + 8] != 0x09)
                                    continue;

                                DWORD og;
                                VirtualProtect(&scanBytes[i + j + k], 6, PAGE_EXECUTE_READWRITE, &og);
                                *(uint32*)(&scanBytes[i + j + k]) = 0x90909090;
                                *(uint16*)(&scanBytes[i + j + k + 4]) = 0x9090;
                                VirtualProtect(&scanBytes[i + j + k], 6, og, &og);
                                if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
                                    Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
                                else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
                                {
                                    DWORD og;
                                    VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
                                    *(uint32*)(&scanBytes[i + j]) = 0x90909090;
                                    *(uint8*)(&scanBytes[i + j + 4]) = 0x90;
                                    VirtualProtect(&scanBytes[i + j], 5, og, &og);
                                }
                                FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
                                FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 6);
                                found = true;
                                break;
                            }
                            else if (scanBytes[i + j + k] == 0x0F && scanBytes[i + j + k + 1] == 0x84)
                            {
                                auto Scuffness = __int64(&scanBytes[i + j + k]);
                                Scuffness = (Scuffness + 6) + *(int32_t*)(Scuffness + 2);

                                if (*(uint32_t*)(Scuffness + 3) != 0x108 && (*(uint8_t*)(Scuffness + 2) != 0xC || *(uint8_t*)(Scuffness + 3) != 0xB)
                                    && *(uint8_t*)(Scuffness + 2) != 0x09)
                                    continue;

                                Patch<uint16_t>(__int64(&scanBytes[i + j + k]), 0xe990);
                                if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
                                    Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
                                else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
                                {
                                    DWORD og;
                                    VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
                                    *(uint32*)(&scanBytes[i + j]) = 0x90909090;
                                    *(uint8*)(&scanBytes[i + j + 4]) = 0x90;
                                    VirtualProtect(&scanBytes[i + j], 5, og, &og);
                                }
                                FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
                                FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 2);
                                found = true;
                                break;
                            }
                        }
                        if (found)
                            break;
                    }
                }
            }
        }
    }
}

int WorldNetMode()
{
    return 1;
}

uint32 CheckCheckpointHeartBeat()
{
    return -1;
}

void* (*SendRequestNowOG)(void* Arg1, void* MCPData, int a3);
void* SendRequestNow(void* Arg1, void* MCPData, int a3)
{
    return SendRequestNowOG(Arg1, MCPData, 3); // CXC_Public
}

float GetMaxTickRate(UEngine* Engine, float DeltaTime, bool bAllowFrameRateSmoothing)
{
    auto NetDriver = GetWorld()->NetDriver;

    if (NetDriver)
        return (float)NetDriver->NetServerMaxTickRate;

    return (float)30.f;
}

static std::unordered_set<uint64_t> g_OurPushed;

void (*ServerMove_PerformMovementOG)(UFortMovementComp_Character* Comp, __int64 MoveData);
void ServerMove_PerformMovement(UFortMovementComp_Character* Comp, __int64 MoveData)
{
    static auto ResolveWeak = (__int64(*)(__int64*))(ImageBase + 0x4B6D02C);

    __int64 a1 = (__int64)Comp;
    auto GetPawn = [&]() -> __int64 { return *(__int64*)(a1 + 0x178); };

    if (bNoFallDamage)
    {
        if (auto Pawn = GetPawn())
        {
            *(uint8_t*)(Pawn + 0x6F0) |= 8;
        }
    }

    __int64 CliData = *(__int64*)(MoveData + 0xC8);
    int32_t CliCount = *(int32_t*)(MoveData + 0xD0);

    bool bHasClimbing = false;
    for (int32_t i = 0; i < CliCount && !bHasClimbing; ++i)
    {
        __int64 Rtd = ResolveWeak((__int64*)(CliData + 0x8 * i));
        if (Rtd && ((UObject*)Rtd)->IsA<UFortMovementMode_ClamberingRuntimeData>())
            bHasClimbing = true;
    }

    bool bHasClimbingPushed = false;
    for (int i = 0; i < Comp->PushedMovementModeExtensionRuntimeData.Num(); ++i)
    {
        auto PushedMME = Comp->PushedMovementModeExtensionRuntimeData[i];
        if (PushedMME && PushedMME->IsA<UFortMovementMode_ClamberingRuntimeData>())
        {
            bHasClimbingPushed = true;
            break;
        }
    }

    if (bHasClimbing || bHasClimbingPushed)
    {
        auto Pawn = (AFortPlayerPawn*)Comp->PawnOwner;
        auto ActiveMMEs = *(TArray<TWeakObjectPtr<UFortMovementMode_BaseExtRuntimeData>>*)(MoveData + 0xC8);

        Comp->MovementModeExtensionRuntimeAddedLastTick.Clear();

        static auto ConstructRuntimeCtx = (void (*)(FMMERuntimeContext*, UObject*))(ImageBase + 0x1BCE080);
        static auto OnExtensionPush = (void (*)(UObject*, FMMERuntimeContext*))(ImageBase + 0x351D4B8);
        static auto OnExtensionDeactivate = (void (*)(UObject*, FMMERuntimeContext*))(ImageBase + 0x2DD1638);

        for (auto& WeakActiveMME : ActiveMMEs)
        {
            auto ActiveMME = WeakActiveMME.Get();

            if (ActiveMME->IsA<UFortMovementMode_SlidingRuntimeData>())
                continue;

            bool bAlreadyPushed = false;
            for (auto& PushedMME : Comp->PushedMovementModeExtensionRuntimeData)
            {
                if (PushedMME->RuntimeID == ActiveMME->RuntimeID &&
                    PushedMME->LogicID.MovementMode_ExtCRC_ID == ActiveMME->LogicID.MovementMode_ExtCRC_ID)
                {
                    bAlreadyPushed = true;
                    break;
                }
            }
            if (bAlreadyPushed) continue;

            static auto GetExtObj = (UFortMovementMode_BaseExtLogic * (*)(UFortMovementComp_Character*, FMovementMode_ExtID*))(ImageBase + 0x2DD10D4);
            auto ExtObj = GetExtObj(Comp, &ActiveMME->LogicID);

            (*(uint8*)(__int64(ExtObj) + 0x64))++;
            if (*(uint8*)(__int64(ExtObj) + 0x64) == 0xff)
                *(uint8*)(__int64(ExtObj) + 0x64) = 0;

            *(bool*)(__int64(ActiveMME) + 0x31) = Pawn->Role == ENetRole::ROLE_Authority;

            Comp->PushedMovementModeExtensionRuntimeData.Add(ActiveMME);
            Comp->NewlyAddedMovementModeExtensionRuntimeData.Add(ActiveMME);
            Comp->MovementModeExtensionRuntimeAddedLastTick.Add(ActiveMME);

            FMMERuntimeContext RuntimeContext;
            ConstructRuntimeCtx(&RuntimeContext, ActiveMME);
            OnExtensionPush(ExtObj, &RuntimeContext);
        }

        for (int i = 0; i < Comp->PushedMovementModeExtensionRuntimeData.Num(); i++)
        {
            auto& PushedMME = Comp->PushedMovementModeExtensionRuntimeData[i];

            if (PushedMME->IsA<UFortMovementMode_SlidingRuntimeData>())
                continue;

            bool bIsActive = false;
            for (auto& WeakActiveMME : ActiveMMEs)
            {
                auto ActiveMME = WeakActiveMME.Get();
                if (ActiveMME->RuntimeID == PushedMME->RuntimeID &&
                    PushedMME->LogicID.MovementMode_ExtCRC_ID == ActiveMME->LogicID.MovementMode_ExtCRC_ID)
                {
                    bIsActive = true;
                    break;
                }
            }

            if (!bIsActive)
            {
                Comp->PushedMovementModeExtensionRuntimeData.Remove(i);

                for (int j = 0; j < Comp->ActiveLayeredMovementModeRuntimeData.Num(); j++)
                {
                    if (Comp->ActiveLayeredMovementModeRuntimeData[j] == PushedMME)
                    {
                        Comp->ActiveLayeredMovementModeRuntimeData.Remove(j);
                        break;
                    }
                }

                for (int j = 0; j < Comp->NewlyAddedMovementModeExtensionRuntimeData.Num(); j++)
                {
                    if (Comp->NewlyAddedMovementModeExtensionRuntimeData[j] == PushedMME)
                    {
                        Comp->NewlyAddedMovementModeExtensionRuntimeData.Remove(j);
                        break;
                    }
                }

                i--;
            }
        }

        if (Comp->PushedMovementModeExtensionRuntimeData.Num() == 0 && Comp->ActiveMovementModeExtension)
        {
            auto ActiveRuntimeData = *(UObject**)(__int64(Comp) + 0x5528);

            FMMERuntimeContext RuntimeContext;
            ConstructRuntimeCtx(&RuntimeContext, ActiveRuntimeData);
            OnExtensionDeactivate(Comp->ActiveMovementModeExtension, &RuntimeContext);

            auto ActiveMME = Comp->ActiveMovementModeExtension;
            Comp->ActiveMovementModeExtension = nullptr;
            *(UObject**)(__int64(Comp) + 0x5528) = nullptr;

            auto SomeDeactivateThing = (void (*)(UFortMovementMode_BaseExtLogic*))ActiveMME->VTable[0x59];
            SomeDeactivateThing(ActiveMME);
        }

        return ServerMove_PerformMovementOG(Comp, MoveData);
    }

    Comp->bIgnoreClientMovementErrorChecksAndCorrection = true;
    Comp->bServerAcceptClientAuthoritativePosition = true;

    static auto PushMME = (__int64(*)(__int64, uint64_t*, __int64))(ImageBase + 0x9E35058);
    static auto RemoveMME = (void(*)(__int64, __int64*, int32_t))(ImageBase + 0x9E53F00);
    static auto GetExtLogic = (void(*)(__int64, __int64*, int32_t))(ImageBase + 0x2AAABA0);
    static auto BuildRTDCtx = (void* (*)(__int64*, __int64))(ImageBase + 0x1BCE080);
    static auto OnDeactivate = (void(*)(__int64, __int64*))(ImageBase + 0x2DD1638);
    static auto ArrayRemove = (void*)(ImageBase + 0x28785E8);
    static auto ArrayFind = (int32_t(*)(__int64, __int64*))(ImageBase + 0x2C62F84);

    auto GetPushedData = [&]() -> __int64* { return *(__int64**)(a1 + 0x5540); };
    auto GetPushedCnt = [&]() -> int32_t { return *(int32_t*)(a1 + 0x5548); };

    auto MakeKey = [&](int32_t TypeId, uint8_t Subtype) -> uint64_t {
        return (uint64_t)(uint32_t)TypeId ^ ((uint64_t)Subtype << 32) ^ ((uint64_t)(uintptr_t)Comp << 8);
        };

    struct MMEIntent { int32_t TypeId; uint8_t Subtype; };
    constexpr int32_t kMaxIntent = 32;
    MMEIntent WantedMMEs[kMaxIntent];
    int32_t WantedCount = 0;

    __int64 Pawn = GetPawn();
    if (Pawn)
    {
        for (int32_t i = 0; i < CliCount && WantedCount < kMaxIntent; ++i)
        {
            __int64 Rtd = ResolveWeak((__int64*)(CliData + 0x8 * i));
            if (!Rtd) continue;

            int32_t TypeId = *(int32_t*)(Rtd + 0x28);
            uint8_t Subtype = *(uint8_t*)(Rtd + 0x2C);

            __int64 ExtLogicBuf[2] = {};
            GetExtLogic(Pawn, ExtLogicBuf, TypeId);
            if (!ExtLogicBuf[0]) continue;

            WantedMMEs[WantedCount++] = { TypeId, Subtype };
        }
    }

    ServerMove_PerformMovementOG(Comp, MoveData);

    Pawn = GetPawn();
    if (!Pawn) return;

    auto UpdateRepMME = [&](__int64 RepPawn, __int64 Rtd)
        {
            if (!RepPawn || !Rtd) return;

            *(__int64*)(RepPawn + 0x1CB8) = Rtd;

            __int64 LayeredPtr = *(__int64*)(RepPawn + 0x1CC0);
            int32_t LayeredCnt = *(int32_t*)(RepPawn + 0x1CC8);
            int32_t LayeredCap = *(int32_t*)(RepPawn + 0x1CCC);
            if (LayeredPtr && LayeredCnt < LayeredCap)
            {
                bool bInLayered = false;
                for (int32_t j = 0; j < LayeredCnt && !bInLayered; ++j)
                    if (*(__int64*)(LayeredPtr + 0x8 * j) == Rtd)
                        bInLayered = true;
                if (!bInLayered)
                {
                    *(__int64*)(LayeredPtr + 0x8 * LayeredCnt) = Rtd;
                    *(int32_t*)(RepPawn + 0x1CC8) = LayeredCnt + 1;
                }
            }

            ++ * (uint8_t*)(RepPawn + 0x1CEA);
        };

    for (int32_t i = 0; i < WantedCount; ++i)
    {
        int32_t TypeId = WantedMMEs[i].TypeId;
        uint8_t Subtype = WantedMMEs[i].Subtype;

        bool bAlreadyPushed = false;
        int32_t Pc = GetPushedCnt();
        __int64* Pd = Pc > 0 ? GetPushedData() : nullptr;
        for (int32_t j = 0; j < Pc && !bAlreadyPushed; ++j)
        {
            __int64 P = Pd[j];
            if (P && *(int32_t*)(P + 0x28) == TypeId && *(uint8_t*)(P + 0x2C) == Subtype)
                bAlreadyPushed = true;
        }
        if (bAlreadyPushed)
        {
            g_OurPushed.erase(MakeKey(TypeId, Subtype));
            continue;
        }

        {
            int32_t RrCount = *(int32_t*)(a1 + 0x55D8);
            bool bInRr = false;
            if (RrCount > 0)
            {
                __int64 RrData = *(__int64*)(a1 + 0x55D0);
                for (int32_t j = 0; j < RrCount && !bInRr; ++j)
                    if (*(int32_t*)(RrData + 0x8 * j) == TypeId && *(uint8_t*)(RrData + 0x8 * j + 0x4) == Subtype)
                        bInRr = true;
            }
            if (bInRr) continue;
        }

        __int64 ExtLogicBuf[2] = {};
        GetExtLogic(Pawn, ExtLogicBuf, TypeId);
        __int64 ExtLogic = ExtLogicBuf[0];
        if (!ExtLogic) continue;
        *(uint8_t*)(ExtLogic + 0x64) = Subtype;

        uint64_t Pair = (uint64_t)(uint32_t)TypeId | ((uint64_t)Subtype << 32);
        PushMME(a1, &Pair, 0);
        g_OurPushed.insert(MakeKey(TypeId, Subtype));

        __int64 RepPawn = *(__int64*)(a1 + 0x160);
        if (RepPawn)
        {
            int32_t PcNow = GetPushedCnt();
            __int64* PdNow = PcNow > 0 ? GetPushedData() : nullptr;
            __int64 PushedRtd = 0;
            for (int32_t j = 0; j < PcNow && !PushedRtd; ++j)
            {
                __int64 P = PdNow[j];
                if (P && *(int32_t*)(P + 0x28) == TypeId && *(uint8_t*)(P + 0x2C) == Subtype)
                    PushedRtd = P;
            }
            UpdateRepMME(RepPawn, PushedRtd);
        }
    }

    constexpr int32_t kMaxMME = 64;
    __int64 ToRemove[kMaxMME];
    int32_t Nr = 0;
    {
        int32_t Pc = GetPushedCnt();
        __int64* Pd = Pc > 0 ? GetPushedData() : nullptr;
        for (int32_t j = 0; j < Pc && Nr < kMaxMME; ++j)
        {
            __int64 PushedRtd = Pd[j];
            if (!PushedRtd) continue;

            int32_t TypeId = *(int32_t*)(PushedRtd + 0x28);
            uint8_t Subtype = *(uint8_t*)(PushedRtd + 0x2C);

            if (!g_OurPushed.count(MakeKey(TypeId, Subtype))) continue;

            bool bClientHas = false;
            for (int32_t i = 0; i < CliCount && !bClientHas; ++i)
            {
                __int64 Crtd = ResolveWeak((__int64*)(CliData + 0x8 * i));
                if (Crtd && *(int32_t*)(Crtd + 0x28) == TypeId && *(uint8_t*)(Crtd + 0x2C) == Subtype)
                    bClientHas = true;
            }
            if (!bClientHas)
                ToRemove[Nr++] = PushedRtd;
        }
    }

    for (int32_t k = 0; k < Nr; ++k)
    {
        __int64 Rtd = ToRemove[k];
        int32_t TypeId = *(int32_t*)(Rtd + 0x28);
        uint8_t Subtype = *(uint8_t*)(Rtd + 0x2C);
        g_OurPushed.erase(MakeKey(TypeId, Subtype));

        __int64 ExtLogicBuf[2] = {};
        GetExtLogic(Pawn, ExtLogicBuf, TypeId);
        __int64 ExtLogic = ExtLogicBuf[0];

        auto FireDeactivate = [&](__int64 RtdForCtx)
            {
                __int64 Ctx[2] = {};
                BuildRTDCtx(Ctx, RtdForCtx);
                if (ExtLogic)
                {
                    OnDeactivate(ExtLogic, Ctx);
                    (*(void(**)(__int64))(*(uint64_t*)ExtLogic + 0x2C8))(ExtLogic);
                }
            };

        if (Rtd == *(__int64*)(a1 + 0x5528))
        {
            FireDeactivate(Rtd);
            *(__int64*)(a1 + 0x5528) = 0;
            *(__int64*)(a1 + 0x5520) = 0;
        }

        if (ArrayFind(a1 + 0x5580, &Rtd) >= 0)
        {
            FireDeactivate(Rtd);
            ((void(*)(__int64, __int64*))ArrayRemove)(a1 + 0x5580, &Rtd);
        }

        __int64 RepPawn = *(__int64*)(a1 + 0x160);
        if (RepPawn)
        {
            if (*(__int64*)(RepPawn + 0x1CB8) == Rtd)
                *(__int64*)(RepPawn + 0x1CB8) = 0;

            __int64 LayeredPtr = *(__int64*)(RepPawn + 0x1CC0);
            int32_t LayeredCnt = *(int32_t*)(RepPawn + 0x1CC8);
            if (LayeredPtr && LayeredCnt > 0)
            {
                for (int32_t j = 0; j < LayeredCnt; ++j)
                {
                    if (*(__int64*)(LayeredPtr + 0x8 * j) == Rtd)
                    {
                        for (int32_t m = j; m < LayeredCnt - 1; ++m)
                            *(__int64*)(LayeredPtr + 0x8 * m) = *(__int64*)(LayeredPtr + 0x8 * (m + 1));
                        *(__int64*)(LayeredPtr + 0x8 * (LayeredCnt - 1)) = 0;
                        *(int32_t*)(RepPawn + 0x1CC8) = LayeredCnt - 1;
                        break;
                    }
                }
            }

            ++ * (uint8_t*)(RepPawn + 0x1CEA);
        }

        RemoveMME(a1, &Rtd, 1);
    }
}

std::unordered_set<AActor*> SpawnedSpawners;
void RegisterToLivingWorldManager(void* IFace, AActor* Actor)
{
    if (auto Spawner = Actor->Cast<AFortAthenaLivingWorldVehiclePointProvider>())
    {
        if (SpawnedSpawners.contains(Actor))
            return;

        SpawnedSpawners.emplace(Actor);

        static UEAllocatedMap<FName, TSoftObjectPtr<UFortVehicleItemDefinition>> VehicleSpawnerMap = {
        };

        if (VehicleSpawnerMap.size() == 0)
        {
            TSoftObjectPtr<UFortVehicleItemDefinition> s;
            s.ObjectID.AssetPath.PackageName = L"/Clyde/Valet/VID_Clyde_Valet_Lowrider_Vehicle";
            s.ObjectID.AssetPath.AssetName = L"VID_Clyde_Valet_Lowrider_Vehicle";

            VehicleSpawnerMap[FName(L"Athena.Vehicle.SpawnLocation.Valet.Lowrider.Base")] = s;

            s.ObjectID.AssetPath.PackageName = L"/Clyde/Valet/VID_Clyde_Valet_Lowrider_Vehicle_Forced";
            s.ObjectID.AssetPath.AssetName = L"VID_Clyde_Valet_Lowrider_Vehicle_Forced";

            VehicleSpawnerMap[FName(L"Athena.Vehicle.SpawnLocation.Valet.Lowrider")] = s;

            s.ObjectID.AssetPath.PackageName = L"/Clyde/Valet/VID_Clyde_Valet_SportsCar_Vehicle";
            s.ObjectID.AssetPath.AssetName = L"VID_Clyde_Valet_SportsCar_Vehicle";

            VehicleSpawnerMap[FName(L"Athena.Vehicle.SpawnLocation.Valet.SportsCar.Base")] = s;

            s.ObjectID.AssetPath.PackageName = L"/Clyde/Valet/VID_Clyde_Valet_BigRig_Vehicle_Forced";
            s.ObjectID.AssetPath.AssetName = L"VID_Clyde_Valet_BigRig_Vehicle_Forced";

            VehicleSpawnerMap[FName(L"Athena.Vehicle.SpawnLocation.Valet.BigRig.Clyde")] = s;
        }

        UFortVehicleItemDefinition* VehicleDefinition = nullptr;
        bool bAlwaysSpawn = false;
        static auto AlwaysSpawnTag = FName(L"Athena.Vehicle.SpawnLocation.AlwaysSpawn");
        for (auto& Tag : Spawner->FiltersTags.GameplayTags)
        {
            if (VehicleSpawnerMap.contains(Tag.TagName))
            {
                VehicleDefinition = VehicleSpawnerMap[Tag.TagName].Get();
                // break;
            }

            if (Tag.TagName == AlwaysSpawnTag)
                bAlwaysSpawn = true;
        }

        if (VehicleDefinition)
        {
            if (!bAlwaysSpawn)
            {
                double Min = std::clamp(EvaluateScalableFloat(VehicleDefinition->VehicleMinSpawnPercent) * 0.01f, 0.0f, 1.0f);
                double Max = std::clamp(EvaluateScalableFloat(VehicleDefinition->VehicleMaxSpawnPercent) * 0.01f, 0.0f, 1.0f);

                auto SpawnPercent = Min + (Max - Min) * (rand() / (float)RAND_MAX);
                auto bShouldSpawn = (rand() / (float)RAND_MAX) <= SpawnPercent;

                if (!bShouldSpawn)
                    return;
            }

            auto Vehicle = SpawnActor<AFortAthenaVehicle>(VehicleDefinition->VehicleActorClass.Get(), Spawner->K2_GetActorLocation(), Spawner->K2_GetActorRotation());

            if (!Vehicle)
                return;

            Vehicle->InitialOverlapBehavior = Spawner->InitialOverlapBehavior;

            static auto GetVehicleModConfigFromTag = (UFortVehicleModConfig * (*)(UFortVehicleInterface*, FGameplayTag*))(ImageBase + 0x9BCADB8);
            static auto ApplyToVehicleInternal = (void (*)(UFortVehicleModConfig*, UFortVehicleInterface*))(ImageBase + 0xA60996C);


            auto VehicleInterface = (UFortVehicleInterface*)GetInterfaceAddress(Vehicle, UFortVehicleInterface::StaticClass());
            static auto SlinkyTag = FName(L"Vehicle.Mod.Slinky.Clyde");
            for (auto& Tag : Spawner->ForceMods.GameplayTags)
            {
                if (Tag.TagName == SlinkyTag)
                {
                    auto ModConfig_C = FindObject<UBlueprintGeneratedClass>(L"/Clyde/Valet/Lowrider/SlinkyMod/VehicleModConfig_Clyde_Slinky.VehicleModConfig_Clyde_Slinky_C");

                    auto ModConfig = (UFortVehicleModConfig*)ModConfig_C->DefaultObject;

                    Vehicle->AppliedVehicleModTags.GameplayTags.Add(FGameplayTag(SlinkyTag));
                    ApplyToVehicleInternal(ModConfig, VehicleInterface);
                }
                else
                    printf("Unknown mod tag: %s\n", Tag.TagName.ToString().c_str());
            }

            auto ForceApplyVehicleCosmetics = (void(*)(UFortVehicleInterface*, FSpawnerInfoForcedCosmetics&))VehicleInterface->VTable[0x193];
            ForceApplyVehicleCosmetics(VehicleInterface, Spawner->ForceCosmetics);
            //if (auto Car = Vehicle->Cast<AFortDagwoodVehicle>())
            //    Car->SetFuel(100.f);
        }
        else
        {
            for (auto& Tag : Spawner->FiltersTags.GameplayTags)
                printf("Unknown vehicle w/ tag: %s\n", Tag.TagName.ToString().c_str());
        }
    }
}

std::string generateHexString(size_t length = 32)
{
    static const char hexChars[] = "0123456789abcdef";
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<> dist(0, 15);

    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; i++)
        result += hexChars[dist(rng)];
    return result;
}

std::string ToString(std::wstring WData)
{
#pragma warning(suppress : 4244)
    return std::string(WData.begin(), WData.end());
}

void OnGetIP(void* _this, FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSucceeded)
{
    auto SessionID = generateHexString();
    auto IP = Response->GetContentAsString();
    auto PlaylistName = Playlist.substr(Playlist.rfind(L'.') + 1);
    auto GameMode = (AFortGameModeAthena*)GetWorld()->AuthorityGameMode;

    GameserverIP = IP.CStr();
    auto MMRequestBody = std::string("{\"type\":\"create\",\"payload\":{\"ip\":\"") + ToString(IP.CStr()) + "\",\"port\":" + std::to_string(Port) + ",\"id\":\"" + SessionID
        + "\",\"region\":\"" + Region + "\",\"playlist\":\"" + ToString(PlaylistName) + "\",\"maxPlayers\":" + std::to_string(GameMode->GameSession->MaxPlayers) + "}}";

    gsSocket->send(MMRequestBody);
    if (bReady)
        gsSocket->send("{\"type\":\"ready\",\"payload\":{}}");

    bInit = true;
}

void Listen()
{
    auto Engine = GetEngine();
    auto World = GetWorld();
    void* WorldCtx = ((void* (*)(UEngine*, UWorld*))(ImageBase + 0x1973A84))(Engine, World);

    auto NetDriverName = FName(L"GameNetDriver");

    auto NetDriver = World->NetDriver = ((UNetDriver * (*)(UEngine*, void*, FName, int))(ImageBase + 0x2EF3758))(Engine, WorldCtx, NetDriverName, 0);

    NetDriver->NetDriverName = NetDriverName;
    NetDriver->World = World;

    for (auto& LevelCollection : World->LevelCollections)
        LevelCollection.NetDriver = NetDriver;

    if (bProd)
        Port = UKismetMathLibrary::RandomIntegerInRange(1025, 49151);

    FURL URL{};
_:
    URL.Port = Port;

    auto InitListen = (bool (*)(UNetDriver*, UWorld*, FURL*, bool, FString&))(ImageBase + 0x734AB88);
    auto SetWorld = (void (*)(UNetDriver*, UWorld*))(ImageBase + 0x29664C8);

    SetWorld(NetDriver, World);
    FString Err;
    if (InitListen(NetDriver, World, &URL, false, Err))
        SetWorld(NetDriver, World);
    else
    {
        printf("Failed to listen!\n");
        if (bProd)
        {
            Port = UKismetMathLibrary::RandomIntegerInRange(1025, 49151);
            goto _;
        }
    }

    if constexpr (bProd)
    {
        gsSocket = easywsclient::WebSocket::from_url("ws://86.48.25.30:2052/gs");

        auto req = CreateRequest();
        req->SetURL(L"https://api.ipify.org");
        req->OnRequestComplete(nullptr, OnGetIP);
        req->SetVerb(L"GET");
        req->ProcessRequest();
    }
}

void (*ProcessEventOG)(UObject* Context, UFunction* Function, void* Parms);
void ProcessEvent(UObject* Context, UFunction* Function, void* Parms)
{
    auto N = Function->Name.ToString();

    if (N != "ReceiveTick" && N != "Get" && N != "GetTimeSeconds" && N != "ClientMoveResponsePacked" && N != "K2_GetActorLocation" && N != "GetControlRotation"
        && N != "ReceiveBeginPlay" && N != "UpdatePreviousPositionAndVelocity" && N != "ServerMovePacked" && N != "ClientAckTimeDilation"
        && N != "ServerSendLatestAsyncPhysicsTimestamp" && N != "OnDayPhaseChanged" && N != "ServerUpdateCamera" && N != "StormCheck" && N != "ServerFireAIDirectorEvent"
        && N != "ServerSendAimbotDetectionStatus" && N != "TimelineInitialStorm__UpdateFunc" && N != "StormFadeTimeline__UpdateFunc" && N != "IsInCurrentSafeZone" && N != "DoTargeting")
        printf("ProcessEvent %s on %s\n", N.c_str(), Context->Name.ToString().c_str());

    return ProcessEventOG(Context, Function, Parms);
}

void (*CreateAndConfigureNavigationSystemOG)(UAthenaNavSystemConfig* ModuleConfig, UWorld* World);
void CreateAndConfigureNavigationSystem(UAthenaNavSystemConfig* ModuleConfig, UWorld* World)
{
    ModuleConfig->bPrioritizeNavigationAroundSpawners = true;
    ModuleConfig->bAutoSpawnMissingNavData = true;
    ModuleConfig->bAllowAutoRebuild = true;
    ModuleConfig->bSupportRuntimeNavmeshDisabling = false;
    return CreateAndConfigureNavigationSystemOG(ModuleConfig, World);
}

void (*PerceptionComp__TickComponentOG)(__int64 a1);
void PerceptionComp__TickComponent(__int64 a1)
{
    auto& CachedAIDirector = *(FWeakObjectPtr*)(a1 + 0xb8);

    if (!CachedAIDirector.Get())
        CachedAIDirector = ((AFortGameModeAthena*)GetWorld()->AuthorityGameMode)->AIDirector;

    return PerceptionComp__TickComponentOG(a1);
}

void (*RegisterTickFunctionOG)(FTickFunction* _this, ULevel* Level);
void RegisterTickFunction(FTickFunction* _this, ULevel* Level)
{
    if (!_this->bAllowTickOnDedicatedServer)
        return;

    return RegisterTickFunctionOG(_this, Level);
}

char GetTickableTickType()
{
    return 2; // Never
}

void* (*GetLinkerForObjectOG)(UObject* Object);
void* GetLinkerForObject(UObject* Object)
{
    static bool bLoggedNullObject = false;

    if (!Object)
    {
        if (!bLoggedNullObject)
        {
            printf("[Remix] Skipping engine linker lookup for null object.\n");
            bLoggedNullObject = true;
        }
        return nullptr;
    }

    auto GetPackageForObject = (UObject * (*)(UObject*))(ImageBase + 0x1656D08);
    if (!GetPackageForObject(Object))
        return nullptr;

    return GetLinkerForObjectOG(Object);
}

void Misc__Init()
{
    Hook(ImageBase + 0x16562A4, WorldNetMode);
    Hook(ImageBase + 0x2196B08, WorldNetMode);
    Hook(ImageBase + 0x1D416EC, GetLinkerForObject, GetLinkerForObjectOG);
    Hook(ImageBase + 0x521AFC0, CheckCheckpointHeartBeat);
    Hook(ImageBase + 0x7DE4ED0, SendRequestNow, SendRequestNowOG);
    Hook(ImageBase + 0x189FE98, GetMaxTickRate);
    Hook(ImageBase + 0x9E5754C, ServerMove_PerformMovement, ServerMove_PerformMovementOG);
    Hook(ImageBase + 0xB4E282C, RegisterToLivingWorldManager);
    Hook(ImageBase + 0xA7EAB6C, CreateAndConfigureNavigationSystem, CreateAndConfigureNavigationSystemOG);
    Hook(ImageBase + 0x9B436C8, PerceptionComp__TickComponent, PerceptionComp__TickComponentOG);
    Hook(ImageBase + 0x19F7AFC, RegisterTickFunction, RegisterTickFunctionOG);
    Hook(ImageBase + 0x4139B6C, GetTickableTickType);
    //Hook(uint64(UObject::GetDefaultObj()->VTable[0x46]), ProcessEvent, ProcessEventOG);

    Patch<uint32_t>(ImageBase + 0x219827C, 0xa90160);
    Hook(ImageBase + 0x2C283E0, Listen);

    PatchAllNetModes(ImageBase + 0x2196B08);
}
