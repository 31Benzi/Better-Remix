#pragma once

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;
typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
inline uint64_t ImageBase = *(uint64_t*)(__readgsqword(0x60) + 0x10);

#include "UnrealContainers.h"
using namespace UC;

class FName
{
public:
    static inline void* AppendString = nullptr;
    int32 EncryptedIndex;

    int32_t DecryptIndex() const
    {
        if (!EncryptedIndex)
            return 0;

        int32_t result = -(int32_t)(((uint32_t)EncryptedIndex - 1) ^ (int32_t)0xFC4AD0A4);
        return result ? result : (int32_t)(-62205787);
    }

    FName(const wchar_t* String)
    {
        static auto FName__Ctor = (void (*)(FName*, const wchar_t*, int))(ImageBase + 0x175DD98);

        FName__Ctor(this, String, 1);
    }

    FName(UEAllocatedWString String)
    {
        static auto FName__Ctor = (void (*)(FName*, const wchar_t*, int))(ImageBase + 0x175DD98);

        FName__Ctor(this, String.c_str(), 1);
    }

    FName(FString String)
    {
        static auto FName__Ctor = (void (*)(FName*, const wchar_t*, int))(ImageBase + 0x175DD98);

        FName__Ctor(this, String.CStr(), 1);
    }
    
	FName(int32 _Ci = 0)
    //    : ComparisonIndex(_Ci)
    {
        if (!_Ci)
            EncryptedIndex = (int32_t)0;
        else
            EncryptedIndex = ((-_Ci ^ (int32_t)0xFC4AD0A4) + 1);
    }

    static void InitInternal()
    {
        AppendString = reinterpret_cast<void*>(ImageBase + 0x175D6E0);
    }
    static void InitManually(void* Location)
    {
        AppendString = reinterpret_cast<void*>(Location);
    }

    int32 GetDisplayIndex() const
    {
        return DecryptIndex();
    }

    bool IsValid() const
    {
        return DecryptIndex() > 0;
    }

    UEAllocatedString GetRawString() const
    {
        FAllocatedString TempString;

        if (!AppendString)
            InitInternal();

        auto AppendString_ = reinterpret_cast<void (*)(const FName*, FString&)>(AppendString);
        AppendString_(this, TempString);

        UEAllocatedString OutputString = TempString.ToString();
        TempString.Free();

        return OutputString;
    }

    UEAllocatedString ToString() const
    {
        UEAllocatedString OutputString = GetRawString();

        size_t pos = OutputString.rfind('/');

        if (pos == std::string::npos)
            return OutputString;

        return OutputString.substr(pos + 1);
    }

    UEAllocatedWString GetRawWString() const
    {
        FAllocatedString TempString;

        if (!AppendString)
            InitInternal();

        auto AppendString_ = reinterpret_cast<void (*)(const FName*, FString&)>(AppendString);
        AppendString_(this, TempString);

        UEAllocatedWString OutputString = TempString.ToWString();
        TempString.Free();

        return OutputString;
    }

    UEAllocatedWString ToWString() const
    {
        UEAllocatedWString OutputString = GetRawWString();

        size_t pos = OutputString.rfind('/');

        if (pos == UEAllocatedString::npos)
            return OutputString;

        return OutputString.substr(pos + 1);
    }

    bool operator==(const FName& Other) const
    {
        return EncryptedIndex == Other.EncryptedIndex;
    }
    bool operator!=(const FName& Other) const
    {
        return EncryptedIndex != Other.EncryptedIndex;
    }

    bool operator<(const FName& Other) const
    {
        return DecryptIndex() < Other.DecryptIndex();
    }

    operator bool() const
    {
        return IsValid();
    }
};

template <class T>
T __ROL__(T value, int count)
{
    const unsigned int nbits = sizeof(T) * 8;

    if (count > 0)
    {
        count %= nbits;
        T high = value >> (nbits - count);
        if (T(-1) < 0) // signed value
            high &= ~((T(-1) << count));
        value <<= count;
        value |= high;
    }
    else
    {
        count = -count % nbits;
        T low = value << (nbits - count);
        value >>= count;
        value |= low;
    }
    return value;
}

inline uint8  __ROL1__(uint8  value, int count) { return __ROL__((uint8)value, count); }
inline uint16 __ROL2__(uint16 value, int count) { return __ROL__((uint16)value, count); }
inline uint32 __ROL4__(uint32 value, int count) { return __ROL__((uint32)value, count); }
inline uint64 __ROL8__(uint64 value, int count) { return __ROL__((uint64)value, count); }
inline uint8  __ROR1__(uint8  value, int count) { return __ROL__((uint8)value, -count); }
inline uint16 __ROR2__(uint16 value, int count) { return __ROL__((uint16)value, -count); }
inline uint32 __ROR4__(uint32 value, int count) { return __ROL__((uint32)value, -count); }
inline uint64 __ROR8__(uint64 value, int count) { return __ROL__((uint64)value, -count); }

