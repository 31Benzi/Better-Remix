#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
INIT_MODULE(Pickups);

bool HasTrait(UFortItemDefinition* ItemDefinition, FName TraitName)
{
    static auto GetTraits = (FItemComponentData_Traits * (*)(UFortItemDefinition*))(ImageBase + 0x21FB6AC);

    auto Traits = GetTraits(ItemDefinition);

    for (auto& Trait : Traits->Traits.GameplayTags)
        if (Trait.TagName == TraitName)
            return true;

    return false;
}

bool AllowMultipleStacks(UFortItemDefinition* ItemDefinition)
{
    return !HasTrait(ItemDefinition, L"Item.Trait.SingleStack");
}

FFortItemComponentData_Pickup* GetPickupComponent(UFortItemDefinition* ItemDefinition)
{
    static auto GetPickupComponent_ = (FFortItemComponentData_Pickup * (*)(UFortItemDefinition*))(ImageBase + 0x1BC59A8);

    return GetPickupComponent_(ItemDefinition);
}

uint8 GetNumberOfSlotsToTake(UFortItemDefinition* ItemDefinition)
{
    static auto GetSlotSize = (FItemComponentData_SlotSize* (*)(UFortItemDefinition*))(ImageBase + 0x8338174);

    return GetSlotSize(ItemDefinition)->SlotSize;
}

