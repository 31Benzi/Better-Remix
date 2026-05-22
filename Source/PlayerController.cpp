#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
#include "Pickups.h"
#include "GamePhaseLogic.h"
#include <algorithm>
#include "CurlHttp.h"
#include "Config.h"
INIT_MODULE(PlayerController);

struct FLateGameItem
{
    uint32 Count;
    UFortItemDefinition* Item;

    FLateGameItem() : Count(0), Item(nullptr) {}
    FLateGameItem(uint32 InCount, UFortItemDefinition* InItem) : Count(InCount), Item(InItem) {}
};

enum class EAmmoType : uint8
{
    Assault = 0,
    Shotgun = 1,
    Submachine = 2,
    Rocket = 3,
    Sniper = 4
};

static FLateGameItem GetShotgun()
{
    static UEAllocatedVector<FLateGameItem> Shotguns {};

    if (Shotguns.size() == 0)
    {
        auto PaprikaPumpLegendary = FindObject<UFortItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaShotgun_Pump/WID_Shotgun_Pump_Paprika_Athena_SR.WID_Shotgun_Pump_Paprika_Athena_SR");
        auto PaprikaPumpEpic = FindObject<UFortItemDefinition>(L"/PaprikaCoreWeapons/Items/Weapons/PaprikaShotgun_Pump/WID_Shotgun_Pump_Paprika_Athena_VR.WID_Shotgun_Pump_Paprika_Athena_VR");
        auto StandardPumpLegendary = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_SR_Ore_T03.WID_Shotgun_Standard_Athena_SR_Ore_T03");
        auto StandardPumpEpic = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_VR_Ore_T03.WID_Shotgun_Standard_Athena_VR_Ore_T03");

        if (PaprikaPumpLegendary) Shotguns.push_back(FLateGameItem(1, PaprikaPumpLegendary));
        if (PaprikaPumpEpic) Shotguns.push_back(FLateGameItem(1, PaprikaPumpEpic));
        if (StandardPumpLegendary) Shotguns.push_back(FLateGameItem(1, StandardPumpLegendary));
        if (StandardPumpEpic) Shotguns.push_back(FLateGameItem(1, StandardPumpEpic));
    }

    if (Shotguns.size() == 0) return FLateGameItem(1, nullptr);
    return Shotguns[rand() % Shotguns.size()];
}

static FLateGameItem GetAssaultRifle()
{
    static UEAllocatedVector<FLateGameItem> AssaultRifles {};

    if (AssaultRifles.size() == 0)
    {
        auto ScarLegendary = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_SR_Ore_T03.WID_Assault_AutoHigh_Athena_SR_Ore_T03");
        auto ScarEpic = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_VR_Ore_T03.WID_Assault_AutoHigh_Athena_VR_Ore_T03");

        if (ScarLegendary) AssaultRifles.push_back(FLateGameItem(1, ScarLegendary));
        if (ScarEpic) AssaultRifles.push_back(FLateGameItem(1, ScarEpic));
    }

    if (AssaultRifles.size() == 0) return FLateGameItem(1, nullptr);
    return AssaultRifles[rand() % AssaultRifles.size()];
}

static FLateGameItem GetUtility()
{
    static UEAllocatedVector<FLateGameItem> Utilities {};

    if (Utilities.size() == 0)
    {
        auto BoltSniper = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03.WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03");
        auto TacSMG = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Pistol_Scavenger_Athena_VR_Ore_T03.WID_Pistol_Scavenger_Athena_VR_Ore_T03");

        if (BoltSniper) Utilities.push_back(FLateGameItem(1, BoltSniper));
        if (TacSMG) Utilities.push_back(FLateGameItem(1, TacSMG));
    }

    if (Utilities.size() == 0) return FLateGameItem(1, nullptr);
    return Utilities[rand() % Utilities.size()];
}

static FLateGameItem GetHeal()
{
    static UEAllocatedVector<FLateGameItem> Heals {};

    if (Heals.size() == 0)
    {
        auto BigPots = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Consumables/Shields/Athena_Shields.Athena_Shields");
        auto Minis = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Consumables/ShieldSmall/Athena_ShieldSmall.Athena_ShieldSmall");
        auto ChugSplash = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Consumables/ChillBronco/Athena_ChillBronco.Athena_ChillBronco");
        auto SlurpJuice = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Consumables/PurpleStuff/Athena_PurpleStuff.Athena_PurpleStuff");

        if (BigPots) Heals.push_back(FLateGameItem(3, BigPots));
        if (Minis) Heals.push_back(FLateGameItem(6, Minis));
        if (ChugSplash) Heals.push_back(FLateGameItem(6, ChugSplash));
        if (SlurpJuice) Heals.push_back(FLateGameItem(2, SlurpJuice));
    }

    if (Heals.size() == 0) return FLateGameItem(1, nullptr);
    return Heals[rand() % Heals.size()];
}

