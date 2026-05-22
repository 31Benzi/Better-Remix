#pragma once
#include "pch.h"

void InternalPickupLogic(TScriptInterface<UFortInventoryInterface>& InventoryInterface, AFortPlayerPawnAthena* Pawn, FFortItemEntry& PickupEntry);
FFortItemComponentData_Pickup* GetPickupComponent(UFortItemDefinition* ItemDefinition);
bool HasTrait(UFortItemDefinition* ItemDefinition, FName TraitName);