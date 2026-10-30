#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CactusProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UPrimitiveComponent;
class UCactusProjectilePool;

// UE標準の移動とSweep衝突。命中・寿命でプールへ返す。
UCLASS()
class ASSAULTANDROIDCACTUS_API ACactusProjectile : public AActor
{
    GENERATED_BODY()

public:
    ACactusProjectile();
    void Launch(FVector Direction, float Speed, float Damage, float Lifetime, bool bEnemyShot = false);
    float GetCollisionRadius() const;

    UFUNCTION(BlueprintCallable, Category = "Projectile")
    void ReturnToPool();

    UFUNCTION(BlueprintPure, Category = "Projectile")
    bool IsShotActive() const { return bShotActive; }

protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
    TObjectPtr<UStaticMeshComponent> Visual;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
    TObjectPtr<UProjectileMovementComponent> Movement;

private:
    friend class UCactusProjectilePool;
    void DeactivateForPool();

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

    float ShotDamage = 0.0f;
    bool bHitProcessed = false;
    bool bShotActive = false;
    uint32 ShotGeneration = 0;
    FTimerHandle ExpiryTimer;
    TWeakObjectPtr<UCactusProjectilePool> Pool;
};
