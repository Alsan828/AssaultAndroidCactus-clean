#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CactusStageData.generated.h"

class ACactusEnemy;

USTRUCT(BlueprintType)
struct FCactusEnemySpawnEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
    TSubclassOf<ACactusEnemy> EnemyClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "1", ClampMax = "1000"))
    int32 Count = 5;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0.0", Units = "s"))
    float SpawnInterval = 0.4f;

    // 空欄ならSpawnerの全地点を使う。番号は0始まり。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0"))
    TArray<int32> SpawnPointIndices;

    // 負数はSpawner設定、0は指定地点のみ、正数は地点周囲の配置範囲。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "-1", ClampMax = "1000", Units = "cm"))
    float SpawnRadiusOverride = -1.0f;
};

USTRUCT(BlueprintType)
struct FCactusWaveEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0.0", Units = "s"))
    float DelayBeforeWave = 2.0f;

    // 1回のTimerで出す合計数。Enemiesの並びをまたいで同じ回に生成できる。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "1", ClampMax = "1000"))
    int32 SpawnBatchSize = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
    TArray<FCactusEnemySpawnEntry> Enemies;
};

// 配置や進行状態を持たない、編集用のステージ設定。
UCLASS(BlueprintType)
class ASSAULTANDROIDCACTUS_API UCactusStageData : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
    TArray<FCactusWaveEntry> Waves;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage", meta = (ClampMin = "1", ClampMax = "1000"))
    int32 MaxAliveEnemies = 100;

    bool Validate(FString& Reason) const;
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
