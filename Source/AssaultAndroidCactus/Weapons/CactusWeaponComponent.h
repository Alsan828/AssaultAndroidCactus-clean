#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CactusWeaponComponent.generated.h"

class ACactusProjectile;
class UNiagaraComponent;
class UNiagaraSystem;
DECLARE_MULTICAST_DELEGATE_OneParam(FCactusProjectileFired, ACactusProjectile*);

UENUM(BlueprintType)
enum class ECactusAttackMode : uint8
{
    Primary,
    Flamethrower
};

// 入力はCharacter、発射間隔と弾の生成はこのComponentが担当する。
UCLASS(ClassGroup = (Cactus), meta = (BlueprintSpawnableComponent))
class ASSAULTANDROIDCACTUS_API UCactusWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCactusWeaponComponent();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void StartFiring();

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void StopFiring();

    UFUNCTION(BlueprintPure, Category = "Weapon")
    bool IsFiring() const { return bTriggerHeld; }

    UFUNCTION(BlueprintPure, Category = "Weapon")
    ECactusAttackMode GetAttackMode() const { return AttackMode; }

    UFUNCTION(BlueprintPure, Category = "Weapon|Flamethrower")
    bool IsFlameActive() const { return bFlameActive; }

    UNiagaraComponent* GetFlameComponent() const { return FlameComponent; }

    // 入力・無敵・切替クールダウンはPlayer側で管理する。
    void ToggleAttackMode();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Flamethrower", meta = (ClampMin = "0.02", Units = "s"))
    float FlameDamageInterval = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Flamethrower", meta = (ClampMin = "0"))
    float FlameDamage = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Flamethrower", meta = (ClampMin = "1", ClampMax = "5000", Units = "cm"))
    float FlameRange = 350.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Flamethrower", meta = (ClampMin = "0", ClampMax = "60", Units = "deg"))
    float FlameHalfAngle = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Flamethrower", meta = (ClampMin = "1", Units = "cm"))
    float FlameStartRadius = 18.0f;

    // 一人称の銃口を視線の右下へ移す。通常弾には適用しない。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Flamethrower", meta = (Units = "cm"))
    FVector FirstPersonFlameOffset = FVector(0,28,-18);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Flamethrower|Effects")
    TObjectPtr<UNiagaraSystem> FlameEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Flamethrower|Effects")
    FVector FlameEffectScale = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Flamethrower|Effects")
    FRotator FlameEffectRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Flamethrower|Effects", meta = (ClampMin = "1", Units = "cm"))
    float FlameEffectReferenceLength = 350.0f;

    // 発射成功の通知。SEやエフェクトなどの接続先。
    FCactusProjectileFired OnProjectileFired;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Pool", meta = (ClampMin = "0", ClampMax = "512"))
    int32 InitialPoolSize = 64;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.02", Units = "s"))
    float FireInterval = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1.0", Units = "cm/s"))
    float ProjectileSpeed = 1800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.0"))
    float Damage = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.05", Units = "s"))
    float ProjectileLifetime = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    TSubclassOf<ACactusProjectile> ProjectileClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Muzzle", meta = (ClampMin = "0.0", Units = "cm"))
    float MuzzleDistance = 65.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Muzzle", meta = (Units = "cm"))
    float TopDownMuzzleHeight = 30.0f;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    void FireWhenReady();
    void FireProjectile();
    void FireInDirection(const FVector& Origin, const FVector& Direction, float Speed, float ShotDamage);
    bool CanFire() const;
    bool GetFireFrame(FVector& Origin, FVector& Direction) const;
    bool GetFlameFrame(FVector& Start, FVector& Direction, float& Length) const;
    void ApplyFlameDamage();
    void UpdateFlameVisual();
    void StopFlameVisual();

    UPROPERTY(Transient)
    TObjectPtr<UNiagaraComponent> FlameComponent;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
    ECactusAttackMode AttackMode = ECactusAttackMode::Primary;

    bool bTriggerHeld = false;
    bool bFlameActive = false;
    double NextShotTime = 0.0;
    FTimerHandle FireTimer;
};
