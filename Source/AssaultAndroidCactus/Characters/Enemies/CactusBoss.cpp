#include "Characters/Enemies/CactusBoss.h"
#include "Characters/CactusPlayerCharacter.h"
#include "Game/CactusGameMode.h"
#include "Weapons/CactusProjectile.h"
#include "Weapons/CactusProjectilePool.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ACactusBoss::ACactusBoss()
{
    AutoPossessAI = EAutoPossessAI::Disabled;
    AIControllerClass = nullptr;
    ContactDamage = 0;
    MaxHealth = 50;
    ProjectileClass = ACactusProjectile::StaticClass();
    GetCapsuleComponent()->InitCapsuleSize(70,144);
    GetMesh()->SetRelativeLocation(FVector(0,0,-144));
    GetMesh()->SetRelativeScale3D(FVector(1.5f));
    GetCharacterMovement()->MaxWalkSpeed = 0;
    GetCharacterMovement()->bOrientRotationToMovement = false;
}

void ACactusBoss::BeginPlay()
{
    Super::BeginPlay();
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->SetComponentTickEnabled(false);
    RingAngle = 0;
    if (UCactusProjectilePool* Pool = GetWorld()->GetSubsystem<UCactusProjectilePool>())
        Pool->Prewarm(ProjectileClass,InitialPoolSize);
    const float Interval = FMath::IsFinite(FireInterval) ? FMath::Max(FireInterval,0.1f) : 1.5f;
    const float Delay = FMath::IsFinite(InitialFireDelay) ? FMath::Max(InitialFireDelay,0.01f) : Interval;
    GetWorldTimerManager().SetTimer(BarrageTimer,this,&ACactusBoss::FireRing,Interval,true,Delay);
}

void ACactusBoss::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(BarrageTimer);
    Super::EndPlay(Reason);
}

void ACactusBoss::FireRing()
{
    const ACactusGameMode* Mode = GetWorld()->GetAuthGameMode<ACactusGameMode>();
    if (IsDead() || IsActorBeingDestroyed() || (Mode && (Mode->IsStageFinished() || Mode->GetStageOutcome()!=ECactusStageOutcome::InProgress)))
    {
        GetWorldTimerManager().ClearTimer(BarrageTimer);
        return;
    }
    const ACactusPlayerCharacter* Player = Cast<ACactusPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    UCactusProjectilePool* Pool = GetWorld()->GetSubsystem<UCactusProjectilePool>();
    if (!IsValid(Player) || Player->IsDowned() || !Pool || !ProjectileClass || ProjectileClass->HasAnyClassFlags(CLASS_Abstract)) return;
    const float Height = FMath::IsFinite(MuzzleHeight) ? FMath::Max(MuzzleHeight,0.0f) : 120.0f;
    const float Padding = FMath::IsFinite(MuzzlePadding) ? FMath::Clamp(MuzzlePadding,0.0f,100.0f) : 12.0f;
    const float Speed = FMath::IsFinite(ProjectileSpeed) ? FMath::Max(ProjectileSpeed,1.0f) : 450.0f;
    const float Damage = FMath::IsFinite(ProjectileDamage) ? FMath::Max(ProjectileDamage,0.0f) : 1.0f;
    const float Lifetime = FMath::IsFinite(ProjectileLifetime) ? FMath::Max(ProjectileLifetime,0.1f) : 4.0f;
    const float Radius = ProjectileClass->GetDefaultObject<ACactusProjectile>()->GetCollisionRadius();
    const FVector Origin = GetActorLocation()+FVector(0,0,Height-GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    const float Distance = GetCapsuleComponent()->GetScaledCapsuleRadius()+Radius+Padding;
    const int32 Count = FMath::Clamp(BulletCount,1,128);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CactusBarrageMuzzle),false,this);
    for (int32 Index=0;Index<Count;++Index)
    {
        const float Angle = FMath::DegreesToRadians(GetActorRotation().Yaw+RingAngle+360.0f*Index/Count);
        const FVector Direction(FMath::Cos(Angle),FMath::Sin(Angle),0);
        FVector Muzzle = Origin+Direction*Distance;
        FHitResult Hit;
        // 壁際や近いPlayerの向こう側へ弾を生成しない。
        if (GetWorld()->SweepSingleByChannel(Hit,Origin,Muzzle,FQuat::Identity,ECC_GameTraceChannel2,FCollisionShape::MakeSphere(Radius),Query))
        {
            if (Hit.bStartPenetrating) continue;
            Muzzle = Hit.Location-Direction*2;
        }
        if (ACactusProjectile* Bullet = Pool->Acquire(ProjectileClass,FTransform(Direction.Rotation(),Muzzle),this,this))
            Bullet->Launch(Direction,Speed,Damage,Lifetime,true);
    }
    if (FMath::IsFinite(RingAngleStep)) RingAngle = FMath::Fmod(RingAngle+FMath::Fmod(RingAngleStep,360.0f),360.0f);
}
