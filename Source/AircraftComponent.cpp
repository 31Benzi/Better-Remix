#include "pch.h"
#include "Shared.h"
INIT_MODULE(AircraftComponent);

void ServerAttemptAircraftJump(UFortControllerComponent_Aircraft* _this, FRotator Rotation)
{
    auto GameMode = GetWorld()->AuthorityGameMode;
    auto PlayerController = (AFortPlayerControllerAthena*)_this->GetOwner();

    PlayerController->StateName = FName(L"Inactive");

    if (PlayerController->Pawn)
        PlayerController->UnPossess();

    //GameMode->RestartPlayer(PlayerController);
    PlayerController->ServerRestartPlayer();
    PlayerController->SetControlRotation(Rotation);
}

void AircraftComponent__Init()
{
    Hook<UFortControllerComponent_Aircraft>(uint32(0xAA), ServerAttemptAircraftJump);
}
