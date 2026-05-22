#include "pch.h"
#include "Shared.h"
#include "Inventory.h"
INIT_MODULE(FortAthenaAISpawnerDataComponent);

void AIBotSkillset__OnSpawned(UFortAthenaAISpawnerDataComponent_AIBotSkillset* AIBotSkillset, AFortPlayerPawn* Pawn)
{
    auto AIBotController = (AFortAthenaAIBotController*)Pawn->Controller;
    auto Skill = EvaluateScalableFloat(AIBotSkillset->Skill);

    if (AIBotController && AIBotController->BotSkillSetClasses.Num() == 0)
    {
        TArray<UClass*> BotSkillSetClasses = AIBotSkillset->GetSkillSets();

        static auto InitializeSkills = (void (*)(AFortAthenaAIBotController*, float*, TArray<UClass*>*))(ImageBase + 0xA789FB0);

        InitializeSkills(AIBotController, &Skill, &BotSkillSetClasses);

        BotSkillSetClasses.Free();
    }
}

void (*CosmeticLoadoutOnSpawnedOG)(UFortAthenaAISpawnerDataComponent_CosmeticLoadout* CosmeticLoadout, AFortAIPawn* Pawn);
void CosmeticLoadoutOnSpawned(UFortAthenaAISpawnerDataComponent_CosmeticLoadout* CosmeticLoadout, AFortAIPawn* Pawn)
{
    CosmeticLoadoutOnSpawnedOG(CosmeticLoadout, Pawn);

    Pawn->FallbackTag = CosmeticLoadout->FallbackTag;
    ((void (*)(APlayerState*, AController*, APawn*))(ImageBase + 0xA2C86CC))(Pawn->PlayerState, Pawn->Controller, Pawn);
}

void FortAthenaAISpawnerDataComponent__Init()
{
    Hook<UFortAthenaAISpawnerDataComponent_AIBotSkillset>(uint32(0x5B), AIBotSkillset__OnSpawned);
    Hook(ImageBase + 0xA8A7A34, CosmeticLoadoutOnSpawned, CosmeticLoadoutOnSpawnedOG);
}