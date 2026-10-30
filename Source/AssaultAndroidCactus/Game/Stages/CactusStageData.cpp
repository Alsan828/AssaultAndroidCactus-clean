#include "Game/Stages/CactusStageData.h"
#include "Characters/Enemies/CactusEnemy.h"
#include "Misc/DataValidation.h"

bool UCactusStageData::Validate(FString& Reason) const
{
    Reason.Reset();
    if (Waves.IsEmpty()) Reason = TEXT("Waves is empty.");
    else if (MaxAliveEnemies < 1 || MaxAliveEnemies > 1000) Reason = TEXT("Max Alive Enemies must be between 1 and 1000.");
    else for (int32 WaveIndex = 0; WaveIndex < Waves.Num(); ++WaveIndex)
    {
        const FCactusWaveEntry& Wave = Waves[WaveIndex];
        if (Wave.SpawnBatchSize < 1 || Wave.SpawnBatchSize > 1000)
            Reason = FString::Printf(TEXT("Wave %d needs Spawn Batch Size between 1 and 1000."), WaveIndex + 1);
        if (!FMath::IsFinite(Wave.DelayBeforeWave) || Wave.DelayBeforeWave < 0 || Wave.Enemies.IsEmpty())
            Reason = FString::Printf(TEXT("Wave %d needs enemy entries and a non-negative finite delay."), WaveIndex + 1);
        for (const FCactusEnemySpawnEntry& Entry : Wave.Enemies)
        {
            if (!Entry.EnemyClass || Entry.EnemyClass->HasAnyClassFlags(CLASS_Abstract) || Entry.Count < 1 || Entry.Count > 1000 ||
                !FMath::IsFinite(Entry.SpawnInterval) || Entry.SpawnInterval < 0 ||
                !FMath::IsFinite(Entry.SpawnRadiusOverride) || Entry.SpawnRadiusOverride < -1 || Entry.SpawnRadiusOverride > 1000)
                Reason = FString::Printf(TEXT("Wave %d has an invalid enemy class, count or interval."), WaveIndex + 1);
            for (int32 Index : Entry.SpawnPointIndices)
                if (Index < 0) Reason = FString::Printf(TEXT("Wave %d has a negative spawn point index."), WaveIndex + 1);
        }
        if (!Reason.IsEmpty()) break;
    }
    return Reason.IsEmpty();
}

#if WITH_EDITOR
EDataValidationResult UCactusStageData::IsDataValid(FDataValidationContext& Context) const
{
    FString Reason;
    if (!Validate(Reason))
    {
        Context.AddError(FText::FromString(Reason));
        return EDataValidationResult::Invalid;
    }
    return EDataValidationResult::Valid;
}
#endif
