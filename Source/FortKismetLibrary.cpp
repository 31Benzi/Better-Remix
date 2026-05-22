#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
#include "Pickups.h"
#include "Looting.h"
INIT_MODULE(FortKismetLibrary);

void (*OpenActorOG)(AActor* OpenableInterfaceActor, AFortPlayerControllerAthena* OptionalControllerInstigator, bool bRequestFastOpen);
void OpenActor(AActor* OpenableInterfaceActor, AFortPlayerControllerAthena* OptionalControllerInstigator, bool bRequestFastOpen)
{
    static auto SetIsDoorOpen = (void (*)(AActor*, uint8_t, APawn*))(ImageBase + 0x955C49C);
    if (OpenableInterfaceActor->IsA<ABuildingWall>())
        SetIsDoorOpen(OpenableInterfaceActor, bRequestFastOpen ? 1 : 0, OptionalControllerInstigator ? OptionalControllerInstigator->Pawn : nullptr);
    else
        return OpenActorOG(OpenableInterfaceActor, OptionalControllerInstigator, bRequestFastOpen);
}

void (*CloseActorOG)(UObject* Context, FFrame& Stack, bool* Ret);
void CloseActor(UObject* Context, FFrame& Stack, bool* Ret)
{
    AActor* OpenableInterfaceActor;
    AFortPlayerControllerAthena* OptionalControllerInstigator;

    Stack.StepCompiledIn(&OpenableInterfaceActor);
    Stack.StepCompiledIn(&OptionalControllerInstigator);
    Stack.IncrementCode();

    static auto SetIsDoorOpen = (void (*)(AActor*, uint8_t, APawn*))(ImageBase + 0x955C49C);
    if (OpenableInterfaceActor->IsA<ABuildingWall>())
        SetIsDoorOpen(OpenableInterfaceActor, 3, OptionalControllerInstigator ? OptionalControllerInstigator->Pawn : nullptr);
    else
        return callOG(UFortKismetLibrary::GetDefaultObj(), Stack.CurrentNativeFunction, CloseActor, OpenableInterfaceActor, OptionalControllerInstigator);
}

void K2_SpawnPickupInWorld(UObject* Object, FFrame& Stack, AFortPickupAthena** Ret)
{
    class UObject* WorldContextObject;
    class UFortItemDefinition* ItemDefinition;
    int32 NumberToSpawn;
    FVector Position;
    FVector Direction;
    int32 OverrideMaxStackCount;
    bool bToss;
    bool bRandomRotation;
    bool bBlockedFromAutoPickup;
    int32 PickupInstigatorHandle;
    EFortPickupSourceTypeFlag SourceType;
    EFortPickupSpawnSource Source;
    class AFortPlayerControllerAthena* OptionalOwnerPC;
    bool bPickupOnlyRelevantToOwner = false;

    Stack.StepCompiledIn(&WorldContextObject);
    Stack.StepCompiledIn(&ItemDefinition);
    Stack.StepCompiledIn(&NumberToSpawn);
    Stack.StepCompiledIn(&Position);
    Stack.StepCompiledIn(&Direction);
    Stack.StepCompiledIn(&OverrideMaxStackCount);
    Stack.StepCompiledIn(&bToss);
    Stack.StepCompiledIn(&bRandomRotation);
    Stack.StepCompiledIn(&bBlockedFromAutoPickup);
    Stack.StepCompiledIn(&PickupInstigatorHandle);
    Stack.StepCompiledIn(&SourceType);
    Stack.StepCompiledIn(&Source);
    Stack.StepCompiledIn(&OptionalOwnerPC);
    Stack.StepCompiledIn(&bPickupOnlyRelevantToOwner);
    Stack.IncrementCode();
    
    *Ret = SpawnPickup(
        Position, ItemDefinition, NumberToSpawn, 0, SourceType, Source, OptionalOwnerPC ? OptionalOwnerPC->MyFortPawn : nullptr, bToss, bRandomRotation);
}

void SpawnItemVariantPickupInWorld(UObject* Object, FFrame& Stack, AFortPickupAthena** Ret)
{
    UObject* WorldContextObject;
    FSpawnItemVariantParams Params;

    Stack.StepCompiledIn(&WorldContextObject);
    Stack.StepCompiledIn(&Params);
    Stack.IncrementCode();

    *Ret = SpawnPickup(Params.Position, Params.WorldItemDefinition, Params.NumberToSpawn, 0, Params.SourceType, Params.Source,
        (AFortPlayerPawn*)Params.OptionalPawnWhoDroppedPickup, Params.bToss, Params.bRandomRotation);
}