struct FUObjectItem
{
public:
    int32 Flags;
    int32 ClusterRootIndex;
    int32 SerialNumber;
    int32 RefCount;
    uint64_t EncryptedObject;
};

class TUObjectArray
{
public:
    static inline auto NumElementsPerChunk = 0x10000;

    static inline FUObjectItem** GetObjects()
    {
        return (FUObjectItem**) ~__ROR8__(*(uint64_t*)(ImageBase + 0x12EAF270) ^ 0xE7F18B24ULL, 16);
    }

    static inline uint32_t Num()
    {
        return ~(*(uint32_t*)(ImageBase + 0x12EAF284) ^ 0xEE9DB424);
    }

    static inline FUObjectItem* GetItemByIndex(const int32 Index)
    {
        if (Index < 0 || Index > Num())
            return nullptr;

        const int32 ChunkIndex = Index / NumElementsPerChunk;
        const int32 ChunkOffset = Index % NumElementsPerChunk;

        return GetObjects()[ChunkIndex] + ChunkOffset;
    }

    
    static inline class UObject* GetObjectByIndex(const int32 Index)
    {
        FUObjectItem* Item = GetItemByIndex(Index);

        return Item ? (class UObject*)~__ROR8__(Item->EncryptedObject ^ 0xD8BBB184LL, 16) : nullptr;
    }

    static UObject* FindObject(const char* Name, class UClass* TargetClass = nullptr);

    static UObject* FindFirstObject(const char* Name);
};

template <int32 _Sl>
struct ConstexprString
{
    char _Ch[_Sl];

    consteval ConstexprString(const char (&_St)[_Sl])
    {
        std::copy_n(_St, _Sl, _Ch);
    }

    operator const char*() const
    {
        return static_cast<const char*>(_Ch);
    }
};

class UClass;

class UClass* InternalStaticClass(const char* ClassName);

template <ConstexprString S>
inline class UClass* StaticClassImpl()
{
    static UClass* Clss = nullptr;

    if (!Clss)
        return Clss = InternalStaticClass(S);

    return Clss;
}

template <class T>
inline T* DefaultObjImpl()
{
    return (T*)T::StaticClass()->DefaultObject;
}

class FTextData final
{
public:
    uint8 Pad_0[0x28];
    class FString TextSource;
};

class FText final
{
public:
    class FTextData* TextData;
    uint8 Pad_8[0x8];

public:
    const class FString& GetStringRef() const
    {
        return TextData->TextSource;
    }
    UEAllocatedString ToString() const
    {
        return TextData->TextSource.ToString();
    }
};

class FWeakObjectPtr
{
public:
    int32 ObjectIndex;
    int32 ObjectSerialNumber;

public:
    FWeakObjectPtr(int32 Index = 0, int32 SerialNumber = 0)
        : ObjectIndex(Index)
        , ObjectSerialNumber(SerialNumber)
    {
    }

    FWeakObjectPtr(class UObject* Object);
    class UObject* Get() const;

    inline UObject* operator->() const
    {
        return Get();
    }

    inline bool operator==(const FWeakObjectPtr& Other) const
    {
        return ObjectIndex == Other.ObjectIndex && ObjectSerialNumber == Other.ObjectSerialNumber;
    }

    inline bool operator!=(const FWeakObjectPtr& Other) const
    {
        return ObjectIndex != Other.ObjectIndex || ObjectSerialNumber != Other.ObjectSerialNumber;
    }

    bool operator==(class UObject* Other) const;
    bool operator!=(class UObject* Other) const;
};

template <typename UEType>
class TWeakObjectPtr : public FWeakObjectPtr
{
public:
    TWeakObjectPtr(int32 Index = 0, int32 SerialNumber = 0)
        : FWeakObjectPtr(Index, SerialNumber)
    {
    }

    TWeakObjectPtr(UEType* Obj)
        : FWeakObjectPtr(Obj)
    {
    }

    UEType* Get() const
    {
        return static_cast<UEType*>(FWeakObjectPtr::Get());
    }

    UEType* operator->() const
    {
        return static_cast<UEType*>(FWeakObjectPtr::Get());
    }
};

struct FScriptDelegate
{
public:
    FWeakObjectPtr Object;
    FName FunctionName;
};

template <typename FunctionSignature>
class TDelegate
{
public:
    uint8 Pad_0[0xC];
};

void __ProcessEvent(UObject* Obj, FName Function, void* Parms);

template <typename Ret, typename... Args>
class TDelegate<Ret(Args...)>
{
public:
    FScriptDelegate BoundFunction;

