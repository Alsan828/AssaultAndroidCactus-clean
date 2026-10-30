#include "Game/CactusGameMode.h"
#include "Characters/CactusPlayerCharacter.h"
#include "UI/CactusAimHUD.h"
#include "UI/CactusScreenWidget.h"
#include "Game/Stages/CactusEnemySpawner.h"
#include "Weapons/CactusWeaponComponent.h"
#include "Weapons/CactusProjectilePool.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ACactusGameMode::ACactusGameMode()
{
    DefaultPawnClass = ACactusPlayerCharacter::StaticClass();
    HUDClass = ACactusAimHUD::StaticClass();
}

void ACactusGameMode::AddScore(int32 Amount)
{
    if (Amount <= 0 || bStageFinished) return;
    Score = static_cast<int32>(FMath::Min<int64>(int64(Score) + Amount, MAX_int32));
}

void ACactusGameMode::BeginPlay()
{
    Super::BeginPlay();
    if (bIsTitleScreen)
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &ACactusGameMode::ShowTitleScreen);
        return;
    }
    for (TActorIterator<ACactusEnemySpawner> It(GetWorld()); It; ++It)
    {
        EnemySpawner = *It; // 開始時に一度だけ取得。毎フレーム全Actorを探さない。
        EnemySpawner->OnWavesCompleted.AddDynamic(this, &ACactusGameMode::HandleWavesCompleted);
        break;
    }
    BatteryCapacity = FMath::IsFinite(BatteryCapacity) ? FMath::Max(BatteryCapacity,1.0f) : 100.0f;
    Battery = BatteryCapacity;
    LastBatteryTime = GetWorld()->GetTimeSeconds();
    if (bBatteryEnabled)
    {
        const float Interval = FMath::IsFinite(BatteryUpdateInterval) ? FMath::Max(BatteryUpdateInterval,0.02f) : 0.1f;
        GetWorldTimerManager().SetTimer(BatteryTimer,this,&ACactusGameMode::UpdateBattery,Interval,true);
    }
}

void ACactusGameMode::UpdateBattery()
{
    if (!IsBatteryEnabled() || bStageFinished || bTravelPending || StageOutcome != ECactusStageOutcome::InProgress) return;
    const double Now = GetWorld()->GetTimeSeconds();
    const float Rate = FMath::IsFinite(BatteryDrainPerSecond) ? FMath::Max(BatteryDrainPerSecond,0.0f) : 0.0f;
    // Timer回数ではなく経過したゲーム時間で減算する。ダウン中も継続し、Pause中は止まる。
    Battery = FMath::Max(0.0f,Battery-static_cast<float>(FMath::Max(Now-LastBatteryTime,0.0))*Rate);
    LastBatteryTime = Now;
    if (Battery <= 0 && !bBatteryFailurePending)
    {
        bBatteryFailurePending = true;
        GetWorldTimerManager().ClearTimer(BatteryTimer);
        // 同フレームの最終撃破通知を受け付けてから判定する。全滅が成立した場合はクリア優先。
        BatteryFailureTimer = GetWorldTimerManager().SetTimerForNextTick(this,&ACactusGameMode::ResolveBatteryDepletion);
    }
}

bool ACactusGameMode::RestoreBattery(float Amount)
{
    if (!IsBatteryEnabled() || bStageFinished || bTravelPending || StageOutcome != ECactusStageOutcome::InProgress ||
        !FMath::IsFinite(Amount) || Amount <= 0) return false;
    UpdateBattery();
    if (Battery <= 0) return false;
    const float Previous = Battery;
    Battery = FMath::Min(Battery+Amount,BatteryCapacity);
    return Battery > Previous;
}

void ACactusGameMode::ConsumeDownedBattery()
{
    if (!IsBatteryEnabled() || bStageFinished || bTravelPending || StageOutcome != ECactusStageOutcome::InProgress) return;
    UpdateBattery();
    const float Ratio = FMath::IsFinite(DownedBatteryCostRatio) ? FMath::Clamp(DownedBatteryCostRatio,0.0f,1.0f) : 0.3f;
    Battery = FMath::Max(0.0f,Battery-BatteryCapacity*Ratio);
    UpdateBattery(); // 0になった場合も通常の電池切れ判定へ合流する。
}

void ACactusGameMode::ResolveBatteryDepletion()
{
    bBatteryFailurePending = false;
    if (bStageFinished || bTravelPending || StageOutcome != ECactusStageOutcome::InProgress || Battery > 0) return;
    if (IsValid(EnemySpawner) && EnemySpawner->GetWaveState() == ECactusWaveState::Completed)
    {
        HandleWavesCompleted();
        return;
    }
    StageOutcome = ECactusStageOutcome::BatteryDepleted;
    if (IsValid(EnemySpawner)) EnemySpawner->StopWaves();
    GetWorldTimerManager().ClearTimer(ResultTimer);
    ShowResultScreen();
}

void ACactusGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearAllTimersForObject(this);
    if (IsValid(EnemySpawner)) EnemySpawner->OnWavesCompleted.RemoveDynamic(this, &ACactusGameMode::HandleWavesCompleted);
    if (CurrentScreen) CurrentScreen->RemoveFromParent();
    CurrentScreen = nullptr;
    Super::EndPlay(Reason);
}

