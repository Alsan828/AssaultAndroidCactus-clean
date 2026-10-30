#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CactusGameMode.generated.h"

class ACactusEnemySpawner;
class UCactusScreenWidget;

UENUM(BlueprintType)
enum class ECactusStageOutcome : uint8
{
    InProgress,
    Cleared,
    BatteryDepleted
};

UCLASS()
class ASSAULTANDROIDCACTUS_API ACactusGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ACactusGameMode();

    UFUNCTION(BlueprintPure, Category = "Stage")
    ACactusEnemySpawner* GetEnemySpawner() const { return EnemySpawner; }

    UFUNCTION(BlueprintCallable, Category = "Score")
    void AddScore(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Score")
    int32 GetScore() const { return Score; }

    UFUNCTION(BlueprintPure, Category = "Battery")
    bool IsBatteryEnabled() const { return bBatteryEnabled && !bIsTitleScreen; }
    UFUNCTION(BlueprintPure, Category = "Battery")
    float GetBattery() const { return Battery; }
    UFUNCTION(BlueprintPure, Category = "Battery")
    float GetBatteryCapacity() const { return BatteryCapacity; }
    UFUNCTION(BlueprintCallable, Category = "Battery")
    bool RestoreBattery(float Amount);
    // ダウンへの遷移時に一度だけ呼ぶ。最大容量に対する割合を消費する。
    void ConsumeDownedBattery();
    UFUNCTION(BlueprintPure, Category = "Stage")
    ECactusStageOutcome GetStageOutcome() const { return StageOutcome; }

    UFUNCTION(BlueprintCallable, Category = "Screens")
    void StartGame();

    UFUNCTION(BlueprintCallable, Category = "Screens")
    void ReturnToTitle();

    UFUNCTION(BlueprintPure, Category = "Screens")
    bool IsStageFinished() const { return bStageFinished; }

    UCactusScreenWidget* GetCurrentScreen() const { return CurrentScreen; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screens")
    bool bIsTitleScreen = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screens")
    TSoftObjectPtr<UWorld> GameplayLevel;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screens")
    TSoftObjectPtr<UWorld> TitleLevel;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screens")
    TSubclassOf<UCactusScreenWidget> TitleScreenClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screens")
    TSubclassOf<UCactusScreenWidget> ResultScreenClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screens")
    TSubclassOf<UCactusScreenWidget> FailureScreenClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battery")
    bool bBatteryEnabled = true;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battery", meta = (ClampMin = "1"))
    float BatteryCapacity = 100.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battery", meta = (ClampMin = "0"))
    float BatteryDrainPerSecond = 1.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battery", meta = (ClampMin = "0", ClampMax = "1"))
    float DownedBatteryCostRatio = 0.3f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battery", meta = (ClampMin = "0.02", Units = "s"))
    float BatteryUpdateInterval = 0.1f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battery")
    float Battery = 100.0f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stage")
    ECactusStageOutcome StageOutcome = ECactusStageOutcome::InProgress;

    // 最終撃破の道具を拾う時間。待機中は得点と移動を維持し、残弾だけを消す。
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screens", meta = (ClampMin = "0", Units = "s"))
    float ResultDelay = 3.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stage")
    TObjectPtr<ACactusEnemySpawner> EnemySpawner;

    // 新しいLevelのGameModeで0から開始。Playerのダウン・復帰では変えない。
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Score")
    int32 Score = 0;

private:
    void ShowTitleScreen();
    void OpenGameLevel(TSoftObjectPtr<UWorld> Level);
    void ActivateScreen(UCactusScreenWidget* Screen);

    UFUNCTION()
    void HandleWavesCompleted();
    void ShowResultScreen();
    FTimerHandle ResultTimer;
    void UpdateBattery();
    void ResolveBatteryDepletion();
    FTimerHandle BatteryTimer;
    FTimerHandle BatteryFailureTimer;
    double LastBatteryTime = 0;
    bool bBatteryFailurePending = false;

    UPROPERTY(Transient)
    TObjectPtr<UCactusScreenWidget> CurrentScreen;

    bool bStageFinished = false;
    bool bResultPending = false;
    bool bTravelPending = false;
};
