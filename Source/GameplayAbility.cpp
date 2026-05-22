#include "pch.h"
#include "Shared.h"
INIT_MODULE(GameplayAbility);

void (*K2_AddGameplayCueWithParamsOG)(UObject* Context, FFrame& Stack);
void K2_AddGameplayCueWithParams(UObject* Context, FFrame& Stack)
{
    auto& GameplayCueTag = Stack.StepCompiledInRef<FGameplayTag>();
    auto& GameplayCueParameter = Stack.StepCompiledInRef<FGameplayCueParameters>();
    bool bRemoveOnAbilityEnd;

    Stack.StepCompiledIn(&bRemoveOnAbilityEnd);
    Stack.IncrementCode();

    auto Ability = (UFortGameplayAbility*)Context;
    callOG(Ability, Stack.CurrentNativeFunction, K2_AddGameplayCueWithParams, GameplayCueTag, GameplayCueParameter, bRemoveOnAbilityEnd);

    FPredictionKey PredictionKey {};

    auto AbilitySystemComponent = (UAbilitySystemComponent*)Ability->GetAbilitySystemComponentFromActorInfo();

    if (AbilitySystemComponent)
        AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded_WithParams(GameplayCueTag, PredictionKey, GameplayCueParameter);
}

void (*K2_AddGameplayCueOG)(UObject* Context, FFrame& Stack);
void K2_AddGameplayCue(UObject* Context, FFrame& Stack)
{
    FGameplayTag GameplayCueTag;
    FGameplayEffectContextHandle EffectContext;
    bool bRemoveOnAbilityEnd;

    Stack.StepCompiledIn(&GameplayCueTag);
    Stack.StepCompiledIn(&EffectContext);
    Stack.StepCompiledIn(&bRemoveOnAbilityEnd);
    Stack.IncrementCode();

    auto Ability = (UFortGameplayAbility*)Context;
    callOG(Ability, Stack.CurrentNativeFunction, K2_AddGameplayCue, GameplayCueTag, EffectContext, bRemoveOnAbilityEnd);

    FPredictionKey PredictionKey {};

    auto AbilitySystemComponent = (UAbilitySystemComponent*)Ability->GetAbilitySystemComponentFromActorInfo();

    if (AbilitySystemComponent)
        AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(GameplayCueTag, PredictionKey, EffectContext);
}

void (*K2_ExecuteGameplayCueOG)(UObject* Context, FFrame& Stack);
void K2_ExecuteGameplayCue(UObject* Context, FFrame& Stack)
{
    auto& GameplayCueTag = Stack.StepCompiledInRef<FGameplayTag>();
    auto& EffectContext = Stack.StepCompiledInRef<FGameplayEffectContextHandle>();
    Stack.IncrementCode();

    auto Ability = (UFortGameplayAbility*)Context;
    callOG(Ability, Stack.CurrentNativeFunction, K2_ExecuteGameplayCue, GameplayCueTag, EffectContext);

    FPredictionKey PredictionKey {};

    auto AbilitySystemComponent = (UAbilitySystemComponent*)Ability->GetAbilitySystemComponentFromActorInfo();

    if (AbilitySystemComponent)
        AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(GameplayCueTag, PredictionKey, EffectContext);
}

void (*K2_ExecuteGameplayCueWithParamsOG)(UObject* Context, FFrame& Stack);
void K2_ExecuteGameplayCueWithParams(UObject* Context, FFrame& Stack)
{
    auto& GameplayCueTag = Stack.StepCompiledInRef<FGameplayTag>();
    auto& GameplayCueParameter = Stack.StepCompiledInRef<FGameplayCueParameters>();
    Stack.IncrementCode();

    auto Ability = (UFortGameplayAbility*)Context;
    callOG(Ability, Stack.CurrentNativeFunction, K2_ExecuteGameplayCueWithParams, GameplayCueTag, GameplayCueParameter);

    FPredictionKey PredictionKey {};

    auto AbilitySystemComponent = (UAbilitySystemComponent*)Ability->GetAbilitySystemComponentFromActorInfo();

    if (AbilitySystemComponent)
        AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted_WithParams(GameplayCueTag, PredictionKey, GameplayCueParameter);
}

void GameplayAbility__Init()
{
    ExecHook(UGameplayAbility::StaticClass()->FindFunction("K2_ExecuteGameplayCue"), K2_ExecuteGameplayCue, K2_ExecuteGameplayCueOG);
    ExecHook(UGameplayAbility::StaticClass()->FindFunction("K2_ExecuteGameplayCueWithParams"), K2_ExecuteGameplayCueWithParams, K2_ExecuteGameplayCueWithParamsOG);
    //ExecHook(UGameplayAbility::StaticClass()->FindFunction("K2_AddGameplayCue"), K2_AddGameplayCue, K2_AddGameplayCueOG);
    //ExecHook(UGameplayAbility::StaticClass()->FindFunction("K2_AddGameplayCueWithParams"), K2_AddGameplayCueWithParams, K2_AddGameplayCueWithParamsOG);
}