    void Process(void* Parms = nullptr)
    {
        auto Obj = BoundFunction.Object.Get();

        if (!Obj)
            return;

        __ProcessEvent(Obj, BoundFunction.FunctionName, Parms);
        //Obj->ProcessEvent(Obj->Class->FindFunction(BoundFunction.FunctionName), Parms);
    }
};

template <typename FunctionSignature>
class TMulticastInlineDelegate
{
public:
    uint8 Pad_0[0x10];
};

template <typename Ret, typename... Args>
class TMulticastInlineDelegate<Ret(Args...)>
{
public:
    TArray<FScriptDelegate> InvocationList;

    void Process(void* Parms = nullptr)
    {
        for (auto& ScriptDelegate : InvocationList)
        {
            auto Obj = ScriptDelegate.Object.Get();

            if (!Obj)
                continue;

            __ProcessEvent(Obj, ScriptDelegate.FunctionName, Parms);
            //Obj->ProcessEvent(Obj->Class->FindFunction(ScriptDelegate.FunctionName), Parms);
        }
    }

    void Bind(UObject* Object, FName FunctionName)
    {
        FScriptDelegate NewDelegate;

        NewDelegate.Object = Object;
        NewDelegate.FunctionName = FunctionName;

        InvocationList.Add(NewDelegate);
    }
};

template <typename TObjectID>
class TPersistentObjectPtr
{
public:
    FWeakObjectPtr WeakPtr;
    TObjectID ObjectID;

public:
    class UObject* Get() const
    {
        return WeakPtr.Get();
    }
    class UObject* operator->() const
    {
        return WeakPtr.Get();
    }
};

class FUniqueObjectGuid final
{
public:
    uint32 A;
    uint32 B;
    uint32 C;
    uint32 D;
};

template <typename UEType>
class TLazyObjectPtr : public TPersistentObjectPtr<FUniqueObjectGuid>
{
public:
    UEType* Get() const
    {
        return static_cast<UEType*>(TPersistentObjectPtr::Get());
    }
    UEType* operator->() const
    {
        return static_cast<UEType*>(TPersistentObjectPtr::Get());
    }
};

namespace FakeSoftObjectPtr
{
    struct FTopLevelAssetPath final
    {
    public:
        class FName PackageName;
        class FName AssetName;
    };

    struct FSoftObjectPath
    {
    public:
        struct FTopLevelAssetPath AssetPath;
        class FString SubPathString;
    };

}

class FSoftObjectPtr : public TPersistentObjectPtr<FakeSoftObjectPtr::FSoftObjectPath>
{
public:
    UObject* Get();
};

template <typename UEType>
class TSoftObjectPtr : public FSoftObjectPtr
{
public:
    UEType* Get()
    {
        return static_cast<UEType*>(FSoftObjectPtr::Get());
    }
    UEType* operator->()
    {
        return static_cast<UEType*>(FSoftObjectPtr::Get());
    }
};

template <typename UEType>
class TSoftClassPtr : public FSoftObjectPtr
{
public:
    UEType* Get()
    {
        return static_cast<UEType*>(FSoftObjectPtr::Get());
    }
    UEType* operator->()
    {
        return static_cast<UEType*>(FSoftObjectPtr::Get());
    }
};

class FScriptInterface
{
public:
    UObject* ObjectPointer;
    void* InterfacePointer;

public:
    class UObject* GetObjectRef() const
    {
        return ObjectPointer;
    }

    void* GetInterfaceRef() const
    {
        return InterfacePointer;
    }

    UObject* operator->()
    {
        return ObjectPointer;
    }

    operator UObject* ()
    {
        return ObjectPointer;
    }
};

template <class InterfaceType>
class TScriptInterface final : public FScriptInterface
{
};

class FFieldPath
{
public:
    class FField* ResolvedField;
    TWeakObjectPtr<class UStruct> ResolvedOwner;
    TArray<FName> Path;
};

template <class PropertyType>
class TFieldPath final : public FFieldPath
{
};

template <typename OptionalType, bool bIsIntrusiveUnsetCheck = false>
class TOptional
{
private:
    template <int32 TypeSize>
    struct OptionalWithBool
    {
        static_assert(TypeSize > 0x0, "TOptional can not store an empty type!");

        uint8 Value[TypeSize];
        bool bIsSet;
    };

private:
    using ValueType = std::conditional_t<bIsIntrusiveUnsetCheck, uint8[sizeof(OptionalType)], OptionalWithBool<sizeof(OptionalType)>>;

private:
    alignas(OptionalType) ValueType StoredValue;

private:
    inline uint8* GetValueBytes()
    {
        if constexpr (!bIsIntrusiveUnsetCheck)
            return StoredValue.Value;

        return StoredValue;
    }

