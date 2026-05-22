#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
INIT_MODULE(FortDecoTool);

void SpawnDecoInternal(AFortDecoTool* _this, FVector& Location, FRotator& Rotation, ABuildingSMActor* AttachedActor, EBuildingAttachmentType InBuildingAttachmentType, bool)
{
    auto ItemDefinition = (UFortDecoItemDefinition*)_this->ItemDefinition;

    if (!ItemDefinition)
        return;

    auto& ShouldAllowServerSpawnDeco = (bool (*&)(AFortDecoTool*, FVector&, FRotator&, ABuildingSMActor*, EBuildingAttachmentType))_this->VTable[0x1A3];

    if (!ShouldAllowServerSpawnDeco(_this, Location, Rotation, AttachedActor, InBuildingAttachmentType))
        return;

    auto& SpawnDeco = (ABuildingSMActor * (*&)(AFortDecoTool*, UClass*&, FVector&, FRotator&, ABuildingSMActor*, EBuildingAttachmentType, int)) _this->VTable[0x198];

    auto BPClass = ItemDefinition->BlueprintClass.Get();
    SpawnDeco(_this, BPClass, Location, Rotation, AttachedActor, InBuildingAttachmentType, 0);

    if (ItemDefinition->bConsumeWhenPlaced)
    {
        auto PlayerController = _this->GetPlayerController();

        auto itemEntry = ((AFortInventory*)PlayerController->WorldInventory.ObjectPointer)
            ->Inventory.ReplicatedEntries.Search(
                [&](FFortItemEntry& entry)
                {
                    return entry.ItemDefinition == _this->ItemDefinition;
                });

        if (!itemEntry)
            return;

        auto item = GetItem(PlayerController->WorldInventory, itemEntry->ItemGuid);

        itemEntry->Count--;
        if (itemEntry->Count <= 0)
            Remove(PlayerController->WorldInventory, itemEntry->ItemGuid);
        else
        {
            item->ItemEntry.Count = itemEntry->Count;
            SetToDirty(PlayerController->WorldInventory, *itemEntry);
            item->ItemEntry.bIsDirty = true;
        }
    }
}

