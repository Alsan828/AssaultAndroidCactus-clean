#include "Weapons/CactusWeaponComponent.h"
#include "Characters/CactusPlayerCharacter.h"
#include "Characters/Enemies/CactusEnemy.h"
#include "Game/CactusGameMode.h"
#include "Weapons/CactusProjectile.h"
#include "Weapons/CactusProjectilePool.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"
#include "TimerManager.h"

UCactusWeaponComponent::UCactusWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    ProjectileClass = ACactusProjectile::StaticClass();
}

void UCactusWeaponComponent::BeginPlay()
{
    Super::BeginPlay();
    if (UCactusProjectilePool* Pool = GetWorld()->GetSubsystem<UCactusProjectilePool>())
        Pool->Prewarm(ProjectileClass, InitialPoolSize);
    AddTickPrerequisiteActor(GetOwner()); // Characterの照準更新後に表示位置を追従する。
}

bool UCactusWeaponComponent::CanFire() const
{
    const ACactusPlayerCharacter* Player = Cast<ACactusPlayerCharacter>(GetOwner());
    if (!IsValid(Player) || Player->IsDowned() || !Player->GetController() || !GetWorld() || GetWorld()->IsPaused()) return false;
    const ACactusGameMode* Mode = GetWorld()->GetAuthGameMode<ACactusGameMode>();
    return !Mode || (!Mode->IsStageFinished() && Mode->GetStageOutcome() == ECactusStageOutcome::InProgress);
}

void UCactusWeaponComponent::StartFiring()
{
    if (bTriggerHeld || !CanFire()) return;
    bTriggerHeld = true;
    UpdateFlameVisual();
    FireWhenReady();
}

void UCactusWeaponComponent::StopFiring()
{
    bTriggerHeld = false;
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(FireTimer);
    StopFlameVisual();
}

void UCactusWeaponComponent::ToggleAttackMode()
{
    AttackMode = AttackMode == ECactusAttackMode::Primary ? ECactusAttackMode::Flamethrower : ECactusAttackMode::Primary;
    // 次弾の時刻と押下状態を保持し、切替で連射間隔を飛ばさない。
    UpdateFlameVisual();
}

void UCactusWeaponComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    StopFiring();
    if (FlameComponent) FlameComponent->DestroyComponent();
    FlameComponent = nullptr;
    Super::EndPlay(Reason);
}

void UCactusWeaponComponent::FireWhenReady()
{
    UWorld* World = GetWorld();
    if (!bTriggerHeld || !World) return;
    if (!CanFire())
    {
        StopFiring();
        return;
    }
    const double Now = World->GetTimeSeconds();
    if (Now >= NextShotTime)
    {
        if (AttackMode == ECactusAttackMode::Primary) FireProjectile();
        else ApplyFlameDamage();
        // 最後の撃破によるゲーム終了などが、ダメージ通知中に射撃を停止する場合がある。
        if (!bTriggerHeld || !CanFire()) { StopFiring(); return; }
        const float Interval = AttackMode == ECactusAttackMode::Primary ? FireInterval : FlameDamageInterval;
        NextShotTime = Now + (FMath::IsFinite(Interval) ? FMath::Max(Interval,0.02f) : 0.15f);
    }
    // 連打でも間隔を飛ばさない。大きな遅延後も一度に大量発射しない。
    const float Wait = FMath::Max(float(NextShotTime - Now), 0.001f);
    World->GetTimerManager().SetTimer(FireTimer, this, &UCactusWeaponComponent::FireWhenReady, Wait, false);
}

void UCactusWeaponComponent::FireProjectile()
{
    FVector Origin, Direction;
    if (ProjectileClass && GetFireFrame(Origin,Direction)) FireInDirection(Origin,Direction,ProjectileSpeed,Damage);
}

