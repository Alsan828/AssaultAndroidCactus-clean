#pragma once

#include "CoreMinimal.h"
#include "Characters/Enemies/CactusEnemy.h"
#include "CactusBoss.generated.h"

class ACactusProjectile;

// 定位置から水平方向へ円形弾を発射するBoss。
UCLASS()
class ASSAULTANDROIDCACTUS_API ACactusBoss : public ACactusEnemy
{
    GENERATED_BODY()

public:
    ACactusBoss();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Barrage")
    TSubclassOf<ACactusProjectile> ProjectileClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Barrage", meta = (ClampMin = "1", ClampMax = "128"))
    int32 BulletCount = 16;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Barrage", meta = (ClampMin = "0.1", Units = "s"))
    float FireInterval = 1.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Barrage", meta = (ClampMin = "0", Units = "s"))
    float InitialFireDelay = 1.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Barrage", meta = (ClampMin = "1", Units = "cm/s"))
    float ProjectileSpeed = 450.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Barrage", meta = (ClampMin = "0"))
    float ProjectileDamage = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Barrage", meta = (ClampMin = "0.1", Units = "s"))
    float ProjectileLifetime = 4.0f;

    // 次の一周を回す角度。0なら毎回同じ方向へ撃つ。
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Barrage", meta = (Units = "deg"))
    float RingAngleStep = 11.25f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Barrage", meta = (ClampMin = "0", Units = "cm"))
    float MuzzleHeight = 120.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Barrage", meta = (ClampMin = "0", Units = "cm"))
    float MuzzlePadding = 12.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Barrage|Pool", meta = (ClampMin = "0", ClampMax = "512"))
    int32 InitialPoolSize = 64;

private:
    void FireRing();
    FTimerHandle BarrageTimer;
    float RingAngle = 0;
};