void ContextTrap_SpawnDecoInternal(
    AFortDecoTool_ContextTrap* _this, FVector& Location, FRotator& Rotation, ABuildingSMActor* AttachedActor, EBuildingAttachmentType InBuildingAttachmentType, bool)
{
    auto ItemDefinition = (UFortDecoItemDefinition*)_this->ItemDefinition;

    switch (InBuildingAttachmentType)
    {
    case EBuildingAttachmentType::ATTACH_Floor:
    case EBuildingAttachmentType::ATTACH_FloorAndStairs:
        ItemDefinition = (UFortDecoItemDefinition*)_this->ContextTrapItemDefinition->FloorTrap;
        break;
    case EBuildingAttachmentType::ATTACH_CeilingAndStairs:
    case EBuildingAttachmentType::ATTACH_Ceiling:
        ItemDefinition = (UFortDecoItemDefinition*)_this->ContextTrapItemDefinition->CeilingTrap;
        break;
    case EBuildingAttachmentType::ATTACH_Wall:
        ItemDefinition = (UFortDecoItemDefinition*)_this->ContextTrapItemDefinition->WallTrap;
        break;
    case EBuildingAttachmentType::ATTACH_StairsBothSides:
        ItemDefinition = (UFortDecoItemDefinition*)_this->ContextTrapItemDefinition->StairTrap;
        break;
    }

    if (!ItemDefinition)
        return;

    auto& ShouldAllowServerSpawnDeco = (bool (*&)(AFortDecoTool*, FVector&, FRotator&, ABuildingSMActor*, EBuildingAttachmentType))_this->VTable[0x1A3];

    if (!ShouldAllowServerSpawnDeco(_this, Location, Rotation, AttachedActor, InBuildingAttachmentType))
        return;

    auto& SpawnDeco = (ABuildingSMActor * (*&)(AFortDecoTool*, UClass*&, FVector&, FRotator&, ABuildingSMActor*, EBuildingAttachmentType, int)) _this->VTable[0x198];

    auto BPClass = ItemDefinition->BlueprintClass.Get();
    SpawnDeco(_this, BPClass, Location, Rotation, AttachedActor, InBuildingAttachmentType, 0);

    if (ItemDefinition->bConsumeWhenPlaced)
    {
        auto PlayerController = _this->GetPlayerController();

        auto itemEntry = ((AFortInventory*)PlayerController->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries.Search(
            [&](FFortItemEntry& entry)
            {
                return entry.ItemDefinition == _this->ItemDefinition;
            });

        if (!itemEntry)
            return;

        auto item = GetItem(PlayerController->WorldInventory, itemEntry->ItemGuid);

        itemEntry->Count--;
        if (itemEntry->Count <= 0)
            Remove(PlayerController->WorldInventory, itemEntry->ItemGuid);
        else
        {
            item->ItemEntry.Count = itemEntry->Count;
            SetToDirty(PlayerController->WorldInventory, *itemEntry);
            item->ItemEntry.bIsDirty = true;
        }
    }
}

void CreateBuildingAndSpawnDecoInternal(AFortDecoTool* _this, FVector_NetQuantize10 BuildingLocation, FRotator BuildingRotation, FVector_NetQuantize10 Location,
    FRotator Rotation, EBuildingAttachmentType InBuildingAttachmentType, bool bSpawnDecoOnExtraPiece, FVector BuildingExtraPieceLocation)
{
    auto PlayerController = _this->GetPlayerController();
    auto ItemDefinition = (UFortDecoItemDefinition*)_this->ItemDefinition;

    if (!ItemDefinition)
        return;

    static auto GetAutoCreateAttachmentBuildingShapes = (void (*)(UFortDecoItemDefinition*, TArray<UBuildingEditModeMetadata*>*))(ImageBase + 0x9C9B350);

    TArray<UBuildingEditModeMetadata*> AutoCreateAttachmentBuildingShapes;
    GetAutoCreateAttachmentBuildingShapes(ItemDefinition, &AutoCreateAttachmentBuildingShapes);

    static auto GetBuildingTypeFromBuildingAttachmentType = (EFortBuildingType (*)(EBuildingAttachmentType))(ImageBase + 0xA679798);

    auto BuildingType = GetBuildingTypeFromBuildingAttachmentType(InBuildingAttachmentType);

    if (BuildingType == EFortBuildingType::None)
        return;

    static auto Tag = FGameplayTag(L"Trap.ExtraPiece.Cost.Ignore");
    auto HasTag = UFortKismetLibrary::DoesItemDefinitionHaveGameplayTag(ItemDefinition, Tag);

    auto FindAvailableBuildingClassForBuildingTypeAndEditModePattern
        = (void (*)(AFortPlayerController*, UClass**, EFortBuildingType, TArray<UBuildingEditModeMetadata*>&, bool, EFortResourceType))(ImageBase + 0xA22B5C8);

    UClass* BuildingClass = nullptr;

    FindAvailableBuildingClassForBuildingTypeAndEditModePattern(
        PlayerController, &BuildingClass, BuildingType, AutoCreateAttachmentBuildingShapes, HasTag, ItemDefinition->AutoCreateAttachmentBuildingResourceType);

    AutoCreateAttachmentBuildingShapes.Free();
    if (!BuildingClass)
        return;

    FBuildingClassData BuildingClassData;
    BuildingClassData.BuildingClass = BuildingClass;
    BuildingClassData.UpgradeLevel = 0;
    BuildingClassData.PreviousBuildingLevel = -1;

    UFortWorldItem* Item = nullptr;
    auto Resource = UFortKismetLibrary::K2_GetResourceItemDefinition(((ABuildingSMActor*)BuildingClass->GetDefaultObj())->ResourceType);

    static auto CanAffordToPlaceBuildableClass = (bool (*)(AFortPlayerController*, FBuildingClassData))(ImageBase + 0xA21CED8);

    if (!CanAffordToPlaceBuildableClass(PlayerController, BuildingClassData))
        return;

    auto OptAdj = EFortBuildPreviewMarkerOptionalAdjustment::None;
    TArray<ABuildingSMActor*> RemoveBuildings;

    static auto CanPlaceBuildableClassInStructuralGrid = (__int64 (*)(
        AFortPlayerController*, UClass**, FVector, FRotator, bool, TArray<ABuildingSMActor*>*, EFortBuildPreviewMarkerOptionalAdjustment*))(ImageBase + 0x91F7F14);
    if (CanPlaceBuildableClassInStructuralGrid(
            PlayerController, &BuildingClass, BuildingLocation, BuildingRotation, false, &RemoveBuildings, &OptAdj))
        return;

    for (auto& RemoveBuilding : RemoveBuildings)
        RemoveBuilding->K2_DestroyActor();
    RemoveBuildings.Free();

    // auto Building = (ABuildingSMActor*)ABuildingActor::K2_SpawnBuildingActor(_this, BuildingClass, SpawnTransform, _this, _this->Pawn, true, true);
    auto Building = SpawnActorUnfinished<ABuildingSMActor>(BuildingClass, BuildingLocation, BuildingRotation, PlayerController);

    if (!Building)
        return;

    Building->InitializeKismetSpawnedBuildingActor(Building, PlayerController, true, nullptr, false);

    Building->CurrentBuildingLevel = BuildingClassData.UpgradeLevel;
    Building->OnRep_CurrentBuildingLevel();

    Building->bPlayerPlaced = true;

    Building->Team = EFortTeam(((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex);

    Building->TeamIndex = uint8(Building->Team);

    FinishSpawnActor(Building, BuildingLocation, BuildingRotation);

    if (!PlayerController->bBuildFree)
    {
        static auto PayBuildableClassPlacementCost = (int (*)(AFortPlayerController*, FBuildingClassData))(ImageBase + 0xA24BD50);

        PayBuildableClassPlacementCost(PlayerController, BuildingClassData);
    }

    _this->ServerSpawnDeco(Location, Rotation, Building, InBuildingAttachmentType);
}


void ContextTrap_CreateBuildingAndSpawnDecoInternal(AFortDecoTool_ContextTrap* _this, FVector_NetQuantize10 BuildingLocation, FRotator BuildingRotation, FVector_NetQuantize10 Location, FRotator Rotation,
    EBuildingAttachmentType InBuildingAttachmentType, bool bSpawnDecoOnExtraPiece, FVector BuildingExtraPieceLocation)
{
    auto PlayerController = _this->GetPlayerController();
    auto ItemDefinition = (UFortDecoItemDefinition*)_this->ItemDefinition;

    switch (InBuildingAttachmentType)
    {
    case EBuildingAttachmentType::ATTACH_Floor:
    case EBuildingAttachmentType::ATTACH_FloorAndStairs:
        ItemDefinition = (UFortDecoItemDefinition*)_this->ContextTrapItemDefinition->FloorTrap;
        break;
    case EBuildingAttachmentType::ATTACH_CeilingAndStairs:
    case EBuildingAttachmentType::ATTACH_Ceiling:
        ItemDefinition = (UFortDecoItemDefinition*)_this->ContextTrapItemDefinition->CeilingTrap;
        break;
    case EBuildingAttachmentType::ATTACH_Wall:
        ItemDefinition = (UFortDecoItemDefinition*)_this->ContextTrapItemDefinition->WallTrap;
        break;
    case EBuildingAttachmentType::ATTACH_StairsBothSides:
        ItemDefinition = (UFortDecoItemDefinition*)_this->ContextTrapItemDefinition->StairTrap;
        break;
    }

    if (!ItemDefinition)
        return;

    static auto GetAutoCreateAttachmentBuildingShapes = (void (*)(UFortDecoItemDefinition*, TArray<UBuildingEditModeMetadata*>*))(ImageBase + 0x9C9B350);

    TArray<UBuildingEditModeMetadata*> AutoCreateAttachmentBuildingShapes;
    GetAutoCreateAttachmentBuildingShapes(ItemDefinition, &AutoCreateAttachmentBuildingShapes);

    static auto GetBuildingTypeFromBuildingAttachmentType = (EFortBuildingType (*)(EBuildingAttachmentType))(ImageBase + 0xA679798);

    auto BuildingType = GetBuildingTypeFromBuildingAttachmentType(InBuildingAttachmentType);

    if (BuildingType == EFortBuildingType::None)
        return;

    static auto Tag = FGameplayTag(L"Trap.ExtraPiece.Cost.Ignore");
    auto HasTag = UFortKismetLibrary::DoesItemDefinitionHaveGameplayTag(ItemDefinition, Tag);

    auto FindAvailableBuildingClassForBuildingTypeAndEditModePattern
        = (void (*)(AFortPlayerController*, UClass**, EFortBuildingType, TArray<UBuildingEditModeMetadata*>&, bool, EFortResourceType))(ImageBase + 0xA22B5C8);

    UClass* BuildingClass = nullptr;

    FindAvailableBuildingClassForBuildingTypeAndEditModePattern(
        PlayerController, &BuildingClass, BuildingType, AutoCreateAttachmentBuildingShapes, HasTag, ItemDefinition->AutoCreateAttachmentBuildingResourceType);

    AutoCreateAttachmentBuildingShapes.Free();
    if (!BuildingClass)
        return;

    FBuildingClassData BuildingClassData;
    BuildingClassData.BuildingClass = BuildingClass;
    BuildingClassData.UpgradeLevel = 0;
    BuildingClassData.PreviousBuildingLevel = -1;

    UFortWorldItem* Item = nullptr;
    auto Resource = UFortKismetLibrary::K2_GetResourceItemDefinition(((ABuildingSMActor*)BuildingClass->GetDefaultObj())->ResourceType);

    static auto CanAffordToPlaceBuildableClass = (bool (*)(AFortPlayerController*, FBuildingClassData))(ImageBase + 0xA21CED8);

    if (!CanAffordToPlaceBuildableClass(PlayerController, BuildingClassData))
        return;

    auto OptAdj = EFortBuildPreviewMarkerOptionalAdjustment::None;
    TArray<ABuildingSMActor*> RemoveBuildings;

    static auto CanPlaceBuildableClassInStructuralGrid
        = (__int64 (*)(AFortPlayerController*, UClass**, FVector, FRotator, bool, TArray<ABuildingSMActor*>*, EFortBuildPreviewMarkerOptionalAdjustment*))(ImageBase + 0x91F7F14);
    if (CanPlaceBuildableClassInStructuralGrid(PlayerController, &BuildingClass, BuildingLocation, BuildingRotation, false, &RemoveBuildings, &OptAdj))
        return;

    for (auto& RemoveBuilding : RemoveBuildings)
        RemoveBuilding->K2_DestroyActor();
    RemoveBuildings.Free();

    // auto Building = (ABuildingSMActor*)ABuildingActor::K2_SpawnBuildingActor(_this, BuildingClass, SpawnTransform, _this, _this->Pawn, true, true);
    auto Building = SpawnActorUnfinished<ABuildingSMActor>(BuildingClass, BuildingLocation, BuildingRotation, PlayerController);

    if (!Building)
        return;

    Building->InitializeKismetSpawnedBuildingActor(Building, PlayerController, true, nullptr, false);

    Building->CurrentBuildingLevel = BuildingClassData.UpgradeLevel;
    Building->OnRep_CurrentBuildingLevel();

    Building->bPlayerPlaced = true;

    Building->Team = EFortTeam(((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex);

    Building->TeamIndex = uint8(Building->Team);

    FinishSpawnActor(Building, BuildingLocation, BuildingRotation);

    if (!PlayerController->bBuildFree)
    {
        static auto PayBuildableClassPlacementCost = (int (*)(AFortPlayerController*, FBuildingClassData))(ImageBase + 0xA24BD50);

        PayBuildableClassPlacementCost(PlayerController, BuildingClassData);
    }

    _this->ServerSpawnDeco(Location, Rotation, Building, InBuildingAttachmentType);
}

void FortDecoTool__Init()
{
    HookEvery<AFortDecoTool>(uint32(0x19B), SpawnDecoInternal);
    Hook<AFortDecoTool_ContextTrap>(uint32(0x19B), ContextTrap_SpawnDecoInternal);
    HookEvery<AFortDecoTool>(uint32(0x19D), CreateBuildingAndSpawnDecoInternal);
    Hook<AFortDecoTool_ContextTrap>(uint32(0x19D), ContextTrap_CreateBuildingAndSpawnDecoInternal);
}
