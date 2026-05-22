#include "pch.h"
#include "Shared.h"

void UObject::ProcessEvent(UFunction* Func, void* Parms)
{
    auto ProcessEvent_ = (void(*)(UObject*, UFunction*, void*)) this->VTable[0x46];

    ProcessEvent_(this, Func, Parms);
}


UFunction* UClass::FindFunction(const char* FuncName)
{
    UEAllocatedString s = FuncName;
    UEAllocatedWString ws(s.begin(), s.end());
    auto PropName = FName(ws);

    for (auto Clss = this; Clss; Clss = *(UClass**)(__int64(Clss) + 0x40))
        for (auto Prop = *(UField**)(__int64(Clss) + 0x78); Prop; Prop = (UField*)~__ROR8__(*(uint64_t*)(__int64(Prop) + 40) ^ 0xAC0E7F64LL, 16))
            if (Prop->Name == PropName)
                return (UFunction*)Prop;

    return nullptr;
}

UFunction* UClass::FindFunction(FName FuncName)
{
    for (auto Clss = this; Clss; Clss = *(UClass**)(__int64(Clss) + 0x40))
        for (auto Prop = *(UField**)(__int64(Clss) + 0x78); Prop; Prop = (UField*)~__ROR8__(*(uint64_t*)(__int64(Prop) + 40) ^ 0xAC0E7F64LL, 16))
            if (Prop->Name == FuncName)
                return (UFunction*)Prop;

    return nullptr;
}

class UClass* InternalStaticClass(const char* ClassName)
{
    UEAllocatedString s = ClassName;
    UEAllocatedWString ws(s.begin(), s.end());
    auto ObjName = FName(ws);

    for (uint32_t i = 0; i < TUObjectArray::Num(); i++)
    {
        const UObject* Obj = TUObjectArray::GetObjectByIndex(i);
        if (Obj && Obj->Class && Obj->Name == ObjName)
            return (UClass*)Obj;
    }

    return nullptr;
}

inline int StartingSerial = 676767676; // scuffed
FWeakObjectPtr::FWeakObjectPtr(UObject* Object)
{
    if (Object)
    {
        ObjectIndex = Object->GetIndex();
        auto Item = TUObjectArray::GetItemByIndex(ObjectIndex);

        if (Item)
        {
            if (Item->SerialNumber == 0)
                Item->SerialNumber = StartingSerial++;

            ObjectSerialNumber = Item->SerialNumber;
        }
        else
        {
            ObjectIndex = -1;
            ObjectSerialNumber = 0;
        }
    }
    else
    {
        ObjectIndex = -1;
        ObjectSerialNumber = 0;
    }
}


UObject* FWeakObjectPtr::Get() const
{
    /*if (!this)
        return nullptr;

    if (ObjectIndex < 0 || ObjectSerialNumber == 0)
        return nullptr;

    auto Item = TUObjectArray::GetItemByIndex(ObjectIndex);

    if (!Item || Item->SerialNumber != ObjectSerialNumber)
        return nullptr;

    return (class UObject*)~__ROR8__(Item->EncryptedObject ^ 0xD8BBB184LL, 16);*/
    static auto Get_ = (UObject * (*)(const FWeakObjectPtr*))(ImageBase + 0x4B6D02C);

    return Get_(this);
}

bool FWeakObjectPtr::operator==(class UObject* Other) const
{
    return ObjectIndex == Other->GetIndex() && ObjectSerialNumber == TUObjectArray::GetItemByIndex(ObjectIndex)->SerialNumber;
}

bool FWeakObjectPtr::operator!=(class UObject* Other) const
{
    return ObjectIndex != Other->GetIndex() || ObjectSerialNumber != TUObjectArray::GetItemByIndex(ObjectIndex)->SerialNumber;
}

UObject* FSoftObjectPtr::Get()
{
    if (!this)
        return nullptr;


    auto LoadSynchronous = (UObject * (*)(FSoftObjectPtr*))(ImageBase + 0x15685E4);
    return LoadSynchronous(this);
}

bool UObject::IsA(UClass* OtherClass)
{
    return this && Class->IsSubclassOf(OtherClass);
}

class FStructBaseChain
{
public:
    FStructBaseChain()
        : StructBaseChainArray(nullptr)
        , NumStructBasesInChainMinusOne(-1)
    {
    }
    ~FStructBaseChain()
    {
        delete[] StructBaseChainArray;
    }

    FStructBaseChain(const FStructBaseChain&) = delete;
    FStructBaseChain& operator=(const FStructBaseChain&) = delete;

    __forceinline bool IsChildOfUsingStructArray(const FStructBaseChain& Parent) const
    {
        int32 NumParentStructBasesInChainMinusOne = Parent.NumStructBasesInChainMinusOne;
        return NumParentStructBasesInChainMinusOne <= NumStructBasesInChainMinusOne && StructBaseChainArray[NumParentStructBasesInChainMinusOne] == &Parent;
    }

private:
    FStructBaseChain** StructBaseChainArray;
    int32 NumStructBasesInChainMinusOne;

    friend class UStruct;
};

bool UClass::IsSubclassOf(UClass* OtherClass)
{
    if (!this || !OtherClass)
        return false;

    auto& ThisChain = *(FStructBaseChain*)(__int64(this) + 0x30);
    auto& OtherChain = *(FStructBaseChain*)(__int64(OtherClass) + 0x30);

    return ThisChain.IsChildOfUsingStructArray(OtherChain);
}

UObject* TUObjectArray::FindObject(const char* Name, UClass* TargetClass)
{
    UEAllocatedString s = Name;
    UEAllocatedWString ws(s.begin(), s.end());
    auto ObjName = FName(ws);

    for (uint32_t i = 0; i < Num(); i++)
    {
        UObject* Obj = GetObjectByIndex(i);

        if (Obj && Obj->Class && Obj->Name == ObjName && (!TargetClass || Obj->IsA(TargetClass)))
            return Obj;
    }
    return nullptr;
}

UObject* TUObjectArray::FindFirstObject(const char* Name)
{
    UClass* TargetClass = (UClass*)FindObject(Name);

    if (TargetClass)
        for (uint32_t i = 0; i < Num(); i++)
        {
            UObject* Obj = GetObjectByIndex(i);

            if (Obj && !Obj->IsDefaultObject() && Obj->IsA(TargetClass))
                return Obj;
        }

    return nullptr;
}


void __ProcessEvent(UObject* Obj, FName Function, void* Parms)
{
    Obj->ProcessEvent(Obj->Class->FindFunction(Function), Parms);
}