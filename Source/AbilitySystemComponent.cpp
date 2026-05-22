#include "pch.h"
#include "Shared.h"
INIT_MODULE(AbilitySystemComponent);

void MarkAbilitySpecDirty(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpec& Spec, bool WasAddOrRemove = false)
{
    if (!AbilitySystemComponent->bCachedIsNetSimulated)
    {
        if (!(Spec.ability && Spec.ability->NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly && !WasAddOrRemove))
        {
            AbilitySystemComponent->ActivatableAbilities.MarkItemDirty(Spec);
        }
    }
    else
    {
        AbilitySystemComponent->ActivatableAbilities.MarkArrayDirty();
    }
}

void InternalServerTryActivateAbility(
    UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool InputPressed, FPredictionKey* PredictionKey, void* TriggerEventData)
{
    auto Spec = AbilitySystemComponent->ActivatableAbilities.Items.Search(
        [&](FGameplayAbilitySpec& item)
        {
            return item.Handle.Handle == Handle.Handle;
        });

    if (!Spec)
        return AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey->Current);

    Spec->InputPressed = true;

    UFortGameplayAbility* InstancedAbility = nullptr;
    auto InternalTryActivateAbility = (bool (*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, FPredictionKey, UFortGameplayAbility**, void*, void*))(ImageBase + 0x7CBB438);

    if (!InternalTryActivateAbility(AbilitySystemComponent, Handle, *PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
    {
        AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey->Current);
        Spec->InputPressed = false;
        
        MarkAbilitySpecDirty(AbilitySystemComponent, *Spec);
    }
}

void AbilitySystemComponent__Init()
{
    HookEvery<UAbilitySystemComponent>(0x11D, InternalServerTryActivateAbility);
}