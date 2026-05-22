#include "pch.h"
#include "Shared.h"
#include "Config.h"
INIT_MODULE(PlayerPawn);

void ServerHandlePickupInfo(AFortPlayerPawnAthena* _this, AFortPickup* Pickup, FFortPickupRequestInfo Params)
{
    if (!Pickup || !_this || !Pickup->IsA<AFortPickup>())
        return;

    static auto SetPickupTarget = (void (*)(AFortPickup*, AFortPlayerPawnAthena*, float, FVector&, bool))(ImageBase + 0x9CDB4C8);

    SetPickupTarget(Pickup, _this, Params.FlyTime / _this->PickupSpeedMultiplier, Params.Direction, Params.bPlayPickupSound);
}

void ServerSendZiplineState(AFortPlayerPawnAthena* _this, FZiplinePawnState& State)
{
    auto Zipline = _this->GetActiveZipline();

    _this->ZiplineState = State;

    static auto HandleZiplineStateChanged = (void (*)(AFortPlayerPawn*))(ImageBase + 0xA190360);

    if (!State.bReachedEnd && State.bJumped)
    {
        static auto ForceLaunchPlayerZiplining = (void (*)(AFortPlayerPawn*))(ImageBase + 0xA18612C);

        ForceLaunchPlayerZiplining(_this);
    }

    HandleZiplineStateChanged(_this);
    //_this->OnRep_ZiplineState(OldState);
}

void (*EmoteStoppedOG)(AFortPlayerPawnAthena* _this, UFortItemDefinition* MontageItemDef);
void EmoteStopped(AFortPlayerPawnAthena* _this, UFortItemDefinition* MontageItemDef)
{
    auto OldEmote = _this->LastReplicatedEmoteExecuted;
    _this->LastReplicatedEmoteExecuted = nullptr;
    _this->OnRep_LastReplicatedEmoteExecuted(OldEmote);

    return EmoteStoppedOG(_this, MontageItemDef);
}

void MovingEmoteStopped(UObject* Context, FFrame& Stack)
{
    Stack.IncrementCode();
    auto Pawn = (AFortPlayerPawnAthena*)Context;

    if (Pawn->bIsPlayingEmote)
        return;

    Pawn->bMovingEmote = false;
    Pawn->bMovingEmoteForwardOnly = false;
    Pawn->bMovingEmoteFollowingOnly = false;

    auto OldEmote = Pawn->LastReplicatedEmoteExecuted;
    Pawn->LastReplicatedEmoteExecuted = nullptr;
    Pawn->OnRep_LastReplicatedEmoteExecuted(OldEmote);
}

// andew
void ServerReviveFromDBNO(AFortPlayerPawnAthena* Pawn, AController* EventInstigator)
{
    if (!Pawn || !EventInstigator || !EventInstigator->IsA<AController>())
        return;

    auto PlayerState = Pawn->PlayerState->Cast<AFortPlayerStateAthena>();
    if (!PlayerState)
        return;

    if (PlayerState->Owner == EventInstigator)
        return;

    auto UpdateDBNOInteractCollision = (void (*)(AFortPlayerPawn*))(ImageBase + 0x3215E94);
    UpdateDBNOInteractCollision(Pawn);

    Pawn->DBNORevivingActorsCount = 0;

    PlayerState->DeathInfo.FinisherOrDowner = nullptr;
    PlayerState->DeathInfo.Downer = nullptr;
    PlayerState->DeathInfo.bDBNO = false;
    PlayerState->DeathInfo.DeathCause = EDeathCause::Unspecified;
    PlayerState->DeathInfo.DeathClassSlot = -1;
    PlayerState->DeathInfo.Distance = 0;
    PlayerState->DeathInfo.DeathLocation = FVector(0, 0, 0);
    PlayerState->DeathInfo.bInitialized = false;
    PlayerState->DeathInfo.DeathTags.GameplayTags.Clear();

    PlayerState->DeathInfo.DeathTags.ParentTags.Clear();

    PlayerState->OnRep_DeathInfo();
    static auto ForceForgetEnemy = (void (*)(AActor*))(ImageBase + 0xA77C940);
    ForceForgetEnemy(Pawn);

    FGameplayEventData EventData {};
    EventData.EventTag = Pawn->EventReviveTag;
    EventData.ContextHandle = PlayerState->AbilitySystemComponent->MakeEffectContext();
    EventData.Instigator = EventInstigator;
    EventData.InstigatorTags = EventInstigator ? ((AFortPlayerPawnAthena*)EventInstigator->Pawn)->GameplayTags : FGameplayTagContainer();
    EventData.Target = Pawn;
    EventData.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Pawn);
    EventData.TargetTags = Pawn->GameplayTags;
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn, Pawn->EventReviveTag, EventData);

    FGameplayTagContainer ReviveTags {};
    AddTag(ReviveTags, FGameplayTag(L"Gameplay.Action.Player.DBNO"));
    AddTag(ReviveTags, FGameplayTag(L"Gameplay.Action.Player.DBNOAthena"));

    PlayerState->AbilitySystemComponent->BP_CancelAbilitiesWithTags(ReviveTags);

    
    float ReviveHealth = EvaluateScalableFloat(Pawn->SetByCallerReviveHealth);

    if (ReviveHealth > 0.0f)
        Pawn->SetHealth(ReviveHealth);

    Pawn->bIsDBNO = false;
    Pawn->OnRep_IsDBNO();
    Pawn->bPlayedDying = false;
    Pawn->bIsDying = false;

    AFortPlayerControllerAthena* Controller = Pawn->Controller->Cast<AFortPlayerControllerAthena>();
    if (Controller)
    {
        static auto OnPawnRevived = (void (*)(AFortPlayerControllerZone*, AFortPlayerControllerZone*))(ImageBase + 0xA2A1D70);

        OnPawnRevived(Controller, (AFortPlayerControllerZone*)EventInstigator);
    }
}