void InternalPickupLogic(TScriptInterface<UFortInventoryInterface>& InventoryInterface, AFortPlayerPawnAthena* Pawn, FFortItemEntry& PickupEntry)
{
    auto Inventory = (AFortInventory*)InventoryInterface.ObjectPointer;
    auto PickupDefinition = (UFortWorldItemDefinition*)PickupEntry.ItemDefinition;
    auto PickupComponent = GetPickupComponent(PickupDefinition);

    auto MaxStack = (int32)PickupDefinition->GetMaxStackSize(Pawn->AbilitySystemComponent);
    int ItemCount = 0;

    if (GetNumberOfSlotsToTake(PickupDefinition) > 0 && IsPrimaryQuickbar(PickupDefinition))
        for (auto& Item : Inventory->Inventory.ReplicatedEntries)
        {
            if (IsPrimaryQuickbar((UFortItemDefinition*)Item.ItemDefinition))
                ItemCount += GetNumberOfSlotsToTake((UFortItemDefinition*)Item.ItemDefinition);
        }

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

    auto GiveOrSwap = [&]()
    {
        if (ItemCount >= 5 && IsPrimaryQuickbar(PickupDefinition) && (!PickupComponent || PickupComponent->bDropCurrentItemOnOverflow))
        {
            if (!Pawn || !Pawn->CurrentWeapon)
                return;

            if (IsPrimaryQuickbar(Pawn->CurrentWeapon->WeaponData))
            {
                auto itemEntry = GetReplicatedItemEntry(Inventory, Pawn->CurrentWeapon->ItemEntryGuid);

                if (itemEntry && itemEntry->Count > 0)
                {
                    auto Loc = VecAdd(VecAdd(Pawn->K2_GetActorLocation(), VecMul(Pawn->GetActorForwardVector(), 70.f)), FVector(0, 0, 50));
                    SpawnPickup(Loc, *itemEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Pawn, -1, true, true, true, nullptr, FinalLoc);
                }
                else
                {
                    auto Loc = VecAdd(VecAdd(Pawn->K2_GetActorLocation(), VecMul(Pawn->GetActorForwardVector(), 70.f)), FVector(0, 0, 50));
                    SpawnPickup(Loc, PickupEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Pawn, -1, true, true, true, nullptr, FinalLoc);
                    return;
                }

                Remove(InventoryInterface, Pawn->CurrentWeapon->ItemEntryGuid);
                auto Item = GiveItem(InventoryInterface, PickupEntry, PickupEntry.Count, true);

                if (Item)
                {
                    if (auto PlayerController = Pawn->Controller->Cast<AFortPlayerController>())
                    {
                        PlayerController->ServerExecuteInventoryItem(Item->ItemEntry.ItemGuid);
                        PlayerController->ClientEquipItem(Item->ItemEntry.ItemGuid, true);
                    }
                }
            }
            else
            {
                auto Loc = VecAdd(VecAdd(Pawn->K2_GetActorLocation(), VecMul(Pawn->GetActorForwardVector(), 70.f)), FVector(0, 0, 50));
                SpawnPickup(
                    Loc, PickupEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Pawn, -1, true, true, true, nullptr, FinalLoc);
                return;
            }
        }
        else
        {
            GiveItem(InventoryInterface, PickupEntry, PickupEntry.Count, true);
        }
    };

    auto GiveOrSwapStack = [&](int32 OriginalCount)
    {
        if (AllowMultipleStacks(PickupDefinition) /*PickupEntry->ItemDefinition->bAllowMultipleStacks*/ && (ItemCount < 5 || (PickupComponent && PickupComponent->bDropCurrentItemOnOverflow)))
        {
            GiveItem(InventoryInterface, PickupEntry, OriginalCount - MaxStack, true);
        }
        else
        {
            auto Loc = VecAdd(VecAdd(Pawn->K2_GetActorLocation(), VecMul(Pawn->GetActorForwardVector(), 70.f)), FVector(0, 0, 50));
            SpawnPickup(Loc, PickupEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Pawn, OriginalCount - MaxStack, true, true,
                true, nullptr, FinalLoc);
        }
    };

    if (MaxStack > 1)
    {
        auto item = Inventory->Inventory.ItemInstances.Search(
            [PickupEntry, MaxStack](UFortWorldItem* entry)
            {
                return entry->ItemEntry.ItemDefinition == PickupEntry.ItemDefinition && entry->ItemEntry.Count < MaxStack;
            });
        auto itemEntry = Inventory->Inventory.ReplicatedEntries.Search(
            [PickupEntry, MaxStack](FFortItemEntry& entry)
            {
                return entry.ItemDefinition == PickupEntry.ItemDefinition && entry.Count < MaxStack;
            });

        if (item && *item)
        {
            bool bFound = false;
            /*for (int i = 0; i < itemEntry->StateValues.Num(); i++)
            {
                auto& StateValue = itemEntry->StateValues.Get(i, FFortItemEntryStateValue::Size());

                if (StateValue.StateType != 2)
                    continue;

                bFound = true;
                StateValue.IntValue = 0;
                break;
            }*/

            auto TheRealOriginalCount = itemEntry->Count;
            if ((itemEntry->Count += PickupEntry.Count) > MaxStack)
            {
                auto OriginalCount = itemEntry->Count;
                itemEntry->Count = MaxStack;

                GiveOrSwapStack(OriginalCount);
            }

            // full proper
            for (auto& StateValue : itemEntry->StateValues)
            {
                if (StateValue.StateType != EFortItemEntryState::ShouldShowItemToast)
                    continue;

                StateValue.IntValue = 1;
                bFound = true;
                break;
            }

            if (!bFound)
            {
                FFortItemEntryStateValue Value {};

                Value.IntValue = 1;
                Value.StateType = EFortItemEntryState::ShouldShowItemToast;

                itemEntry->StateValues.Add(Value);
            }

            auto Gained = itemEntry->Count - TheRealOriginalCount;

            (*item)->ItemEntry.Count = itemEntry->Count;
            SetToDirty(InventoryInterface, *itemEntry);
        }
        else
        {
            auto itemEntry2 = Inventory->Inventory.ReplicatedEntries.Search(
                [PickupEntry, MaxStack](FFortItemEntry& entry)
                {
                    return entry.ItemDefinition == PickupEntry.ItemDefinition && entry.Count >= MaxStack;
                });

            if (!itemEntry && itemEntry2 && !AllowMultipleStacks(PickupDefinition) /*&& !PickupEntry.ItemDefinition->bAllowMultipleStacks*/)
            {
                auto Loc = VecAdd(VecAdd(Pawn->K2_GetActorLocation(), VecMul(Pawn->GetActorForwardVector(), 70.f)), FVector(0, 0, 50));
                SpawnPickup(Loc, PickupEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, Pawn, PickupEntry.Count, true, true, true,
                    nullptr, FinalLoc);
                return;
            }

            if (PickupEntry.Count > MaxStack)
            {
                auto OriginalCount = PickupEntry.Count;
                PickupEntry.Count = MaxStack;

                GiveOrSwapStack(OriginalCount);
            }

            GiveOrSwap();
        }
    }
    else
        GiveOrSwap();
}