static UFortItemDefinition* GetAmmo(EAmmoType AmmoType)
{
    static UEAllocatedVector<UFortItemDefinition*> Ammos {};
    if (Ammos.size() == 0)
    {
        Ammos.push_back(FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsLight.AthenaAmmoDataBulletsLight"));
        Ammos.push_back(FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataShells.AthenaAmmoDataShells"));
        Ammos.push_back(FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsMedium.AthenaAmmoDataBulletsMedium"));
        
        auto Rockets = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Ammo/AmmoDataRockets.AthenaAmmoDataRockets");
        if (!Rockets) Rockets = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Ammo/AmmoDataRockets.AmmoDataRockets");
        Ammos.push_back(Rockets);
        
        Ammos.push_back(FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsHeavy.AthenaAmmoDataBulletsHeavy"));
    }

    return Ammos[(uint8)AmmoType];
}

void ServerAcknowledgePossession(AFortPlayerControllerAthena* _this, APawn* Pawn)
{
    if (!Pawn || !Pawn->IsA<APawn>())
        return;

    _this->AcknowledgedPawn = Pawn;

    if ((!bKeepInventory || bLateGame) && _this->WorldInventory.ObjectPointer)
    {
        UEAllocatedVector<FGuid> GuidsToRemove;
        for (auto& Entry : ((AFortInventory*)_this->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries)
        {
            auto PickupComponent = GetPickupComponent((UFortItemDefinition*)Entry.ItemDefinition);
            if (PickupComponent && PickupComponent->bCanBeDroppedFromInventory)
            {
                GuidsToRemove.push_back(Entry.ItemGuid);
            }
        }

        for (auto& Guid : GuidsToRemove)
            Remove(_this->WorldInventory, Guid);
    }

    static auto ApplyCharacterCustomization = (void (*)(AActor*, APawn*, int))(ImageBase + 0xA2C731C);
    static auto InitializePlayerGameplayAbilities = (void (*)(UInterface*))(ImageBase + 0x9A8894C);

    auto CosmeticLoadout = (UFortControllerComponent_CosmeticLoadout*)_this->GetComponentByClass(UFortControllerComponent_CosmeticLoadout::StaticClass());

    if (_this->PlayerState)
    {
        ApplyCharacterCustomization(_this->PlayerState, Pawn, 0);
        auto AbilitySystemInterface = GetInterfaceAddress(_this->PlayerState, UFortAbilitySystemInterface::StaticClass());
        if (AbilitySystemInterface)
        {
            InitializePlayerGameplayAbilities(AbilitySystemInterface);
        }
    }

    if (((AFortInventory*)_this->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries.Num() == 0)
    {
        auto GameMode = (AFortGameModeAthena*)GetWorld()->AuthorityGameMode;
        for (auto& StartingItem : GameMode->StartingItems)
            if (StartingItem.Count && !StartingItem.Item->IsA<UFortSmartBuildingItemDefinition>())
                GiveItem(_this->WorldInventory, (UFortItemDefinition*)StartingItem.Item, StartingItem.Count);

        static auto DefaultPickaxe = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");

        GiveItem(_this->WorldInventory, CosmeticLoadout->CachedAthenaLoadout.Pickaxe->WeaponDefinition);
        //GiveItem((AFortInventory*)_this->WorldInventory.ObjectPointer, _this->CosmeticLoadoutPC.Pickaxe->WeaponDefinition);
    }

    if (bLateGame)
    {
        if (_this->MyFortPawn)
        {
            _this->MyFortPawn->SetShield(100.f);
        }

        if (((AFortInventory*)_this->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries.Num() <= 1)
        {
            auto Shotgun = GetShotgun();
            auto AssaultRifle = GetAssaultRifle();
            auto Sniper = GetUtility();
            auto Heal = GetHeal();
            auto HealSlot2 = GetHeal();

            int ShotgunClipSize = 0;
            int AssaultRifleClipSize = 0;
            int SniperClipSize = 0;
            int HealClipSize = 0;
            int HealSlot2ClipSize = 0;

            if (Shotgun.Item)
            {
                if (auto Weapon = Shotgun.Item->IsA<UFortGadgetItemDefinition>() ? ((UFortGadgetItemDefinition*)Shotgun.Item)->GetWeaponItemDefinition() : Shotgun.Item->Cast<UFortWeaponItemDefinition>())
                {
                    auto Stats = GetStats(Weapon);
                    if (Stats) ShotgunClipSize = Stats->ClipSize;
                }
            }
            if (AssaultRifle.Item)
            {
                if (auto Weapon = AssaultRifle.Item->IsA<UFortGadgetItemDefinition>() ? ((UFortGadgetItemDefinition*)AssaultRifle.Item)->GetWeaponItemDefinition() : AssaultRifle.Item->Cast<UFortWeaponItemDefinition>())
                {
                    auto Stats = GetStats(Weapon);
                    if (Stats) AssaultRifleClipSize = Stats->ClipSize;
                }
            }
            if (Sniper.Item)
            {
                if (auto Weapon = Sniper.Item->IsA<UFortGadgetItemDefinition>() ? ((UFortGadgetItemDefinition*)Sniper.Item)->GetWeaponItemDefinition() : Sniper.Item->Cast<UFortWeaponItemDefinition>())
                {
                    auto Stats = GetStats(Weapon);
                    if (Stats) SniperClipSize = Stats->ClipSize;
                }
            }
            if (Heal.Item)
            {
                if (auto Weapon = Heal.Item->IsA<UFortGadgetItemDefinition>() ? ((UFortGadgetItemDefinition*)Heal.Item)->GetWeaponItemDefinition() : Heal.Item->Cast<UFortWeaponItemDefinition>())
                {
                    auto Stats = GetStats(Weapon);
                    if (Stats) HealClipSize = Stats->ClipSize;
                }
            }
            if (HealSlot2.Item)
            {
                if (auto Weapon = HealSlot2.Item->IsA<UFortGadgetItemDefinition>() ? ((UFortGadgetItemDefinition*)HealSlot2.Item)->GetWeaponItemDefinition() : HealSlot2.Item->Cast<UFortWeaponItemDefinition>())
                {
                    auto Stats = GetStats(Weapon);
                    if (Stats) HealSlot2ClipSize = Stats->ClipSize;
                }
            }

            GiveItem(_this->WorldInventory, UFortKismetLibrary::K2_GetResourceItemDefinition(EFortResourceType::Wood), 500);
            GiveItem(_this->WorldInventory, UFortKismetLibrary::K2_GetResourceItemDefinition(EFortResourceType::Stone), 500);
            GiveItem(_this->WorldInventory, UFortKismetLibrary::K2_GetResourceItemDefinition(EFortResourceType::Metal), 500);

            if (auto Ammo = GetAmmo(EAmmoType::Assault)) GiveItem(_this->WorldInventory, (UFortItemDefinition*)Ammo, 250);
            if (auto Ammo = GetAmmo(EAmmoType::Shotgun)) GiveItem(_this->WorldInventory, (UFortItemDefinition*)Ammo, 50);
            if (auto Ammo = GetAmmo(EAmmoType::Submachine)) GiveItem(_this->WorldInventory, (UFortItemDefinition*)Ammo, 400);
            if (auto Ammo = GetAmmo(EAmmoType::Rocket)) GiveItem(_this->WorldInventory, (UFortItemDefinition*)Ammo, 6);
            if (auto Ammo = GetAmmo(EAmmoType::Sniper)) GiveItem(_this->WorldInventory, (UFortItemDefinition*)Ammo, 20);

            if (Shotgun.Item) GiveItem(_this->WorldInventory, (UFortItemDefinition*)Shotgun.Item, Shotgun.Count, ShotgunClipSize);
            if (AssaultRifle.Item) GiveItem(_this->WorldInventory, (UFortItemDefinition*)AssaultRifle.Item, AssaultRifle.Count, AssaultRifleClipSize);
            if (Sniper.Item) GiveItem(_this->WorldInventory, (UFortItemDefinition*)Sniper.Item, Sniper.Count, SniperClipSize);
            if (Heal.Item) GiveItem(_this->WorldInventory, (UFortItemDefinition*)Heal.Item, Heal.Count, HealClipSize);
            if (HealSlot2.Item) GiveItem(_this->WorldInventory, (UFortItemDefinition*)HealSlot2.Item, HealSlot2.Count, HealSlot2ClipSize);
        }
    }

    static auto Effect = FindObject<UClass>(L"/Game/Athena/SafeZone/GE_OutsideSafeZoneDamage.GE_OutsideSafeZoneDamage_C");

    bool Found = false;
    auto AbilitySystemComponent = _this->MyFortPawn->AbilitySystemComponent;

    for (auto& ActiveEffect : AbilitySystemComponent->ActiveGameplayEffects.GameplayEffects_Internal)
    {
        if (ActiveEffect.Spec.Def)
            if (ActiveEffect.Spec.Def->IsA(Effect))
            {
                Found = true;
                break;
            }
    }

    if (!Found)
    {
        auto EffectHandle = FGameplayEffectContextHandle();
        auto SpecHandle = AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(Effect, 0.f, EffectHandle);

        // AbilitySystemComponent->SetActiveGameplayEffectLevel(SpecHandle, 1);

        AbilitySystemComponent->UpdateActiveGameplayEffectSetByCallerMagnitude(SpecHandle, FGameplayTag(FName(L"SetByCaller.StormCampingDamage")), 1.f);
    }

    _this->MyFortPawn->bIsInAnyStorm = false;
    _this->MyFortPawn->OnRep_IsInAnyStorm();
    _this->MyFortPawn->bIsInsideSafeZone = true;
    _this->MyFortPawn->OnRep_IsInsideSafeZone();
}

void (*GetPlayerViewPointOG)(AFortPlayerControllerAthena* _this, FVector& Loc, FRotator& Rot);
void GetPlayerViewPoint(AFortPlayerControllerAthena* _this, FVector& Loc, FRotator& Rot)
{
    auto ViewTarget = _this->GetViewTarget();

    if (ViewTarget)
        return ViewTarget->GetActorEyesViewPoint(&Loc, &Rot);

    return GetPlayerViewPointOG(_this, Loc, Rot);
}

void ServerExecuteInventoryItem(AFortPlayerControllerAthena* _this, FGuid& ItemGuid)
{
    if (!_this)
        return;

    auto Item = GetItem(_this->WorldInventory, ItemGuid);

    if (!Item)
    {
        printf("Failed to find item?\n");
        return;
    }

    UFortItemDefinition* ItemDefinition = (UFortItemDefinition*)Item->ItemEntry.ItemDefinition;

    bool (*&ServerExecute)(UFortItemDefinition*, UFortItem*, UObject*) = decltype(ServerExecute)(ItemDefinition->VTable[0x9C]);
    ServerExecute(ItemDefinition, Item, _this);

    return;
}

void ServerCreateBuildingActor(AFortPlayerControllerAthena* _this, FCreateBuildingActorData& CreateBuildingData)
{
    auto GameState = (AFortGameStateAthena*)GetWorld()->GameState;

    FBuildingClassData BuildingClassData = CreateBuildingData.BuildingClassData;

    if (!GameState->AllPlayerBuildableClasses.IsValidIndex(CreateBuildingData.BuildingClassHandle))
        return;

    auto BuildingClass = GameState->AllPlayerBuildableClasses[CreateBuildingData.BuildingClassHandle];
    if (!BuildingClass)
        return;

    BuildingClassData.BuildingClass = BuildingClass;

    UFortWorldItem* Item = nullptr;
    auto Resource = UFortKismetLibrary::K2_GetResourceItemDefinition(((ABuildingSMActor*)BuildingClass->GetDefaultObj())->ResourceType);

    static auto CanAffordToPlaceBuildableClass = (bool (*)(AFortPlayerControllerAthena*, FBuildingClassData))(ImageBase + 0xA21CED8);

    if (!bInfiniteMats && !CanAffordToPlaceBuildableClass(_this, BuildingClassData))
        return;

    auto OptAdj = EFortBuildPreviewMarkerOptionalAdjustment::None;
    TArray<ABuildingSMActor*> RemoveBuildings;

    static auto CanPlaceBuildableClassInStructuralGrid = (__int64 (*)(
        AFortPlayerControllerAthena*, UClass**, FVector, FRotator, bool, TArray<ABuildingSMActor*>*, EFortBuildPreviewMarkerOptionalAdjustment*))(
        ImageBase + 0x91F7F14);
    if (CanPlaceBuildableClassInStructuralGrid(
        _this, &BuildingClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, CreateBuildingData.bMirrored, &RemoveBuildings, &OptAdj))
        return;

    for (auto& RemoveBuilding : RemoveBuildings)
        RemoveBuilding->K2_DestroyActor();
    RemoveBuildings.Free();

    //auto Building = (ABuildingSMActor*)ABuildingActor::K2_SpawnBuildingActor(_this, BuildingClass, SpawnTransform, _this, _this->Pawn, true, true);
    auto Building = SpawnActorUnfinished<ABuildingSMActor>(BuildingClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, _this);

    if (!Building)
        return;

    Building->NetCullDistanceSquared = 784000000.0f; // iris retarded
    Building->CullDistance = 28000.0f; // iris retarded

    Building->InitializeKismetSpawnedBuildingActor(Building, _this, true, nullptr, false);

    Building->CurrentBuildingLevel = BuildingClassData.UpgradeLevel;
    Building->OnRep_CurrentBuildingLevel();

    Building->SetMirrored(CreateBuildingData.bMirrored);

    Building->bPlayerPlaced = true;

    Building->Team = EFortTeam(((AFortPlayerStateAthena*)_this->PlayerState)->TeamIndex);

    Building->TeamIndex = uint8(Building->Team);

    FinishSpawnActor(Building, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot);

    if (!bInfiniteMats)
    {
        static auto PayBuildableClassPlacementCost = (int (*)(AFortPlayerControllerAthena*, FBuildingClassData))(ImageBase + 0xA24BD50);

        PayBuildableClassPlacementCost(_this, BuildingClassData);
    }
}

bool CanBePlayerEdited(ABuildingSMActor* _this, AFortPlayerController* EditingPC)
{
    static auto CanBePlayerEdited_ = decltype(&CanBePlayerEdited)(ImageBase + 0x952C704);

    return CanBePlayerEdited_(_this, EditingPC);
}

void SetEditingPlayer(ABuildingSMActor* _this, AFortPlayerStateZone* NewEditingPlayer)
{
    static auto SetEditingPlayer_ = decltype(&SetEditingPlayer)(ImageBase + 0x9539A08);

    return SetEditingPlayer_(_this, NewEditingPlayer);
}

void ServerBeginEditingBuildingActor(AFortPlayerControllerAthena* _this, ABuildingSMActor* Building)
{
    if (!_this || !_this->MyFortPawn || !Building->IsA<ABuildingSMActor>() || !CanBePlayerEdited(Building, _this))
        return;

    AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)_this->PlayerState;
    if (!PlayerState)
        return;

    SetEditingPlayer(Building, PlayerState);

    if (!_this->MyFortPawn->CurrentWeapon->IsA<AFortWeap_EditingTool>())
    {
        auto EditToolEntry = ((AFortInventory*)_this->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries.Search(
            [&](FFortItemEntry& entry)
            {
                return entry.ItemDefinition->Class == UFortEditToolItemDefinition::StaticClass();
            });

        _this->ServerExecuteInventoryItem(EditToolEntry->ItemGuid);
        //_this->MyFortPawn->EquipWeaponDefinition(
        //    (UFortWeaponItemDefinition*)EditToolEntry->ItemDefinition, EditToolEntry->ItemGuid, EditToolEntry->TrackerGuid, false);
    }

    if (auto EditTool = _this->MyFortPawn->CurrentWeapon->Cast<AFortWeap_EditingTool>())
    {
        EditTool->EditActor = Building;
        EditTool->ForceNetUpdate();
        EditTool->OnRep_EditActor();
    }
}

void ServerEditBuildingActor(AFortPlayerControllerAthena* _this, ABuildingSMActor* Building, UClass** NewClassRef, uint8 RotationIterations, bool bMirrored)
{

    if (!_this || !Building || !NewClassRef || !*NewClassRef || !Building->IsA<ABuildingSMActor>() || Building->EditingPlayer != _this->PlayerState
        || Building->bDestroyed)
        return;

    auto NewClass = *NewClassRef;

    if (!((ABuildingSMActor*)NewClass->DefaultObject)->bIsPlayerBuildable)
        return;

    SetEditingPlayer(Building, nullptr);

    static auto ReplaceBuildingActor
        = (ABuildingSMActor * (*)(ABuildingSMActor*, unsigned int, UClass**, unsigned int, int, bool, AFortPlayerControllerAthena*))(ImageBase + 0x9538A78);

    ABuildingSMActor* NewBuild = ReplaceBuildingActor(Building, 1, &NewClass, Building->CurrentBuildingLevel, RotationIterations, bMirrored, _this);

    if (NewBuild)
        NewBuild->bPlayerPlaced = true;
}

void ServerEndEditingBuildingActor(AFortPlayerControllerAthena* _this, ABuildingSMActor* Building)
{
    if (!_this || !_this->MyFortPawn || !Building || !Building->IsA<ABuildingSMActor>() || Building->EditingPlayer != _this->PlayerState || Building->bDestroyed)
        return;

    SetEditingPlayer(Building, nullptr);

    auto EditToolEntry = ((AFortInventory*)_this->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries.Search(
        [&](FFortItemEntry& entry)
        {
            return entry.ItemDefinition->Class == UFortEditToolItemDefinition::StaticClass();
        });

    if (!EditToolEntry)
        return;

    auto EditToolPtr = _this->MyFortPawn->CurrentWeaponList.Search(
        [&](AFortWeapon* Weapon)
        {
            return GuidEquals(Weapon->ItemEntryGuid, EditToolEntry->ItemGuid);
        });

    if (!EditToolPtr)
        return;

    if (auto EditTool = *(AFortWeap_EditingTool**)EditToolPtr)
    {
        EditTool->EditActor = nullptr;
        EditTool->ForceNetUpdate();
        EditTool->OnRep_EditActor();
    }
}

void ServerOnMaterialSelection(AFortPlayerControllerAthena* _this, EFortResourceType NewResourceType, EFortResourceLevel NewResourceLevel)
{
    _this->CurrentResourceType = NewResourceType;
    _this->CurrentResourceLevel = NewResourceLevel;
}

void ServerAttemptInventoryDrop(AFortPlayerControllerAthena* _this, FGuid ItemGuid, int32 Count, bool bTrash)
{
    if (!_this || !_this->Pawn)
        return;

    auto Item = GetItem(_this->WorldInventory, ItemGuid);
    if (!Item)
        return;

    Item->ItemEntry.Count -= Count;

    FVector FinalLoc = _this->Pawn->K2_GetActorLocation();

    FVector ForwardVector = _this->Pawn->GetActorForwardVector();
    // ForwardVector.Z = 0.0f;
    // ForwardVector.Normalize();

    FinalLoc = VecAdd(FinalLoc, VecMul(ForwardVector, 450.f));
    FinalLoc.Z += 50.f;

    const float RandomAngleVariation = ((float)rand() * 0.00109866634f) - 18.f;
    const float FinalAngle = (RandomAngleVariation + 360.f / ((float)rand() * 0.00015259254737998596f)) * 0.017453292519943295f;

    FinalLoc.X += cos(FinalAngle) * 100.f;
    FinalLoc.Y += sin(FinalAngle) * 100.f;

    auto Loc = VecAdd(VecAdd(_this->Pawn->K2_GetActorLocation(), VecMul(_this->Pawn->GetActorForwardVector(), 70.f)), FVector(0, 0, 50));
    SpawnPickup(Loc, Item->ItemEntry, EFortPickupSourceTypeFlag::Player,
        EFortPickupSpawnSource::Unset, _this->MyFortPawn, Count, true, true, true, nullptr, FinalLoc);
    if (Item->ItemEntry.Count <= 0 || Count < 0)
        Remove(_this->WorldInventory, ItemGuid);
    else
    {
        SetToDirty(_this->WorldInventory, Item->ItemEntry);
    }
}

void (*OnPawnDiedOG)(AFortPlayerControllerAthena* PlayerController, AFortPlayerPawnAthena* KilledPawn, double Damage, FGameplayTagContainer* InTags,
    const FGameplayEffectContextHandle* EffectContext, AController* EventInstigator, AActor* DamageCauser, AController* DBNOFinisher);
void OnPawnDied(AFortPlayerControllerAthena* PlayerController, AFortPlayerPawnAthena* KilledPawn, double Damage, FGameplayTagContainer* InTags,
    const FGameplayEffectContextHandle* EffectContext, AController* EventInstigator, AActor* DamageCauser, AController* DBNOFinisher)
{
    auto DeathLocation = KilledPawn ? KilledPawn->K2_GetActorLocation() :
                        (PlayerController->Pawn ? PlayerController->Pawn->K2_GetActorLocation() : FVector());

    OnPawnDiedOG(PlayerController, KilledPawn, Damage, InTags, EffectContext, EventInstigator, DamageCauser, DBNOFinisher);

    auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;

    auto KillerPlayerController = EventInstigator->Cast<AFortPlayerControllerAthena>();
    auto KillerPlayerState = KillerPlayerController ? (AFortPlayerStateAthena*) KillerPlayerController->PlayerState : nullptr;
    auto KillerPawn = KillerPlayerController ? (AFortPlayerPawnAthena*) KillerPlayerController->Pawn : nullptr;

    auto GameMode = (AFortGameModeAthena*)GetWorld()->AuthorityGameMode;
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;

    PlayerState->PawnDeathLocation = DeathLocation;

    memset(&PlayerState->DeathInfo, 0, sizeof(FDeathInfo));

    PlayerState->DeathInfo.bDBNO = KilledPawn ? KilledPawn->IsDBNO() : false;
    PlayerState->DeathInfo.DeathLocation = PlayerState->PawnDeathLocation;
    PlayerState->DeathInfo.DeathTags
        = /**InTags*/ KilledPawn ? *(FGameplayTagContainer*)(__int64(&KilledPawn->MoveSoundStimulusBroadcastInterval) + 0x10)
        : FGameplayTagContainer();
    PlayerState->DeathInfo.DeathClassSlot = -1;
    PlayerState->DeathInfo.DeathCause = AFortPlayerStateAthena::ToDeathCause(PlayerState->DeathInfo.DeathTags, PlayerState->DeathInfo.bDBNO);
    // PlayerState->DeathInfo.Downer = KillerPlayerState;
    PlayerState->DeathInfo.FinisherOrDowner = KillerPlayerState ? KillerPlayerState : PlayerState;
    PlayerState->DeathInfo.FinisherOrDownerTags = KillerPawn ? KillerPawn->GameplayTags : KilledPawn->GameplayTags;
    PlayerState->DeathInfo.VictimTags = KilledPawn->GameplayTags;
    PlayerState->DeathInfo.Distance
        = KilledPawn ? (PlayerState->DeathInfo.DeathCause != EDeathCause::FallDamage ? (KillerPawn ? KillerPawn->GetDistanceTo(KilledPawn) : 0) : KilledPawn->LastFallDistance) : 0;
    PlayerState->DeathInfo.DeathServerTime = (float)UGameplayStatics::GetTimeSeconds(GameState);
    PlayerState->DeathInfo.bInitialized = true;

    PlayerState->OnRep_DeathInfo();

    if (KillerPlayerState && KillerPawn && KillerPawn->Controller && KillerPawn->Controller != PlayerController)
    {
        KillerPlayerState->KillScore++;
        KillerPlayerState->OnRep_Kills();
        KillerPlayerState->TeamKillScore++;
        KillerPlayerState->OnRep_TeamKillScore();

        KillerPlayerState->ClientReportKill(PlayerState);
        KillerPlayerState->ClientReportTeamKill(KillerPlayerState->TeamKillScore);

        auto KillerAccountId = UFortKismetLibrary::GetDebugStringForUniqueId(KillerPlayerState->UniqueID);
        //printf("KillerAccountId: %ls\n", KillerAccountId.CStr());

        if (KillerAccountId.Num() > 0)
        {
            auto KillURL = std::wstring(L"http://86.48.25.30/rmx/server/api/v1/kill/") + KillerAccountId.CStr();
            auto req = CreateRequest();
            req->SetURL(KillURL.c_str());
            req->SetHeader(L"Content-Type", L"application/json");
            req->SetHeader(L"X-Api-Key", L"Nd47IaHAFKsyMFlheEDppjc0Qi2Tu23N");
            req->SetVerb(L"POST");
            req->ProcessRequest();
        }
    }

    auto bReload = GameState->CurrentPlaylistInfo.BasePlaylist->GameType == EFortGameType::BlastBerry;
    
    bool bRespawnAllowed = false;
    
    if (bReload)
    {
        auto BlastBerryManager = (UFortGameStateComponent_BlastBerryManager*)GameState->GetComponentByClass(UFortGameStateComponent_BlastBerryManager::StaticClass());

        bRespawnAllowed = BlastBerryManager->AreRespawnsActive() && BlastBerryManager->GetTeamLivesRemaining(PlayerState->TeamIndex) > 1;
    }
    else
        bRespawnAllowed = GameState->IsRespawningAllowed(PlayerState);

    if (KillerPawn && KillerPlayerState && KillerPawn->Controller != PlayerController)
    {
        if (bReload)
        {
            // very proper bro
            SpawnPickup(PlayerState->PawnDeathLocation, UFortKismetLibrary::K2_GetResourceItemDefinition(EFortResourceType::Wood), 50, 0, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination, KilledPawn);
            SpawnPickup(PlayerState->PawnDeathLocation, UFortKismetLibrary::K2_GetResourceItemDefinition(EFortResourceType::Stone), 50, 0, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination, KilledPawn);
            SpawnPickup(PlayerState->PawnDeathLocation, UFortKismetLibrary::K2_GetResourceItemDefinition(EFortResourceType::Metal), 50, 0, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination, KilledPawn);

            SpawnPickup(PlayerState->PawnDeathLocation, FindObject<UFortItemDefinition>(L"/PaprikaConsumables/Gameplay/ShieldSmallRefresh/WID_Paprika_ShieldSmall.WID_Paprika_ShieldSmall"), 2, 0, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination, KilledPawn);
        }

        if (SiphonAmount > 0)
        {
            auto Health = KillerPawn->GetHealth();
            auto Shield = KillerPawn->GetShield();

            const float Siphon = (float)SiphonAmount;
            const float MaxHealth = 100.f;
            const float MaxShield = 100.f;

            float Amount = Siphon < MaxHealth - Health ? Siphon : MaxHealth - Health;
            float Overflow = Siphon - Amount;

            KillerPawn->SetHealth(Health + Amount);

            if (Overflow > 0.f)
            {
                float NewShield = Shield + Overflow < MaxShield ? Shield + Overflow : MaxShield;
                KillerPawn->SetShield(NewShield);
            }

            if (auto ASC = KillerPawn->AbilitySystemComponent)
            {
                auto Handle = ASC->MakeEffectContext();
                FGameplayTag CueTag(FName(L"GameplayCue.Shield.PotionConsumed"));
                FPredictionKey PredKey;
                memset(&PredKey, 0, sizeof(FPredictionKey));
                ASC->NetMulticast_InvokeGameplayCueAdded(CueTag, PredKey, Handle);
                ASC->NetMulticast_InvokeGameplayCueExecuted(CueTag, PredKey, Handle);
            }
        }
    }


    if (!bRespawnAllowed && (KilledPawn ? !KilledPawn->IsDBNO() : true))
    {
        PlayerState->Place = GameState->PlayersLeft;
        PlayerState->OnRep_Place();

        AFortWeapon* DamageCauserWeapon = nullptr;

        if (DamageCauser ? DamageCauser->IsA<AFortProjectileBase>() : false)
        {
            auto Owner = DamageCauser->Owner;

            if (Owner)
            {
                if (Owner->Cast<AFortWeapon>())
                    DamageCauserWeapon = (AFortWeapon*)Owner;
                else if (auto Controller = Owner->Cast<AFortPlayerControllerAthena>())
                {
                    if (Controller->MyFortPawn)
                        DamageCauserWeapon = (AFortWeapon*)Controller->MyFortPawn->CurrentWeapon;
                }
                else if (auto Pawn = Owner->Cast<AFortPlayerPawnAthena>())
                    DamageCauserWeapon = (AFortWeapon*)Pawn->CurrentWeapon;
            }
        }
        else if (auto Weapon = DamageCauser ? DamageCauser->Cast<AFortWeapon>() : nullptr)
            DamageCauserWeapon = Weapon;

        auto RemoveFromAlivePlayers
            = (void (*)(AFortGameMode*, AFortPlayerControllerAthena*, AFortPlayerStateAthena*, AFortPlayerPawnAthena*, UFortItemDefinition*, EDeathCause, char))(ImageBase + 0x918B8D4);
        RemoveFromAlivePlayers(GameMode, PlayerController, KillerPlayerState == PlayerState ? nullptr : KillerPlayerState, KillerPawn,
            (DamageCauserWeapon && DamageCauserWeapon->IsA<AFortWeapon>()) ? DamageCauserWeapon->WeaponData : nullptr, PlayerState->DeathInfo.DeathCause, 0);

        if (PlayerController->Pawn && KillerPlayerState && KillerPlayerState != PlayerState && KillerPlayerState->Place == 1)
        {
            auto KillerAccountId = UFortKismetLibrary::GetDebugStringForUniqueId(KillerPlayerState->UniqueID);
            // printf("KillerAccountId: %ls\n", KillerAccountId.CStr());

            if (KillerAccountId.Num() > 0)
            {
                auto KillURL = std::wstring(L"http://86.48.25.30/rmx/server/api/v1/win/") + KillerAccountId.CStr();
                auto req = CreateRequest();
                req->SetURL(KillURL.c_str());
                req->SetHeader(L"Content-Type", L"application/json");
                req->SetVerb(L"POST");
                req->ProcessRequest();
            }
            /*if (PlayerState->Place == 1)
            {
                KillerPlayerState = PlayerState;
                KillerPawn = (AFortPlayerPawnAthena*)PlayerController->Pawn;
            }*/

            auto KillerWeapon = DamageCauserWeapon ? DamageCauserWeapon->WeaponData : nullptr;

            KillerPlayerController->PlayWinEffects(KillerPawn, KillerWeapon, PlayerState->DeathInfo.DeathCause, false, ERoundVictoryAnimation::Default);
            KillerPlayerController->ClientNotifyWon(KillerPawn, KillerWeapon, PlayerState->DeathInfo.DeathCause);
            KillerPlayerController->ClientNotifyTeamWon(KillerPawn, KillerWeapon, PlayerState->DeathInfo.DeathCause);

            if (KillerPlayerState != PlayerState)
            {
                auto Crown = FindObject<UFortItemDefinition>(L"/VictoryCrownsGameplay/Items/AGID_VictoryCrown.AGID_VictoryCrown");

                TArray<FFortItemEntryStateValue> StateValues {};
                FFortItemEntryStateValue Value;

                Value.IntValue = 1;
                Value.StateType = EFortItemEntryState::ShouldShowItemToast;
                StateValues.Add(Value);

                GiveItem(KillerPlayerController->WorldInventory, Crown, 1, 0, 0, true, 0, StateValues);
                StateValues.Free();
            }

            GameState->WinningTeam = KillerPlayerState->TeamIndex;
            GameState->OnRep_WinningTeam();
            GameState->WinningPlayerState = KillerPlayerState;
            GameState->OnRep_WinningPlayerState();
        }
    }

    if (GameMode->AlivePlayers.Num() == 0)
    {
    }

    /*if (PlayerController->Pawn && KillerPlayerState && KillerPlayerState->AbilitySystemComponent && KillerPawn
        && KillerPawn->Controller != PlayerController)
    {
        auto Handle = KillerPlayerState->AbilitySystemComponent->MakeEffectContext();
        FGameplayTag Tag;
        static auto Cue = FName(L"GameplayCue.Shield.PotionConsumed");
        Tag.TagName = Cue;
        auto PredictionKey = (FPredictionKey*)malloc(FPredictionKey::Size());
        memset((PBYTE)PredictionKey, 0, FPredictionKey::Size());
        KillerPlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(Tag, *PredictionKey, Handle);
        KillerPlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(Tag, *PredictionKey, Handle);
        free(PredictionKey);

        auto Health = KillerPawn->GetHealth();
        auto Shield = KillerPawn->GetShield();

        if (Health == 100)
        {
            Shield += Shield + FConfiguration::SiphonAmount;
        }
        else if (Health + FConfiguration::SiphonAmount > 100)
        {
            Health = 100;
            Shield += (Health + FConfiguration::SiphonAmount) - 100;
        }
        else if (Health + FConfiguration::SiphonAmount <= 100)
        {
            Health += FConfiguration::SiphonAmount;
        }

        KillerPawn->SetHealth(Health);
        KillerPawn->SetShield(Shield);
        // forgot to add this back
    }*/
}

void ServerPlayEmoteItem(AFortPlayerControllerAthena* _this, UFortMontageItemDefinitionBase* EmoteAsset, float EmoteRandomNumber)
{
    if (!_this || !_this->MyFortPawn || !EmoteAsset->Cast<UFortMontageItemDefinitionBase>())
        return;

    _this->MyFortPawn->EmoteRandomNum = EmoteRandomNumber;

    auto AbilitySystemComponent = ((AFortPlayerStateAthena*)_this->PlayerState)->AbilitySystemComponent;

    if (auto CharacterVehicle = _this->Pawn->Cast<AFortCharacterVehicle>())
        AbilitySystemComponent = CharacterVehicle->OverrideAbilitySystemComponent;

    UClass* AbilityToUse = nullptr;

    if (EmoteAsset->IsA(UAthenaSprayItemDefinition::StaticClass()))
    {
        static auto SprayAbilityClass = FindObject<UClass>(L"/Game/Abilities/Sprays/GAB_Spray_Generic.GAB_Spray_Generic_C");
        AbilityToUse = SprayAbilityClass;
    }
    else if (auto ToyAsset = EmoteAsset->Cast<UAthenaToyItemDefinition>())
    {
        AbilityToUse = ToyAsset->ToySpawnAbility.Get();
    }
    else if (auto DanceAsset = EmoteAsset->Cast<UAthenaDanceItemDefinition>())
    {
        _this->MyFortPawn->bMovingEmote = DanceAsset->bMovingEmote;
        _this->MyFortPawn->EmoteWalkSpeed = DanceAsset->WalkForwardSpeed;
        _this->MyFortPawn->bMovingEmoteForwardOnly = DanceAsset->bMoveForwardOnly;
        _this->MyFortPawn->bMovingEmoteFollowingOnly = DanceAsset->bMoveFollowingOnly;

        auto CustomAbility = DanceAsset->CustomDanceAbility.Get();

        if (CustomAbility)
            AbilityToUse = CustomAbility;
        else
        {
            static auto EmoteAbilityClass = FindObject<UClass>(L"/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C");
            AbilityToUse = EmoteAbilityClass;
        }
    }

    if (AbilityToUse)
    {
        static auto GiveAbilityAndActivateOnce = (void (*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec*, void*))(ImageBase + 0x7CBA17C);
        static auto ConstructAbilitySpec = (void (*)(FGameplayAbilitySpec*, UClass**, int, int, UObject*))(ImageBase + 0x7CF6F98);

        FGameplayAbilitySpec Spec;
        ConstructAbilitySpec(&Spec, &AbilityToUse, 1, -1, EmoteAsset);

        FGameplayAbilitySpecHandle handle;
        GiveAbilityAndActivateOnce(AbilitySystemComponent, &handle, &Spec, nullptr);

        auto OldEmote = _this->MyFortPawn->LastReplicatedEmoteExecuted;
        _this->MyFortPawn->LastReplicatedEmoteExecuted = EmoteAsset;
        _this->MyFortPawn->OnRep_LastReplicatedEmoteExecuted(OldEmote);
    }
}

void DropItemsOnPawnDestruction(AFortPlayerControllerAthena* _this, uint8_t DestructionReason, const FGameplayTagContainer* ContextTags, AFortPlayerPawnAthena* DestructionPawn)
{
    if (bKeepInventory && !bLateGame)
        return;

    if (DestructionPawn && _this->WorldInventory.ObjectPointer && DestructionPawn->bShouldDropItemsOnDeath)
    {
        UEAllocatedVector<FGuid> GuidsToRemove;
        for (auto& Entry : ((AFortInventory*)_this->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries)
        {
            auto PickupComponent = GetPickupComponent((UFortItemDefinition*)Entry.ItemDefinition);
            if (PickupComponent && PickupComponent->bCanBeDroppedFromInventory)
            {
                SpawnPickup(DestructionPawn->K2_GetActorLocation(), Entry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination, DestructionPawn);

                GuidsToRemove.push_back(Entry.ItemGuid);
            }
        }

        for (auto& Guid : GuidsToRemove)
            Remove(_this->WorldInventory, Guid);
    }
}

void ServerSpectateActor(AFortPlayerControllerAthena* _this, class AActor* NewViewTarget, bool bAllowStateChange)
{
    static auto SpectatingName = FName(L"Spectating");

    if (_this->StateName != SpectatingName && !bAllowStateChange)
    {
        printf("[PlayerControllerModule] Spectating is not allowed as we cannot change state.\n");
        return;
    }

    static auto CanSpectateActor = (bool (*)(AFortPlayerControllerAthena*, AActor*))(ImageBase + 0x91F920C);

    if (!NewViewTarget || !CanSpectateActor(_this, NewViewTarget))
    {
        printf("[PlayerControllerModule] Spectating is not allowed as CanSpectateActor returned false.\n");
        return;
    }

    static auto ShouldSuicideOnSpectate = (bool (*)(AFortPlayerController*, AActor*))(ImageBase + 0x922D66C);

    if (ShouldSuicideOnSpectate(_this, NewViewTarget))
    {
        printf("[PlayerControllerModule] Trying to spectate while still alive, suiciding!\n");
        _this->ServerSuicide();
    }

    static auto BeginSpectating = (void (*)(APlayerState*, APlayerState*))(ImageBase + 0xA2C9730);

    if (auto Pawn = NewViewTarget->Cast<APawn>())
    {
        auto PlayerState = Pawn->PlayerState->Cast<AFortPlayerStateZone>();

        if (PlayerState && UFortRuntimeOptions::GetDefaultObj()->bEnableSpectatorUpdates)
        {
            //printf("BeginSpectating\n");
            BeginSpectating(_this->PlayerState, PlayerState);
        }
        else if (auto TargetPawn = NewViewTarget->Cast<AFortPlayerPawn>())
        {
            if (TargetPawn->CurrentVehicle)
            {
                // auto VehicleController = TargetPawn->Controller; // should be an interface call, but im lazy.
                auto PlayerState = TargetPawn->PlayerState->Cast<AFortPlayerStateZone>();

                if (TargetPawn->CurrentVehicle->IsA<AFortPlayerPawnAthena>())
                    NewViewTarget = TargetPawn->CurrentVehicle;

                if (PlayerState && UFortRuntimeOptions::GetDefaultObj()->bEnableSpectatorUpdates)
                    BeginSpectating(_this->PlayerState, PlayerState);
            }
        }
    }

    auto& ChangeState = (void(*&)(AController*, FName))_this->VTable[0xED];

    ChangeState(_this, SpectatingName);
    _this->ClientGotoState(SpectatingName);

    auto SetViewTargetAndNotifyClient = (void (*)(AFortPlayerControllerAthena*, AActor*, FViewTargetTransitionParams&))(ImageBase + 0x9229B74);

    FViewTargetTransitionParams Params {};
    Params.BlendTime = 0.f;
    Params.BlendFunction = EViewTargetBlendFunction::VTBlend_Cubic;
    Params.BlendExp = 2.f;
    Params.bLockOutgoing = false;

    SetViewTargetAndNotifyClient(_this, NewViewTarget, Params);
    //_this->ClientSetViewTarget(NewViewTarget, )
}

void ServerSetMultiProductCosmeticLoadout(AFortPlayerControllerAthena* _this, FCosmeticLoadout& Loadout, TArray<FCosmeticLoadoutActiveArchetype>& Archetypes)
{
    auto CosmeticLoadout = (UFortControllerComponent_CosmeticLoadout*)_this->GetComponentByClass(UFortControllerComponent_CosmeticLoadout::StaticClass());

    auto SetCosmeticLoadoutController = (void (*)(UFortControllerComponent_CosmeticLoadout*, FCosmeticLoadout&))(ImageBase + 0x9672F80);

    SetCosmeticLoadoutController(CosmeticLoadout, Loadout);

    if (_this->MyFortPawn)
        _this->MyFortPawn->CosmeticLoadoutComponent->SetCosmeticLoadout(Loadout);

    CosmeticLoadout->ActiveArchetypes.Clear();

    for (auto& Archetype : Archetypes)
        CosmeticLoadout->ActiveArchetypes.Add(Archetype);

    CosmeticLoadout->OnRep_ActiveArchetypes();
}

void ServerSetCosmeticLoadout(AFortPlayerControllerAthena* _this, struct FFortAthenaLoadout& Loadout, bool bRefreshPawn)
{
    _this->CosmeticLoadoutPC = Loadout;
}

void ServerCheat(AFortPlayerController* _this, FString Msg)
{
    auto fullCommand = Msg.ToString();

    std::vector<UEAllocatedString> args;

    size_t pos = 0, lastPos = 0;
    while ((pos = fullCommand.find(' ', lastPos)) != std::string::npos)
    {
        args.push_back(fullCommand.substr(lastPos, pos - lastPos));

        lastPos = pos + 1;
    }

    args.push_back(fullCommand.substr(lastPos));

    if (args.size())
    {
        auto& command = args[0];
        std::transform(command.begin(), command.end(), command.begin(), tolower);

        switch (hash(command.c_str()))
        {
        case hash("startaircraft"):
        {
            auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(_this);

            StartAircraftPhase(GamePhaseLogic);
            break;
        }
        case hash("infiniteammo"):
            _this->bInfiniteAmmo ^= 1;
            break;
        case hash("dc"):
            _this->CheatManager->ToggleDebugCamera();
            break;
        case hash("buildfree"):
            _this->bBuildFree ^= 1;
            break;
        case hash("speed"):
        {
            float Speed = 1.0f;

            if (args.size() > 1)
            {
                try
                {
                    Speed = std::stof(std::string(args[1]));
                }
                catch (...)
                {
                    break;
                }
            }

            _this->MyFortPawn->SetMovementSpeedMultiplier(Speed);
            break;
        }
        case hash("startevent"):
        {
            TArray<AActor*> MeshActors;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpecialEventScriptMeshActor::StaticClass(), &MeshActors);

            TArray<AActor*> EventScripts;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpecialEventScript::StaticClass(), &EventScripts);

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

            break;
        }
        case hash("giveitem"):
        {
            if (args.size() != 2 && args.size() != 3)
                return;

            auto ItemDefinition = FindObject<UFortItemDefinition>(UEAllocatedWString(args[1].begin(), args[1].end()));
            if (!ItemDefinition)
                ItemDefinition = (UFortItemDefinition*)TUObjectArray::FindObject(args[1].c_str(), UFortItemDefinition::StaticClass());

            if (!ItemDefinition)
                return;

            int32 Count = 1;

            if (args.size() == 3)
            {
                Count = strtol(args[2].c_str(), nullptr, 10);
                if (Count <= 0)
                    Count = 1;
            }

            auto Pawn = _this->MyFortPawn;

            if (!Pawn)
                return;

            FVector FinalLoc = Pawn ? Pawn->K2_GetActorLocation() : FVector();

            FVector ForwardVector = Pawn ? Pawn->GetActorForwardVector() : FVector();
            ForwardVector.Z = 0.0f;
            // ForwardVector.Normalize();

            FinalLoc = VecAdd(FinalLoc, VecMul(ForwardVector, 450.f));
            FinalLoc.Z += 50.f;

            const float RandomAngleVariation = ((float)rand() * 0.00109866634f) - 18.f;
            const float FinalAngle = RandomAngleVariation * 0.017453292519943295f;

            FinalLoc.X += cos(FinalAngle) * 100.f;
            FinalLoc.Y += sin(FinalAngle) * 100.f;

            auto Pickup = SpawnPickup(FinalLoc, ItemDefinition, Count, 0, EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::Unset, Pawn);

            Pawn->ServerHandlePickup(Pickup, Pickup->PickupLocationData.FlyTime, FVector(), true);
            // PlayerController->WorldInventory->GiveItem(ItemDefinition, Count);
        }
        }
    }
}

void ForceEquipValidWeapon(AFortPlayerControllerAthena* _this)
{
    auto LastItem = (AFortWeapon*)_this->MyFortPawn->PreviousWeapon;

    if (LastItem)
    {
        _this->ServerExecuteInventoryItem(LastItem->ItemEntryGuid);
        _this->ClientEquipItem(LastItem->ItemEntryGuid, true);
    }
    else
    {
        auto pickaxeEntry = ((AFortInventory*)_this->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries.Search(
            [](FFortItemEntry& entry)
            {
                return entry.ItemDefinition->IsA<UFortWeaponMeleeItemDefinition>();
            });

        if (pickaxeEntry)
        {
            _this->ServerExecuteInventoryItem(pickaxeEntry->ItemGuid);
            _this->ClientEquipItem(pickaxeEntry->ItemGuid, true);
        }
    }
}

void PlayerController__Init()
{
    Hook<AFortPlayerControllerAthena>(uint32(0x135), ServerAcknowledgePossession);
    Hook(ImageBase + 0x1C9AC68, GetPlayerViewPoint, GetPlayerViewPointOG);
    Hook<AFortPlayerControllerAthena>(uint32(0x232), ServerExecuteInventoryItem);

    Hook<AFortPlayerControllerAthena>(uint32(0x253), ServerCreateBuildingActor);
    Hook<AFortPlayerControllerAthena>(uint32(0x25A), ServerBeginEditingBuildingActor);
    Hook<AFortPlayerControllerAthena>(uint32(0x255), ServerEditBuildingActor);
    Hook<AFortPlayerControllerAthena>(uint32(0x258), ServerEndEditingBuildingActor);

    Hook<AFortPlayerControllerAthena>(uint32(0x12D), AFortPlayerControllerZone::GetDefaultObj()->VTable[0x12D]); // ServerRestartPlayer

    Hook<AFortPlayerControllerAthena>(uint32(0x247), ServerOnMaterialSelection);

    Hook<AFortPlayerControllerAthena>(uint32(0x23D), ServerAttemptInventoryDrop);

    Hook(ImageBase + 0x921754C, OnPawnDied, OnPawnDiedOG);

    Hook<AFortPlayerControllerAthena>(uint32(0x1FD), ServerPlayEmoteItem);

    Hook<AFortPlayerControllerAthena>(uint32(0x39F), DropItemsOnPawnDestruction);
    Hook<AFortPlayerControllerAthena>(uint32(0x48C), ServerSpectateActor);

    Hook<AFortPlayerControllerAthena>(uint32(0x28C), AFortPlayerControllerZone::GetDefaultObj()->VTable[0x28C]); // ServerSuicide
    Hook<AFortPlayerControllerAthena>(uint32(0x4CE), ServerSetMultiProductCosmeticLoadout);
    //Hook<AFortPlayerControllerAthena>(uint32(0x4CF), ServerSetCosmeticLoadout); // not used?
    Patch<uint8_t>(ImageBase + 0x96774B4, 0xC3); // fixes sum with mcp loadout?
    
    if (!bProd)
        Hook<AFortPlayerControllerAthena>(uint32(0x1FB), ServerCheat);

    Hook<AFortPlayerControllerAthena>(uint32(0x283), AFortPlayerControllerZone::GetDefaultObj()->VTable[0x283]); // ServerReturnToMainMenu
    Hook<AFortPlayerControllerAthena>(uint32(0x2F6), ForceEquipValidWeapon);
}
    