void K2_SpawnPickupInWorldWithClassAndItemEntry(UObject* Context, FFrame& Stack, AFortPickupAthena** Ret)
{
    UObject* WorldContextObject;
    FFortItemEntry Entry;
    UClass* PickupClass;
    FVector Position;
    FVector Direction;
    int32 OverrideMaxStackCount;
    bool bToss;
    bool bRandomRotation;
    bool bBlockedFromAutoPickup;
    EFortPickupSourceTypeFlag SourceType;
    EFortPickupSpawnSource Source;
    class AFortPlayerControllerAthena* OptionalOwnerPC;
    bool bPickupOnlyRelevantToOwner;

    Stack.StepCompiledIn(&WorldContextObject);
    Stack.StepCompiledIn(&Entry);
    Stack.StepCompiledIn(&PickupClass);
    Stack.StepCompiledIn(&Position);
    Stack.StepCompiledIn(&Direction);
    Stack.StepCompiledIn(&OverrideMaxStackCount);
    Stack.StepCompiledIn(&bToss);
    Stack.StepCompiledIn(&bRandomRotation);
    Stack.StepCompiledIn(&bBlockedFromAutoPickup);
    Stack.StepCompiledIn(&SourceType);
    Stack.StepCompiledIn(&Source);
    Stack.StepCompiledIn(&OptionalOwnerPC);
    Stack.StepCompiledIn(&bPickupOnlyRelevantToOwner);
    Stack.IncrementCode();

    *Ret = SpawnPickup(
        Position, Entry, SourceType, Source, OptionalOwnerPC ? OptionalOwnerPC->MyFortPawn : nullptr, bToss, bRandomRotation);
}

void GiveItemToInventoryOwner(UObject* Object, FFrame& Stack)
{
    TScriptInterface<class IFortInventoryOwnerInterface> InventoryOwner;
    UFortItemDefinition* ItemDefinition;
    FGuid ItemVariantGuid;
    int32 NumberToGive;
    bool bNotifyPlayer;
    int32 ItemLevel = -1;
    int32 PickupInstigatorHandle = 0;
    bool bUseItemPickupAnalyticEvent = false;
    int32 WeaponAmmoOverride = -1;
    Stack.StepCompiledIn(&InventoryOwner);
    Stack.StepCompiledIn(&ItemDefinition);
    Stack.StepCompiledIn(&ItemVariantGuid);
    Stack.StepCompiledIn(&NumberToGive);
    Stack.StepCompiledIn(&bNotifyPlayer);
    Stack.StepCompiledIn(&ItemLevel);
    Stack.StepCompiledIn(&PickupInstigatorHandle);
    Stack.StepCompiledIn(&bUseItemPickupAnalyticEvent);
    Stack.StepCompiledIn(&WeaponAmmoOverride);
    Stack.IncrementCode();

    auto Controller = (AController*)InventoryOwner.ObjectPointer;
    auto ItemEntry = MakeItemEntry(ItemDefinition, NumberToGive, ItemLevel);
    if (WeaponAmmoOverride != -1)
        ItemEntry.LoadedAmmo = WeaponAmmoOverride;

    TScriptInterface<UFortInventoryInterface> InventoryInterface;

    if (auto PlayerController = Controller->Cast<AFortPlayerController>())
        InventoryInterface = PlayerController->WorldInventory;
    else if (auto BotController = Controller->Cast<AFortAthenaAIBotController>())
        InventoryInterface = BotController->Inventory;

    InternalPickupLogic(InventoryInterface, (AFortPlayerPawnAthena*)Controller->Pawn, ItemEntry);
}

void K2_RemoveItemFromPlayer(UObject* Context, FFrame& Stack, int32* Ret)
{
    AFortPlayerControllerAthena* PlayerController;
    UFortItemDefinition* ItemDefinition;
    FGuid ItemVariantGuid {};
    int32 AmountToRemove;
    bool bForceRemoval = false;

    Stack.StepCompiledIn(&PlayerController);
    Stack.StepCompiledIn(&ItemDefinition);
    Stack.StepCompiledIn(&ItemVariantGuid);
    Stack.StepCompiledIn(&AmountToRemove);
    Stack.StepCompiledIn(&bForceRemoval);
    Stack.IncrementCode();

    if (!PlayerController)
    {
        *Ret = 0;
        return;
    }

    auto Inventory = (AFortInventory*)PlayerController->WorldInventory.ObjectPointer;
    auto ItemP = Inventory->Inventory.ItemInstances.Search(
        [&](UFortWorldItem* entry)
        {
            return entry->ItemEntry.ItemDefinition == ItemDefinition;
        });
    auto itemEntry = Inventory->Inventory.ReplicatedEntries.Search(
        [&](FFortItemEntry& entry)
        {
            return entry.ItemDefinition == ItemDefinition;
        });
    if (!ItemP)
    {
        *Ret = 0;
        return;
    }
    auto Item = *ItemP;

    auto RemoveCount = max(AmountToRemove, 0);
    itemEntry->Count -= RemoveCount;

    if (AmountToRemove < 0 || itemEntry->Count <= 0)
    {
        RemoveCount += itemEntry->Count;
        Remove(PlayerController->WorldInventory, itemEntry->ItemGuid);
    }
    else
    {
        Item->ItemEntry.Count = itemEntry->Count;
        SetToDirty(PlayerController->WorldInventory, *itemEntry);
        Item->ItemEntry.bIsDirty = true;
    }

    *Ret = RemoveCount;
}

