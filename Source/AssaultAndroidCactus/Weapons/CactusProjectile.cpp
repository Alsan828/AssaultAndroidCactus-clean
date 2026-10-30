#include "Weapons/CactusProjectile.h"
#include "Weapons/CactusProjectilePool.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"

ACactusProjectile::ACactusProjectile()
{
    PrimaryActorTick.bCanEverTick = false;
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitSphereRadius(6.0f);
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Collision->SetCollisionObjectType(ECC_GameTraceChannel1);
    Collision->SetCollisionResponseToAllChannels(ECR_Block);
    // 弾同士と照準用のレイには反応しない。
    Collision->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    Collision->SetGenerateOverlapEvents(false);
    Collision->OnComponentHit.AddDynamic(this, &ACactusProjectile::OnHit);

    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(Collision);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetRelativeScale3D(FVector(0.12f));
    Visual->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded()) Visual->SetStaticMesh(Sphere.Object);

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->SetUpdatedComponent(Collision);
    Movement->SetAutoActivate(false);
    Movement->ProjectileGravityScale = 0.0f;
    Movement->bRotationFollowsVelocity = true;
    Movement->bShouldBounce = false;
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    Movement->SetComponentTickEnabled(false);
}

float ACactusProjectile::GetCollisionRadius() const
{
    return Collision->GetScaledSphereRadius();
}

void ACactusProjectile::Launch(FVector Direction, float Speed, float Damage, float Lifetime, bool bEnemyShot)
{
    Direction = Direction.GetSafeNormal();
    if (Direction.ContainsNaN() || Direction.IsNearlyZero())
    {
        ReturnToPool();
        return;
    }
    ++ShotGeneration;
    SetLifeSpan(0.0f);
    GetWorldTimerManager().ClearTimer(ExpiryTimer);
    bHitProcessed = false;
    bShotActive = true;
    ShotDamage = FMath::Max(Damage, 0.0f);
    // 敵弾は敵を通過し、プレイヤー弾はプレイヤーを通過する。
    Collision->SetCollisionObjectType(bEnemyShot ? ECC_GameTraceChannel2 : ECC_GameTraceChannel1);
    Collision->ClearMoveIgnoreActors();
    Collision->IgnoreActorWhenMoving(GetOwner(), true);
    if (GetInstigator()) Collision->IgnoreActorWhenMoving(GetInstigator(), true);
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    // 命中時にUEがUpdatedComponentを解除するため、毎回結び直す。
    Movement->SetUpdatedComponent(Collision);
    Movement->bSimulationEnabled = true;
    Movement->ClearPendingForce(true);
    Movement->PreviousHitTime = 1.0f;
    Movement->PreviousHitNormal = FVector::ZeroVector;
    Movement->ResetInterpolation();
    Movement->InitialSpeed = FMath::Max(Speed, 1.0f);
    Movement->MaxSpeed = Movement->InitialSpeed;
    Movement->Velocity = Direction * Movement->InitialSpeed;
    Movement->Activate(true);
    Movement->SetComponentTickEnabled(true);
    GetWorldTimerManager().SetTimer(ExpiryTimer, this, &ACactusProjectile::ReturnToPool, FMath::Max(Lifetime, 0.05f), false);
}

void ACactusProjectile::DeactivateForPool()
{
    ++ShotGeneration;
    bShotActive = false;
    bHitProcessed = false;
    ShotDamage = 0;
    SetLifeSpan(0.0f);
    GetWorldTimerManager().ClearTimer(ExpiryTimer);
    SetActorEnableCollision(false);
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Collision->ClearMoveIgnoreActors();
    Movement->StopMovementImmediately();
    Collision->SetCollisionObjectType(ECC_GameTraceChannel1);
    Movement->ClearPendingForce(true);
    Movement->bSimulationEnabled = false;
    Movement->Deactivate();
    Movement->SetUpdatedComponent(nullptr);
    Movement->SetComponentTickEnabled(false);
    SetActorTickEnabled(false);
    SetActorHiddenInGame(true);
    SetOwner(nullptr);
    SetInstigator(nullptr);
}

void ACactusProjectile::ReturnToPool()
{
    if (UCactusProjectilePool* OwningPool = Pool.Get())
        OwningPool->Release(this);
    else
        Destroy(); // プール外から直接生成した弾だけの互換処理。
}

void ACactusProjectile::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(ExpiryTimer);
    Super::EndPlay(Reason);
}

void ACactusProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
    if (!bShotActive || bHitProcessed || OtherActor == GetOwner() || OtherActor == GetInstigator()) return;
    const uint32 Generation = ShotGeneration;
    bHitProcessed = true;
    bShotActive = false;
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (IsValid(OtherActor))
    {
        UGameplayStatics::ApplyPointDamage(OtherActor, ShotDamage, Movement->Velocity.GetSafeNormal(), Hit, GetInstigatorController(), GetOwner(), nullptr);
    }
    // ダメージ通知中に局の初期化が起きても、新しい発射状態を返却しない。
    if (Generation == ShotGeneration) ReturnToPool();
}