void ACactusGameMode::ActivateScreen(UCactusScreenWidget* Screen)
{
    if (CurrentScreen) CurrentScreen->RemoveFromParent();
    CurrentScreen = Screen;
    CurrentScreen->AddToViewport(100);
    APlayerController* Player = CurrentScreen->GetOwningPlayer();
    Player->bShowMouseCursor = true;
    Player->DefaultMouseCursor = EMouseCursor::Default;
    FInputModeUIOnly Input;
    Input.SetWidgetToFocus(CurrentScreen->TakeWidget());
    Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Player->SetInputMode(Input);
    CurrentScreen->FocusInitialButton();
}

void ACactusGameMode::ShowTitleScreen()
{
    APlayerController* Player = UGameplayStatics::GetPlayerController(this, 0);
    if (!Player || !Player->IsLocalController() || !TitleScreenClass) return;
    if (UCactusScreenWidget* Screen = CreateWidget<UCactusScreenWidget>(Player, TitleScreenClass)) ActivateScreen(Screen);
}

void ACactusGameMode::HandleWavesCompleted()
{
    // 画面を設定しない単体確認Levelでは、Spawnerだけを利用できる。
    if (bIsTitleScreen || bStageFinished || bResultPending || StageOutcome == ECactusStageOutcome::BatteryDepleted) return;
    StageOutcome = ECactusStageOutcome::Cleared;
    GetWorldTimerManager().ClearTimer(BatteryTimer);
    GetWorldTimerManager().ClearTimer(BatteryFailureTimer);
    bBatteryFailurePending = false;
    if (!ResultScreenClass) return;
    bResultPending = true;
    if (ACactusPlayerCharacter* Character = Cast<ACactusPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        Character->GetWeapon()->StopFiring();
        Character->SetCanBeDamaged(false);
    }
    if (UCactusProjectilePool* Pool = GetWorld()->GetSubsystem<UCactusProjectilePool>()) Pool->ResetPool();
    const float Delay = FMath::IsFinite(ResultDelay) ? FMath::Max(ResultDelay, 0.0f) : 0.0f;
    if (Delay > 0) GetWorldTimerManager().SetTimer(ResultTimer, this, &ACactusGameMode::ShowResultScreen, Delay, false);
    else ShowResultScreen();
}

void ACactusGameMode::ShowResultScreen()
{
    if (bStageFinished || bTravelPending) return;
    bStageFinished = true;
    GetWorldTimerManager().ClearTimer(BatteryTimer);
    GetWorldTimerManager().ClearTimer(BatteryFailureTimer);
    GetWorldTimerManager().ClearTimer(ResultTimer);
    if (UCactusProjectilePool* Pool = GetWorld()->GetSubsystem<UCactusProjectilePool>()) Pool->ResetPool();
    APlayerController* Player = UGameplayStatics::GetPlayerController(this, 0);
    if (!Player) return;
    if (ACactusPlayerCharacter* Character = Cast<ACactusPlayerCharacter>(Player->GetPawn()))
    {
        Character->GetWeapon()->StopFiring();
        Character->ConsumeMovementInputVector();
        Character->GetCharacterMovement()->StopMovementImmediately();
        Character->SetCanBeDamaged(false);
    }
    if (AHUD* HUD = Player->GetHUD()) HUD->bShowHUD = false;
    Player->SetIgnoreMoveInput(true);
    Player->SetIgnoreLookInput(true);
    const TSubclassOf<UCactusScreenWidget> ScreenClass = StageOutcome == ECactusStageOutcome::BatteryDepleted && FailureScreenClass ? FailureScreenClass : ResultScreenClass;
    if (Player->IsLocalController() && ScreenClass)
        if (UCactusScreenWidget* Screen = CreateWidget<UCactusScreenWidget>(Player,ScreenClass))
        {
            ActivateScreen(Screen);
            Screen->SetFinalScore(Score);
        }
    UGameplayStatics::SetGamePaused(this, true);
}

void ACactusGameMode::OpenGameLevel(TSoftObjectPtr<UWorld> Level)
{
    if (bTravelPending || Level.IsNull()) return;
    bTravelPending = true;
    GetWorldTimerManager().ClearTimer(BatteryTimer);
    GetWorldTimerManager().ClearTimer(BatteryFailureTimer);
    GetWorldTimerManager().ClearTimer(ResultTimer);
    if (CurrentScreen) CurrentScreen->RemoveFromParent();
    CurrentScreen = nullptr;
    APlayerController* Player = UGameplayStatics::GetPlayerController(this, 0);
    if (Player)
    {
        Player->ResetIgnoreMoveInput();
        Player->ResetIgnoreLookInput();
        Player->SetInputMode(FInputModeGameOnly());
    }
    UGameplayStatics::SetGamePaused(this, false);
    // Levelを作り直し、敵・弾・道具・スコア・タイマーを新しい局へ持ち越さない。
    UGameplayStatics::OpenLevelBySoftObjectPtr(this, Level);
}

void ACactusGameMode::StartGame()
{
    OpenGameLevel(GameplayLevel);
}

void ACactusGameMode::ReturnToTitle()
{
    OpenGameLevel(TitleLevel);
}
