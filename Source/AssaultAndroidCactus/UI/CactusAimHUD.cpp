#include "UI/CactusAimHUD.h"
#include "Characters/CactusPlayerCharacter.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "Game/CactusGameMode.h"
#include "Game/Stages/CactusEnemySpawner.h"
#include "Weapons/CactusWeaponComponent.h"
#include "Engine/World.h"

void ACactusAimHUD::DrawHUD()
{
    Super::DrawHUD();
    const ACactusPlayerCharacter* Character = PlayerOwner ? Cast<ACactusPlayerCharacter>(PlayerOwner->GetPawn()) : nullptr;
    if (!Canvas || !Character) return;

    constexpr float ReticleSize = 6.0f;
    constexpr float Padding = 18.0f;
    const bool bFirstPerson = Character->GetCameraMode() == ECactusCameraMode::FirstPerson;
    FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
    if (bFirstPerson || PlayerOwner->ProjectWorldLocationToScreen(Character->GetAimPoint(), Center))
    {
        DrawLine(Center.X - ReticleSize, Center.Y, Center.X + ReticleSize, Center.Y, FLinearColor::White, 1.5f);
        DrawLine(Center.X, Center.Y - ReticleSize, Center.X, Center.Y + ReticleSize, FLinearColor::White, 1.5f);
    }
    const FString Help = bFirstPerson
        ? TEXT("FIRST PERSON   WASD: move   Mouse: look   V: top down   Esc: quit")
        : TEXT("TOP DOWN   WASD: move   Mouse: aim   V: first person   Esc: quit");
    //DrawText(Help, FLinearColor::White, Padding, Canvas->ClipY - Padding * 2.0f);

    // 仮の体力表示。後でUMGへ移す場合もCharacterの値をそのまま参照する。
    const float Ratio = FMath::Clamp(Character->GetHealth() / FMath::Max(Character->GetMaxHealth(), 1.0f), 0.0f, 1.0f);
    DrawText(FString::Printf(TEXT("HP %.0f / %.0f"), Character->GetHealth(), Character->GetMaxHealth()), FLinearColor::White, Padding, Padding);
    DrawRect(FLinearColor(0.03f, 0.03f, 0.03f, 0.85f), Padding, Padding + 24, 184, 16);
    DrawRect(Character->IsInvulnerable() ? FLinearColor::Yellow : FLinearColor(0.2f, 0.9f, 0.35f), Padding + 2, Padding + 26, 180 * Ratio, 12);
    if (Character->IsDowned()) DrawText(TEXT("DOWN - recovering..."), FLinearColor::Yellow, Padding, Padding + 48);

    if (const UCactusWeaponComponent* Weapon = Character->GetWeapon())
    {
        const TCHAR* Name = Weapon->GetAttackMode() == ECactusAttackMode::Primary ? TEXT("AUTO") : TEXT("FLAME");
        const float Wait = Character->GetWeaponSwitchCooldownRemaining();
        const FString State = Wait > 0 ? FString::Printf(TEXT("SWITCH %.1fs"),Wait) : TEXT("SPACE / LB: SWITCH");
        DrawText(FString::Printf(TEXT("%s   %s"),Name,*State),FLinearColor(0.3f,0.9f,1.0f),Padding,Padding+188);
    }

    if (const ACactusGameMode* Mode = GetWorld()->GetAuthGameMode<ACactusGameMode>())
    {
        DrawText(FString::Printf(TEXT("SCORE %d"), Mode->GetScore()), FLinearColor::Yellow, Padding, Padding + 104);
        if (Mode->IsBatteryEnabled())
        {
            const float BatteryRatio = FMath::Clamp(Mode->GetBattery()/FMath::Max(Mode->GetBatteryCapacity(),1.0f),0.0f,1.0f);
            const FLinearColor BatteryColor = BatteryRatio <= 0.2f ? FLinearColor(1,0.2f,0.1f) : FLinearColor(0.1f,0.9f,0.8f);
            DrawText(FString::Printf(TEXT("BATTERY %.0f / %.0f"),Mode->GetBattery(),Mode->GetBatteryCapacity()),BatteryColor,Padding,Padding+134);
            DrawRect(FLinearColor(0.03f,0.03f,0.03f,0.85f),Padding,Padding+158,184,16);
            DrawRect(BatteryColor,Padding+2,Padding+160,180*BatteryRatio,12);
        }
        if (const ACactusEnemySpawner* Spawner = Mode->GetEnemySpawner())
        {
            FString Text = FString::Printf(TEXT("WAVE %d / %d   ENEMIES %d   TO SPAWN %d"),
                Spawner->GetWaveNumber(), Spawner->GetWaveCount(), Spawner->GetAliveCount(), Spawner->GetRemainingToSpawn());
            if (Spawner->GetWaveState() == ECactusWaveState::Waiting) Text += TEXT("   READY...");
            if (Spawner->GetWaveState() == ECactusWaveState::Completed) Text = TEXT("WAVES COMPLETE");
            if (Spawner->GetWaveState() == ECactusWaveState::Stopped) Text += TEXT("   STOPPED");
            if (Spawner->GetWaveState() == ECactusWaveState::Error) Text = TEXT("WAVE ERROR - Check EnemySpawner / Last Error");
            DrawText(Text, FLinearColor::White, Padding, Padding + 78);
        }
    }
}
