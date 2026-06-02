#pragma once
#include "pch.h"
#include "MinHook.h"
#include <unordered_map>

UWorld* GetWorld();
UFortEngine* GetEngine();
bool StartSpecialEventSequence();

UInterface* GetInterfaceAddress(UObject* Object, UClass* Class);
template <typename T>
inline UInterface* GetInterfaceAddress(UObject* Object)
{
    return GetInterfaceAddress(Object, T::StaticClass());
}

FQuat RotatorToQuat(FRotator Rot);
FRotator QuatToRotator(FQuat Rot);
FTransform ConstructTrans(FVector Vec, FRotator Rot, FVector scale = { 1, 1, 1 });
FTransform ConstructTrans(FVector Vec, FQuat Rot, FVector scale = { 1, 1, 1 });

UObject* FindObject(const wchar_t* ObjectPath, UClass* Class);

template <typename _Ot>
inline _Ot* FindObject(const wchar_t* ObjectPath, UClass* Class = _Ot::StaticClass())
{
    return (_Ot*)FindObject(ObjectPath, Class);
}

template <typename _Ot>
inline _Ot* FindObject(UEAllocatedWString ObjectPath, UClass* Class = _Ot::StaticClass())
{
    return (_Ot*)FindObject(ObjectPath.c_str(), Class);
}

template <typename _Ot>
inline _Ot* FindObject(UEAllocatedString ObjectPath, UClass* Class = _Ot::StaticClass())
{
    return (_Ot*)FindObject(std::wstring(ObjectPath.begin(), ObjectPath.end()).c_str(), Class);
}

template <typename _Ot>
inline _Ot* FindObject(const char* ObjectPath, UClass* Class = _Ot::StaticClass())
{
    return FindObject<_Ot>(UEAllocatedString(ObjectPath), Class);
}

struct FActorSpawnParameters
{
    /* A name to assign as the Name of the Actor being spawned. If no value is specified, the name of the spawned Actor will be automatically generated using the form
     * [Class]_[Number]. */
    FName Name = FName(0);

    /* An Actor to use as a template when spawning the new Actor. The spawned Actor will be initialized using the property values of the template Actor. If left NULL the class
     * default object (CDO) will be used to initialize the spawned Actor. */
    AActor* Template = nullptr;

    /* The Actor that spawned this Actor. (Can be left as NULL). */
    AActor* Owner = nullptr;

    /* The APawn that is responsible for damage done by the spawned Actor. (Can be left as NULL). */
    APawn* Instigator = nullptr;

    /* The ULevel to spawn the Actor in, i.e. the Outer of the Actor. If left as NULL the Outer of the Owner is used. If the Owner is NULL the persistent level is used. */
    class ULevel* OverrideLevel = nullptr;

#if WITH_EDITOR
    /* The UPackage to set the Actor in. If left as NULL the Package will not be set and the actor will be saved in the same package as the persistent level. */
    class UPackage* OverridePackage;

    /** The Guid to set to this actor. Should only be set when reinstancing blueprint actors. */
    FGuid OverrideActorGuid;
#endif

    /* The parent component to set the Actor in. */
    class UChildActorComponent* OverrideParentComponent = nullptr;

    /** Method for resolving collisions at the spawn point. Undefined means no override, use the actor's setting. */
    ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined;

    /** Determines whether to multiply or override root component with provided spawn transform */
    ESpawnActorScaleMethod TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

private:
    /* Is the actor remotely owned. This should only be set true by the package map when it is creating an actor on a client that was replicated from the server. */
    uint8 bRemoteOwned : 1 = false;

public:
    bool IsRemoteOwned() const
    {
        return bRemoteOwned;
    }

    /* Determines whether spawning will not fail if certain conditions are not met. If true, spawning will not fail because the class being spawned is `bStatic=true` or because the
     * class of the template Actor is not the same as the class of the Actor being spawned. */
    uint8 bNoFail : 1 = false;

    /* Determines whether the construction script will be run. If true, the construction script will not be run on the spawned Actor. Only applicable if the Actor is being spawned
     * from a Blueprint. */
    uint8 bDeferConstruction : 1 = false;

    /* Determines whether or not the actor may be spawned when running a construction script. If true spawning will fail if a construction script is being run. */
    uint8 bAllowDuringConstructionScript : 1 = false;

#if !WITH_EDITOR
    /* Force the spawned actor to use a globally unique name (provided name should be none). */
    uint8 bForceGloballyUniqueName : 1 = false;
#else
    /* Determines whether the begin play cycle will run on the spawned actor when in the editor. */
    uint8 bTemporaryEditorActor : 1;

