#pragma once

#include "CoreMinimal.h"
#include "Characters/Enemies/CactusEnemy.h"
#include "CactusRangedEnemy.generated.h"

class ACactusProjectile;

// 射程内で弾の通り道が開いていれば停止して射撃。遮られたらNavMeshで追う。
UCLASS()
class ASSAULTANDROIDCACTUS_API ACactusRangedEnemy : public ACactusEnemy
{
    GENERATED_BODY()

public:
    ACactusRangedEnemy();
    virtual bool TryAttackPlayer(ACactusPlayerCharacter* Player) override;

    UFUNCTION(BlueprintPure, Category = "Enemy|Shooting")
    bool CanShootAt(const ACactusPlayerCharacter* Player) const;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Shooting")
    TSubclassOf<ACactusProjectile> ProjectileClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Shooting", meta = (ClampMin = "100.0", Units = "cm"))
    float ShootingRange = 900.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Shooting", meta = (ClampMin = "0.1", Units = "s"))
    float FireInterval = 1.0f;

    // falseなら追跡中も撃ち、近距離では接触攻撃を優先する。
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Shooting")
    bool bStopToShoot = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Shooting", meta = (ClampMin = "0.0"))
    float ProjectileDamage = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Shooting", meta = (ClampMin = "1.0", Units = "cm/s"))
    float ProjectileSpeed = 800.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Shooting", meta = (ClampMin = "0.1", Units = "s"))
    float ProjectileLifetime = 3.0f;

private:
    // 射線判定と弾の生成で使う高さを揃える。
    static FVector GetShotPosition(const AActor* Actor);
    bool TraceShot(const ACactusPlayerCharacter* Player, FHitResult& Hit) const;
    double NextShotTime = 0;
};
