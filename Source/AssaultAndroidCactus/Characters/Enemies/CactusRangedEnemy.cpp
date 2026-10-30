#include "Characters/Enemies/CactusRangedEnemy.h"
#include "Characters/CactusPlayerCharacter.h"
#include "Weapons/CactusProjectile.h"
#include "Weapons/CactusProjectilePool.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

ACactusRangedEnemy::ACactusRangedEnemy()
{
    ProjectileClass = ACactusProjectile::StaticClass();
    ContactDamage = 0; // この敵は射撃を担当する。
}

void ACactusRangedEnemy::BeginPlay()
{
    Super::BeginPlay();
    if (UCactusProjectilePool* Pool = GetWorld()->GetSubsystem<UCactusProjectilePool>())
        Pool->Prewarm(ProjectileClass, 16);
}

bool ACactusRangedEnemy::CanShootAt(const ACactusPlayerCharacter* Player) const
{
    FHitResult Hit;
    return TraceShot(Player, Hit);
}

FVector ACactusRangedEnemy::GetShotPosition(const AActor* Actor)
{
    return Actor->GetActorLocation() + FVector(0, 0, 30);
}

bool ACactusRangedEnemy::TraceShot(const ACactusPlayerCharacter* Player, FHitResult& Hit) const
{
    if (!IsValid(Player) || Player->IsDowned() || Player->IsActorBeingDestroyed() || IsDead() || !ProjectileClass ||
        FVector::DistSquared(GetActorLocation(), Player->GetActorLocation()) > FMath::Square(FMath::Max(ShootingRange, 100.0f))) return false;
    const FVector Origin = GetShotPosition(this);
    const FVector Target = GetShotPosition(Player);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CactusEnemyShot), false, this);
    const float Radius = ProjectileClass->GetDefaultObject<ACactusProjectile>()->GetCollisionRadius();
    // 銃口までを含む球Sweepで、壁際や薄い遮蔽物の向こうへ発射しない。
    return GetWorld()->SweepSingleByChannel(Hit, Origin, Target, FQuat::Identity, ECC_GameTraceChannel2,
        FCollisionShape::MakeSphere(Radius), Query) && Hit.GetActor() == Player && !Hit.bStartPenetrating;
}

bool ACactusRangedEnemy::TryAttackPlayer(ACactusPlayerCharacter* Player)
{
    if (!bStopToShoot && IsValid(Player) &&
        FVector::DistSquared2D(GetActorLocation(), Player->GetActorLocation()) <= FMath::Square(FMath::Max(StoppingDistance, 0.0f)))
        return false;
    FHitResult Hit;
    if (!TraceShot(Player, Hit)) return false;
    const FVector Direction = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    if (bStopToShoot)
    {
        GetCharacterMovement()->StopMovementImmediately();
        SetActorRotation(FRotator(0, Direction.Rotation().Yaw, 0));
    }
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now >= NextShotTime)
    {
        // 近距離でも相手のCapsuleの内側へ弾を出さない。
        const FVector Muzzle = GetShotPosition(this) + Direction * FMath::Clamp(Hit.Distance - 2.0f, 0.0f, 65.0f);
        UCactusProjectilePool* Pool = GetWorld()->GetSubsystem<UCactusProjectilePool>();
        if (ACactusProjectile* Bullet = Pool ? Pool->Acquire(ProjectileClass, FTransform(Direction.Rotation(), Muzzle), this, this) : nullptr)
            Bullet->Launch(Direction, ProjectileSpeed, ProjectileDamage, ProjectileLifetime, true);
        NextShotTime = Now + FMath::Max(FireInterval, 0.1f);
    }
    return bStopToShoot; // 大型敵は射撃できてもNavMeshの追跡を続ける。
}