void K2_RemoveItemFromPlayerByGuid(UObject* Context, FFrame& Stack, int32* Ret)
{
    class AFortPlayerControllerAthena* PlayerController;
    struct FGuid ItemGuid;
    int32 AmountToRemove;
    bool bForceRemoval;
    Stack.StepCompiledIn(&PlayerController);
    Stack.StepCompiledIn(&ItemGuid);
    Stack.StepCompiledIn(&AmountToRemove);
    Stack.StepCompiledIn(&bForceRemoval);
    Stack.IncrementCode();

    auto Inventory = (AFortInventory*)PlayerController->WorldInventory.ObjectPointer;

    auto Item = GetItem(PlayerController->WorldInventory, ItemGuid);

    if (!Item)
    {
        *Ret = 0;
        return;
    }
    
    auto itemEntry = GetReplicatedItemEntry(Inventory, ItemGuid);

    auto RemoveCount = max(AmountToRemove, 0);
    itemEntry->Count -= RemoveCount;

    if (AmountToRemove < 0 || itemEntry->Count <= 0)
    {
        RemoveCount += itemEntry->Count;
        Remove(PlayerController->WorldInventory, itemEntry->ItemGuid);
    }
    else
    {
        Item->ItemEntry.Count = itemEntry->Count;
        SetToDirty(PlayerController->WorldInventory, *itemEntry);
        Item->ItemEntry.bIsDirty = true;
    }

    *Ret = RemoveCount;
    return;
}

void PickLootDrops(UObject* Object, FFrame& Stack, bool* Ret)
{
    UObject* WorldContextObject;
    FName TierGroupName;
    int32 WorldLevel;
    int32 ForcedLootTier;
    FGameplayTagContainer OptionalLootTags {};

    Stack.StepCompiledIn(&WorldContextObject);
    auto& OutLootToDrop = Stack.StepCompiledInRef<TArray<FFortItemEntry>>();
    Stack.StepCompiledIn(&TierGroupName);
    Stack.StepCompiledIn(&WorldLevel);
    Stack.StepCompiledIn(&ForcedLootTier);
    Stack.StepCompiledIn(&OptionalLootTags);
    Stack.IncrementCode();

    ChooseLootForContainer(OutLootToDrop, TierGroupName, ForcedLootTier, WorldLevel);

    *Ret = OutLootToDrop.Num() > 0;
}

void (*ServerOnAttemptInteractOG)(UFortInteractInterface* _this, FInteractionType* InteractType);
void ServerOnAttemptInteract(UFortInteractInterface* _this, FInteractionType* InteractType)
{
    auto Consumable = (ABuildingGameplayActorConsumable*)(__int64(_this) - 0x2A8);

    if (InteractType->InteractionBeingAttempted == EInteractionBeingAttempted::FirstInteraction)
    {
        auto Pawn = InteractType->RequestingPawn.Get();

        if (Pawn && Pawn->AbilitySystemComponent && Consumable->OnConsumeGameplayEffect)
        {
            auto Handle = FGameplayEffectContextHandle();
            Pawn->AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(Consumable->OnConsumeGameplayEffect, 1.f, Handle);
        }
    }

    return ServerOnAttemptInteractOG(_this, InteractType);
}

void FortKismetLibrary__Init()
{
    Hook(ImageBase + 0x94F0474, ServerOnAttemptInteract, ServerOnAttemptInteractOG);

    Hook(ImageBase + 0x8E47384, OpenActor, OpenActorOG);
    //Hook(ImageBase + 0x09AB8AAC, CloseActor, CloseActorOG);
    ExecHook(UFortKismetLibrary::StaticClass()->FindFunction("CloseActor"), CloseActor, CloseActorOG);

    ExecHook(UFortKismetLibrary::StaticClass()->FindFunction("K2_SpawnPickupInWorld"), K2_SpawnPickupInWorld);
    ExecHook(UFortKismetLibrary::StaticClass()->FindFunction("SpawnItemVariantPickupInWorld"), SpawnItemVariantPickupInWorld);
    ExecHook(UFortKismetLibrary::StaticClass()->FindFunction("K2_SpawnPickupInWorldWithClassAndItemEntry"), K2_SpawnPickupInWorldWithClassAndItemEntry);

    ExecHook(UFortKismetLibrary::StaticClass()->FindFunction("GiveItemToInventoryOwner"), GiveItemToInventoryOwner);

    ExecHook(UFortKismetLibrary::StaticClass()->FindFunction("K2_RemoveItemFromPlayer"), K2_RemoveItemFromPlayer);
    ExecHook(UFortKismetLibrary::StaticClass()->FindFunction("K2_RemoveItemFromPlayerByGuid"), K2_RemoveItemFromPlayerByGuid);

    ExecHook(UFortKismetLibrary::StaticClass()->FindFunction("PickLootDrops"), PickLootDrops);
}