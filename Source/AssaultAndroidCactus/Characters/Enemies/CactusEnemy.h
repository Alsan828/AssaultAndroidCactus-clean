#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CactusEnemy.generated.h"

class UCapsuleComponent;
class UPrimitiveComponent;
class ACactusPlayerCharacter;
class ACactusEnemySpawner;
class ACactusItemPickup;

// AIControllerがNavMeshで追跡する基本の敵。体力が0になると消える。
UCLASS()
class ASSAULTANDROIDCACTUS_API ACactusEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    ACactusEnemy();
    virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    UFUNCTION(BlueprintPure, Category = "Enemy|Health")
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintPure, Category = "Enemy|Health")
    bool IsDead() const { return bDead; }

    UFUNCTION(BlueprintPure, Category = "Enemy|Chase")
    float GetStoppingDistance() const { return StoppingDistance; }

    // 射撃可能な位置ならtrueを返し、AIControllerの追跡を止める。
    virtual bool TryAttackPlayer(ACactusPlayerCharacter* Player) { return false; }

    UFUNCTION(BlueprintPure, Category = "Enemy|Spawn")
    ACactusEnemySpawner* GetSpawnSource() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Contact")
    TObjectPtr<UCapsuleComponent> ContactArea;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Contact", meta = (ClampMin = "0.0"))
    float ContactDamage = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Contact", meta = (ClampMin = "0.1", Units = "s"))
    float ContactInterval = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Health", meta = (ClampMin = "1.0"))
    float MaxHealth = 10.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Health")
    float Health = 10.0f;

    // カプセル中心間の距離。近づいたら停止し、離れたら追跡を再開する。
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Chase", meta = (ClampMin = "0.0", Units = "cm"))
    float StoppingDistance = 120.0f;

    // 未設定ならドロップなし。効果・得点・外見は道具BPで設定する。
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drops")
    TSubclassOf<ACactusItemPickup> DropItemClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drops", meta = (ClampMin = "0", ClampMax = "1"))
    float DropChance = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drops", meta = (ClampMin = "0", Units = "cm"))
    float DropHeight = 30.0f;

private:
    friend class ACactusEnemySpawner;
    TWeakObjectPtr<ACactusEnemySpawner> SpawnSource;
    UFUNCTION()
    void OnContactBegin(UPrimitiveComponent* Component, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
        int32 BodyIndex, bool bFromSweep, const FHitResult& Hit);

    UFUNCTION()
    void OnContactEnd(UPrimitiveComponent* Component, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 BodyIndex);

    void ApplyContactDamage();
    void SpawnDeathDrop();
    FTimerHandle ContactTimer;
    TWeakObjectPtr<ACactusPlayerCharacter> ContactPlayer;
    double NextContactTime = 0;
    bool bDead = false;
};
