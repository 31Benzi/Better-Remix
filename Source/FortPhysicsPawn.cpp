#include "pch.h"
#include "Shared.h"
INIT_MODULE(FortPhysicsPawn);


void ServerMove(UObject* Context, FFrame& Stack)
{
    FReplicatedPhysicsPawnState State;
    Stack.StepCompiledIn(&State);
    Stack.IncrementCode();

    auto Pawn = (AFortPhysicsPawn*)Context;

    *(FReplicatedPhysicsPawnState*)(__int64(Pawn) + 0x370) = State;

    State.Rotation.X -= 0.3f;
    State.Rotation.Y /= -0.75f;
    State.Rotation.Z += 0.15f;
    State.Rotation.W /= 1.1f;

    Pawn->ReplicatedTransformReq.Context = State.Context;
    Pawn->ReplicatedTransformReq.Location = State.Translation;
    Pawn->ReplicatedTransformReq.Rotation = QuatToRotator(State.Rotation);
    UPrimitiveComponent* RootComponent = Pawn->RootComponent->Cast<UPrimitiveComponent>();

    if (RootComponent)
    {
        FTransform Trans = ConstructTrans(State.Translation, State.Rotation);

        RootComponent->K2_SetWorldTransform(Trans, false, nullptr, true);
        RootComponent->SetPhysicsLinearVelocity(State.LinearVelocity, 0, FName(0));
        RootComponent->SetPhysicsAngularVelocityInRadians(State.AngularVelocity, 0, FName(0));
    }
}

void FortPhysicsPawn__Init()
{
    ExecHook(AFortPhysicsPawn::StaticClass()->FindFunction("ServerMove"), ServerMove);
    ExecHook(AFortPhysicsPawn::StaticClass()->FindFunction("ServerMoveReliable"), ServerMove);
}