void PlayGroupEmote(UObject* Context, FFrame& Stack)
{
    UFortMontageItemDefinitionBase* EmoteAsset;

    Stack.StepCompiledIn(&EmoteAsset);
    Stack.IncrementCode();
    auto Pawn = (AFortPlayerPawn*)Context;

    if (!Pawn || !EmoteAsset || !EmoteAsset->IsA<UFortMontageItemDefinitionBase>())
        return;

    if (Pawn->Controller->IsA<AFortPlayerController>())
        ((AFortPlayerControllerAthena*)Pawn->Controller)->ServerPlayEmoteItem(EmoteAsset, 6.7f);
}

void (*ServerThrowCarriedPlayerOG)(AFortPlayerPawn* Pawn);
void ServerThrowCarriedPlayer(AFortPlayerPawn* Pawn)
{
    ServerThrowCarriedPlayerOG(Pawn);
    Pawn->LocalThrowCarriedPlayer();
}

void HoistDBNOPlayer(AFortPlayerPawn* _this, AFortPlayerPawn* InDBNOHoister, EFortDBNOCarryEvent CarryEvent)
{
    _this->DBNOHoisterData.DBNOHoister = InDBNOHoister;
    _this->DBNOHoisterData.DBNOCarryEvent = CarryEvent;
    InDBNOHoister->DBNOHoistee = _this;

    auto OnDBNOHoisterChanged = (void (*)(AFortPlayerPawn*, AFortPlayerPawn*, EFortDBNOCarryEvent))(ImageBase + 0x37B8694);

    OnDBNOHoisterChanged(_this, nullptr, CarryEvent);

    _this->ForceNetUpdate();
}

void ServerHoistDBNOPlayer(AFortPlayerPawn* Pawn, AFortPlayerPawn* InDBNOHoistee)
{
    if (!Pawn || !InDBNOHoistee->IsA<AFortPlayerPawn>())
        return;

    //printf(__FUNCTION__ " %s\n", InDBNOHoistee->Name.ToString().c_str());

    return HoistDBNOPlayer(InDBNOHoistee, Pawn, EFortDBNOCarryEvent::PickedUp);
}

void ServerInterrogateDBNOPlayer(AFortPlayerPawn* Pawn, AFortPlayerPawn* InDBNOHoistee)
{
    if (!Pawn || !InDBNOHoistee->IsA<AFortPlayerPawn>())
        return;

    //printf(__FUNCTION__ " %s\n", InDBNOHoistee->Name.ToString().c_str());

    return HoistDBNOPlayer(InDBNOHoistee, Pawn, EFortDBNOCarryEvent::Interrogating);
}


void PlayerPawn__Init()
{
    Hook<AFortPlayerPawnAthena>(uint32(0x252), ServerHandlePickupInfo);

    Hook<AFortPlayerPawnAthena>(uint32(0x261), ServerSendZiplineState);

    //Hook<AFortPlayerPawnAthena>(uint32(0x1B5), EmoteStopped, EmoteStoppedOG);
    ExecHook(AFortPlayerPawnAthena::StaticClass()->FindFunction("MovingEmoteStopped"), MovingEmoteStopped);

    //Hook<AFortPlayerPawnAthena>(uint32(0x23A), ServerReviveFromDBNO);

    ExecHook(AFortPlayerPawnAthena::StaticClass()->FindFunction("PlayGroupEmote"), PlayGroupEmote);

    Hook<AFortPlayerPawnAthena>(uint32(0x23E), ServerHoistDBNOPlayer);
    Hook<AFortPlayerPawnAthena>(uint32(0x23D), ServerInterrogateDBNOPlayer);
    Hook<AFortPlayerPawnAthena>(uint32(0x23C), ServerThrowCarriedPlayer, ServerThrowCarriedPlayerOG);
}