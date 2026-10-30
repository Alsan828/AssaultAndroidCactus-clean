#include "Items/CactusBatteryPickup.h"
#include "Game/CactusGameMode.h"
#include "Engine/World.h"

bool ACactusBatteryPickup::ApplyEffect(ACactusPlayerCharacter* Player)
{
    ACactusGameMode* Mode = GetWorld()->GetAuthGameMode<ACactusGameMode>();
    // 満充電・無効・終了済みなら道具を残す。体力やダウン復帰とは独立した局の電池を補充する。
    return Mode && Mode->RestoreBattery(RestoreAmount);
}