bool UCactusWeaponComponent::GetFireFrame(FVector& Origin, FVector& Direction) const
{
    const ACactusPlayerCharacter* Character = Cast<ACactusPlayerCharacter>(GetOwner());
    if (!IsValid(Character)) return false;
    Direction = Character->GetAimDirection().GetSafeNormal();
    if (Direction.ContainsNaN() || Direction.IsNearlyZero()) return false;
    Origin = Character->GetCameraMode() == ECactusCameraMode::FirstPerson
        ? Character->GetFirstPersonCamera()->GetComponentLocation()
        : Character->GetActorLocation() + FVector(0.0f, 0.0f, TopDownMuzzleHeight);
    return !Origin.ContainsNaN();
}

bool UCactusWeaponComponent::GetFlameFrame(FVector& Start, FVector& Direction, float& Length) const
{
    FVector Origin;
    if (!GetFireFrame(Origin,Direction)) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CactusFlameWall),false,GetOwner());
    FHitResult Hit;
    Start = Origin + Direction*FMath::Max(MuzzleDistance,0.0f);
    const ACactusPlayerCharacter* Player = Cast<ACactusPlayerCharacter>(GetOwner());
    if (Player->GetCameraMode() == ECactusCameraMode::FirstPerson && !FirstPersonFlameOffset.ContainsNaN())
        Start += Player->GetFirstPersonCamera()->GetComponentQuat().RotateVector(FirstPersonFlameOffset);
    // Pawn/道具はVisibilityを無視する。銃口が壁の先にある場合は噴射しない。
    if (GetWorld()->SweepSingleByChannel(Hit,Origin,Start,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(6.0f),Query)) return false;
    Length = FMath::IsFinite(FlameRange) ? FMath::Clamp(FlameRange,1.0f,5000.0f) : 350.0f;
    if (GetWorld()->LineTraceSingleByChannel(Hit,Start,Start+Direction*Length,ECC_Visibility,Query))
        Length = FMath::Max(0.0f,Hit.Distance-2.0f);
    return Length > 1.0f;
}

void UCactusWeaponComponent::ApplyFlameDamage()
{
    FVector Start, Direction;
    float Length;
    if (!GetFlameFrame(Start,Direction,Length) || !FMath::IsFinite(FlameDamage) || FlameDamage <= 0) return;
    const float Slope = FMath::Tan(FMath::DegreesToRadians(FMath::IsFinite(FlameHalfAngle) ? FMath::Clamp(FlameHalfAngle,0.0f,60.0f) : 15.0f));
    const float BaseRadius = FMath::IsFinite(FlameStartRadius) ? FMath::Max(FlameStartRadius,1.0f) : 18.0f;
    const float EndRadius = BaseRadius+Length*Slope;
    TArray<FOverlapResult> Overlaps;
    const float SearchRadius = FMath::Sqrt(FMath::Square(Length*0.5f)+FMath::Square(EndRadius));
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CactusFlame),false,GetOwner());
    GetWorld()->OverlapMultiByObjectType(Overlaps,Start+Direction*(Length*0.5f),FQuat::Identity,
        FCollisionObjectQueryParams(ECC_Pawn),FCollisionShape::MakeSphere(SearchRadius),Query);
    TSet<ACactusEnemy*> Processed;
    for (const FOverlapResult& Overlap : Overlaps)
    {
        if (!bTriggerHeld || !CanFire()) break;
        ACactusEnemy* Enemy = Cast<ACactusEnemy>(Overlap.GetActor());
        if (!IsValid(Enemy) || Enemy->IsDead() || Processed.Contains(Enemy)) continue;
        Processed.Add(Enemy);
        const UCapsuleComponent* Capsule = Enemy->GetCapsuleComponent();
        FVector Target = Enemy->GetActorLocation();
        const float CenterAlong = FVector::DotProduct(Target-Start,Direction);
        const float HalfSegment = FMath::Max(Capsule->GetScaledCapsuleHalfHeight()-Capsule->GetScaledCapsuleRadius(),0.0f);
        // カプセルの縦軸で照準に近い高さを選び、一人称で頭付近へ向けた炎も判定する。
        Target.Z = FMath::Clamp((Start+Direction*CenterAlong).Z,Target.Z-HalfSegment,Target.Z+HalfSegment);
        const FVector Offset = Target-Start;
        const float Along = FVector::DotProduct(Offset,Direction);
        const float Radius = Capsule->GetScaledCapsuleRadius();
        if (Along < 0 || Along > Length+Radius) continue;
        const float AllowedRadius = BaseRadius+FMath::Clamp(Along,0.0f,Length)*Slope+Radius;
        if ((Offset-Direction*Along).SizeSquared() > FMath::Square(AllowedRadius)) continue;
        FHitResult Cover;
        if (GetWorld()->LineTraceSingleByChannel(Cover,Start,Target,ECC_Visibility,Query)) continue;
        UGameplayStatics::ApplyDamage(Enemy,FlameDamage,Cast<APawn>(GetOwner())->GetController(),GetOwner(),nullptr);
    }
}

void UCactusWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime,TickType,ThisTickFunction);
    if (!CanFire()) { StopFiring(); return; }
    UpdateFlameVisual();
}

void UCactusWeaponComponent::UpdateFlameVisual()
{
    FVector Start, Direction;
    float Length;
    if (!bTriggerHeld || AttackMode != ECactusAttackMode::Flamethrower || !CanFire())
    {
        StopFlameVisual();
        return;
    }
    SetComponentTickEnabled(true);
    bFlameActive = GetFlameFrame(Start,Direction,Length);
    if (!bFlameActive || !FlameEffect)
    {
        if (FlameComponent) FlameComponent->DeactivateImmediate();
        return;
    }
    if (!FlameComponent)
    {
        FlameComponent = NewObject<UNiagaraComponent>(GetOwner(),TEXT("FlameEffect"));
        FlameComponent->SetAutoActivate(false);
        FlameComponent->SetCanEverAffectNavigation(false);
        FlameComponent->SetupAttachment(Cast<ACactusPlayerCharacter>(GetOwner())->GetRootComponent());
        FlameComponent->RegisterComponent();
    }
    if (FlameComponent->GetAsset() != FlameEffect) FlameComponent->SetAsset(FlameEffect);
    FlameComponent->SetWorldLocationAndRotation(Start,(Direction.Rotation().Quaternion()*FlameEffectRotation.Quaternion()));
    const float Reference = FMath::IsFinite(FlameEffectReferenceLength) ? FMath::Max(FlameEffectReferenceLength,1.0f) : 350.0f;
    FlameComponent->SetWorldScale3D(FVector(Length/Reference,1,1)*FlameEffectScale);
    if (!FlameComponent->IsActive()) FlameComponent->Activate(true);
}

void UCactusWeaponComponent::StopFlameVisual()
{
    bFlameActive = false;
    SetComponentTickEnabled(false);
    if (FlameComponent) FlameComponent->DeactivateImmediate();
}

void UCactusWeaponComponent::FireInDirection(const FVector& Origin, const FVector& Direction, float Speed, float ShotDamage)
{
    ACactusPlayerCharacter* Character = Cast<ACactusPlayerCharacter>(GetOwner());
    FVector Muzzle = Origin + Direction * FMath::Max(MuzzleDistance, 0.0f);

    // 銃口だけが壁の向こう側へ出ないよう、発射位置まで球で確認する。
    const float Radius = ProjectileClass->GetDefaultObject<ACactusProjectile>()->GetCollisionRadius();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CactusMuzzle), false, Character);
    FHitResult Hit;
    if (GetWorld()->SweepSingleByChannel(Hit, Origin, Muzzle, FQuat::Identity, ECC_GameTraceChannel1, FCollisionShape::MakeSphere(Radius), Query))
    {
        if (Hit.bStartPenetrating) return;
        Muzzle = Hit.Location - Direction * 2.0f;
    }

    UCactusProjectilePool* Pool = GetWorld()->GetSubsystem<UCactusProjectilePool>();
    ACactusProjectile* Projectile = Pool ? Pool->Acquire(ProjectileClass, FTransform(Direction.Rotation(), Muzzle), Character, Character) : nullptr;
    if (Projectile)
    {
        Projectile->Launch(Direction, Speed, ShotDamage, ProjectileLifetime);
        if (Projectile->IsShotActive()) OnProjectileFired.Broadcast(Projectile);
    }
}
