// https://github.com/kem0x/memcury

#pragma once

#include <DbgHelp.h>
#include <Windows.h>
#include <format>
#include <intrin.h>
#include <source_location>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#pragma comment(lib, "Dbghelp.lib")

#define MemcuryAssert(cond)                                                                                                                                                        \
    if (!(cond))                                                                                                                                                                   \
    {                                                                                                                                                                              \
        MessageBoxA(nullptr, #cond, __FUNCTION__, MB_ICONERROR | MB_OK);                                                                                                           \
        Memcury::Safety::FreezeCurrentThread();                                                                                                                                    \
    }

#define MemcuryAssertM(cond, msg)                                                                                                                                                  \
    if (!(cond))                                                                                                                                                                   \
    {                                                                                                                                                                              \
        MessageBoxA(nullptr, msg, __FUNCTION__, MB_ICONERROR | MB_OK);                                                                                                             \
    }

#define MemcuryThrow(msg)                                                                                                                                                          \
    MessageBoxA(nullptr, msg, __FUNCTION__, MB_ICONERROR | MB_OK);                                                                                                                 \
    Memcury::Safety::FreezeCurrentThread();

extern uint64_t ImageBase;
inline bool bUE51 = false;
namespace Memcury
{

    inline auto GetCurrentModule() -> HMODULE
    {
        return reinterpret_cast<HMODULE>(ImageBase);
    }

    namespace Util
    {
        template <typename T>
        constexpr static auto IsInRange(T value, T min, T max) -> bool
        {
            return value >= min && value < max;
        }

        constexpr auto StrHash(const char* str, int h = 0) -> unsigned int
        {
            return !str[h] ? 5381 : (StrHash(str, h + 1) * 33) ^ str[h];
        }

        inline auto IsSamePage(void* A, void* B) -> bool
        {
            MEMORY_BASIC_INFORMATION InfoA;
            if (!VirtualQuery(A, &InfoA, sizeof(InfoA)))
            {
                return true;
            }

            MEMORY_BASIC_INFORMATION InfoB;
            if (!VirtualQuery(B, &InfoB, sizeof(InfoB)))
            {
                return true;
            }

            return InfoA.BaseAddress == InfoB.BaseAddress;
        }

        inline auto GetModuleStartAndEnd() -> std::pair<uintptr_t, uintptr_t>
        {
            auto HModule = GetCurrentModule();
            auto NTHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>((uintptr_t)HModule + reinterpret_cast<PIMAGE_DOS_HEADER>((uintptr_t)HModule)->e_lfanew);

            uintptr_t dllStart = (uintptr_t)HModule;
            uintptr_t dllEnd = (uintptr_t)HModule + NTHeaders->OptionalHeader.SizeOfImage;

            return { dllStart, dllEnd };
        }

        inline auto CopyToClipboard(std::string str)
        {
            auto mem = GlobalAlloc(GMEM_FIXED, str.size() + 1);
            memcpy(mem, str.c_str(), str.size() + 1);

            OpenClipboard(nullptr);
            EmptyClipboard();
            SetClipboardData(CF_TEXT, mem);
            CloseClipboard();

            GlobalFree(mem);
        }
    }

    namespace Safety
    {
        enum class ExceptionMode
        {
            None,
            CatchDllExceptionsOnly,
            CatchAllExceptions
        };

        static auto FreezeCurrentThread() -> void
        {
            SuspendThread(GetCurrentThread());
        }

        static auto PrintStack(CONTEXT* ctx) -> void
        {
            STACKFRAME64 stack;
            memset(&stack, 0, sizeof(STACKFRAME64));

            auto process = GetCurrentProcess();
            auto thread = GetCurrentThread();

            SymInitialize(process, NULL, TRUE);

            bool result;
            DWORD64 displacement = 0;

            char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)] { 0 };
            char name[256] { 0 };
            char module[256] { 0 };

            PSYMBOL_INFO symbolInfo = (PSYMBOL_INFO)buffer;

            for (ULONG frame = 0;; frame++)
            {
                result = StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &stack, ctx, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL);

                if (!result)
                    break;

                symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
                symbolInfo->MaxNameLen = MAX_SYM_NAME;
                SymFromAddr(process, (ULONG64)stack.AddrPC.Offset, &displacement, symbolInfo);

                HMODULE hModule = NULL;
                lstrcpyA(module, "");
                GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (const wchar_t*)(stack.AddrPC.Offset), &hModule);

                if (hModule != NULL)
                    GetModuleFileNameA(hModule, module, 256);

                printf("[%lu] Name: %s - Address: %p  - Module: %s\n", frame, symbolInfo->Name, (void*)symbolInfo->Address, module);
            }
        }
    }

    namespace Globals
    {
        constexpr const bool bLogging = true;

        static const char* moduleName = nullptr;
    }

    namespace ASM
    {
        enum MNEMONIC : uint8_t
        {
            JMP_REL8 = 0xEB,
            JMP_REL32 = 0xE9,
            JMP_EAX = 0xE0,
            CALL = 0xE8,
            LEA = 0x8D,
            CDQ = 0x99,
            CMOVL = 0x4C,
            CMOVS = 0x48,
            CMOVNS = 0x49,
            NOP = 0x90,
            INT3 = 0xCC,
            RETN_REL8 = 0xC2,
            RETN = 0xC3,
            NONE = 0x00
        };

        constexpr int SIZE_OF_JMP_RELATIVE_INSTRUCTION = 5;
        constexpr int SIZE_OF_JMP_ABSLOUTE_INSTRUCTION = 13;

        constexpr auto MnemonicToString(MNEMONIC e) -> const char*
        {
            switch (e)
            {
            case JMP_REL8:
                return "JMP_REL8";
            case JMP_REL32:
                return "JMP_REL32";
            case JMP_EAX:
                return "JMP_EAX";
            case CALL:
                return "CALL";
            case LEA:
                return "LEA";
            case CDQ:
                return "CDQ";
            case CMOVL:
                return "CMOVL";
            case CMOVS:
                return "CMOVS";
            case CMOVNS:
                return "CMOVNS";
            case NOP:
                return "NOP";
            case INT3:
                return "INT3";
            case RETN_REL8:
                return "RETN_REL8";
            case RETN:
                return "RETN";
            case NONE:
                return "NONE";
            default:
                return "UNKNOWN";
            }
        }

        constexpr auto Mnemonic(const char* s) -> MNEMONIC
        {
            switch (Util::StrHash(s))
            {
            case Util::StrHash("JMP_REL8"):
                return JMP_REL8;
            case Util::StrHash("JMP_REL32"):
                return JMP_REL32;
            case Util::StrHash("JMP_EAX"):
                return JMP_EAX;
            case Util::StrHash("CALL"):
                return CALL;
            case Util::StrHash("LEA"):
                return LEA;
            case Util::StrHash("CDQ"):
                return CDQ;
            case Util::StrHash("CMOVL"):
                return CMOVL;
            case Util::StrHash("CMOVS"):
                return CMOVS;
            case Util::StrHash("CMOVNS"):
                return CMOVNS;
            case Util::StrHash("NOP"):
                return NOP;
            case Util::StrHash("INT3"):
                return INT3;
            case Util::StrHash("RETN_REL8"):
                return RETN_REL8;
            case Util::StrHash("RETN"):
                return RETN;
            default:
                return NONE;
            }
        }

        inline auto byteIsA(uint8_t byte, MNEMONIC opcode) -> bool
        {
            return byte == opcode;
        }

        inline auto byteIsAscii(uint8_t byte) -> bool
        {
            static constexpr bool isAscii[0x100] = { false, false, false, false, false, false, false, false, false, true, true, false, false, true, false, false, false, false,
                false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true,
                true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
                true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
                true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
                true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
                false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
                false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
                false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
                false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
                false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false };

            return isAscii[byte];
        }

        inline bool isJump(uint8_t byte)
        {
            return byte >= 0x70 && byte <= 0x7F;
        }

        static auto pattern2bytes(const char* pattern) -> std::vector<int>
        {
            auto bytes = std::vector<int> {};
            const auto start = const_cast<char*>(pattern);
            const auto end = const_cast<char*>(pattern) + strlen(pattern);

            for (auto current = start; current < end; ++current)
            {
                if (*current == '?')
                {
                    ++current;
                    if (*current == '?')
                        ++current;
                    bytes.push_back(-1);
                }
                else
                {
                    bytes.push_back(strtoul(current, &current, 16));
                }
            }
            return bytes;
        }
    }

    namespace PE
    {
        inline auto SetCurrentModule(const char* moduleName) -> void
        {
            Globals::moduleName = moduleName;
        }

        inline auto GetModuleBase() -> uintptr_t
        {
            return ImageBase;
        }

        inline auto GetDOSHeader() -> PIMAGE_DOS_HEADER
        {
            return reinterpret_cast<PIMAGE_DOS_HEADER>(GetModuleBase());
        }

        inline auto GetNTHeaders() -> PIMAGE_NT_HEADERS
        {
            return reinterpret_cast<PIMAGE_NT_HEADERS>(GetModuleBase() + GetDOSHeader()->e_lfanew);
        }

        class Address
        {
            uintptr_t _address;

        public:
            Address()
            {
                _address = 0;
            }

            Address(uintptr_t address)
                : _address(address)
            {
            }

            Address(void* address)
                : _address(reinterpret_cast<uintptr_t>(address))
            {
            }

            auto operator=(uintptr_t address) -> Address
            {
                _address = address;
                return *this;
            }

            auto operator=(void* address) -> Address
            {
                _address = reinterpret_cast<uintptr_t>(address);
                return *this;
            }

            auto operator+(uintptr_t offset) -> Address
            {
                return Address(_address + offset);
            }

            bool operator>(uintptr_t offset)
            {
                return _address > offset;
            }

            bool operator>(Address address)
            {
                return _address > address._address;
            }

            bool operator<(uintptr_t offset)
            {
                return _address < offset;
            }

            bool operator<(Address address)
            {
                return _address < address._address;
            }

            bool operator>=(uintptr_t offset)
            {
                return _address >= offset;
            }

            bool operator>=(Address address)
            {
                return _address >= address._address;
            }

            bool operator<=(uintptr_t offset)
            {
                return _address <= offset;
            }

            bool operator<=(Address address)
            {
                return _address <= address._address;
            }

            bool operator==(uintptr_t offset)
            {
                return _address == offset;
            }

            bool operator==(Address address)
            {
                return _address == address._address;
            }

            bool operator!=(uintptr_t offset)
            {
                return _address != offset;
            }

            bool operator!=(Address address)
            {
                return _address != address._address;
            }

            auto RelativeOffset(uint32_t offset, uint32_t off2 = 0) -> Address
            {
                if (_address)
                    _address = ((_address + offset + 4 + off2) + *(int32_t*)(_address + offset));
                return *this;
            }

            auto AbsoluteOffset(uint32_t offset) -> Address
            {
                _address = _address + offset;
                return *this;
            }

            auto Jump() -> Address
            {
                if (ASM::isJump(*reinterpret_cast<UINT8*>(_address)))
                {
                    UINT8 toSkip = *reinterpret_cast<UINT8*>(_address + 1);
                    _address = _address + 2 + toSkip;
                }

                return *this;
            }

            auto Get() -> uintptr_t
            {
                return _address;
            }

            template <typename T>
            auto GetAs() -> T
            {
                return reinterpret_cast<T>(_address);
            }

            auto IsValid() -> bool
            {
                return _address != 0;
            }
        };

        class Section
        {
        public:
            std::string sectionName;
            IMAGE_SECTION_HEADER rawSection;

            static auto GetAllSections() -> std::vector<Section>
            {
                std::vector<Section> sections;

                auto sectionsSize = GetNTHeaders()->FileHeader.NumberOfSections;
                auto section = IMAGE_FIRST_SECTION(GetNTHeaders());

                for (WORD i = 0; i < sectionsSize; i++, section++)
                {
                    auto secName = std::string((char*)section->Name);

                    sections.push_back({ secName, *section });
                }

                return sections;
            }

            static auto GetSection(std::string sectionName) -> Section
            {
                for (auto& section : GetAllSections())
                {
                    if (section.sectionName == sectionName)
                    {
                        return section;
                    }
                }

                MemcuryThrow("Section not found");
                return Section {};
            }

            auto GetSectionSize() -> uint32_t
            {
                return rawSection.Misc.VirtualSize;
            }

            auto GetSectionStart() -> Address
            {
                return Address(GetModuleBase() + rawSection.VirtualAddress);
            }

            auto GetSectionEnd() -> Address
            {
                return Address(GetSectionStart() + GetSectionSize());
            }

            auto isInSection(Address address) -> bool
            {
                return address >= GetSectionStart() && address < GetSectionEnd();
            }
        };
    }

    class Scanner
    {
        PE::Address _address;

    public:
        Scanner(PE::Address address)
            : _address(address)
        {
        }

        static auto SetTargetModule(const char* moduleName) -> void
        {
            PE::SetCurrentModule(moduleName);
        }

        static auto FindPatternEx(HANDLE handle, const char* pattern, const char* mask, uint64_t begin, uint64_t end) -> Scanner
        {
            auto scan = [](const char* pattern, const char* mask, char* begin, unsigned int size) -> char*
            {
                size_t patternLen = strlen(mask);
                for (unsigned int i = 0; i < size - patternLen; i++)
                {
                    bool found = true;
                    for (unsigned int j = 0; j < patternLen; j++)
                    {
                        if (mask[j] != '?' && pattern[j] != *(begin + i + j))
                        {
                            found = false;
                            break;
                        }
                    }

                    if (found)
                        return (begin + i);
                }
                return nullptr;
            };

            uint64_t match = NULL;
            SIZE_T bytesRead;
            char* buffer = nullptr;
            MEMORY_BASIC_INFORMATION mbi = { 0 };

            for (uint64_t curr = begin; curr < end; curr += mbi.RegionSize)
            {
                if (!VirtualQueryEx(handle, (void*)curr, &mbi, sizeof(mbi)))
                    continue;

                if (mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS)
                    continue;

                buffer = new char[mbi.RegionSize];

                if (ReadProcessMemory(handle, mbi.BaseAddress, buffer, mbi.RegionSize, &bytesRead))
                {
                    char* internalAddr = scan(pattern, mask, buffer, (unsigned int)bytesRead);

                    if (internalAddr != nullptr)
                    {
                        match = curr + (uint64_t)(internalAddr - buffer);
                        break;
                    }
                }
            }
            delete[] buffer;

            MemcuryAssertM(match != 0, "FindPatternEx return nullptr");

            return Scanner(match);
        }

        static auto FindPatternEx(HANDLE handle, const char* sig) -> Scanner
        {
            char pattern[100];
            char mask[100];

            char lastChar = ' ';
            unsigned int j = 0;

            for (unsigned int i = 0; i < strlen(sig); i++)
            {
                if ((sig[i] == '?' || sig[i] == '*') && (lastChar != '?' && lastChar != '*'))
                {
                    pattern[j] = mask[j] = '?';
                    j++;
                }

                else if (isspace(lastChar))
                {
                    pattern[j] = lastChar = (char)strtol(&sig[i], 0, 16);
                    mask[j] = 'x';
                    j++;
                }
                lastChar = sig[i];
            }
            pattern[j] = mask[j] = '\0';

            auto module = (uint64_t)GetModuleHandle(nullptr);

            return FindPatternEx(handle, pattern, mask, module, module + Memcury::PE::GetNTHeaders()->OptionalHeader.SizeOfImage);
        }

        static inline auto HasAVX2 = IsProcessorFeaturePresent(PF_AVX2_INSTRUCTIONS_AVAILABLE);
        static auto FindPattern(const char* signature, bool bWarnIfNotFound = true, bool bInRData = false) -> Scanner
        {
            PE::Address add { nullptr };

            auto patternBytes = ASM::pattern2bytes(signature);

            auto scanSect = bInRData ? PE::Section::GetSection(".rdata") : PE::Section::GetSection(".text");
            const auto scanBytes = (uint8_t*)scanSect.GetSectionStart().Get();
            const auto sizeOfImage = scanSect.GetSectionSize();

            const auto s = patternBytes.size();
            const auto d = patternBytes.data();
            auto startingByte = d[0];
            uint32_t h = 1;
            while (startingByte == -1 && h < s)
                startingByte = d[h++];

            auto i = 0ul;
            if (HasAVX2)
            {
                __m256i t = _mm256_set1_epi8((char)startingByte);

                for (; i < (sizeOfImage - s) - ((sizeOfImage - s) % 32); i += 32)
                {
                    auto bytes = _mm256_load_si256((const __m256i*)(scanBytes + i));
                    int offset = _mm256_movemask_epi8(_mm256_cmpeq_epi8(bytes, t));

                    if (offset == 0)
                        continue;

                    for (int q = 0; q < 32; q++)
                    {
                        int c = offset & (1 << q);
                        if (c)
                        {
                            bool found = true;
                            for (auto j = h; j < s; ++j)
                            {
                                if (scanBytes[i + q + j] != d[j] && d[j] != -1)
                                {
                                    found = false;
                                    break;
                                }
                            }

                            if (found)
                            {
                                add = reinterpret_cast<uintptr_t>(&scanBytes[i + q]);
                                goto _out;
                            }
                        }
                    }
                }
            }
            else
            {
                __m128i t = _mm_set1_epi8((char)startingByte);

                for (; i < (sizeOfImage - s) - ((sizeOfImage - s) % 16); i += 16)
                {
                    auto bytes = _mm_load_si128((const __m128i*)(scanBytes + i));
                    int offset = _mm_movemask_epi8(_mm_cmpeq_epi8(bytes, t));

                    if (offset == 0)
                        continue;

                    for (int q = 0; q < 16; q++)
                    {
                        int c = offset & (1 << q);
                        if (c)
                        {
                            bool found = true;
                            for (auto j = h; j < s; ++j)
                            {
                                if (scanBytes[i + q + j] != d[j] && d[j] != -1)
                                {
                                    found = false;
                                    break;
                                }
                            }

                            if (found)
                            {
                                add = reinterpret_cast<uintptr_t>(&scanBytes[i + q]);
                                goto _out;
                            }
                        }
                    }
                }
            }
        _out:

            return Scanner(add);
        }

        static auto FindPointerRef(void* Pointer, int useRefNum = 0, bool bUseFirstResult = false, bool bWarnIfNotFound = true) -> Scanner
        {
            PE::Address add { nullptr };

            if (!Pointer)
                return Scanner(add);

            auto textSection = PE::Section::GetSection(".text");

            const auto scanBytes = reinterpret_cast<std::uint8_t*>(textSection.GetSectionStart().Get());

            __m128i t = _mm_set1_epi8((char)0x89);
            __m128i s = _mm_set1_epi8((char)0xf9);

            __m128i t2 = _mm_set1_epi8((char)0xe8);
            DWORD i = 0;

            for (; i < textSection.GetSectionSize() - (textSection.GetSectionSize() % 16); i += 16)
            {
                auto bytes = _mm_load_si128((const __m128i*)(scanBytes + i));
                __m128i masked = _mm_and_si128(bytes, s);
                int offset = _mm_movemask_epi8(_mm_cmpeq_epi8(masked, t));

                if (offset != 0)
                {
                    for (int q = 0; q < 16; q++)
                    {
                        int c = offset & (1 << q);
                        if (c)
                        {
                            if (PE::Address(&scanBytes[i + q]).RelativeOffset(2).GetAs<void*>() == Pointer)
                            {
                                auto sub = (scanBytes[i + q - 1] & 0xFB) == 0x48;
                                add = PE::Address(&scanBytes[i + q - sub]);

                                if (bUseFirstResult)
                                    return Scanner(add);
                            }
                        }
                    }
                }

                int offset2 = _mm_movemask_epi8(_mm_cmpeq_epi8(bytes, t2));

                if (offset2 != 0)
                {
                    for (int q = 0; q < 16; q++)
                    {
                        int c = offset2 & (1 << q);
                        if (c)
                        {
                            if (PE::Address(&scanBytes[i + q]).RelativeOffset(1).GetAs<void*>() == Pointer)
                            {
                                add = PE::Address(&scanBytes[i + q]);

                                if (bUseFirstResult)
                                    return Scanner(add);
                            }
                        }
                    }
                }
            }

            return Scanner(add);
        }

        template <typename T = const wchar_t*>
        static auto FindStringRef(T string, bool bWarnIfNotFound = true, int useRefNum = 0, bool bIsInFunc = false, bool bSkunky = false) -> Scanner
        {
            PE::Address add { nullptr };

            constexpr auto bIsWide = std::is_same<T, const wchar_t*>::value;
            constexpr auto bIsChar = std::is_same<T, const char*>::value;

            constexpr auto bIsPtr = bIsWide || bIsChar;

            auto textSection = PE::Section::GetSection(".text");
            auto rdataSection = PE::Section::GetSection(".rdata");

            const auto scanBytes = reinterpret_cast<std::uint8_t*>(textSection.GetSectionStart().Get());

            int aa = 0;

            size_t slen = 0;
            if constexpr (bIsWide)
                slen = (wcslen(string) + 1) * sizeof(wchar_t);
            else if constexpr (bIsChar)
                slen = strlen(string) + 1;

            DWORD i = 0x0;
            if (HasAVX2)
            {
                __m256i t = _mm256_set1_epi8((char)0x8d);
                for (; i < textSection.GetSectionSize() - (textSection.GetSectionSize() % 32); i += 32)
                {
                    auto bytes = _mm256_load_si256((const __m256i*)(scanBytes + i));
                    int offset = _mm256_movemask_epi8(_mm256_cmpeq_epi8(bytes, t));

                    if (offset == 0)
                        continue;

                    for (int q = 0; q < 32; q++)
                    {
                        int c = offset & (1 << q);
                        if (c)
                        {
                            if ((scanBytes[i + q - 1] & 0xFB) == 0x48)
                            {
                                auto stringAdd = PE::Address(&scanBytes[i + q]).RelativeOffset(2);

                                if (rdataSection.isInSection(stringAdd))
                                {
                                    auto strBytes = stringAdd.GetAs<std::uint8_t*>();

                                    if (bUE51 && bIsInFunc)
                                    {
                                        auto pointerToRef = *(LPVOID*)strBytes;
                                        if (rdataSection.isInSection(pointerToRef))
                                        {
                                            strBytes = (std::uint8_t*)pointerToRef;
                                            stringAdd = PE::Address(pointerToRef);
                                        }
                                    }

                                    {
                                        if constexpr (!bIsPtr)
                                        {
                                            using char_type = std::decay_t<std::remove_pointer_t<T>>;

                                            auto lea = stringAdd.GetAs<char_type*>();

                                            T leaT(lea);

                                            if (leaT == string)
                                            {
                                                add = PE::Address(&scanBytes[i + q - 1]);

                                                if (++aa > useRefNum)
                                                    goto _out;
                                            }
                                        }
                                        else
                                        {
                                            auto lea = stringAdd.GetAs<T>();

                                            if constexpr (bIsWide)
                                            {
                                                if (memcmp(string, lea, slen) == 0)
                                                {
                                                    add = PE::Address(&scanBytes[i + q - 1]);

                                                    if (++aa > useRefNum)
                                                        goto _out;
                                                }
                                            }
                                            else
                                            {
                                                if (memcmp(string, lea, slen) == 0)
                                                {
                                                    add = PE::Address(&scanBytes[i + q - 1]);

                                                    if (++aa > useRefNum)
                                                        goto _out;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                __m128i t = _mm_set1_epi8((char)0x8d);
                for (; i < textSection.GetSectionSize() - (textSection.GetSectionSize() % 16); i += 16)
                {
                    auto bytes = _mm_load_si128((const __m128i*)(scanBytes + i));
                    int offset = _mm_movemask_epi8(_mm_cmpeq_epi8(bytes, t));

                    if (offset == 0)
                        continue;

                    for (int q = 0; q < 16; q++)
                    {
                        int c = offset & (1 << q);
                        if (c)
                        {
                            if ((scanBytes[i + q - 1] & 0xFB) == 0x48)
                            {
                                auto stringAdd = PE::Address(&scanBytes[i + q]).RelativeOffset(2);

                                if (rdataSection.isInSection(stringAdd))
                                {
                                    auto strBytes = stringAdd.GetAs<std::uint8_t*>();

                                    if (bUE51 && bIsInFunc)
                                    {
                                        auto pointerToRef = *(LPVOID*)strBytes;
                                        if (rdataSection.isInSection(pointerToRef))
                                        {
                                            strBytes = (std::uint8_t*)pointerToRef;
                                            stringAdd = PE::Address(pointerToRef);
                                        }
                                    }

                                    {
                                        if constexpr (!bIsPtr)
                                        {
                                            using char_type = std::decay_t<std::remove_pointer_t<T>>;

                                            auto lea = stringAdd.GetAs<char_type*>();

                                            T leaT(lea);

                                            if (leaT == string)
                                            {
                                                add = PE::Address(&scanBytes[i + q - 1]);

                                                if (++aa > useRefNum)
                                                    goto _out;
                                            }
                                        }
                                        else
                                        {
                                            auto lea = stringAdd.GetAs<T>();

                                            if constexpr (bIsWide)
                                            {
                                                if (memcmp(string, lea, slen) == 0)
                                                {
                                                    add = PE::Address(&scanBytes[i + q - 1]);

                                                    if (++aa > useRefNum)
                                                        goto _out;
                                                }
                                            }
                                            else
                                            {
                                                if (memcmp(string, lea, slen) == 0)
                                                {
                                                    add = PE::Address(&scanBytes[i + q - 1]);

                                                    if (++aa > useRefNum)
                                                        goto _out;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        _out:

            if (add.Get())
            {
                if (bIsInFunc && !bUE51)
                {
                    for (int i = 0; i < 300; i++)
                    {
                        if (*(uint8_t*)(add.Get() - i) == 0x48 && *(uint8_t*)(add.Get() - i + 1) == 0x83 && *(uint8_t*)(add.Get() - i + 2) == 0xEC)
                        {
                            int sub = 0;
                            if (*(uint8_t*)(add.Get() - i - 3) == 0x4C && *(uint8_t*)(add.Get() - i - 2) == 0x8B && *(uint8_t*)(add.Get() - i - 1) == 0xDC)
                                sub = 3;

                            auto beginFunc = Scanner(add.Get() - i - sub);

                            auto ref = FindPointerRef(beginFunc.GetAs<void*>());

                            if (ref.Get())
                                return ref;
                        }
                    }
                }
            }

            return Scanner(add);
        }

        auto Jump() -> Scanner
        {
            _address.Jump();
            return *this;
        }

        inline auto ScanFor(std::vector<uint8_t> opcodesToFind, bool forward = true, int toSkip = 0, int bytesToSkip = 1, int Radius = 2048, bool bIgnoreFF = false) -> Scanner
        {
            const auto scanBytes = _address.GetAs<std::uint8_t*>();
            if (!scanBytes)
                return *this;

            bool bFound = false;

            for (auto i = (forward ? bytesToSkip : -bytesToSkip); forward ? (i < Radius + bytesToSkip) : (i > -Radius - bytesToSkip); forward ? i++ : i--)
            {
                bool found = true;

                for (int k = 0; k < opcodesToFind.size() && found; k++)
                {
                    auto& currentOpcode = opcodesToFind[k];

                    if (bIgnoreFF && currentOpcode == 0xFF)
                        continue;

                    found = currentOpcode == scanBytes[i + k];
                }

                if (found)
                {
                    _address = &scanBytes[i];
                    if (toSkip != 0)
                    {
                        return ScanFor(opcodesToFind, forward, toSkip - 1);
                    }

                    bFound = true;

                    break;
                }
            }

            return *this;
        }

        inline auto ScanFor(const char* pattern, bool forward = true, int toSkip = 0, int bytesToSkip = 1, int Radius = 2048) -> Scanner
        {
            const auto scanBytes = _address.GetAs<std::uint8_t*>();

            bool bFound = false;

            auto opcodesToFind = ASM::pattern2bytes(pattern);
            for (auto i = (forward ? bytesToSkip : -bytesToSkip); forward ? (i < Radius + bytesToSkip) : (i > -Radius - bytesToSkip); forward ? i++ : i--)
            {
                bool found = true;

                for (int k = 0; k < opcodesToFind.size() && found; k++)
                {
                    auto& currentOpcode = opcodesToFind[k];

                    if (currentOpcode == -1)
                        continue;

                    found = currentOpcode == scanBytes[i + k];
                }

                if (found)
                {
                    _address = &scanBytes[i];
                    if (toSkip != 0)
                    {
                        return ScanFor(pattern, forward, toSkip - 1);
                    }

                    bFound = true;

                    break;
                }
            }

            return *this;
        }

        auto FindFunctionBoundary(bool forward = false) -> Scanner
        {
            const auto scanBytes = _address.GetAs<std::uint8_t*>();

            for (auto i = (forward ? 1 : -1); forward ? (i < 2048) : (i > -2048); forward ? i++ : i--)
            {
                if (ASM::byteIsA(scanBytes[i], ASM::MNEMONIC::RETN_REL8) || ASM::byteIsA(scanBytes[i], ASM::MNEMONIC::RETN) || ASM::byteIsA(scanBytes[i], ASM::MNEMONIC::INT3))
                {
                    _address = (uintptr_t)&scanBytes[i + 1];
                    break;
                }
            }

            return *this;
        }

        auto RelativeOffset(uint32_t offset, uint32_t off2 = 0) -> Scanner
        {
            if (!_address.Get())
            {
                return *this;
            }

            _address.RelativeOffset(offset, off2);

            return *this;
        }

        auto AbsoluteOffset(uint32_t offset) -> Scanner
        {
            _address.AbsoluteOffset(offset);

            return *this;
        }

        template <typename T>
        auto GetAs() -> T
        {
            return _address.GetAs<T>();
        }

        auto Get() -> uintptr_t
        {
            return _address.Get();
        }

        auto IsValid() -> bool
        {
            return _address.IsValid();
        }
    };
}
