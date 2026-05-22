#include "pch.h"
#include "Shared.h"
#include "Config.h"
INIT_MODULE(NetDriver);

enum class EReplicationSystemSendPass : unsigned
{
    Invalid,
    PostTickDispatch,
    TickFlush,
};

struct FSendUpdateParams
{
    EReplicationSystemSendPass SendPass = EReplicationSystemSendPass::TickFlush;
    float DeltaSeconds = 0.f;
};

void SendClientMoveAdjustments(UNetDriver* Driver)
{
    static auto SendClientAdjustment = (void (*)(APlayerController*))(ImageBase + 0x71152EC);
    if (SendClientAdjustment)
    {
        for (UNetConnection* Connection : Driver->ClientConnections)
        {
            if (Connection == nullptr || Connection->ViewTarget == nullptr)
                continue;

            if (auto PC = Connection->PlayerController)
            {
                if (PC->Pawn)
                    SendClientAdjustment(PC);
            }

            for (UNetConnection* ChildConnection : Connection->Children)
            {
                if (ChildConnection == nullptr || ChildConnection->ViewTarget == nullptr)
                    continue;

                if (auto PC = ChildConnection->PlayerController)
                {
                    if (PC->Pawn)
                        SendClientAdjustment(PC);
                }
            }
        }
    }
}

void (*TickFlushOG)(UNetDriver* Driver, float DeltaSeconds);
void TickFlush(UNetDriver* Driver, float DeltaSeconds)
{
    for (auto& Ticker : Tickers)
        Ticker();

    if (Driver->ClientConnections.Num() > 0)
    {
        auto ReplicationSystem = *(UObject**)(__int64(&Driver->ReplicationDriver) + 0x10);

        if (ReplicationSystem)
        {
            static void (*UpdateIrisReplicationViews)(UNetDriver*) = decltype(UpdateIrisReplicationViews)(ImageBase + 0x705EBE4);
            static void (*PreSendUpdate)(UObject*, FSendUpdateParams&) = decltype(PreSendUpdate)(ImageBase + 0x63BF4A4);

            UpdateIrisReplicationViews(Driver);
            SendClientMoveAdjustments(Driver);
            FSendUpdateParams Params;
            Params.DeltaSeconds = DeltaSeconds;
            PreSendUpdate(ReplicationSystem, Params);
        }
    }

    return TickFlushOG(Driver, DeltaSeconds);
}

void NetDriver__Init()
{
    Hook(ImageBase + 0x23CE5C4, TickFlush, TickFlushOG);
}