    inline const uint8* GetValueBytes() const
    {
        if constexpr (!bIsIntrusiveUnsetCheck)
            return StoredValue.Value;

        return StoredValue;
    }

public:
    inline OptionalType& GetValueRef()
    {
        return *reinterpret_cast<OptionalType*>(GetValueBytes());
    }

    inline const OptionalType& GetValueRef() const
    {
        return *reinterpret_cast<const OptionalType*>(GetValueBytes());
    }

    inline bool IsSet() const
    {
        if constexpr (!bIsIntrusiveUnsetCheck)
            return StoredValue.bIsSet;

        constexpr char ZeroBytes[sizeof(OptionalType)];

        return memcmp(GetValueBytes(), &ZeroBytes, sizeof(OptionalType)) == 0;
    }

    inline explicit operator bool() const
    {
        return IsSet();
    }
};

class alignas(0x01) FMulticastSparseDelegateProperty
{
    unsigned __int8 Pad[0x1];
};

class FOutputDevice
{
public:
    bool bSuppressEventTag;
    bool bAutoEmitLineTerminator;
    uint8_t _Padding1[0x6];
};

struct FOutParmRec
{
    void* Property;
    uint8* PropAddr;
    FOutParmRec* NextOutParm;
};

class FFrame : public FOutputDevice
{
public:
    void** VTable;
    class UFunction* Node;
    UObject* Object;
    uint8* Code;
    uint8* Locals;
    void* MostRecentProperty;
    uint8_t* MostRecentPropertyAddress;
    uint8* MostRecentPropertyContainer;
    TArray<void*> FlowStack;
    FFrame* PreviousFrame;
    FOutParmRec* OutParms;
    uint8_t _Padding1[0x20]; // wtf else do they store here
    FField* PropertyChainForCompiledIn;
    class UFunction* CurrentNativeFunction;
    FFrame* PreviousTrackingFrame;
    bool bArrayContextFailed;

public:
    void StepCompiledIn(void* const Result, bool ForceExplicitProp = false)
    {
        if (Code && !ForceExplicitProp)
        {
            static auto Step = (void (*)(FFrame*, UObject*, void* const))(ImageBase + 0x160F4E4);
            Step(this, Object, Result);
        }
        else
        {
            FField* _Prop = PropertyChainForCompiledIn;
            PropertyChainForCompiledIn = (FField*)~__ROR8__(*(uint64_t*)(__int64(_Prop) + 16) ^ 0xAC0E7F64LL, 16);

            static auto StepExplicitProperty = (void (*)(FFrame*, void* const, FField*))(ImageBase + 0x1B5C108);
            StepExplicitProperty(this, Result, _Prop);
        }
    }

    template <typename T>
    T& StepCompiledInRef()
    {
        T TempVal {};
        MostRecentPropertyAddress = nullptr;

        if (Code)
        {
            static auto Step = (void (*)(FFrame*, UObject*, void* const))(ImageBase + 0x160F4E4);
            Step(this, Object, &TempVal);
        }
        else
        {
            FField* _Prop = PropertyChainForCompiledIn;
            PropertyChainForCompiledIn = (FField*)~__ROR8__(*(uint64_t*)(__int64(_Prop) + 16) ^ 0xAC0E7F64LL, 16);

            static auto StepExplicitProperty = (void (*)(FFrame*, void* const, FField*))(ImageBase + 0x1B5C108);
            StepExplicitProperty(this, &TempVal, _Prop);
        }

        return MostRecentPropertyAddress ? *(T*)MostRecentPropertyAddress : TempVal;
    }

    void IncrementCode()
    {
        Code = (uint8_t*)(__int64(Code) + (bool)Code);
    }
};

struct FFastArraySerializerItem
{
public:
    int32 ReplicationID;
    int32 ReplicationKey;
    int32 MostRecentArrayReplicationKey;
};

struct alignas(0x08) FFastArraySerializer
{
public:
    TMap<int32, int32> ItemMap;
    int32 IDCounter;
    int32 ArrayReplicationKey;
    char GuidReferencesMap[0x50];
    char GuidReferencesMap_StructDelta[0x50];

    int32 CachedNumItems;
    int32 CachedNumItemsToConsiderForWriting;

    uint8_t DeltaFlags;

    uint8 _Padding1[0x7];

    void MarkItemDirty(FFastArraySerializerItem& Item, bool markArrayDirty = true)
    {
        if (Item.ReplicationID == -1)
        {
            Item.ReplicationID = ++IDCounter;
            if (IDCounter == -1)
            {
                IDCounter++;
            }
        }

        Item.ReplicationKey++;
        if (markArrayDirty)
            MarkArrayDirty();
    }

    void MarkArrayDirty()
    {
        ArrayReplicationKey++;
        if (ArrayReplicationKey == -1)
        {
            ArrayReplicationKey++;
        }

        CachedNumItems = -1;
        CachedNumItemsToConsiderForWriting = -1;
    }
};