#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
INIT_MODULE(FortVehicleSeatWeaponComponent);

void (*EquipVehicleWeaponOG)(UFortVehicleSeatWeaponComponent* _this, AFortPawn* FortPawn, FWeaponSeatDefinition* WeaponSeatDefinition);
void EquipVehicleWeapon(UFortVehicleSeatWeaponComponent* _this, AFortPawn* FortPawn, FWeaponSeatDefinition* WeaponSeatDefinition)
{
    if (!WeaponSeatDefinition->VehicleWeapon)
        return;

    auto& VehicleGrantedWeaponItem = *(TWeakObjectPtr<UFortWorldItem>*)(__int64(WeaponSeatDefinition) + 0x34);
    auto& VehicleGrantedWeapon = *(TWeakObjectPtr<AFortWeapon>*)(__int64(WeaponSeatDefinition) + 0x3C);
    AFortPlayerController* PlayerController = nullptr;

    if (FortPawn->Controller->IsA<AFortPlayerController>())
        PlayerController = (AFortPlayerController*)FortPawn->Controller;
    else if (auto CharacterVehicle = _this->GetOwner()->Cast<AFortCharacterVehicle>())
        PlayerController = (AFortPlayerController*)CharacterVehicle->Controller;

    if (!PlayerController)
        return;

    auto Item = GiveItem(PlayerController->WorldInventory, WeaponSeatDefinition->VehicleWeapon, 1, GetStats(WeaponSeatDefinition->VehicleWeapon)->ClipSize);
    VehicleGrantedWeaponItem = Item;

    PlayerController->ServerExecuteInventoryItem(Item->ItemEntry.ItemGuid);
    PlayerController->ClientEquipItem(Item->ItemEntry.ItemGuid, true);

    auto EquippedWeapon = FortPawn->CurrentWeapon->Cast<AFortWeaponRangedForVehicle>();
    auto EquippedDualWeapon = FortPawn->CurrentWeapon->Cast<AFortWeaponRangedDualForVehicle>();

    if (EquippedWeapon || EquippedDualWeapon)
        VehicleGrantedWeapon = FortPawn->CurrentWeapon;

    FMountedWeaponInfoRepped MountedWeaponInfo {};
    MountedWeaponInfo.HostVehicleCached.ObjectPointer = _this->GetVehicle();
    MountedWeaponInfo.HostVehicleCached.InterfacePointer = GetInterfaceAddress<UFortVehicleInterface>(MountedWeaponInfo.HostVehicleCached.ObjectPointer);
    MountedWeaponInfo.HostVehicleSeatIndexCached = WeaponSeatDefinition->SeatIndex;

    WeaponSeatDefinition->LastEquippedVehicleWeapon = WeaponSeatDefinition->VehicleWeapon;

    if (EquippedWeapon)
    {
        MountedWeaponInfo.DriverCameraLocationCached = EquippedWeapon->MountedWeaponInfoRepped.DriverCameraLocationCached;
        MountedWeaponInfo.DriverCameraDirectionCached = EquippedWeapon->MountedWeaponInfoRepped.DriverCameraDirectionCached;
        EquippedWeapon->MountedWeaponInfoRepped = MountedWeaponInfo;
        EquippedWeapon->OnRep_MountedWeaponInfoRepped();
    }
    else if (EquippedDualWeapon)
    {
        MountedWeaponInfo.DriverCameraLocationCached = EquippedDualWeapon->MountedWeaponInfoRepped.DriverCameraLocationCached;
        MountedWeaponInfo.DriverCameraDirectionCached = EquippedDualWeapon->MountedWeaponInfoRepped.DriverCameraDirectionCached;
        EquippedDualWeapon->MountedWeaponInfoRepped = MountedWeaponInfo;
        EquippedDualWeapon->OnRep_MountedWeaponInfoRepped();
    }

    EquipVehicleWeaponOG(_this, FortPawn, WeaponSeatDefinition);
}

void FortVehicleSeatWeaponComponent__Init()
{
    Hook(ImageBase + 0xA610714, EquipVehicleWeapon, EquipVehicleWeaponOG);
}