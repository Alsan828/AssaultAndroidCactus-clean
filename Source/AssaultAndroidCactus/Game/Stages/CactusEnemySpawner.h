#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/Stages/CactusStageData.h"
#include "CactusEnemySpawner.generated.h"

class ACactusEnemy;
class ATargetPoint;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCactusWavesCompleted);

UENUM(BlueprintType)
enum class ECactusWaveState : uint8
{
    Idle, Waiting, Spawning, WaitingForClear, Completed, Stopped, Error
};

// StageDataを読むだけにし、Timerと撃破通知で生成・波の進行を管理する。
UCLASS()
class ASSAULTANDROIDCACTUS_API ACactusEnemySpawner : public AActor
{
    GENERATED_BODY()
public:
    ACactusEnemySpawner();

    UPROPERTY(BlueprintAssignable, Category = "Waves")
    FCactusWavesCompleted OnWavesCompleted;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Waves")
    TObjectPtr<UCactusStageData> StageData;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Waves")
    TArray<TObjectPtr<ATargetPoint>> SpawnPoints;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves")
    bool bAutoStart = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves", meta = (ClampMin = "0.1", Units = "s"))
    float BlockedSpawnTimeout = 10.0f;

    // 同時生成用に地点周辺も使う。0なら従来通りTargetPointの位置だけ。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves", meta = (ClampMin = "0.0", ClampMax = "1000.0", Units = "cm"))
    float SpawnRadius = 0.0f;

    UFUNCTION(BlueprintCallable, Category = "Waves")
    bool StartWaves();

    UFUNCTION(BlueprintCallable, Category = "Waves")
    void StopWaves();

    UFUNCTION(BlueprintCallable, Category = "Waves")
    void ResetSpawner();

    UFUNCTION(BlueprintPure, Category = "Waves")
    ECactusWaveState GetWaveState() const { return State; }

    UFUNCTION(BlueprintPure, Category = "Waves")
    int32 GetWaveNumber() const { return WaveIndex + 1; }

    UFUNCTION(BlueprintPure, Category = "Waves")
    int32 GetWaveCount() const { return RuntimeWaves.IsEmpty() ? (StageData ? StageData->Waves.Num() : 0) : RuntimeWaves.Num(); }

    UFUNCTION(BlueprintPure, Category = "Waves")
    int32 GetAliveCount() const { return AliveEnemies.Num(); }

    UFUNCTION(BlueprintPure, Category = "Waves")
    int32 GetRemainingToSpawn() const { return RemainingToSpawn; }

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Waves")
    FString LastError;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    void QueueNextWave();
    void BeginWave();
    void SpawnNextBatch();
    void ScheduleSpawn(float Delay);
    ACactusEnemy* SpawnFromEntry(const FCactusEnemySpawnEntry& Entry);
    void CheckWaveClear();
    void Fail(const FString& Reason);

    UFUNCTION()
    void OnManagedEnemyDestroyed(AActor* Enemy);

    UPROPERTY(Transient)
    TArray<FCactusWaveEntry> RuntimeWaves;

    TSet<TWeakObjectPtr<ACactusEnemy>> AliveEnemies;
    ECactusWaveState State = ECactusWaveState::Idle;
    int32 WaveIndex = -1;
    int32 EntryIndex = 0;
    int32 SpawnedInEntry = 0;
    int32 RemainingToSpawn = 0;
    int32 SpawnPointCursor = 0;
    int32 AliveLimit = 100;
    double BlockedSince = -1;
    FTimerHandle WaveTimer;
};
