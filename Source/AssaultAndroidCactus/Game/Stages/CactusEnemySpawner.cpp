#include "Game/Stages/CactusEnemySpawner.h"
#include "Characters/Enemies/CactusEnemy.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

ACactusEnemySpawner::ACactusEnemySpawner()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ACactusEnemySpawner::BeginPlay()
{
    Super::BeginPlay();
    if (bAutoStart) StartWaves();
}

void ACactusEnemySpawner::EndPlay(const EEndPlayReason::Type Reason)
{
    StopWaves();
    for (const TWeakObjectPtr<ACactusEnemy>& Enemy : AliveEnemies)
        if (Enemy.IsValid()) Enemy->OnDestroyed.RemoveDynamic(this, &ACactusEnemySpawner::OnManagedEnemyDestroyed);
    AliveEnemies.Reset();
    Super::EndPlay(Reason);
}

void ACactusEnemySpawner::Fail(const FString& Reason)
{
    GetWorldTimerManager().ClearTimer(WaveTimer);
    LastError = Reason;
    State = ECactusWaveState::Error;
    UE_LOG(LogTemp, Display, TEXT("CactusEnemySpawner: %s"), *Reason);
}

bool ACactusEnemySpawner::StartWaves()
{
    if (State == ECactusWaveState::Waiting || State == ECactusWaveState::Spawning || State == ECactusWaveState::WaitingForClear) return false;
    FString Reason;
    if (!StageData || !StageData->Validate(Reason))
    {
        Fail(StageData ? Reason : TEXT("Stage Data is not assigned."));
        return false;
    }
    if (SpawnPoints.IsEmpty()) { Fail(TEXT("Spawn Points is empty.")); return false; }
    if (!FMath::IsFinite(SpawnRadius) || SpawnRadius < 0 || SpawnRadius > 1000)
        { Fail(TEXT("Spawn Radius must be finite and between 0 and 1000 cm.")); return false; }
    for (ATargetPoint* Point : SpawnPoints)
        if (!IsValid(Point) || Point->GetWorld() != GetWorld()) { Fail(TEXT("Spawn Points contains a missing point or another World.")); return false; }
    for (const FCactusWaveEntry& Wave : StageData->Waves)
        for (const FCactusEnemySpawnEntry& Entry : Wave.Enemies)
            for (int32 Index : Entry.SpawnPointIndices)
                if (!SpawnPoints.IsValidIndex(Index)) { Fail(TEXT("Spawn Point Indices is outside the Spawn Points array.")); return false; }

    ResetSpawner();
    RuntimeWaves = StageData->Waves; // 実行用コピー。Data Asset自体は変更しない。
    AliveLimit = StageData->MaxAliveEnemies;
    QueueNextWave();
    return true;
}

void ACactusEnemySpawner::StopWaves()
{
    GetWorldTimerManager().ClearTimer(WaveTimer);
    State = ECactusWaveState::Stopped;
}

void ACactusEnemySpawner::ResetSpawner()
{
    StopWaves();
    const TSet<TWeakObjectPtr<ACactusEnemy>> Previous = MoveTemp(AliveEnemies);
    AliveEnemies.Reset();
    for (const TWeakObjectPtr<ACactusEnemy>& Enemy : Previous)
        if (Enemy.IsValid())
        {
            Enemy->OnDestroyed.RemoveDynamic(this, &ACactusEnemySpawner::OnManagedEnemyDestroyed);
            Enemy->Destroy();
        }
    RuntimeWaves.Reset();
    WaveIndex = -1;
    EntryIndex = SpawnedInEntry = RemainingToSpawn = SpawnPointCursor = 0;
    BlockedSince = -1;
    LastError.Reset();
    State = ECactusWaveState::Idle;
}

void ACactusEnemySpawner::QueueNextWave()
{
    if (WaveIndex + 1 >= RuntimeWaves.Num())
    {
        State = ECactusWaveState::Completed;
        OnWavesCompleted.Broadcast();
        return;
    }
    ++WaveIndex;
    EntryIndex = SpawnedInEntry = RemainingToSpawn = 0;
    BlockedSince = -1;
    for (const FCactusEnemySpawnEntry& Entry : RuntimeWaves[WaveIndex].Enemies) RemainingToSpawn += Entry.Count;
    State = ECactusWaveState::Waiting;
    GetWorldTimerManager().SetTimer(WaveTimer, this, &ACactusEnemySpawner::BeginWave,
        FMath::Max(RuntimeWaves[WaveIndex].DelayBeforeWave, 0.01f), false);
}

void ACactusEnemySpawner::BeginWave()
{
    if (State != ECactusWaveState::Waiting) return;
    State = ECactusWaveState::Spawning;
    SpawnNextBatch();
}

void ACactusEnemySpawner::ScheduleSpawn(float Delay)
{
    GetWorldTimerManager().SetTimer(WaveTimer, this, &ACactusEnemySpawner::SpawnNextBatch, FMath::Max(Delay, 0.01f), false);
}