    /* Determines whether or not the actor should be hidden from the Scene Outliner */
    uint8 bHideFromSceneOutliner : 1;

    /** Determines whether to create a new package for the actor or not, if the level supports it. */
    uint16 bCreateActorPackage : 1;
#endif

    /* Modes that SpawnActor can use the supplied name when it is not None. */
    enum class ESpawnActorNameMode : uint8
    {
        /* Fatal if unavailable, application will assert */
        Required_Fatal,

        /* Report an error return null if unavailable */
        Required_ErrorAndReturnNull,

        /* Return null if unavailable */
        Required_ReturnNull,

        /* If the supplied Name is already in use the generate an unused one using the supplied version as a base */
        Requested
    };

    /* In which way should SpawnActor should treat the supplied Name if not none. */
    ESpawnActorNameMode NameMode = ESpawnActorNameMode::Required_Fatal;

    /* Flags used to describe the spawned actor/object instance. */
    int32 ObjectFlags = 0x00000008;

    /* Custom function allowing the caller to specific a function to execute post actor construction but before other systems see this actor spawn. */
    void* CustomPreSpawnInitalization_Callable = nullptr;
    void* CustomPreSpawnInitalization_HeapAllocation = nullptr;
};

AActor* SpawnActor(UClass* Class, FTransform Transform, AActor* Owner = nullptr,
    ESpawnActorCollisionHandlingMethod CollisionOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
AActor* SpawnActor(UClass* Class, FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr,
    ESpawnActorCollisionHandlingMethod CollisionOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

template <typename T>
inline T* SpawnActor(UClass* Class, FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
{
    return (T*)SpawnActor(Class, Loc, Rot, Owner);
}

template <typename T>
inline T* SpawnActor(UClass* Class, FTransform& Transform, AActor* Owner = nullptr)
{
    return (T*)SpawnActor(Class, Transform, Owner);
}

template <typename T>
inline T* SpawnActor(FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
{
    return (T*)SpawnActor(T::StaticClass(), Loc, Rot, Owner);
}

template <typename T>
inline T* SpawnActor(FTransform& Transform, AActor* Owner = nullptr)
{
    return (T*)SpawnActor(T::StaticClass(), Transform, Owner);
}

template <typename T = AActor>
static T* SpawnActorUnfinished(UClass* Class, FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
{
    static auto SpawnActorInternal = (AActor * (*)(UWorld*, const UClass*, FVector*, FRotator*, void*))(ImageBase + 0x2FA7A58);

    FActorSpawnParameters SpawnParameters {};

    SpawnParameters.Owner = Owner;
    SpawnParameters.bDeferConstruction = false;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    SpawnParameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
    SpawnParameters.bDeferConstruction = true;

    return (T*)SpawnActorInternal(GetWorld(), Class, &Loc, &Rot, &SpawnParameters);
}

template <typename T = AActor>
static T* FinishSpawnActor(T* Actor, FVector Loc, FRotator Rot)
{
    static auto FinishSpawning = (void (*)(AActor*, FTransform*, void*, void*, ESpawnActorScaleMethod))(ImageBase + 0x21F5F6C);


    auto Transform = ConstructTrans(Loc, Rot);

    FinishSpawning(Actor, &Transform, nullptr, nullptr, ESpawnActorScaleMethod::MultiplyWithRoot);

    return Actor;
};

inline void* _NpFH = nullptr;

template <class _Ot = void*>
void Hook(uint64_t _Ptr, void* _Detour, _Ot& _Orig = _NpFH)
{
    MH_CreateHook((LPVOID)_Ptr, _Detour, (LPVOID*)(std::is_same_v<_Ot, void*> ? nullptr : &_Orig));
}

inline void VirtualSwap(void** VTable, int Index, void* Detour, void** Original = nullptr)
{
    if (Original)
        *Original = VTable[Index];

    DWORD dwProt;
    VirtualProtect(&VTable[Index], sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProt);

    VTable[Index] = Detour;

    DWORD dwTemp;
    VirtualProtect(&VTable[Index], sizeof(void*), dwProt, &dwTemp);
}

template <typename _Ct, typename _Ot = void*>
__forceinline void Hook(uint32_t _Ind, void* _Detour, _Ot& _Orig = _NpFH)
{
    auto _Vt = _Ct::GetDefaultObj()->VTable;
    if (!std::is_same_v<_Ot, void*>)
        _Orig = (_Ot)_Vt[_Ind];

    DWORD _Vo;
    VirtualProtect(_Vt + _Ind, 8, PAGE_EXECUTE_READWRITE, &_Vo);
    _Vt[_Ind] = _Detour;
    VirtualProtect(_Vt + _Ind, 8, _Vo, &_Vo);
}

inline bool ReturnFalse()
{
    return false;
}

inline bool ReturnTrue()
{
    return true;
}

template <typename _Ct>
__forceinline static void HookEvery(uint32_t _Ind, void* _Detour)
{
    for (uint32_t i = 0; i < TUObjectArray::Num(); i++)
    {
        auto Obj = TUObjectArray::GetObjectByIndex(i);

        if (Obj && Obj->ObjectFlags & 0x10 && Obj->IsA<_Ct>())
        {
            auto _Vt = Obj->VTable;

            DWORD _Vo;
            VirtualProtect(_Vt + _Ind, 8, PAGE_EXECUTE_READWRITE, &_Vo);
            _Vt[_Ind] = _Detour;
            VirtualProtect(_Vt + _Ind, 8, _Vo, &_Vo);
        }
    }
}

template <typename _Ot = void*>
__forceinline void ExecHook(UFunction* _Fn, void* _Detour, _Ot& _Orig = _NpFH)
{
    if (!_Fn)
        return;
    if (!std::is_same_v<_Ot, void*>)
        _Orig = (_Ot)_Fn->ExecFunction;

    _Fn->ExecFunction = (void (*)(void*, void*, void*))_Detour;
}

template <typename _Is>
__forceinline void Patch(uintptr_t ptr, _Is byte)
{
    DWORD og;
    VirtualProtect(LPVOID(ptr), sizeof(_Is), PAGE_EXECUTE_READWRITE, &og);
    *(_Is*)ptr = byte;
    VirtualProtect(LPVOID(ptr), sizeof(_Is), og, &og);
}

inline bool IsReadableAddress(const void* Address, size_t Size = sizeof(void*))
{
    if (!Address || Size == 0)
        return false;

    const auto Ptr = reinterpret_cast<uintptr_t>(Address);
    if (Ptr < 0x10000 || Ptr == UINTPTR_MAX || Ptr > UINTPTR_MAX - Size)
        return false;

    MEMORY_BASIC_INFORMATION Info {};
    if (!VirtualQuery(Address, &Info, sizeof(Info)))
        return false;

    if (Info.State != MEM_COMMIT || (Info.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
        return false;

    const auto RegionStart = reinterpret_cast<uintptr_t>(Info.BaseAddress);
    const auto RegionEnd = RegionStart + Info.RegionSize;
    if (RegionEnd < RegionStart)
        return false;

    return Ptr >= RegionStart && Ptr + Size <= RegionEnd;
}

inline bool IsValidUObject(const UObject* Object, UClass* ExpectedClass = nullptr)
{
    if (!IsReadableAddress(Object, sizeof(UObject)))
        return false;

    if (!IsReadableAddress(Object->Class, sizeof(UClass)))
        return false;

    auto MutableObject = const_cast<UObject*>(Object);
    const int32 ObjectIndex = MutableObject->GetIndex();
    if (ObjectIndex < 0 || (uint32_t)ObjectIndex >= TUObjectArray::Num())
        return false;

    if (TUObjectArray::GetObjectByIndex(ObjectIndex) != MutableObject)
        return false;

    return !ExpectedClass || MutableObject->IsA(ExpectedClass);
}

inline bool GuidEquals(FGuid Guid, FGuid OtherGuid)
{
    return Guid.A == OtherGuid.A && Guid.B == OtherGuid.B && Guid.C == OtherGuid.C && Guid.D == OtherGuid.D;
}

inline TMap<FName, uint8_t*>& GetRowMap(UDataTable* DataTable)
{
    return *(TMap<FName, uint8*>*)(__int64(DataTable) + 0x30);
}

inline FVector VecMul(FVector Vec, double MulBy)
{
    Vec.X *= MulBy;
    Vec.Y *= MulBy;
    Vec.Z *= MulBy;

    return Vec;
}

inline FVector VecMul(FVector Vec, FVector MulBy)
{
    Vec.X *= MulBy.X;
    Vec.Y *= MulBy.Y;
    Vec.Z *= MulBy.Z;

    return Vec;
}

inline FVector VecAdd(FVector Vec, double ToAdd)
{
    Vec.X += ToAdd;
    Vec.Y += ToAdd;
    Vec.Z += ToAdd;

    return Vec;
}

inline FVector VecAdd(FVector Vec, FVector ToAdd)
{
    Vec.X += ToAdd.X;
    Vec.Y += ToAdd.Y;
    Vec.Z += ToAdd.Z;

    return Vec;
}

inline bool HasTag(FGameplayTagContainer& Container, FGameplayTag& TagToCheck)
{
    for (auto& Tag : Container.GameplayTags)
        if (Tag.TagName == TagToCheck.TagName)
            return true;

    for (auto& Tag : Container.ParentTags)
        if (Tag.TagName == TagToCheck.TagName)
            return true;

    return false;
}

inline void AddTag(FGameplayTagContainer& Container, FGameplayTag Tag)
{
    static auto AddTag_ = (void (*)(FGameplayTagContainer*, const FGameplayTag&))(ImageBase + 0x1895690);

    AddTag_(&Container, Tag);
}

template <typename T>
inline void CopyArray(TArray<T>& OutArray, TArray<T>& Array)
{
    OutArray.MaxElements = Array.MaxElements;
    OutArray.NumElements = Array.NumElements;
    OutArray.Data = FMemory::MallocForType<T>(OutArray.MaxElements);
    
    memcpy(OutArray.Data, Array.Data, Array.NumElements * sizeof(T));
}

inline std::vector<void (*)()> Initters;
inline std::vector<void (*)()> Tickers;

#define INIT_MODULE(Name)                                                                                                                                                          \
    void Name##__Init();                                                                                                                                                           \
    auto Initter__##Name = ([]() { Initters.push_back(Name##__Init); return 1; })();

#define INIT_TICKER(Name)                                                                                                                                                          \
    void Name##__Tick();                                                                                                                                                           \
    auto Ticker__##Name = ([]() { Tickers.push_back(Name##__Tick); return 1; })();

#define callOG(_Tr, _Fn, _Th, ...)                                                                                                                                                 \
    (                                                                                                                                                                              \
        [&]()                                                                                                                                                                      \
        {                                                                                                                                                                          \
            _Fn->ExecFunction = (void(*)(void*,void*,void*))_Th##OG;                                                                                                               \
            _Tr->_Th(##__VA_ARGS__);                                                                                                                                               \
            _Fn->ExecFunction = (void (*)(void*, void*, void*))_Th;                                                                                                                \
        })()
#define callOGWithRet(_Tr, _Fn, _Th, ...)                                                                                                                                          \
    (                                                                                                                                                                              \
        [&]()                                                                                                                                                                      \
        {                                                                                                                                                                          \
            _Fn->ExecFunction = (void (*)(void*, void*, void*))_Th##OG;                                                                                                            \
            auto _Rt = _Tr->_Th(##__VA_ARGS__);                                                                                                                                    \
            _Fn->ExecFunction = (void (*)(void*, void*, void*))_Th;                                                                                                                \
            return _Rt;                                                                                                                                                            \
        })()

template <typename T>
inline T* GetSparseClassData(UObject* Obj)
{
    static T* (*GetSparseClassDataOG)(UObject*, uint8) = decltype(GetSparseClassDataOG)(ImageBase + 0x1BBBDC8);

    return GetSparseClassDataOG(Obj->Class, 1);
}

inline float EvaluateScalableFloat(FScalableFloat& ScalableFloat, float i = 0.f)
{
    return UFortScalableFloatUtils::GetValueAtLevel(ScalableFloat, i);
    /*if (!ScalableFloat.Curve.CurveTable || !ScalableFloat.Curve.RowName.IsValid())
        return ScalableFloat.Value;

    float Out = 0.f;
    UDataTableFunctionLibrary::EvaluateCurveTableRow(ScalableFloat.Curve.CurveTable, ScalableFloat.Curve.RowName, i, nullptr, &Out, FString());
    return ScalableFloat.Value * Out;*/
}

constexpr unsigned long long hash(const char* input)
{
    return *input ? static_cast<unsigned long long>(*input) + (33 + __COUNTER__ & 0xff) * hash(input + 1) : 5381;
}

UFortAssetManager* GetAssetManager();
UGameDataBR* GetGameData();
