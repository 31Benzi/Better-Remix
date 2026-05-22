#include "pch.h"
#include "Shared.h"
#pragma comment(lib, "MinHook.lib")

UWorld* GetWorld()
{
    return *(UWorld**)(ImageBase + 0x12ED8E38);
}

UFortEngine* GetEngine()
{
    return *(UFortEngine**)(ImageBase + 0x12D6E978);
}

UInterface* GetInterfaceAddress(UObject* Object, UClass* Class)
{
    if (!Object || !Class)
        return nullptr;
    static auto GetInterfaceAddress_ = (UInterface * (*)(UObject*, UClass*))(ImageBase + 0x16574E4);
    return GetInterfaceAddress_(Object, Class);
}

FQuat RotatorToQuat(FRotator Rot)
{
    double halfOfARadian = 0.008726646259971648;
    double sinPitch = sin(Rot.pitch * halfOfARadian), sinYaw = sin(Rot.Yaw * halfOfARadian), sinRoll = sin(Rot.Roll * halfOfARadian);
    double cosPitch = cos(Rot.pitch * halfOfARadian), cosYaw = cos(Rot.Yaw * halfOfARadian), cosRoll = cos(Rot.Roll * halfOfARadian);

    FQuat out {};
    out.X = cosRoll * sinPitch * sinYaw - sinRoll * cosPitch * cosYaw;
    out.Y = -cosRoll * sinPitch * cosYaw - sinRoll * cosPitch * sinYaw;
    out.Z = cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw;
    out.W = cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw;
    return out;
}

double ClampAxis(double Angle)
{
    Angle = fmod(Angle, 360.f); // rat

    if (Angle < 0.)
        Angle += 360.;
    return Angle;
}

double NormalizeAxis(double Angle)
{
    Angle = ClampAxis(Angle);

    if (Angle > 180.)
        Angle -= 360.;
    return Angle;
}

FRotator QuatToRotator(FQuat Quat)
{
    const double SingularityTest = Quat.Z * Quat.X - Quat.W * Quat.Y;
    const double YawY = 2.f * (Quat.W * Quat.Z + Quat.X * Quat.Y);
    const double YawX = (1.f - 2.f * ((Quat.Y * Quat.Y) + (Quat.Z * Quat.Z)));

    const double SINGULARITY_THRESHOLD = 0.4999995;
    const double RAD_TO_DEG = 57.29577951308232;
    FRotator RotatorFromQuat {};

    if (SingularityTest < -SINGULARITY_THRESHOLD)
    {
        RotatorFromQuat.pitch = -90.;
        RotatorFromQuat.Yaw = atan2(YawY, YawX) * RAD_TO_DEG;
        RotatorFromQuat.Roll = NormalizeAxis(-RotatorFromQuat.Yaw - (2.f * atan2(Quat.X, Quat.W) * RAD_TO_DEG));
    }
    else if (SingularityTest > SINGULARITY_THRESHOLD)
    {
        RotatorFromQuat.pitch = 90.;
        RotatorFromQuat.Yaw = atan2(YawY, YawX) * RAD_TO_DEG;
        RotatorFromQuat.Roll = NormalizeAxis(RotatorFromQuat.Yaw - (2.f * atan2(Quat.X, Quat.W) * RAD_TO_DEG));
    }
    else
    {
        RotatorFromQuat.pitch = asin(2.f * SingularityTest) * RAD_TO_DEG;
        RotatorFromQuat.Yaw = atan2(YawY, YawX) * RAD_TO_DEG;
        RotatorFromQuat.Roll = atan2(-2.f * (Quat.W * Quat.X + Quat.Y * Quat.Z), (1.f - 2.f * ((Quat.X * Quat.X) + (Quat.Y * Quat.Y)))) * RAD_TO_DEG;
    }

    return RotatorFromQuat;
}

FTransform ConstructTrans(FVector Vec, FRotator Rot, FVector scale)
{
    FTransform Trans;

    Trans.Rotation = RotatorToQuat(Rot);
    Trans.Translation = Vec;
    Trans.Scale3D = scale;

    return Trans;
}

FTransform ConstructTrans(FVector Vec, FQuat Rot, FVector scale)
{
    FTransform Trans;

    Trans.Rotation = Rot;
    Trans.Translation = Vec;
    Trans.Scale3D = scale;

    return Trans;
}

__forceinline UObject* StaticFindObject(const wchar_t* ObjectPath, UClass* Class)
{
    auto StaticFindObjectInternal = (UObject * (*)(const UClass*, UObject*, const wchar_t*, bool))(ImageBase + 0x1A2CAB8);
    return StaticFindObjectInternal(Class, nullptr, ObjectPath, false);
}

__forceinline UObject* StaticLoadObject(const wchar_t* ObjectPath, UClass* InClass, UObject* Outer = nullptr)
{
    auto StaticLoadObjectInternal = (UObject * (*)(const UClass*, UObject*, const wchar_t*, const wchar_t*, uint32_t, UObject*, bool, void*))(ImageBase + 0x1CB5254);
    return StaticLoadObjectInternal(InClass, Outer, ObjectPath, nullptr, 0, nullptr, false, nullptr);
}

UObject* FindObject(const wchar_t* ObjectPath, UClass* Class)
{
    auto Object = StaticFindObject(ObjectPath, Class);
    return Object ? Object : StaticLoadObject(ObjectPath, Class);
}

AActor* SpawnActor(UClass* Class, FTransform Transform, AActor* Owner, ESpawnActorCollisionHandlingMethod CollisionOverride)
{
    static auto SpawnActorInternal = (AActor * (*)(UWorld*, const UClass*, FTransform*, void*))(ImageBase + 0x1B54FA8);

    FActorSpawnParameters SpawnParameters {};

    SpawnParameters.Owner = Owner;
    SpawnParameters.bDeferConstruction = false;
    SpawnParameters.SpawnCollisionHandlingOverride = CollisionOverride;
    SpawnParameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

    return SpawnActorInternal(GetWorld(), Class, &Transform, &SpawnParameters);

    //auto Actor = UGameplayStatics::BeginDeferredActorSpawnFromClass(GetWorld(), Class, Transform, CollisionOverride, Owner, ESpawnActorScaleMethod::MultiplyWithRoot);

    //return UGameplayStatics::FinishSpawningActor(Actor, Transform, ESpawnActorScaleMethod::MultiplyWithRoot);
}

AActor* SpawnActor(UClass* Class, FVector Loc, FRotator Rot, AActor* Owner, ESpawnActorCollisionHandlingMethod CollisionOverride)
{
    static auto SpawnActorInternal = (AActor * (*)(UWorld*, const UClass*, FVector*, FRotator*, void*))(ImageBase + 0x2FA7A58);

    FActorSpawnParameters SpawnParameters {};

    SpawnParameters.Owner = Owner;
    SpawnParameters.bDeferConstruction = false;
    SpawnParameters.SpawnCollisionHandlingOverride = CollisionOverride;
    SpawnParameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

    return SpawnActorInternal(GetWorld(), Class, &Loc, &Rot, &SpawnParameters);
}


UFortAssetManager* GetAssetManager()
{
    static UFortAssetManager* AssetManager = nullptr;

    if (!AssetManager)
        AssetManager = (UFortAssetManager*)GetEngine()->AssetManager;

    return AssetManager;
}

UGameDataBR* GetGameData()
{
    UGameDataBR* GameData = nullptr;

    if (!GameData)
        GameData = GetAssetManager()->GameDataBR;

    return GameData;
}