#include "Weapons/CactusProjectilePool.h"
#include "Weapons/CactusProjectile.h"
#include "Engine/World.h"

bool UCactusProjectilePool::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

int32 UCactusProjectilePool::GetCapacityPerClass() const
{
    return FMath::Clamp(MaxProjectilesPerClass, 1, 4096);
}

void UCactusProjectilePool::PruneInvalid(FCactusProjectilePoolBucket& Bucket)
{
    Bucket.Available.RemoveAllSwap([](const TObjectPtr<ACactusProjectile>& Projectile) { return !IsValid(Projectile); });
    for (auto It = Bucket.InUse.CreateIterator(); It; ++It)
        if (!IsValid(*It)) It.RemoveCurrent();
}

ACactusProjectile* UCactusProjectilePool::CreateIdle(TSubclassOf<ACactusProjectile> ProjectileClass)
{
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ACactusProjectile* Projectile = GetWorld()->SpawnActor<ACactusProjectile>(ProjectileClass, FTransform::Identity, Spawn);
    if (Projectile)
    {
        Projectile->Pool = this;
        Projectile->DeactivateForPool();
    }
    return Projectile;
}

void UCactusProjectilePool::Prewarm(TSubclassOf<ACactusProjectile> ProjectileClass, int32 Count)
{
    if (bShuttingDown || !ProjectileClass || ProjectileClass->HasAnyClassFlags(CLASS_Abstract)) return;
    FCactusProjectilePoolBucket& Bucket = Buckets.FindOrAdd(ProjectileClass);
    PruneInvalid(Bucket);
    const int32 Target = FMath::Clamp(Count, 0, GetCapacityPerClass());
    while (Bucket.Available.Num() + Bucket.InUse.Num() < Target)
    {
        ACactusProjectile* Projectile = CreateIdle(ProjectileClass);
        if (!Projectile) break;
        Bucket.Available.Add(Projectile);
    }
}

ACactusProjectile* UCactusProjectilePool::Acquire(TSubclassOf<ACactusProjectile> ProjectileClass, const FTransform& Transform, AActor* Owner, APawn* Instigator)
{
    if (bShuttingDown || !ProjectileClass || ProjectileClass->HasAnyClassFlags(CLASS_Abstract) || Transform.ContainsNaN()) return nullptr;
    FCactusProjectilePoolBucket& Bucket = Buckets.FindOrAdd(ProjectileClass);
    ACactusProjectile* Projectile = nullptr;
    while (!Bucket.Available.IsEmpty() && !IsValid(Projectile))
        Projectile = Bucket.Available.Pop(EAllowShrinking::No);
    if (!IsValid(Projectile)) Projectile = nullptr;
    if (!Projectile)
    {
        // 通常の取得・返却はO(1)。外部で破棄された参照の整理は上限時だけ行う。
        if (Bucket.InUse.Num() >= GetCapacityPerClass()) PruneInvalid(Bucket);
        if (Bucket.InUse.Num() < GetCapacityPerClass()) Projectile = CreateIdle(ProjectileClass);
    }
    if (!Projectile) return nullptr; // 使用中の弾を奪わず、上限では今回の発射を見送る。

    Bucket.InUse.Add(Projectile);
    Projectile->DeactivateForPool();
    Projectile->SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
    Projectile->SetOwner(Owner);
    Projectile->SetInstigator(Instigator);
    return Projectile;
}

void UCactusProjectilePool::Release(ACactusProjectile* Projectile)
{
    if (bShuttingDown || !IsValid(Projectile) || Projectile->Pool.Get() != this) return;
    FCactusProjectilePoolBucket* Bucket = Buckets.Find(Projectile->GetClass());
    if (!Bucket || Bucket->InUse.Remove(Projectile) == 0) return; // 二重返却を除外する。
    Projectile->DeactivateForPool();
    Bucket->Available.Add(Projectile);
}

void UCactusProjectilePool::ResetPool()
{
    if (bShuttingDown) return;
    TArray<ACactusProjectile*> Active;
    for (const auto& Entry : Buckets)
        for (ACactusProjectile* Projectile : Entry.Value.InUse) Active.Add(Projectile);
    for (ACactusProjectile* Projectile : Active) Release(Projectile);
}

int32 UCactusProjectilePool::GetTotalCount(TSubclassOf<ACactusProjectile> ProjectileClass) const
{
    const FCactusProjectilePoolBucket* Bucket = Buckets.Find(ProjectileClass);
    if (!Bucket) return 0;
    int32 Count = GetActiveCount(ProjectileClass);
    for (const ACactusProjectile* Projectile : Bucket->Available) if (IsValid(Projectile)) ++Count;
    return Count;
}

int32 UCactusProjectilePool::GetActiveCount(TSubclassOf<ACactusProjectile> ProjectileClass) const
{
    const FCactusProjectilePoolBucket* Bucket = Buckets.Find(ProjectileClass);
    if (!Bucket) return 0;
    int32 Count = 0;
    for (const ACactusProjectile* Projectile : Bucket->InUse) if (IsValid(Projectile)) ++Count;
    return Count;
}

void UCactusProjectilePool::Deinitialize()
{
    if (bShuttingDown) return;
    bShuttingDown = true;
    for (auto& Entry : Buckets)
    {
        TArray<TObjectPtr<ACactusProjectile>> All = Entry.Value.Available;
        All.Append(Entry.Value.InUse.Array());
        for (ACactusProjectile* Projectile : All)
        {
            if (!IsValid(Projectile)) continue;
            Projectile->DeactivateForPool();
            Projectile->Pool.Reset();
            Projectile->Destroy();
        }
    }
    Buckets.Empty();
    Super::Deinitialize();
}
