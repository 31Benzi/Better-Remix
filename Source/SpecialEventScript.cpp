#include "pch.h"
#include "Shared.h"
INIT_MODULE(SpecialEventScript);

void (*ActivatePhaseOG)(ASpecialEventScript* _this, int IndexToActivate, float a3);
void ActivatePhase(ASpecialEventScript* _this, int IndexToActivate, float a3)
{
    if (!IsValidUObject(_this, ASpecialEventScript::StaticClass()))
        return;

    if (!_this->PhaseInfoArray.IsValidIndex(IndexToActivate))
    {
        printf("[SpecialEventScript] Ignoring invalid event phase index %d (phase count: %d).\n", IndexToActivate, _this->PhaseInfoArray.Num());
        return;
    }

    /*// for some reason the 2 functions below dont handle datalayers
    // should be in UnloadLevelsAtPhaseEnd
    if (_this->ReplicatedActivePhaseIndex >= 0)
    {
        auto& OldPhaseInfo = _this->PhaseInfoArray[_this->ReplicatedActivePhaseIndex];

        for (auto& DL : OldPhaseInfo.DataLayers)
            GetWorld()->GetDataLayerManager()->SetDataLayerRuntimeState(DL.DataLayerAsset, EDataLayerRuntimeState::Unloaded, DL.bIsRecursive);
    }

    // should be in LoadLevelsAtIndex
    auto& PhaseInfo = _this->PhaseInfoArray[IndexToActivate];
    for (auto& DL : PhaseInfo.DataLayers)
        GetWorld()->GetDataLayerManager()->SetDataLayerRuntimeState(DL.DataLayerAsset, EDataLayerRuntimeState::Activated, DL.bIsRecursive);*/

    _this->ReplicatedActivePhaseIndex = IndexToActivate;

    ActivatePhaseOG(_this, IndexToActivate, a3);
}

bool StartSpecialEventSequence()
{
    auto World = GetWorld();
    if (!World)
        return false;

    TArray<AActor*> MeshActors;
    UGameplayStatics::GetAllActorsOfClass(World, ASpecialEventScriptMeshActor::StaticClass(), &MeshActors);

    TArray<AActor*> EventScripts;
    UGameplayStatics::GetAllActorsOfClass(World, ASpecialEventScript::StaticClass(), &EventScripts);

    bool bStarted = false;
    if (MeshActors.Num() > 0 && EventScripts.Num() > 0)
    {
        auto MeshActor = (ASpecialEventScriptMeshActor*)MeshActors[0];
        auto Script = (ASpecialEventScript*)EventScripts[0];

        if (IsValidUObject(MeshActor, ASpecialEventScriptMeshActor::StaticClass()) && IsValidUObject(Script, ASpecialEventScript::StaticClass()))
        {
            Script->DelayAfterConentLoad.Curve.CurveTable = nullptr;
            Script->DelayAfterConentLoad.Value = 0.6f;

            int StartingIndex = Script->GetStartingIndex();
            if (StartingIndex < 0 && Script->PhaseInfoArray.Num() > 0)
            {
                printf("[SpecialEventScript] Forcing invalid starting phase %d to 0 to start the event manually!\n", StartingIndex);
                StartingIndex = 0;
            }

            if (!Script->PhaseInfoArray.IsValidIndex(StartingIndex))
            {
                printf("[SpecialEventScript] Event script is not ready or has invalid starting phase %d (phase count: %d).\n", StartingIndex, Script->PhaseInfoArray.Num());
            }
            else
            {
                auto MeshNetworkSubsystem = (UMeshNetworkSubsystem*)TUObjectArray::FindFirstObject("MeshNetworkSubsystem");
                const bool bHasMeshNetworkSubsystem = IsValidUObject(MeshNetworkSubsystem, UMeshNetworkSubsystem::StaticClass());
                const auto OldRootStartTime = MeshActor->RootStartTime;
                EMeshNetworkNodeType OldNodeType {};
                EMeshNetworkNodeType OldGameServerNodeType {};

                if (bHasMeshNetworkSubsystem)
                {
                    OldNodeType = MeshNetworkSubsystem->NodeType;
                    OldGameServerNodeType = MeshNetworkSubsystem->GameServerNodeType;
                    MeshNetworkSubsystem->NodeType = EMeshNetworkNodeType::Root;
                    MeshNetworkSubsystem->GameServerNodeType = EMeshNetworkNodeType::Root;
                }

                MeshActor->MeshRootStartEvent();
                if (MeshActor->RootStartTime.Ticks != OldRootStartTime.Ticks)
                    MeshActor->OnRep_RootStartTime(OldRootStartTime);
                Script->StartEventAtIndex(StartingIndex, 0.f);

                if (bHasMeshNetworkSubsystem)
                {
                    MeshNetworkSubsystem->NodeType = OldNodeType;
                    MeshNetworkSubsystem->GameServerNodeType = OldGameServerNodeType;
                }

                bStarted = true;
            }
        }
        else
        {
            printf("[SpecialEventScript] Found event actors, but one of them is invalid.\n");
        }
    }
    else
    {
        printf("[SpecialEventScript] Could not start old event sequence. Mesh actors: %d, event scripts: %d.\n", MeshActors.Num(), EventScripts.Num());
    }

    if (!bStarted)
    {
        TArray<AActor*> AllActors;
        UClass* ActorClass = (UClass*)TUObjectArray::FindObject("Actor", nullptr);
        if (ActorClass)
        {
            UGameplayStatics::GetAllActorsOfClass(World, ActorClass, &AllActors);
            int FoundTriggers = 0;
            
            for (int i = 0; i < AllActors.Num(); i++)
            {
                AActor* Actor = AllActors[i];
                if (!Actor || !Actor->Class) continue;
                
                UFunction* StartFunc = Actor->Class->FindFunction("CheatStartEvent");
                if (!StartFunc) StartFunc = Actor->Class->FindFunction("CheatStartLiveEvent");
                
                if (!StartFunc)
                {
                    std::string ClassName = Actor->Class->Name.ToString().c_str();
                    bool bIsEventActor = (ClassName.find("EventDirector") != std::string::npos ||
                                          ClassName.find("LiveStreamEvent") != std::string::npos ||
                                          ClassName.find("Quail") != std::string::npos);
                                          
                    if (bIsEventActor)
                    {
                        StartFunc = Actor->Class->FindFunction("StartEvent");
                        if (!StartFunc) StartFunc = Actor->Class->FindFunction("StartLiveEvent");
                    }
                }
                
                if (StartFunc)
                {
                    printf("[SpecialEventScript] Triggered an event function on an actor!\n");
                    Actor->ProcessEvent(StartFunc, nullptr);
                    bStarted = true;
                    FoundTriggers++;
                    break;
                }
            }
            AllActors.Free();
            printf("[SpecialEventScript] Searched %d actors, found %d triggers.\n", AllActors.Num(), FoundTriggers);
        }
    }

    MeshActors.Free();
    EventScripts.Free();
    return bStarted;
}

void SpecialEventScript__Init()
{
    Hook(ImageBase + 0xC2AA930, ActivatePhase, ActivatePhaseOG);
}
