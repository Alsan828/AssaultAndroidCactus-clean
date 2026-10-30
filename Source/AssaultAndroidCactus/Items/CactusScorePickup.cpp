#include "Items/CactusScorePickup.h"
#include "Game/CactusGameMode.h"
#include "Engine/World.h"

bool ACactusScorePickup::ApplyEffect(ACactusPlayerCharacter* Player)
{
    ACactusGameMode* Mode = GetWorld()->GetAuthGameMode<ACactusGameMode>();
    if (!Mode || Mode->IsStageFinished() || ScoreValue <= 0) return false;
    Mode->AddScore(ScoreValue);
    return true;
}
