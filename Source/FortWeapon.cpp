#include "pch.h"
#include "Shared.h"
INIT_MODULE(FortWeapon);

void RemoveWeaponAbilities(AFortWeapon* Weapon)
{
    static auto ClearAbility = (void (*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle&))(ImageBase + 0x7CA8DBC);
    auto Controller = ((AFortPlayerPawnAthena*)Weapon->Instigator)->Controller;

    if (!Controller)
        return;

    auto AbilitySystemComponent = ((AFortPlayerStateAthena*)Controller->PlayerState)->AbilitySystemComponent;

    if (Weapon->PrimaryAbilitySpecHandle.Handle != -1)
    {
        ClearAbility(AbilitySystemComponent, Weapon->PrimaryAbilitySpecHandle);
        Weapon->PrimaryAbilitySpecHandle.Handle = -1;
    }
    if (Weapon->SecondaryAbilitySpecHandle.Handle != -1)
    {
        ClearAbility(AbilitySystemComponent, Weapon->SecondaryAbilitySpecHandle);
        Weapon->SecondaryAbilitySpecHandle.Handle = -1;
    }
    if (Weapon->ReloadAbilitySpecHandle.Handle != -1)
    {
        ClearAbility(AbilitySystemComponent, Weapon->ReloadAbilitySpecHandle);
        Weapon->ReloadAbilitySpecHandle.Handle = -1;
    }
    if (Weapon->ImpactAbilitySpecHandle.Handle != -1)
    {
        ClearAbility(AbilitySystemComponent, Weapon->ImpactAbilitySpecHandle);
        Weapon->ImpactAbilitySpecHandle.Handle = -1;
    }
    if (Weapon->EquippedAbilityHandles.Num())
    {
        for (auto& Handle : Weapon->EquippedAbilityHandles)
        {
            if (Handle.Handle != -1)
            {
                ClearAbility(AbilitySystemComponent, Handle);
            }
        }
        Weapon->EquippedAbilityHandles.ResetNum();
    }

    /*if (Weapon->WeaponData->EquippedAbilitySet)
    {
        bool bRemoved = false;
        for (auto& [Key, Value] : *(TMap<FGuid, FFortAbilitySetHandle>*)& PlayerController->AppliedInGameModifierAbilitySetHandles)
            if (Key == Weapon->ItemEntryGuid)
            {
                UFortKismetLibrary::UnequipFortAbilitySet(Value);
                bRemoved = true;
                break;
            }

        if (bRemoved)
            PlayerController->ClientRemoveItemAbilitySet(Weapon->WeaponData->EquippedAbilitySet, Weapon->ItemEntryGuid);
    }*/
    if (Weapon->EquippedAbilitySetHandles.Num())
    {
        for (auto& Handle : Weapon->EquippedAbilitySetHandles)
        {
            UFortKismetLibrary::UnequipFortAbilitySet(Handle);
        }

        auto BotController = Controller->Cast<AFortAthenaAIBotController>();
        if (BotController)
            BotController->AppliedInGameModifierAbilitySetHandles.Reset();
        else
            ((AFortPlayerControllerAthena*)Controller)->AppliedInGameModifierAbilitySetHandles.Reset();

        Weapon->EquippedAbilitySetHandles.ResetNum();
    }
}

void (*OnUnEquipOG)(AFortWeapon* _this);
void OnUnEquip(AFortWeapon* _this)
{
    RemoveWeaponAbilities(_this);

    return OnUnEquipOG(_this);
}

void FortWeapon__Init()
{
    Hook(ImageBase + 0xA68A44C, OnUnEquip, OnUnEquipOG);
}