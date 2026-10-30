#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CactusProjectilePool.generated.h"

class ACactusProjectile;
class APawn;

USTRUCT()
struct FCactusProjectilePoolBucket
{
    GENERATED_BODY()

    UPROPERTY(Transient)
    TArray<TObjectPtr<ACactusProjectile>> Available;

    UPROPERTY(Transient)
    TSet<TObjectPtr<ACactusProjectile>> InUse;
};

// Worldごとに1個。弾クラス別に待機・使用中を管理する。
UCLASS(Config = Game)
class ASSAULTANDROIDCACTUS_API UCactusProjectilePool : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void Prewarm(TSubclassOf<ACactusProjectile> ProjectileClass, int32 Count);
    ACactusProjectile* Acquire(TSubclassOf<ACactusProjectile> ProjectileClass, const FTransform& Transform, AActor* Owner, APawn* Instigator);
    void Release(ACactusProjectile* Projectile);

    UFUNCTION(BlueprintCallable, Category = "Projectile Pool")
    void ResetPool();

    UFUNCTION(BlueprintPure, Category = "Projectile Pool")
    int32 GetTotalCount(TSubclassOf<ACactusProjectile> ProjectileClass) const;

    UFUNCTION(BlueprintPure, Category = "Projectile Pool")
    int32 GetActiveCount(TSubclassOf<ACactusProjectile> ProjectileClass) const;

    int32 GetCapacityPerClass() const;
    virtual void Deinitialize() override;

protected:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

    UPROPERTY(Config)
    int32 MaxProjectilesPerClass = 512;

private:
    ACactusProjectile* CreateIdle(TSubclassOf<ACactusProjectile> ProjectileClass);
    void PruneInvalid(FCactusProjectilePoolBucket& Bucket);

    UPROPERTY(Transient)
    TMap<TSubclassOf<ACactusProjectile>, FCactusProjectilePoolBucket> Buckets;

    bool bShuttingDown = false;
};
