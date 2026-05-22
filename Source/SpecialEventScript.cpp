#include "pch.h"
#include "Shared.h"
INIT_MODULE(SpecialEventScript);

void (*ActivatePhaseOG)(ASpecialEventScript* _this, int IndexToActivate, float a3);
void ActivatePhase(ASpecialEventScript* _this, int IndexToActivate, float a3)
{
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

void SpecialEventScript__Init()
{
    Hook(ImageBase + 0xC2AA930, ActivatePhase, ActivatePhaseOG);
}