ACactusEnemy* ACactusEnemySpawner::SpawnFromEntry(const FCactusEnemySpawnEntry& Entry)
{
    UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!Navigation) return nullptr;
    const ACactusEnemy* Defaults = Entry.EnemyClass->GetDefaultObject<ACactusEnemy>();
    const float HalfHeight = Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const float Radius = Defaults->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float Spacing = FMath::Max(Radius * 2 + 12, 20.0f);
    const float PlacementRadius = Entry.SpawnRadiusOverride >= 0 ? Entry.SpawnRadiusOverride : SpawnRadius;
    const bool bExactPoint = Entry.SpawnRadiusOverride == 0;
    const int32 Rings = FMath::FloorToInt(PlacementRadius / Spacing);
    const int32 Side = Rings * 2 + 1;
    const int32 Slots = Side * Side;
    const FNavAgentProperties& Agent = Defaults->GetCharacterMovement()->GetNavAgentPropertiesRef();
    const int32 PointCount = Entry.SpawnPointIndices.IsEmpty() ? SpawnPoints.Num() : Entry.SpawnPointIndices.Num();
    for (int32 Attempt = 0; Attempt < PointCount * Slots; ++Attempt)
    {
        const int32 Cursor = (SpawnPointCursor + Attempt) % (PointCount * Slots);
        const int32 Point = Cursor % PointCount;
        const int32 Slot = Cursor / PointCount;
        // 中心を先に使い、以後はCapsule間隔の格子へ順に分散する。
        const int32 Grid = Slot == 0 ? Slots / 2 : (Slot == Slots / 2 ? 0 : Slot);
        const FVector Offset((Grid % Side - Rings) * Spacing, (Grid / Side - Rings) * Spacing, 0);
        if (Offset.SizeSquared() > FMath::Square(PlacementRadius)) continue;
        const int32 Index = Entry.SpawnPointIndices.IsEmpty() ? Point : Entry.SpawnPointIndices[Point];
        if (!SpawnPoints.IsValidIndex(Index) || !IsValid(SpawnPoints[Index])) continue;
        FNavLocation Location;
        const FVector Candidate = SpawnPoints[Index]->GetActorLocation() + Offset;
        const FVector Extent = bExactPoint ? FVector(1,1,250)
            : PlacementRadius == 0 ? FVector(150,150,250) : FVector(Spacing * 0.45f, Spacing * 0.45f, 250);
        if (!Navigation->ProjectPointToNavigation(Candidate, Location, Extent, &Agent)) continue;
        if (bExactPoint && FVector::DistSquared2D(Location.Location,Candidate) > 1) continue;
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
        // 固定地点はTargetPointを足元として使い、NavMeshの高さや探索位置へずらさない。
        const FVector Position = (bExactPoint ? Candidate : Location.Location) + FVector(0,0,HalfHeight + 2);
        const UCapsuleComponent* Capsule = Defaults->GetCapsuleComponent();
        FCollisionQueryParams Query(SCENE_QUERY_STAT(CactusSpawn), false, this);
        const FCollisionResponseParams Responses(Capsule->GetCollisionResponseToChannels());
        if (GetWorld()->OverlapBlockingTestByChannel(Position, FQuat::Identity, Capsule->GetCollisionObjectType(),
            FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), HalfHeight), Query, Responses)) continue;
        ACactusEnemy* Enemy = GetWorld()->SpawnActor<ACactusEnemy>(Entry.EnemyClass, Position, SpawnPoints[Index]->GetActorRotation(), Parameters);
        if (IsValid(Enemy) && !Enemy->IsActorBeingDestroyed())
        {
            SpawnPointCursor = Cursor + 1;
            return Enemy;
        }
    }
    return nullptr;
}

void ACactusEnemySpawner::SpawnNextBatch()
{
    if (State != ECactusWaveState::Spawning) return;
    float Delay = 0;
    for (int32 Count = 0; Count < RuntimeWaves[WaveIndex].SpawnBatchSize && RemainingToSpawn > 0; ++Count)
    {
        if (AliveEnemies.Num() >= AliveLimit) { ScheduleSpawn(0.2f); return; }
        const FCactusEnemySpawnEntry& Entry = RuntimeWaves[WaveIndex].Enemies[EntryIndex];
        ACactusEnemy* Enemy = SpawnFromEntry(Entry);
        if (!Enemy)
        {
            const double Now = GetWorld()->GetTimeSeconds();
            if (BlockedSince < 0) BlockedSince = Now;
            if (Now - BlockedSince >= FMath::Max(BlockedSpawnTimeout, 0.1f)) Fail(TEXT("No free navigable spawn point. Check points, collision and NavMesh."));
            else ScheduleSpawn(0.25f);
            return; // 失敗した生成を出現済みとして数えない。
        }
        BlockedSince = -1;
        Enemy->SpawnSource = this; // PawnのOwnerはAIControllerが使うため、生成元は別の弱参照。
        AliveEnemies.Add(Enemy);
        Enemy->OnDestroyed.AddDynamic(this, &ACactusEnemySpawner::OnManagedEnemyDestroyed);
        --RemainingToSpawn;
        ++SpawnedInEntry;
        Delay = Entry.SpawnInterval;
        if (SpawnedInEntry >= Entry.Count) { ++EntryIndex; SpawnedInEntry = 0; }
    }
    if (RemainingToSpawn == 0)
    {
        State = ECactusWaveState::WaitingForClear;
        CheckWaveClear();
    }
    else ScheduleSpawn(Delay);
}

void ACactusEnemySpawner::OnManagedEnemyDestroyed(AActor* Actor)
{
    AliveEnemies.Remove(Cast<ACactusEnemy>(Actor));
    CheckWaveClear();
}

void ACactusEnemySpawner::CheckWaveClear()
{
    // 生成途中に敵が0体になっても、未出現分がある間は次へ進めない。
    if (State == ECactusWaveState::WaitingForClear && RemainingToSpawn == 0 && AliveEnemies.IsEmpty()) QueueNextWave();
}