void (*GivePickupToOG)(AFortPickup* _this, UFortInventoryOwnerInterface* OwnerInterface, bool bDestroyAfterPickup);
void GivePickupTo(AFortPickup* _this, UFortInventoryOwnerInterface* OwnerInterface, bool bDestroyAfterPickup)
{
    auto Interface__GetOwner = (UObject * (*)(UInterface*)) OwnerInterface->VTable[0x1];
    auto InterfaceOwner = (AController*)Interface__GetOwner(OwnerInterface);
    auto& PickupEntry = _this->PrimaryPickupItemEntry;

    auto Pawn = (AFortPlayerPawnAthena*)InterfaceOwner->Pawn;
    TScriptInterface<UFortInventoryInterface> InventoryInterface;

    if (auto PlayerController = InterfaceOwner->Cast<AFortPlayerController>())
        InventoryInterface = PlayerController->WorldInventory;
    else if (auto BotController = InterfaceOwner->Cast<AFortAthenaAIBotController>())
        InventoryInterface = BotController->Inventory;
    
    if (!InventoryInterface.ObjectPointer)
        return GivePickupToOG(_this, OwnerInterface, bDestroyAfterPickup);

    InternalPickupLogic(InventoryInterface, Pawn, PickupEntry);

    return GivePickupToOG(_this, OwnerInterface, bDestroyAfterPickup);
}

void (*TryToAutoPickupOG)(AFortPlayerPawn* _this, AFortPickup* Pickup);
void TryToAutoPickup(AFortPlayerPawn* _this, AFortPickup* Pickup)
{
    auto Controller = _this->Controller;

    if (Pickup && Pickup->PawnWhoDroppedPickup != _this)
    {
        AFortInventory* Inventory = nullptr;

        if (auto PlayerController = Controller->Cast<AFortPlayerController>())
            Inventory = (AFortInventory*)PlayerController->WorldInventory.ObjectPointer;
        else if (auto BotController = Controller->Cast<AFortAthenaAIBotController>())
            Inventory = (AFortInventory*)BotController->Inventory.ObjectPointer;

        if (!Inventory)
            return TryToAutoPickupOG(_this, Pickup);

        auto PickupDefinition = (UFortItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition;
        auto MaxStack = PickupDefinition->GetMaxStackSize(nullptr);
        auto itemEntry = Inventory->Inventory.ReplicatedEntries.Search(
            [&](FFortItemEntry& entry)
            {
                return entry.ItemDefinition == PickupDefinition && entry.Count <= MaxStack;
            });

        if ((!itemEntry && !IsPrimaryQuickbar(PickupDefinition))
            || (itemEntry && itemEntry->Count < MaxStack))
        {
            _this->ServerHandlePickup(Pickup, Pickup->PickupLocationData.FlyTime, FVector(), true);
            return;
        }
    }

    return TryToAutoPickupOG(_this, Pickup);
}

void Pickups__Init()
{
    Hook<AFortPickupAthena>(uint32(0xEB), GivePickupTo, GivePickupToOG);
    Hook(ImageBase + 0xA1A5418, TryToAutoPickup, TryToAutoPickupOG);
}